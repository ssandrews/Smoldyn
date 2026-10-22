/* Coupled backward Euler filament dynamics. LGPL, as the surrounding sources.
   Newton uses the complete existing force function, not a local-filament
   approximation. Restarted GMRES differentiates it by directional differences;
   the diagonal stiffness estimate is only a preconditioner. No dense Jacobian.
   Branch-point constraints are eliminated through the existing node unions. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "smolprofile.h"
#include "Geometry.h"
#include "Sphere.h"
#ifdef _OPENMP
#include <omp.h>
#endif

filamentworkptr filWorkAlloc(filamentptr fil,int maxseg);
void filPrepareThermalForces(filamentptr fil);
void filNodes2Angles(filamentptr fil,int lo,int hi);

#define FIL_GMRES_RESTART 20
struct filamentimplicitstruct {
 int capacity,n,workers;
 double *storage,*x,*r,*trial,*rt,*dx,*diag,*v,*jv,*b,*scratch;
 double hess[21][20],cs[20],sn[20],g[21],y[20];
 double tolerance,lastresidual,maxcontact,maxstretch;
 unsigned long long steps,newton,krylov,failures;
};

int filImplicitEnabled(const simptr sim) {
 int ft;
 if(!sim || !sim->filss) return 0;
 for(ft=0;ft<sim->filss->ntype;ft++)
  if(sim->filss->filtypes[ft]->dynamics==FDimplicitnetwork) return 1;
 return 0;
}

void filImplicitFree(struct filamentstericstruct *w) {
 if(!w || !w->implicit) return;
 free(w->implicit->storage);free(w->implicit);w->implicit=NULL;
}

static double filImplicitDot(const double *a,const double *b,int n) {
 double sum=0;int i;for(i=0;i<n;i++) sum+=a[i]*b[i];return sum;
}
static double filImplicitMax(const double *a,int n) {
 double value=0;int i;for(i=0;i<n;i++) {
  if(!isfinite(a[i])) return DBL_MAX;
  value=fmax(value,fabs(a[i]));
 }
 return value;
}

static int filImplicitAlloc(struct filamentstericstruct *w,int n) {
 struct filamentimplicitstruct *p;double *s;
 if(!w->implicit) w->implicit=(struct filamentimplicitstruct*)calloc(1,sizeof(*p));
 if(!(p=w->implicit)) return 1;
 if(n>p->capacity) {
  s=(double*)realloc(p->storage,(size_t)n*(FIL_GMRES_RESTART+10)*sizeof(double));
  if(!s) return 1;
  p->storage=s;p->capacity=n;
 }
 p->n=n;s=p->storage;
 p->x=s;s+=n;p->r=s;s+=n;p->trial=s;s+=n;p->rt=s;s+=n;
 p->dx=s;s+=n;p->diag=s;s+=n;p->jv=s;s+=n;p->b=s;s+=n;
 p->scratch=s;s+=n;p->v=s;
 return 0;
}

/* Displacements, rather than absolute coordinates, keep finite differences
   accurate even when the network is far from the coordinate origin. Angles are
   multiplied by standard_length so all residual components have length units.
   A redundant/fixed node has residual x=0; only root coordinates enter forces. */
static int filImplicitInstallFil(simptr sim,int f,const double *x) {
 struct filamentstericstruct *w=sim->filss->steric;
 FilStericFilament *record=&w->filaments[f];filamentptr fil=record->fil;
 int j,d,root,offset=record->nodeoffset,dim=sim->dim;double axis[3];
 if(offset<0 || fil->filtype->dynamics==FDnone) return 0;
 for(j=0;j<=fil->nseg;j++) {
  root=w->nodes[offset+j].parent;
  for(d=0;d<dim;d++) {
   FilStericNode *base=&w->nodes[root];
   double old=base->fil->filtype->dynamics==FDnone?base->fil->nodes[base->node][d]:base->fil->filwork->wnodes[1][base->node][d];
   fil->nodes[j][d]=old+x[dim*root+d];
  }
 }
 if(dim==3) {
  for(d=0;d<3;d++) axis[d]=fil->filwork->wnodes[1][1][d]-fil->filwork->wnodes[1][0][d];
  Sph_RotateVectorAxisAngle(fil->filwork->wseg0up[1],axis,x[dim*w->nnode+offset]/fil->filtype->stdlen,fil->seg0up);
  for(j=1;j<fil->nseg;j++) fil->segments[j]->ypr[2]=fil->filwork->wroll[1][j]+x[dim*w->nnode+offset+j]/fil->filtype->stdlen;
 }
 for(j=0;j<fil->nseg;j++) {
  double len=0;
  for(d=0;d<dim;d++) {double delta=fil->nodes[j+1][d]-fil->nodes[j][d];len+=delta*delta;}
  if(!isfinite(len) || len<1e-12*fil->filtype->stdlen*fil->filtype->stdlen) return 1;
 }
 filNodes2Angles(fil,-1,-1);
 for(j=0;j<fil->nseg;j++) for(d=0;d<3;d++) if(!isfinite(fil->segments[j]->ypr[d])) return 1;
 return 0;
}

