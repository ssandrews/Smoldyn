/* Coupled backward Euler filament dynamics. LGPL, as the surrounding sources.
   Newton uses the complete existing force function, not a local-filament
   approximation. GMRES uses residual differences (default) or an optional cached
   force derivative operator. Nonlinear acceptance always uses the original forces.
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
void filComputeForces(filamentptr fil,int lo,int hi);

#define FIL_GMRES_RESTART 20
/* Avoid team overhead for small networks. Dot products retain serial summation
   order; pointwise corrections/maxima can be distributed without roundoff drift. */
#ifdef _OPENMP
#define FIL_VECTOR_LOOP(body) do { \
 if(p->workers>1 && p->n>=65536) { \
  _Pragma("omp parallel for num_threads(p->workers) schedule(static)") \
  for(i=0;i<p->n;i++) {body;} \
 } else for(i=0;i<p->n;i++) {body;} \
} while(0)
#else
#define FIL_VECTOR_LOOP(body) do {for(i=0;i<p->n;i++) {body;}} while(0)
#endif
struct filamentimplicitstruct {
 int capacity,n,workers,banded,bandedactive,projectlength,jacobiantrial,lengthtangent;
 struct filamentprecondstruct *preconditioner;
 struct filamentconstraintstruct *length;
 struct filamentoperatorstruct *forceoperator;
 int cached,cacheactive,cachedblocked;
 double *storage,*x,*r,*trial,*rt,*dx,*diag,*v,*jv,*b,*scratch;
 double hess[21][20],cs[20],sn[20],g[21],y[20];
 double tolerance,lastresidual,maxcontact,maxstretch;
 double epsilon;
 double projecteddefect,maxprojecteddefect,lengthscale;
 unsigned long long steps,newton,krylov,failures;
 unsigned long long residuals,precondbuilds,precondapplies,precondfallbacks;
 unsigned long long projectedsteps,projectedfallbacks;
 unsigned long long operatorfallbacks;
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
 filPrecondFree(w->implicit->preconditioner);
 filLengthFree(w->implicit->length);
 filOperatorFree(w->implicit->forceoperator);
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
static int filImplicitAngles(simptr sim,int f) {
 filamentptr fil=sim->filss->steric->filaments[f].fil;int j,d;
 if(!fil->nseg || fil->filtype->dynamics==FDnone) return 0;
 filNodes2Angles(fil,-1,-1);
 for(j=0;j<fil->nseg;j++) for(d=0;d<3;d++) if(!isfinite(fil->segments[j]->ypr[d])) return 1;
 return 0;
}

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
    if(w->implicit->jacobiantrial) {
     old=base->fil->filtype->dynamics==FDnone?base->fil->nodes[base->node][d]:base->fil->filwork->flatnodes[0][dim*base->node+d];
     fil->nodes[j][d]=old+w->implicit->epsilon*w->implicit->scratch[dim*root+d];
    } else fil->nodes[j][d]=old+x[dim*root+d];
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
 return w->implicit->projectlength?0:filImplicitAngles(sim,f);
}

/* Each node owns its residual entries and gathers its physical root force.
   This can share a worker team with geometry/ordinary forces when contacts are off. */
static void filImplicitNodeResidual(simptr sim,const double *x,double *r,int i) {
 struct filamentstericstruct *w=sim->filss->steric;
 FilStericNode *node=&w->nodes[i];filamentptr fil=node->fil;filamenttypeptr t=fil->filtype;
 int d,dim=sim->dim;
 filStericNodeForce(sim,i,1);
 for(d=0;d<dim;d++) {
  r[dim*i+d]=x[dim*i+d];
  if(node->parent==i && !node->fixed) r[dim*i+d]-=sim->dt*node->force[d]/node->drag;
 }
 if(dim==3) {
  r[dim*w->nnode+i]=x[dim*w->nnode+i];
  if(node->node<fil->nseg && t->dynamics!=FDnone) {
   double mu=t->mobility*fil->nodemobility[node->node?1:0];
   r[dim*w->nnode+i]-=sim->dt*t->stdlen*mu*fil->filwork->torques[node->node];
  }
 }
}

/* Retain the original contiguous scatter for one worker; a gather is useful
   for ownership across workers, but adds pointer chasing to a serial pass. */
static void filImplicitSerialResidual(simptr sim,const double *x,double *r) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p=w->implicit;
 int i,d,root,dim=sim->dim;
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
}

