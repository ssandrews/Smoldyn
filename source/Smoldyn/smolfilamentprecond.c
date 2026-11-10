/* Optional banded filament preconditioner for coupled backward Euler. LGPL.
   Stretching and bending outer products follow Steven Andrews' analytic
   filComputeDerivForceMat and upstream thickness-scaled force conventions.
   These are local translational blocks, not a full network Hessian.
   Relative material rolls retain the existing diagonal preconditioner.
   This approximation never changes forces, constraints, or acceptance tests. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "Sphere.h"
#ifdef _OPENMP
#include <omp.h>
#endif


struct filamentprecondstruct {
 size_t capacity;
 int width,workers;
 double *storage,*factor,*rhs,*stiffness,*scale,*weight;
 const double *diagonal; /* borrowed rotational scaling, valid through GMRES */
};

void filPrecondFree(struct filamentprecondstruct *p) {
 if(p) {free(p->storage);free(p);}
}

/* Symmetric lower band, with the diagonal in column zero. Its half-bandwidth
   is 3*dim-1: a bend couples three consecutive vector-valued nodes. */
static void filPrecondRank(double *a,int width,const int *idx,const double *g,int n,double k) {
 int i,j;
 if(k<=0) return;
 for(i=0;i<n;i++) for(j=0;j<=i;j++)
  a[(size_t)idx[i]*width+idx[i]-idx[j]]+=k*g[i]*g[j];
}

/* The local part of each branch couple also fits in the filament band. Keep
   the other filament's orientation frozen in this approximation; shared-point
   diagonal assembly still gathers both partners. Cross-filament derivatives
   remain in the matrix-free operator. */
static void filPrecondJunction(simptr sim,filamentptr fil,double *band,int width) {
 int dim=sim->dim,j,d,ma,mb,lo,idx[6];double tm[3],td[3],lm,ld,c,s,g[6],kappa,theta,kaz,kj;
 /* One block per upstream junction side; the cross-filament part stays in
    the operator. Ordered node pairs keep the Cholesky band indexing valid. */
 for(j=0;j<2+fil->njunct;j++) {
  filjunctionptr joint=j<2?fil->endjunct[j]:fil->juncts[j-2];
  if(!joint || !joint->fil->nseg || !joint->target->nseg ||
     !filJunctionGeometry(joint,dim,tm,td,&lm,&ld,&c,&s,&ma,&mb)) continue;
  filJunctionParams(joint,dim,&kappa,&theta,&kaz,&kj);
  lo=j<2?(joint->end=='m'?0:fil->nseg-1):(ma<mb?ma:mb);
  for(d=0;d<2*dim;d++) idx[d]=dim*lo+d;
  for(d=0;d<dim;d++) {g[d]=j<2?-(tm[d]-c*td[d])/(ld*s):-(td[d]-c*tm[d])/(lm*s);g[dim+d]=-g[d];}
  filPrecondRank(band,width,idx,g,2*dim,kappa);
 }
}