static int filImplicitResidual(simptr sim,const double *x,double *r) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p=w->implicit;
 int f,i,d,root,er=0,dim=sim->dim;
 if(p->workers>1) {
#ifdef _OPENMP
#pragma omp parallel for num_threads(p->workers) schedule(static,1) reduction(|:er)
#endif
  for(f=0;f<w->nfilament;f++) er|=filImplicitInstallFil(sim,f,x);
 } else for(f=0;f<w->nfilament;f++) er|=filImplicitInstallFil(sim,f,x);
 if(er) return 1;
 SMOL_PROFILE_RUN(SP_FORCES,filStericOrdinaryForces(sim,p->workers,0));
 if(filStericEnabled(sim)) {SMOL_PROFILE_RUN(SP_STERIC,er=filStericForces(sim));if(er) return 1;}
 memcpy(r,x,(size_t)p->n*sizeof(double));
 for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) w->nodes[i].force[d]=0;
 for(i=0;i<w->nnode;i++) {
  FilStericNode *node=&w->nodes[i];filamentptr fil=node->fil;root=node->parent;
  if(fil->filtype->dynamics!=FDnone)
   for(d=0;d<dim;d++) w->nodes[root].force[d]+=fil->filwork->forces[node->node][d];
 }
 for(i=0;i<w->nnode;i++) {
  FilStericNode *node=&w->nodes[i];filamentptr fil=node->fil;filamenttypeptr t=fil->filtype;
  if(node->parent==i && !node->fixed)
   for(d=0;d<dim;d++) r[dim*i+d]-=sim->dt*node->force[d]/node->drag;
  if(dim==3 && node->node<fil->nseg && t->dynamics!=FDnone) {
   /* Preserve the author's rotational mobility convention: node 0 for the
      first material frame, node 1 for subsequent relative roll coordinates. */
   double mu=t->mobility*fil->nodemobility[node->node?1:0];
   r[dim*w->nnode+i]-=sim->dt*t->stdlen*mu*fil->filwork->torques[node->node];
  }
 }
 return filImplicitMax(r,p->n)==DBL_MAX;
}

static int filImplicitProduct(simptr sim,const double *v,double *out) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 double vmax=filImplicitMax(v,p->n),epsilon;int i;
 if(vmax==0) {memset(out,0,(size_t)p->n*sizeof(double));return 0;}
 /* Perturb by approximately 1e-9 length units, above geometry roundoff. */
 epsilon=1e-9/vmax;
 for(i=0;i<p->n;i++) p->trial[i]=p->x[i]+epsilon*v[i];
 if(filImplicitResidual(sim,p->trial,p->rt)) return 1;
 for(i=0;i<p->n;i++) out[i]=(p->rt[i]-p->r[i])/(epsilon*p->diag[i]);
 return 0;
}

/* Left-preconditioned restarted GMRES. Only vectors and a small Hessenberg
   matrix are stored. Modified Gram-Schmidt is reorthogonalized for robustness. */