/* Keep the same team across dependent worksharing loops. The barriers between
   loops are required: junction forces read other filaments' installed geometry,
   and shared-node gathers read their completed force arrays. */
static int filImplicitTrial(simptr sim,const double *x,double *r,int steric) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p=w->implicit;
 int f,er=0,hasclamp=sim->srfss && sim->srfss->nforceclamp;
#ifdef _OPENMP
 int i,slot;
#endif
 if(p->workers>1) {
  filWorkSchedule(sim,p->workers);
#ifdef _OPENMP
#pragma omp parallel num_threads(p->workers) shared(er)
  {
#pragma omp for schedule(static,1) reduction(|:er)
   for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext)
    er|=filImplicitInstallFil(sim,next,x);
   if(p->projectlength) {
#pragma omp single
     {if(!er && !p->jacobiantrial) {SMOL_PROFILE_RUN(SP_LENGTH_PROJECT,er=filLengthProject(sim,p->length));}}
     int valid=!er;
#pragma omp for schedule(static,1) reduction(|:er)
     for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext)
      if(valid) er|=filImplicitAngles(sim,next);
   }
   if(hasclamp) {
#pragma omp single
    {if(!er) er=surfforceclampsolve(sim);}
   }
#pragma omp for schedule(static,1)
   for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext) {
    filamentptr fil=w->filaments[next].fil;
    if(!er && fil->nseg && fil->filtype->dynamics!=FDnone) filComputeForces(fil,-1,-1);
   }
   if(!steric) {
#pragma omp for schedule(static)
    for(i=0;i<w->nnode;i++) if(!er) filImplicitNodeResidual(sim,x,r,i);
   }
  }
#endif
 } else {
  for(f=0;f<w->nfilament;f++) er|=filImplicitInstallFil(sim,f,x);
  if(!er && p->projectlength) {
   if(!p->jacobiantrial) {SMOL_PROFILE_RUN(SP_LENGTH_PROJECT,er=filLengthProject(sim,p->length));}
   for(f=0;f<w->nfilament && !er;f++) er=filImplicitAngles(sim,f);
  }
  if(!er && hasclamp) er=surfforceclampsolve(sim);
  if(!er) filStericOrdinaryForces(sim,1,0);
  if(!er && !steric) filImplicitSerialResidual(sim,x,r);
 }
 return er;
}

static int filImplicitResidual(simptr sim,const double *x,double *r) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p=w->implicit;
 int i,er=0,steric=filStericEnabled(sim);
 p->residuals++;
 SMOL_PROFILE_RUN(SP_IMPLICIT_TRIAL,er=filImplicitTrial(sim,x,r,steric));
 if(er) return 1;
 if(steric) {
  SMOL_PROFILE_RUN(SP_STERIC,er=filStericForces(sim));if(er) return 1;
  if(p->workers>1) {
#ifdef _OPENMP
#pragma omp parallel for num_threads(p->workers) schedule(static)
#endif
   for(i=0;i<w->nnode;i++) filImplicitNodeResidual(sim,x,r,i);
  } else filImplicitSerialResidual(sim,x,r);
 }
 return filImplicitMax(r,p->n)==DBL_MAX;
}

static int filImplicitLengthPrecondition(simptr sim,const double *in,double *out) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;int er;
 SMOL_PROFILE_RUN(SP_PRECOND_APPLY,er=filLengthPrecondition(sim,p->length,p->bandedactive?p->preconditioner:NULL,in,out));
 if(p->bandedactive) p->precondapplies++;
 return er;
}