static void filPrecondElastic(simptr sim,int f,struct filamentprecondstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;
 FilStericFilament *record=&w->filaments[f];filamentptr fil=record->fil;
 filamenttypeptr type=fil->filtype;
 int dim=sim->dim,width=p->width,j,d,a,i,idx[9];
 double *band,g[9],u[3],xm[3],xp[3],lm2,lp2,len,kperp,k,cm[3],cp3[3],e[3][3],unit[3],wa[3],cy,sy,cp,sp;
 if(record->nodeoffset<0 || type->dynamics==FDnone) return;
 band=p->factor+(size_t)dim*record->nodeoffset*width;
 memset(band,0,(size_t)dim*(fil->nseg+1)*width*sizeof(double));
 for(j=0;j<fil->nseg;j++) {
  len=0;
  for(d=0;d<dim;d++) {u[d]=fil->nodes[j+1][d]-fil->nodes[j][d];len+=u[d]*u[d];}
  len=sqrt(len);for(d=0;d<dim;d++) u[d]/=len;
  /* Use the upstream thickness-scaled axial stiffness. Positive transverse
     stiffness is retained in tension; omit it in compression. */
  {double rest=type->elongmodel?filSegmentRestLength(fil,j):type->stdlen;
   k=fmax(0,filSegmentStiffness(fil,j));kperp=len>rest?k*(1-rest/len):0;}
  for(d=0;d<dim;d++) {idx[d]=dim*j+d;idx[dim+d]=dim*(j+1)+d;g[d]=-u[d];g[dim+d]=u[d];}
  filPrecondRank(band,width,idx,g,2*dim,k-kperp);
  for(a=0;a<dim;a++) {
   for(d=0;d<2*dim;d++) g[d]=0;
   g[a]=-1;g[dim+a]=1;filPrecondRank(band,width,idx,g,2*dim,kperp);
  }
 }
 for(j=1;j<fil->nseg;j++) {
  lm2=lp2=0;
  for(d=0;d<dim;d++) {
   xm[d]=fil->nodes[j][d]-fil->nodes[j-1][d];xp[d]=fil->nodes[j+1][d]-fil->nodes[j][d];
   lm2+=xm[d]*xm[d];lp2+=xp[d]*xp[d];
  }
  for(i=0;i<3*dim;i++) idx[i]=dim*(j-1)+i;
  if(dim==2) {
   cm[0]=-xm[1]/lm2;cm[1]=xm[0]/lm2;cp3[0]=-xp[1]/lp2;cp3[1]=xp[0]/lp2;
   for(d=0;d<dim;d++) {g[d]=cm[d];g[dim+d]=-cm[d]-cp3[d];g[2*dim+d]=cp3[d];}
   filPrecondRank(band,width,idx,g,6,0.5*(fil->segments[j-1]->thk+fil->segments[j]->thk)*type->kypr[0]);
  } else {
   for(a=0;a<3;a++) {unit[0]=unit[1]=unit[2]=0;unit[a]=1;Sph_QtniRotate(fil->segments[j-1]->qabs,unit,e[a]);}
   cy=cos(fil->segments[j]->ypr[0]);sy=sin(fil->segments[j]->ypr[0]);
   cp=fmax(cos(fil->segments[j]->ypr[1]),0.05);sp=sin(fil->segments[j]->ypr[1]);
   for(a=0;a<2;a++) {
    /* Upstream Euler-angle gradient outer products form a positive local
       surrogate; the full force operator retains nonlinear corrections. */
    for(d=0;d<3;d++) wa[d]=a==0?e[2][d]+sp/cp*(cy*e[0][d]+sy*e[1][d]):-sy*e[0][d]+cy*e[1][d];
    for(d=0;d<3;d++) {
     int b=(d+1)%3,c=(d+2)%3;
     cm[d]=(wa[b]*xm[c]-wa[c]*xm[b])/lm2;cp3[d]=(wa[b]*xp[c]-wa[c]*xp[b])/lp2;
     g[d]=cm[d];g[3+d]=-cm[d]-cp3[d];g[6+d]=cp3[d];
    }
    filPrecondRank(band,width,idx,g,9,0.5*(fil->segments[j-1]->thk+fil->segments[j]->thk)*type->kypr[a]);
   }
  }
 }
 filPrecondJunction(sim,fil,band,width);
}

static void filPrecondAddDiagonal(simptr sim,struct filamentprecondstruct *p,int node,int axis,double k) {
 struct filamentstericstruct *w=sim->filss->steric;int root=w->nodes[node].parent,d;
 if(axis>=0) p->stiffness[sim->dim*root+axis]+=k;
 else for(d=0;d<sim->dim;d++) p->stiffness[sim->dim*root+d]+=k;
}

/* Local estimates for effects not put in the band. Active rectangular walls
   use their true normal stiffness; a clamped wall's global height response is
   left to the full residual. Contact row bounds are already available from the
   accepted residual, so this adds no segment-distance searches. */
static void filPrecondOther(simptr sim,struct filamentprecondstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int f,j,d,ps,b,s;
 for(f=0;f<w->nfilament;f++) {
  FilStericFilament *record=&w->filaments[f];filamentptr fil=record->fil;filamenttypeptr t=fil->filtype;
  int offset=record->nodeoffset;
  if(offset<0 || t->dynamics==FDnone) continue;
  if(t->confinesrf && t->confineforce>0) for(ps=0;ps<PSMAX;ps++) for(b=0;b<t->confinesrf->npanel[ps];b++) {
   panelptr pnl=t->confinesrf->panels[ps][b];
   if(ps==PSrect) {
    double sign=(t->confineface==PFfront?1:-1)*pnl->front[0],position;
    d=(int)pnl->front[1];position=pnl->forceclamp && pnl->forceclamp->intrial?pnl->forceclamp->trialposition:pnl->point[0][d];
    for(j=0;j<=fil->nseg;j++) if(sign*(fil->nodes[j][d]-position)<t->stericradius)
     filPrecondAddDiagonal(sim,p,offset+j,d,t->confineforce);
   } else for(j=0;j<=fil->nseg;j++) filPrecondAddDiagonal(sim,p,offset+j,-1,t->confineforce);
  }
 }
 if(filStericEnabled(sim)) for(s=0;s<w->nsegment;s++) {
  FilStericSegment *segment=&w->segments[s];int offset=segment->fil->stericnodeoffset;
  if(offset<0 || segment->fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<2;j++) filPrecondAddDiagonal(sim,p,offset+segment->index+j,-1,0.5*segment->contactrow[j]);
 }
}