static int filImplicitLinear(simptr sim,int limit) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 int i,j,k,pass,total=0,m,n=p->n;double beta,initial,target,dot,c,s,a,b;
 memset(p->dx,0,(size_t)n*sizeof(double));
 for(i=0;i<n;i++) p->b[i]=-p->r[i]/p->diag[i];
 initial=sqrt(filImplicitDot(p->b,p->b,n));target=fmax(1e-14,0.05*initial);
 while(total<limit) {
  if(total) {
   if(filImplicitProduct(sim,p->dx,p->jv)) return 1;
   for(i=0;i<n;i++) p->b[i]=-p->r[i]/p->diag[i]-p->jv[i];
  }
  beta=sqrt(filImplicitDot(p->b,p->b,n));if(beta<=target) return 0;
  for(i=0;i<n;i++) p->v[i]=p->b[i]/beta;
  memset(p->hess,0,sizeof(p->hess));memset(p->g,0,sizeof(p->g));p->g[0]=beta;
  m=0;
  for(j=0;j<FIL_GMRES_RESTART && total<limit;j++,total++) {
   if(filImplicitProduct(sim,p->v+(size_t)j*n,p->jv)) return 1;
   p->krylov++;
   for(pass=0;pass<2;pass++) for(k=0;k<=j;k++) {
    dot=filImplicitDot(p->jv,p->v+(size_t)k*n,n);p->hess[k][j]+=dot;
    for(i=0;i<n;i++) p->jv[i]-=dot*p->v[(size_t)k*n+i];
   }
   p->hess[j+1][j]=sqrt(filImplicitDot(p->jv,p->jv,n));
   if(p->hess[j+1][j]>1e-30)
    for(i=0;i<n;i++) p->v[(size_t)(j+1)*n+i]=p->jv[i]/p->hess[j+1][j];
   for(k=0;k<j;k++) {
    a=p->hess[k][j];b=p->hess[k+1][j];
    p->hess[k][j]=p->cs[k]*a+p->sn[k]*b;p->hess[k+1][j]=-p->sn[k]*a+p->cs[k]*b;
   }
   a=p->hess[j][j];b=p->hess[j+1][j];dot=hypot(a,b);
   if(dot<1e-30) return 1;
   c=a/dot;s=b/dot;p->cs[j]=c;p->sn[j]=s;
   p->hess[j][j]=dot;p->hess[j+1][j]=0;
   p->g[j+1]=-s*p->g[j];p->g[j]*=c;m=j+1;
   if(fabs(p->g[j+1])<=target) {total++;break;}
  }
  for(k=m-1;k>=0;k--) {
   a=p->g[k];for(j=k+1;j<m;j++) a-=p->hess[k][j]*p->y[j];
   p->y[k]=a/p->hess[k][k];
  }
  for(k=0;k<m;k++) for(i=0;i<n;i++) p->dx[i]+=p->y[k]*p->v[(size_t)k*n+i];
  if(fabs(p->g[m])<=target) return 0;
 }
 return 1;
}