static int filImplicitProduct(simptr sim,const double *v,double *out) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 double vmax=filImplicitMax(v,p->n),epsilon;int i,er;
 if(vmax==0) {memset(out,0,(size_t)p->n*sizeof(double));return 0;}
 if(p->cacheactive) {
  const double *motion=v;
  if(p->lengthtangent) {
   SMOL_PROFILE_RUN(SP_LENGTH_PROJECT,er=filLengthTangent(sim,p->length,v,p->scratch));if(er) return 1;
   motion=p->scratch;
  }
  SMOL_PROFILE_RUN(SP_OPERATOR_APPLY,er=filOperatorApply(sim,p->forceoperator,motion,out));if(er) return 1;
  FIL_VECTOR_LOOP(out[i]=v[i]-out[i]);
  if(p->lengthtangent) {
   if(filImplicitLengthPrecondition(sim,out,p->scratch)) return 1;
   memcpy(out,p->scratch,(size_t)p->n*sizeof(double));
  } else if(p->bandedactive) {
   SMOL_PROFILE_RUN(SP_PRECOND_APPLY,filPrecondApply(sim,p->preconditioner,out,p->scratch));p->precondapplies++;
   memcpy(out,p->scratch,(size_t)p->n*sizeof(double));
  } else FIL_VECTOR_LOOP(out[i]/=p->diag[i]);
  return 0;
 }
 /* Perturb by approximately 1e-9 length units, above geometry roundoff. */
 epsilon=1e-9/vmax;
 FIL_VECTOR_LOOP(p->trial[i]=p->x[i]+epsilon*v[i]);
 if(p->lengthtangent) {
  SMOL_PROFILE_RUN(SP_LENGTH_PROJECT,er=filLengthTangent(sim,p->length,v,p->scratch));if(er) return 1;
  p->jacobiantrial=1;p->epsilon=epsilon;
 }
 er=filImplicitResidual(sim,p->trial,p->rt);p->jacobiantrial=0;
 if(er) return 1;
 if(p->lengthtangent) {
  FIL_VECTOR_LOOP(out[i]=(p->rt[i]-p->r[i])/epsilon);
  if(filImplicitLengthPrecondition(sim,out,p->scratch)) return 1;
  memcpy(out,p->scratch,(size_t)p->n*sizeof(double));
 } else if(p->bandedactive) {
  FIL_VECTOR_LOOP(out[i]=(p->rt[i]-p->r[i])/epsilon);
  SMOL_PROFILE_RUN(SP_PRECOND_APPLY,filPrecondApply(sim,p->preconditioner,out,p->scratch));p->precondapplies++;
  memcpy(out,p->scratch,(size_t)p->n*sizeof(double));
 } else FIL_VECTOR_LOOP(out[i]=(p->rt[i]-p->r[i])/(epsilon*p->diag[i]));
 return 0;
}

/* Left-preconditioned restarted GMRES. Only vectors and a small Hessenberg
   matrix are stored. Modified Gram-Schmidt is reorthogonalized for robustness. */
static int filImplicitLinear(simptr sim,int limit,int projected) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 int i,j,k,pass,total=0,m,n=p->n;double beta,initial,target,dot,c,s,a,b;
 memset(p->dx,0,(size_t)n*sizeof(double));
 if(p->lengthtangent) {
  if(filImplicitLengthPrecondition(sim,p->r,p->b)) return 1;
  FIL_VECTOR_LOOP(p->b[i]=-p->b[i]);
 } else if(p->bandedactive) {
  SMOL_PROFILE_RUN(SP_PRECOND_APPLY,filPrecondApply(sim,p->preconditioner,p->r,p->b));p->precondapplies++;
  FIL_VECTOR_LOOP(p->b[i]=-p->b[i]);
 } else FIL_VECTOR_LOOP(p->b[i]=-p->r[i]/p->diag[i]);
 /* A fixed relative Krylov error can dominate timestep refinement when there
    is no outer Newton correction. Scale the predictor's target with its RMS
    displacement/segment length, so the linear error tends to O(h^2). Cap at
    1 percent for large steps; final acceptance still uses the full defect.
    Keep the original Newton path bitwise unchanged. */
 initial=sqrt(filImplicitDot(p->b,p->b,n));
 target=fmax(1e-14,(projected?fmin(0.01,initial/(sqrt((double)n)*p->lengthscale)):0.05)*initial);
 /* Do not resolve tiny startup motions far below the requested physical
    residual tolerance. Use a conservative diagonal scale for this absolute
    inner floor; acceptance is always checked with the unscaled full defect.
    With tolerance refinement this floor also vanishes. */
 if(projected) target=fmax(target,0.1*p->tolerance/fmax(1,filImplicitMax(p->diag,n)));
 while(total<limit) {
  if(total) {
   if(filImplicitProduct(sim,p->dx,p->jv)) return 1;
   if(p->lengthtangent) {
    if(filImplicitLengthPrecondition(sim,p->r,p->b)) return 1;
    FIL_VECTOR_LOOP(p->b[i]=-p->b[i]-p->jv[i]);
   } else if(p->bandedactive) {
    SMOL_PROFILE_RUN(SP_PRECOND_APPLY,filPrecondApply(sim,p->preconditioner,p->r,p->b));p->precondapplies++;
    FIL_VECTOR_LOOP(p->b[i]=-p->b[i]-p->jv[i]);
   } else FIL_VECTOR_LOOP(p->b[i]=-p->r[i]/p->diag[i]-p->jv[i]);
  }
  beta=sqrt(filImplicitDot(p->b,p->b,n));if(beta<=target) return 0;
  FIL_VECTOR_LOOP(p->v[i]=p->b[i]/beta);
  memset(p->hess,0,sizeof(p->hess));memset(p->g,0,sizeof(p->g));p->g[0]=beta;
  m=0;
  for(j=0;j<FIL_GMRES_RESTART && total<limit;j++,total++) {
   if(filImplicitProduct(sim,p->v+(size_t)j*n,p->jv)) return 1;
   p->krylov++;
   for(pass=0;pass<2;pass++) for(k=0;k<=j;k++) {
    dot=filImplicitDot(p->jv,p->v+(size_t)k*n,n);p->hess[k][j]+=dot;
    FIL_VECTOR_LOOP(p->jv[i]-=dot*p->v[(size_t)k*n+i]);
   }
   p->hess[j+1][j]=sqrt(filImplicitDot(p->jv,p->jv,n));
   if(p->hess[j+1][j]>1e-30) {
    FIL_VECTOR_LOOP(p->v[(size_t)(j+1)*n+i]=p->jv[i]/p->hess[j+1][j]);
   }
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
  if(p->workers>1 && n>=65536) {
   FIL_VECTOR_LOOP(int col;for(col=0;col<m;col++) p->dx[i]+=p->y[col]*p->v[(size_t)col*n+i]);
  } else for(k=0;k<m;k++) for(i=0;i<n;i++) p->dx[i]+=p->y[k]*p->v[(size_t)k*n+i];
  if(fabs(p->g[m])<=target) return 0;
 }
 return 1;
}