static int filPrecondFactor(simptr sim,int f,struct filamentprecondstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;FilStericFilament *record=&w->filaments[f];
 int offset=record->nodeoffset,dim=sim->dim,width=p->width,n,i,j,k,first,root;
 double *a,value,si,sj;
 if(offset<0 || record->fil->filtype->dynamics==FDnone) return 0;
 n=dim*(record->fil->nseg+1);a=p->factor+(size_t)dim*offset*width;
 for(i=0;i<n;i++) {
  root=w->nodes[offset+i/dim].parent;si=p->scale[root];
  for(j=i-width+1>0?i-width+1:0;j<i;j++) {
   sj=p->scale[w->nodes[offset+j/dim].parent];a[(size_t)i*width+i-j]*=sim->dt*si*sj;
  }
  a[(size_t)i*width]=1+sim->dt*si*si*p->stiffness[dim*root+i%dim];
 }
 /* Cholesky of I+h*sqrt(M)*K*sqrt(M). Every retained elastic term is a
    positive outer product and other-filament diagonal contributions are
    nonnegative, including shared nodes. Fixed variables become identity. */
 for(i=0;i<n;i++) {
  first=i-width+1>0?i-width+1:0;
  for(j=first;j<i;j++) {
   value=a[(size_t)i*width+i-j];
   for(k=first;k<j;k++) value-=a[(size_t)i*width+i-k]*a[(size_t)j*width+j-k];
   a[(size_t)i*width+i-j]=value/a[(size_t)j*width];
  }
  value=a[(size_t)i*width];
  for(j=first;j<i;j++) value-=a[(size_t)i*width+i-j]*a[(size_t)i*width+i-j];
  if(!(value>0) || !isfinite(value)) return 1;
  a[(size_t)i*width]=sqrt(value);
 }
 return 0;
}

static void filPrecondAssembleDiagonal(simptr sim,struct filamentprecondstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int dim=sim->dim,f,i,d,j,root;
 for(i=0;i<dim*w->nnode;i++) p->stiffness[i]=0;
 /* Ascending node order makes assembly/gather independent of worker count. */
 for(f=0;f<w->nfilament;f++) {
  FilStericFilament *record=&w->filaments[f];
  if(record->nodeoffset<0 || record->fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<=record->fil->nseg;j++) {
   i=record->nodeoffset+j;root=w->nodes[i].parent;p->weight[root]+=1;
   for(d=0;d<dim;d++) p->stiffness[dim*root+d]+=p->factor[(size_t)(dim*i+d)*p->width];
  }
 }
 for(i=0;i<w->nnode;i++) if(p->weight[i]>0) p->weight[i]=1/sqrt(p->weight[i]);
 filPrecondOther(sim,p);
}