int filImplicitStep(simptr sim,int workers) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p;
 int f,i,j,d,n,iter,limit=1,krylov=1,er=0,dim=sim->dim;double norm,next,alpha;
 if(w->nnode>INT_MAX/(dim+1)) return 1;
 n=w->nnode*(dim==3?4:2);if(!n) return 0;
 if(filImplicitAlloc(w,n)) return 1;
 p=w->implicit;p->workers=workers;p->tolerance=DBL_MAX;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;filamenttypeptr t=fil->filtype;
  if(!fil->nseg || t->dynamics==FDnone) continue;
  if(!fil->filwork && !filWorkAlloc(fil,fil->maxseg)) return 1;
  if(!(t->stdlen>0)) return 1;
  if(t->dynamics!=FDnone) {
   p->tolerance=fmin(p->tolerance,t->implicittol);limit=limit>t->implicititerations?limit:t->implicititerations;
   krylov=krylov>t->implicitkrylov?krylov:t->implicitkrylov;
  }
  for(j=0;j<=fil->nseg;j++) memcpy(fil->filwork->wnodes[1][j],fil->nodes[j],3*sizeof(double));
  memcpy(fil->filwork->wseg0up[1],fil->seg0up,3*sizeof(double));
  for(j=0;j<fil->nseg;j++) fil->filwork->wroll[1][j]=fil->segments[j]->ypr[2];
 }
 memset(p->x,0,(size_t)n*sizeof(double));
 /* Draw thermal forces exactly once per accepted mechanical interval. They
    remain fixed across Newton, Jacobian products, and rejected line searches. */
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;
  if(!fil->nseg || fil->filtype->dynamics==FDnone) continue;
  fil->filwork->thermtime=-DBL_MAX;filPrepareThermalForces(fil);
 }
 for(i=0;i<n;i++) p->diag[i]=1;
 for(i=0;i<w->nnode;i++) {
  FilStericNode *node=&w->nodes[i];filamenttypeptr t=node->fil->filtype;
  int root=node->parent;double length=t->stdlen,mu,stiff;
  if(node->fixed || w->nodes[root].fixed || t->dynamics==FDnone) continue;
  mu=1/w->nodes[root].drag;
  stiff=2*t->klen+8*fmax(t->kypr[0],t->kypr[1])/(length*length)+t->confineforce;
  stiff+=4*(t->branchforceangle+t->branchforceazimuth)/(length*length)+4*t->sterick;
  for(d=0;d<dim;d++) p->diag[dim*root+d]+=sim->dt*mu*stiff;
  if(dim==3) p->diag[dim*w->nnode+i]+=4*sim->dt*t->mobility*t->kypr[2];
  p->maxstretch=fmax(p->maxstretch,sim->dt*t->mobility*t->klen);
 }
 if(filImplicitResidual(sim,p->x,p->r)) er=1;
 for(iter=0;!er && iter<limit;iter++) {
  p->lastresidual=filImplicitMax(p->r,n);
  if(p->lastresidual<=p->tolerance) break;
  p->newton++;
  if(filImplicitLinear(sim,krylov)) {er=1;break;}
  norm=filImplicitDot(p->r,p->r,n);alpha=1;
  for(j=0;j<12;j++,alpha*=0.5) {
   for(i=0;i<n;i++) p->trial[i]=p->x[i]+alpha*p->dx[i];
   if(filImplicitResidual(sim,p->trial,p->rt)) continue;
   next=filImplicitDot(p->rt,p->rt,n);
   if(next<=(1-1e-4*alpha)*norm || filImplicitMax(p->rt,n)<=p->tolerance) break;
  }
  if(j==12) {er=1;break;}
  memcpy(p->x,p->trial,(size_t)n*sizeof(double));memcpy(p->r,p->rt,(size_t)n*sizeof(double));
 }
 p->lastresidual=filImplicitMax(p->r,n);
 if(er || p->lastresidual>p->tolerance) {
  p->failures++;memset(p->x,0,(size_t)n*sizeof(double));filImplicitResidual(sim,p->x,p->r);
  simLog(sim,9,"ERROR: implicit_network did not converge (residual %g, tolerance %g, h %g); restored mechanical step. Reduce time_step or increase steric_substeps.\n",p->lastresidual,p->tolerance,sim->dt);
  return 1;
 }
 /* Last residual evaluation installed the accepted state and refreshed contacts. */
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;
  if(fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<fil->nseg;j++) fil->roll[j]=fil->segments[j]->ypr[2];
 }
 p->maxcontact=fmax(p->maxcontact,w->contactbound);p->steps++;return 0;
}

/* Save the outer mechanical interval in the author's existing spare work arrays.
   Chemistry is already accepted; on solver failure all mechanical substeps are
   rolled back together. No automatic retry/redraw of stochastic increments. */
int filImplicitSave(simptr sim,int restore) {
 struct filamentstericstruct *w=sim->filss->steric;int f,j;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;filamentworkptr fw;
  if(!fil->nseg || fil->filtype->dynamics==FDnone) continue;
  if(!fil->filwork && !filWorkAlloc(fil,fil->maxseg)) return 1;
  fw=fil->filwork;
  for(j=0;j<=fil->nseg;j++)
   memcpy(restore?fil->nodes[j]:fw->wnodes[2][j],restore?fw->wnodes[2][j]:fil->nodes[j],3*sizeof(double));
  memcpy(restore?fil->seg0up:fw->wseg0up[2],restore?fw->wseg0up[2]:fil->seg0up,3*sizeof(double));
  for(j=0;j<fil->nseg;j++) {
   if(restore) {fil->segments[j]->ypr[2]=fw->wroll[2][j];fil->roll[j]=fw->flatnodes[2][j];}
   else {fw->wroll[2][j]=fil->segments[j]->ypr[2];fw->flatnodes[2][j]=fil->roll[j];}
  }
  if(restore) filNodes2Angles(fil,-1,-1);
 }
 if(restore) {
  filStericOrdinaryForces(sim,w->workersused,0);
  if(filStericEnabled(sim)) return filStericForces(sim);
 }
 return 0;
}

void filImplicitReport(simptr sim) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 if(!p) return;
 simLog(sim,2,"  implicit_network: %llu accepted steps, %llu Newton iterations, %llu Krylov iterations, %llu failures; last residual %g (length units)\n",p->steps,p->newton,p->krylov,p->failures,p->lastresidual);
 simLog(sim,2,"  largest accepted contact relaxation factor %g; largest attempted h*mobility*force_length %g\n",p->maxcontact,p->maxstretch);
}