/* Reused by regular linearizations and a checked finite-difference retry.
   The saved flat coordinates must be owned storage, never wnodes[0]. */
static void filImplicitFreezeLength(simptr sim) {
 struct filamentstericstruct *w=sim->filss->steric;struct filamentimplicitstruct *p=w->implicit;int f,j,er;
 p->lengthtangent=0;if(!p->projectlength) return;
 SMOL_PROFILE_RUN(SP_LENGTH_PROJECT,er=filLengthFreeze(sim,p->length,p->diag));
 if(er) return;
 p->lengthtangent=1;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;if(!fil->nseg || fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<=fil->nseg;j++) memcpy(fil->filwork->flatnodes[0]+sim->dim*j,fil->nodes[j],(size_t)sim->dim*sizeof(double));
 }
}
static int filImplicitOperatorFallback(simptr sim) {
 struct filamentimplicitstruct *p=sim->filss->steric->implicit;
 p->cacheactive=0;p->cachedblocked=1;p->operatorfallbacks++;
 if(filImplicitResidual(sim,p->x,p->r)) return 1;
 filImplicitFreezeLength(sim);return 0;
}

int filImplicitStep(simptr sim,int workers) {
 struct filamentstericstruct *w=sim->filss->steric;
 struct filamentimplicitstruct *p;
 int f,i,j,d,n,iter,limit=1,krylov=1,er=0,dim=sim->dim,projected=0,acceptedprojected=0,cachedtrial=0;
 double norm,next,alpha,projectedtol=DBL_MAX;
 if(w->nnode>INT_MAX/(dim+1)) return 1;
 if(surfforceclampstep(sim)) return 1;
 n=w->nnode*(dim==3?4:2);if(!n) return surfforceclampsolve(sim);
 if(filImplicitAlloc(w,n)) return 1;
 p=w->implicit;p->workers=workers;p->tolerance=DBL_MAX;p->banded=p->bandedactive=0;
 p->lengthscale=DBL_MAX;
 p->cached=p->cacheactive=p->cachedblocked=0;
 p->projectlength=filLengthEnabled(sim);
 p->jacobiantrial=p->lengthtangent=0;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;filamenttypeptr t=fil->filtype;
  if(!fil->nseg || t->dynamics==FDnone) continue;
  if(!fil->filwork && !filWorkAlloc(fil,fil->maxseg)) return 1;
  if(!(t->stdlen>0)) return 1;
  if(t->dynamics!=FDnone) {
   p->tolerance=fmin(p->tolerance,t->implicittol);limit=limit>t->implicititerations?limit:t->implicititerations;
   krylov=krylov>t->implicitkrylov?krylov:t->implicitkrylov;
   p->banded=t->implicitpreconditioner;
   p->lengthscale=fmin(p->lengthscale,t->stdlen);
   projected=t->implicitmethod;
   p->cached=t->implicitjacobian;
   projectedtol=fmin(projectedtol,t->projectedtol);
  }
  for(j=0;j<=fil->nseg;j++) memcpy(fil->filwork->wnodes[1][j],fil->nodes[j],3*sizeof(double));
  memcpy(fil->filwork->wseg0up[1],fil->seg0up,3*sizeof(double));
  for(j=0;j<fil->nseg;j++) fil->filwork->wroll[1][j]=fil->segments[j]->ypr[2];
 }
 if(p->projectlength && filLengthPrepare(sim,&p->length)) return 1;
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
  stiff=2*fmax(t->klen,0)+8*fmax(t->kypr[0],t->kypr[1])/(length*length)+t->confineforce;
  if(t->elongmodel && t->klen<0 && node->node>=node->fil->nseg-1)
   stiff+=fmax(0,filSegmentStiffness(node->fil,node->fil->nseg-1));
  stiff+=4*(t->branchforceangle+t->branchforceazimuth)/(length*length)+4*t->sterick;
  for(d=0;d<dim;d++) p->diag[dim*root+d]+=sim->dt*mu*stiff;
  if(dim==3) p->diag[dim*w->nnode+i]+=4*sim->dt*t->mobility*t->kypr[2];
  p->maxstretch=fmax(p->maxstretch,sim->dt*t->mobility*(t->elongmodel && t->klen<0?filSegmentStiffness(node->fil,node->fil->nseg-1):t->klen));
 }
 if(filImplicitResidual(sim,p->x,p->r)) {
  /* An infeasible clamp or invalid geometry can fail before r is initialized. */
  p->lastresidual=DBL_MAX;p->failures++;
  simLog(sim,9,"ERROR: implicit_network could not evaluate its initial residual; check fixed-length geometry and force-clamp load/travel bounds.\n");
  return 1;
 }
 for(iter=0;!er && iter<limit;iter++) {
  p->lastresidual=filImplicitMax(p->r,n);
  if(p->lastresidual<=p->tolerance && (!p->projectlength || filLengthSatisfied(p->length))) break;
  p->newton++;
  p->lengthtangent=0;
  /* Freeze factors throughout a GMRES solve; refresh at the next accepted
     Newton geometry. This operation never evaluates or alters physical forces. */
  if(p->banded) {
   p->precondbuilds++;
   SMOL_PROFILE_RUN(SP_PRECOND_BUILD,p->bandedactive=!filPrecondBuild(sim,&p->preconditioner,p->diag));
   if(!p->bandedactive) {
    if(!p->precondfallbacks) simLog(sim,5,"WARNING: banded preconditioner setup failed; using diagonal scaling for this Newton correction\n");
    p->precondfallbacks++;
   }
  }
  filImplicitFreezeLength(sim);
  p->cacheactive=0;
  if(p->cached && !p->cachedblocked) {
   int buildfailed=1;
   if(!p->projectlength || p->lengthtangent) {SMOL_PROFILE_RUN(SP_OPERATOR_BUILD,buildfailed=filOperatorBuild(sim,&p->forceoperator,workers));}
   if(buildfailed) {p->cachedblocked=1;p->operatorfallbacks++;}
   else p->cacheactive=1;
  }
retrylinear:
  if(filImplicitLinear(sim,krylov,projected && iter==0)) {
   if(p->cacheactive) {
    if(filImplicitOperatorFallback(sim)) {er=1;break;}
    goto retrylinear;
   }
   /* The predictor asks for a tighter inner solve. Exhausting its Krylov
      budget is not a failed physical step if ordinary inexact Newton can
      still proceed. The frozen factors, base residual and noise are valid. */
   if(!projected || iter!=0) {er=1;break;}
   p->projectedfallbacks++;projected=0;p->projecteddefect=DBL_MAX;
   if(filImplicitLinear(sim,krylov,0)) {er=1;break;}
  }
  /* Optional linearly implicit constrained step. At the initial geometry the
     existing product is I-h M J P, where P removes first-order length change.
     Solving once for the free proposal, then retracting along M G0^T, gives
     the projected semi-implicit update without nested outer Newton solves.
     The actual final forces, lengths and clamp load are reevaluated. A large
     unpreconditioned displacement defect falls through to ordinary Newton,
     with the same cached thermal forces and normal line-search safeguards. */
  cachedtrial=0;
  if(projected && iter==0) {
   FIL_VECTOR_LOOP(p->trial[i]=p->x[i]+p->dx[i]);
   cachedtrial=!filImplicitResidual(sim,p->trial,p->rt);
   p->projecteddefect=cachedtrial?filImplicitMax(p->rt,n):DBL_MAX;
   if(cachedtrial && p->projecteddefect<=projectedtol && filLengthSatisfied(p->length)) {
    memcpy(p->x,p->trial,(size_t)n*sizeof(double));memcpy(p->r,p->rt,(size_t)n*sizeof(double));
    acceptedprojected=1;p->projectedsteps++;
    p->maxprojecteddefect=fmax(p->maxprojecteddefect,p->projecteddefect);break;
   }
   p->projectedfallbacks++;
  }
  norm=filImplicitDot(p->r,p->r,n);alpha=1;
  for(j=0;j<12;j++,alpha*=0.5) {
   FIL_VECTOR_LOOP(p->trial[i]=p->x[i]+alpha*p->dx[i]);
   if(!(j==0 && cachedtrial) && filImplicitResidual(sim,p->trial,p->rt)) continue;
   next=filImplicitDot(p->rt,p->rt,n);
   if(next<=(1-1e-4*alpha)*norm || filImplicitMax(p->rt,n)<=p->tolerance) break;
  }
  if(j==12) {
   if(p->cacheactive) {
    if(filImplicitOperatorFallback(sim)) {er=1;break;}
    goto retrylinear;
   }
   er=1;break;
  }
  memcpy(p->x,p->trial,(size_t)n*sizeof(double));memcpy(p->r,p->rt,(size_t)n*sizeof(double));
  /* Allow one reference-operator Newton phase if cached corrections consume
     their budget. Keep the accepted geometry and the same thermal forces. */
  if(iter==limit-1 && p->cached && !p->cachedblocked && filImplicitMax(p->r,n)>p->tolerance) {
   p->cacheactive=0;p->cachedblocked=1;p->operatorfallbacks++;projected=0;iter=-1;
  }
 }
 p->lastresidual=filImplicitMax(p->r,n);
 if(er || (!acceptedprojected && p->lastresidual>p->tolerance)) {
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
 if(!restore && surfforceclampbegin(sim)) return 1;
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
  if(surfforceclampend(sim,0)) return 1;
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
 simLog(sim,2,"  implicit residual evaluations %llu; banded setups %llu, applications %llu, diagonal fallbacks %llu\n",p->residuals,p->precondbuilds,p->precondapplies,p->precondfallbacks);
 if(p->projectedsteps || p->projectedfallbacks)
  simLog(sim,2,"  projected semi-implicit: %llu accepted predictors, %llu Newton fallbacks; largest accepted defect %g, last attempted defect %g (length units)\n",p->projectedsteps,p->projectedfallbacks,p->maxprojecteddefect,p->projecteddefect);
 if(p->projectlength) filLengthReport(sim,p->length);
 if(p->cached || p->forceoperator) {
  simLog(sim,2,"  cached Jacobian: %llu finite-difference fallback phases\n",p->operatorfallbacks);
  filOperatorReport(sim,p->forceoperator);
 }
 if(sim->srfss && sim->srfss->nforceclamp) for(int s=0;s<sim->srfss->nsrf;s++) {
  surfaceptr surf=sim->srfss->srflist[s];
  for(int j=0;j<surf->npanel[PSrect];j++) {
   panelptr pnl=surf->panels[PSrect][j];panelforceclampptr c=pnl->forceclamp;
   if(c) simLog(sim,2,"  force_clamp %s:%s position %g, target %g, reaction %g, error %g; %llu contact reductions\n",
    surf->sname,pnl->pname,pnl->point[0][(int)pnl->front[1]],c->force,c->reaction,c->reaction-c->force,c->evaluations);
  }
 }

}