int filPrecondBuild(simptr sim,struct filamentprecondstruct **work,const double *diagonal) {
 struct filamentstericstruct *w=sim->filss->steric;struct filamentprecondstruct *p=*work;
 int dim=sim->dim,f,i,er=0;size_t count;double *s;
 if(!p) {p=(struct filamentprecondstruct*)calloc(1,sizeof(*p));if(!p) return 1;*work=p;}
 p->width=3*dim;p->diagonal=diagonal;p->workers=w->workersused>0?w->workersused:1;
 if((size_t)w->nnode>SIZE_MAX/((dim*p->width+2*dim+2)*sizeof(double))) return 1;
 count=(size_t)w->nnode*(dim*p->width+2*dim+2);
 if(count>p->capacity) {s=(double*)realloc(p->storage,count*sizeof(double));if(!s) return 1;p->storage=s;p->capacity=count;}
 s=p->storage;p->factor=s;s+=(size_t)dim*w->nnode*p->width;p->rhs=s;s+=(size_t)dim*w->nnode;
 p->stiffness=s;s+=(size_t)dim*w->nnode;p->scale=s;s+=w->nnode;p->weight=s;
 for(i=0;i<w->nnode;i++) {p->scale[i]=w->nodes[i].fixed || w->nodes[i].parent!=i?0:1/sqrt(w->nodes[i].drag);p->weight[i]=0;}
#ifdef _OPENMP
 if(p->workers>1) {
  int slot;filWorkSchedule(sim,p->workers);
#pragma omp parallel num_threads(p->workers) shared(er)
  {
#pragma omp for schedule(static,1)
   for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext) filPrecondElastic(sim,next,p);
#pragma omp single
   filPrecondAssembleDiagonal(sim,p);
#pragma omp for schedule(static,1) reduction(|:er)
   for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext) er|=filPrecondFactor(sim,next,p);
  }
  return er;
 }
#endif
 for(f=0;f<w->nfilament;f++) filPrecondElastic(sim,f,p);
 filPrecondAssembleDiagonal(sim,p);
 for(f=0;f<w->nfilament;f++) er|=filPrecondFactor(sim,f,p);
 return er;
}

static void filPrecondSolve(simptr sim,int f,struct filamentprecondstruct *p,const double *in) {
 struct filamentstericstruct *w=sim->filss->steric;FilStericFilament *record=&w->filaments[f];
 int offset=record->nodeoffset,dim=sim->dim,width=p->width,n,i,j,root;double *rhs,*a;
 if(offset<0 || record->fil->filtype->dynamics==FDnone) return;
 n=dim*(record->fil->nseg+1);rhs=p->rhs+dim*offset;a=p->factor+(size_t)dim*offset*width;
 for(i=0;i<n;i++) {
  root=w->nodes[offset+i/dim].parent;
  rhs[i]=p->scale[root]>0?in[dim*root+i%dim]*p->weight[root]/p->scale[root]:0;
  for(j=i-width+1>0?i-width+1:0;j<i;j++) rhs[i]-=a[(size_t)i*width+i-j]*rhs[j];
  rhs[i]/=a[(size_t)i*width];
 }
 for(i=n-1;i>=0;i--) {
  for(j=i+1;j<n && j<i+width;j++) rhs[i]-=a[(size_t)j*width+j-i]*rhs[j];
  rhs[i]/=a[(size_t)i*width];
 }
}

static void filPrecondGather(simptr sim,int i,struct filamentprecondstruct *p,const double *in,double *out) {
 struct filamentstericstruct *w=sim->filss->steric;int j,d,dim=sim->dim;
 for(d=0;d<dim;d++) {
  if(w->nodes[i].parent==i && p->scale[i]>0) {
   double sum=0;
   for(j=w->memberfirst[i];j>=0;j=w->membernext[j]) if(w->nodes[j].fil->filtype->dynamics!=FDnone) sum+=p->rhs[dim*j+d];
   out[dim*i+d]=p->scale[i]*p->weight[i]*sum;
  } else out[dim*i+d]=in[dim*i+d]/p->diagonal[dim*i+d];
 }
 if(dim==3) out[dim*w->nnode+i]=in[dim*w->nnode+i]/p->diagonal[dim*w->nnode+i];
}

void filPrecondApply(simptr sim,struct filamentprecondstruct *p,const double *in,double *out) {
 struct filamentstericstruct *w=sim->filss->steric;int f,i;
 /* in and out must not alias: different blocks read the same shared roots.
    Reuse the author's length-balanced mechanical schedule and one worker team.
    A barrier completes all block solves before the ordered shared-node gather. */
#ifdef _OPENMP
 if(p->workers>1) {
  int slot;
#pragma omp parallel num_threads(p->workers)
  {
#pragma omp for schedule(static,1)
   for(slot=0;slot<p->workers;slot++) for(int next=w->workfirst[slot];next>=0;next=w->filaments[next].worknext)
    filPrecondSolve(sim,next,p,in);
#pragma omp for schedule(static)
   for(i=0;i<w->nnode;i++) filPrecondGather(sim,i,p,in,out);
  }
  return;
 }
#endif
 for(f=0;f<w->nfilament;f++) filPrecondSolve(sim,f,p,in);
 for(i=0;i<w->nnode;i++) filPrecondGather(sim,i,p,in,out);
}
