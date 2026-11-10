/* Fixed-length network segments. LGPL, as the surrounding Smoldyn sources.
   Uses upstream's force_length<0 and SHAKE convention: corrections follow
   start-of-step gradients, never the endpoint gradients. Extends G M G0^T
   to the existing shared-node roots and their summed physical drag.
   Geometry is projected inside every implicit force trial, before contacts
   and the clamp, rather than correcting an already accepted force balance. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"

typedef struct fillengthedge {
 filamentptr fil;
 int segment,a,b,next[2],leaf,parent;
 double g0[3],g[3],error,pivot,lower[3],upper[3],solution;
 double ppivot,plower[3],pupper[3];
} FilLengthEdge;
typedef struct fillengthnode {
 double mobility,q[9],rhs[3],dual[3];
 int head,degree;
} FilLengthNode;
struct filamentconstraintstruct {
 FilLengthEdge *edge;
 FilLengthNode *node;
 int *queue,*order,*mode,n,maxedge,maxnode,maxtype,ntype,nnode,forest,limit,factored,pfactored,frozen;
 const double *diagonal;
 double *precwork;size_t maxprec;
 double *bicg;int maxbicg;
 unsigned long long topology,builds,projections,corrections,cyclesolves,fallbacks,lineariterations;
 double maxrelative,maxratio,norm;
};

int filLengthEnabled(const simptr sim) {
 int ft,f;if(!sim || !sim->filss) return 0;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];
  if(t->klen<0 && t->dynamics!=FDnone) {
   if(!t->elongmodel) return 1;
   for(f=0;f<t->nfil;f++) if(t->fillist[f]->nseg>1 || (t->fillist[f]->nseg && t->fillist[f]->capped)) return 1;
  }
 }
 return 0;
}

void filLengthFree(struct filamentconstraintstruct *p) {
 if(!p) return;
 free(p->edge);free(p->node);free(p->queue);free(p->order);free(p->mode);free(p->bicg);free(p->precwork);free(p);
}

/* Length constraints are edges on physical roots, not on duplicate branch
   records. A leaf elimination ordering is cached with the existing movement
   topology. It is linear-time for trees, including high-degree branch points. */
static void filLengthOrder(struct filamentconstraintstruct *p) {
 int i,side,head=0,tail=0,count=0,link,e,a,b;
 for(i=0;i<p->nnode;i++) {p->node[i].head=-1;p->node[i].degree=0;}
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[i];row->leaf=-1;
  for(side=0;side<2;side++) {
   a=side?row->b:row->a;row->next[side]=p->node[a].head;p->node[a].head=2*i+side;p->node[a].degree++;
  }
 }
 for(i=0;i<p->nnode;i++) if(p->node[i].degree==1) p->queue[tail++]=i;
 while(head<tail) {
  a=p->queue[head++];if(p->node[a].degree!=1) continue;
  link=p->node[a].head;
  while(link>=0 && p->edge[link/2].leaf>=0) link=p->edge[link/2].next[link%2];
  if(link<0) continue;
  e=link/2;b=p->edge[e].a==a?p->edge[e].b:p->edge[e].a;
  p->edge[e].leaf=a;p->edge[e].parent=b;p->order[count++]=e;
  p->node[a].degree=0;p->node[b].degree--;
  if(p->node[b].degree==1) p->queue[tail++]=b;
 }
 p->forest=count==p->n;
}

int filLengthPrepare(simptr sim,struct filamentconstraintstruct **work) {
 struct filamentstericstruct *w=sim->filss->steric;struct filamentconstraintstruct *p=*work;
 int ft,f,j,i,d,n=0,rebuild=0,hybrid=0;double len;void *buffer;
 for(f=0;f<w->nfilament;f++) if(w->filaments[f].fil->filtype->klen<0 && w->filaments[f].fil->filtype->dynamics!=FDnone) {
  if(w->filaments[f].fil->nseg>INT_MAX-n) return 1;
  n+=w->filaments[f].fil->nseg-(w->filaments[f].fil->filtype->elongmodel && !w->filaments[f].fil->capped && w->filaments[f].fil->nseg>0?1:0);
 }
 if(!n) {if(p) p->n=0;return 0;}
 if(!p) {p=(struct filamentconstraintstruct*)calloc(1,sizeof(*p));if(!p) return 1;*work=p;rebuild=1;}
 if(n>INT_MAX/2 || (size_t)n>SIZE_MAX/sizeof(FilLengthEdge) || (size_t)w->nnode>SIZE_MAX/sizeof(FilLengthNode)) return 1;
 if(n>p->maxedge) {
  buffer=realloc(p->edge,(size_t)n*sizeof(*p->edge));if(!buffer) return 1;p->edge=(FilLengthEdge*)buffer;
  buffer=realloc(p->order,(size_t)n*sizeof(*p->order));if(!buffer) return 1;p->order=(int*)buffer;p->maxedge=n;rebuild=1;
 }
 if(w->nnode>p->maxnode) {
  buffer=realloc(p->node,(size_t)w->nnode*sizeof(*p->node));if(!buffer) return 1;p->node=(FilLengthNode*)buffer;
  buffer=realloc(p->queue,(size_t)w->nnode*sizeof(*p->queue));if(!buffer) return 1;p->queue=(int*)buffer;p->maxnode=w->nnode;rebuild=1;
 }
 if(sim->filss->ntype>p->maxtype) {
  buffer=realloc(p->mode,(size_t)sim->filss->ntype*sizeof(*p->mode));if(!buffer) return 1;p->mode=(int*)buffer;p->maxtype=sim->filss->ntype;rebuild=1;
 }
 if(p->n!=n || p->nnode!=w->nnode || p->ntype!=sim->filss->ntype || p->topology!=w->movementrebuilds) rebuild=1;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];int mode=t->klen<0 && t->dynamics!=FDnone;
  if(mode && t->elongmodel) hybrid=1;
  if(!rebuild && p->mode[ft]!=mode) rebuild=1;
  p->mode[ft]=mode;
 }
 p->n=n;p->nnode=w->nnode;p->ntype=sim->filss->ntype;p->limit=1;
 /* Capping one growing tip while uncapping another can preserve the total
    edge count but change edge identities. Check the cached membership rather
    than using count/topology alone; branch-node ownership remains unchanged. */
 if(!rebuild && hybrid) for(f=0,i=0;f<w->nfilament && !rebuild;f++) {
  filamentptr fil=w->filaments[f].fil;
  if(fil->filtype->klen>=0 || fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<fil->nseg;j++) if(filSegmentIsConstrained(fil,j)) {
   if(i>=n || p->edge[i].fil!=fil || p->edge[i].segment!=j) {rebuild=1;break;}
   i++;
  }
 }
 if(rebuild) {
  for(f=0,i=0;f<w->nfilament;f++) {
   FilStericFilament *record=&w->filaments[f];filamentptr fil=record->fil;
   if(fil->filtype->klen>=0 || fil->filtype->dynamics==FDnone) continue;
   for(j=0;j<fil->nseg;j++,i++) {
    if(!filSegmentIsConstrained(fil,j)) {i--;continue;}
    FilLengthEdge *row=&p->edge[i];row->fil=fil;row->segment=j;
    row->a=w->nodes[record->nodeoffset+j].parent;row->b=w->nodes[record->nodeoffset+j+1].parent;
   }
  }
  filLengthOrder(p);p->topology=w->movementrebuilds;p->builds++;
 }
 for(i=0;i<w->nnode;i++) p->node[i].mobility=w->nodes[i].parent==i && !w->nodes[i].fixed?1/w->nodes[i].drag:0;
 for(i=0;i<n;i++) {
  FilLengthEdge *row=&p->edge[i];double *a=w->nodes[row->a].fil->nodes[w->nodes[row->a].node],*b=w->nodes[row->b].fil->nodes[w->nodes[row->b].node];
  len=0;for(d=0;d<sim->dim;d++) {row->g0[d]=b[d]-a[d];len+=row->g0[d]*row->g0[d];}
  if(!(len>0) || !isfinite(len)) return 1;
  len=sqrt(len);for(d=0;d<sim->dim;d++) row->g0[d]/=len;
  if(row->fil->filtype->constraintiterations>p->limit) p->limit=row->fil->filtype->constraintiterations;
 }
 return 0;
}

static int filLengthMeasure(simptr sim,struct filamentconstraintstruct *p) {
 int i,d;struct filamentstericstruct *w=sim->filss->steric;p->maxrelative=p->maxratio=p->norm=0;
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[i];double len=0,L=row->fil->filtype->elongmodel?filSegmentRestLength(row->fil,row->segment):row->fil->filtype->stdlen,relative;
  double *a=w->nodes[row->a].fil->nodes[w->nodes[row->a].node],*b=w->nodes[row->b].fil->nodes[w->nodes[row->b].node];
  for(d=0;d<sim->dim;d++) {row->g[d]=b[d]-a[d];len+=row->g[d]*row->g[d];}
  if(!(len>0) || !isfinite(len)) return 1;
  len=sqrt(len);row->error=len-L;relative=fabs(row->error)/L;
  p->maxrelative=fmax(p->maxrelative,relative);p->maxratio=fmax(p->maxratio,relative/row->fil->filtype->constrainttol);
  p->norm+=relative*relative;
  for(d=0;d<sim->dim;d++) row->g[d]/=len;
 }
 return 0;
}

/* Eliminate a leaf edge of G M G0^T. The remaining interactions at its parent
   are represented by a dim*dim matrix Q. A rank-one update of Q accounts for
   all remaining incident edges; no junction clique or dense matrix is stored.
   Distinct left/current and right/start gradients make this a nonsymmetric
   elimination, rather than a Cholesky approximation of the SHAKE matrix. */
static int filLengthFactor(simptr sim,struct filamentconstraintstruct *p,int precondition) {
 int i,j,k,dim=sim->dim;
 for(i=0;i<p->nnode;i++) {
  FilLengthNode *node=&p->node[i];
  for(j=0;j<9;j++) node->q[j]=0;
  for(j=0;j<dim;j++) node->q[3*j+j]=node->mobility/(precondition?p->diagonal[dim*i]:1);
 }
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[p->order[i]];FilLengthNode *leaf=&p->node[row->leaf],*parent=&p->node[row->parent];
  double sign=row->leaf==row->a?-1:1,l[3],r[3],qr[3]={0,0,0},ql[3]={0,0,0},pivot=0;
  for(j=0;j<dim;j++) {l[j]=-sign*row->g[j];r[j]=-sign*row->g0[j];}
  for(j=0;j<dim;j++) for(k=0;k<dim;k++) {
   qr[j]+=parent->q[3*j+k]*r[k];ql[j]+=parent->q[3*k+j]*l[k];
   pivot+=l[j]*(parent->q[3*j+k]+leaf->q[3*j+k])*r[k];
  }
  if(!isfinite(pivot) || fabs(pivot)<128*DBL_EPSILON*(p->node[row->a].mobility+p->node[row->b].mobility)) return 1;
  if(pivot==0) return 1;
  if(precondition) row->ppivot=pivot;else row->pivot=pivot;
  for(j=0;j<dim;j++) {
   if(precondition) {row->plower[j]=qr[j]/pivot;row->pupper[j]=ql[j]/pivot;}
   else {row->lower[j]=qr[j]/pivot;row->upper[j]=ql[j]/pivot;}
  }
  for(j=0;j<dim;j++) {
   for(k=0;k<dim;k++) parent->q[3*j+k]-=qr[j]*ql[k]/pivot;
  }
 }
 return 0;
}

static void filLengthTreeSolve(simptr sim,struct filamentconstraintstruct *p,int precondition) {
 int i,d,dim=sim->dim;
 for(i=0;i<p->nnode;i++) for(d=0;d<dim;d++) p->node[i].rhs[d]=p->node[i].dual[d]=0;
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[p->order[i]];double sign=row->leaf==row->a?1:-1,b=row->error;
  for(d=0;d<dim;d++) b+=sign*row->g[d]*(p->node[row->parent].rhs[d]-p->node[row->leaf].rhs[d]);
  row->solution=b/(precondition?row->ppivot:row->pivot);
  for(d=0;d<dim;d++) p->node[row->parent].rhs[d]-=(precondition?row->plower[d]:row->lower[d])*b;
 }
 for(i=p->n-1;i>=0;i--) {
  FilLengthEdge *row=&p->edge[p->order[i]];double value=row->solution;
  for(d=0;d<dim;d++) value-=(precondition?row->pupper[d]:row->upper[d])*p->node[row->parent].dual[d];
  row->solution=value;
  for(d=0;d<dim;d++) {p->node[row->a].dual[d]-=row->g0[d]*value;p->node[row->b].dual[d]+=row->g0[d]*value;}
 }
}

static double filLengthDot(const double *a,const double *b,int n) {
 int i;double sum=0;for(i=0;i<n;i++) sum+=a[i]*b[i];return sum;
}
static void filLengthMultiply(simptr sim,struct filamentconstraintstruct *p,const double *x,double *y) {
 int i,d,dim=sim->dim;
 for(i=0;i<p->nnode;i++) for(d=0;d<dim;d++) p->node[i].dual[d]=0;
 for(i=0;i<p->n;i++) for(d=0;d<dim;d++) {p->node[p->edge[i].a].dual[d]-=p->edge[i].g0[d]*x[i];p->node[p->edge[i].b].dual[d]+=p->edge[i].g0[d]*x[i];}
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[i];y[i]=0;
  for(d=0;d<dim;d++) y[i]+=row->g[d]*(p->node[row->b].mobility*p->node[row->b].dual[d]-p->node[row->a].mobility*p->node[row->a].dual[d]);
 }
}

/* Matrix-free BiCGSTAB fallback for cycles or singular leaf pivots. Upstream
   also uses BiCGSTAB for G M G0^T. This path never assumes independent chains. */
static int filLengthBiCG(simptr sim,struct filamentconstraintstruct *p) {
 double *x,*r,*r0,*v,*z,*s,*t,*ph,*sh,rho=1,alpha=1,omega=1,next,beta,norm,target,den,tt;
 int i,j,n=p->n,er=1;void *buffer;
 if(n>p->maxbicg) {if((size_t)n>SIZE_MAX/(9*sizeof(double))) return 1;buffer=realloc(p->bicg,(size_t)n*9*sizeof(double));if(!buffer) return 1;p->bicg=(double*)buffer;p->maxbicg=n;}
 x=p->bicg;r=x+n;r0=r+n;v=r0+n;z=v+n;s=z+n;t=s+n;ph=t+n;sh=ph+n;
 for(i=0;i<n;i++) {x[i]=v[i]=z[i]=0;r[i]=r0[i]=p->edge[i].error;}
 norm=filLengthDot(r,r,n);target=fmax(DBL_MIN,norm*1e-20);p->cyclesolves++;
 for(j=0;j<1000;j++) {
  if(filLengthDot(r,r,n)<=target) {er=0;break;}
  next=filLengthDot(r0,r,n);if(next==0 || omega==0 || !isfinite(next)) break;
  beta=(next/rho)*(alpha/omega);rho=next;
  for(i=0;i<n;i++) {
   double diagonal=0;int d;FilLengthEdge *row=&p->edge[i];
   for(d=0;d<sim->dim;d++) diagonal+=row->g[d]*row->g0[d];
   diagonal*=(p->node[row->a].mobility+p->node[row->b].mobility);row->pivot=diagonal!=0?diagonal:1;
   z[i]=r[i]+beta*(z[i]-omega*v[i]);ph[i]=z[i]/row->pivot;
  }
  filLengthMultiply(sim,p,ph,v);den=filLengthDot(r0,v,n);if(den==0 || !isfinite(den)) break;
  alpha=rho/den;
  for(i=0;i<n;i++) {s[i]=r[i]-alpha*v[i];x[i]+=alpha*ph[i];}
  if(filLengthDot(s,s,n)<=target) {er=0;break;}
  for(i=0;i<n;i++) sh[i]=s[i]/p->edge[i].pivot;
  filLengthMultiply(sim,p,sh,t);tt=filLengthDot(t,t,n);if(tt==0 || !isfinite(tt)) break;
  omega=filLengthDot(t,s,n)/tt;
  for(i=0;i<n;i++) {x[i]+=omega*sh[i];r[i]=s[i]-omega*t[i];}
 }
 p->lineariterations+=j+1;
 if(er) return 1;
 for(i=0;i<n;i++) p->edge[i].solution=x[i];
 /* Reinstall the complete correction field, not the last Krylov product. */
 filLengthMultiply(sim,p,x,v);return 0;
}

int filLengthProject(simptr sim,struct filamentconstraintstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int iter,i,d,f,j,search;double alpha,norm;
 if(!p || !p->n) return 0;
 p->frozen=0;
 p->projections++;
 for(iter=0;iter<=p->limit;iter++) {
  if(filLengthMeasure(sim,p)) return 1;
  if(p->maxratio<=1) return 0;
  if(iter==p->limit) return 1;
  if(!p->forest || filLengthFactor(sim,p,0)) {
   if(p->forest) p->fallbacks++;
   if(filLengthBiCG(sim,p)) return 1;
  } else filLengthTreeSolve(sim,p,0);
  norm=p->norm;
  for(i=0;i<w->nnode;i++) if(w->nodes[i].parent==i) for(d=0;d<sim->dim;d++) {
   double *position=w->nodes[i].fil->nodes[w->nodes[i].node];
   w->nodes[i].position[d]=position[d];
  }
  /* Damping keeps a large trial from increasing geometric constraint error.
     Every correction still lies in M G0^T, as required by upstream SHAKE. */
  for(search=0,alpha=1;search<24;search++,alpha*=0.5) {
   for(f=0;f<w->nfilament;f++) {
    FilStericFilament *record=&w->filaments[f];filamentptr fil=record->fil;
    if(record->nodeoffset<0 || fil->filtype->dynamics==FDnone) continue;
    for(j=0;j<=fil->nseg;j++) for(d=0;d<sim->dim;d++) {
     int root=w->nodes[record->nodeoffset+j].parent;
     fil->nodes[j][d]=w->nodes[root].position[d]-alpha*p->node[root].mobility*p->node[root].dual[d];
    }
   }
   if(!filLengthMeasure(sim,p) && (p->maxratio<=1 || p->norm<=(1-1e-4*alpha)*norm)) break;
  }
  if(search==24) return 1;
  p->corrections++;
 }
 return 1;
}

int filLengthSatisfied(struct filamentconstraintstruct *p) {return !p || !p->n || p->maxratio<=1;}

/* Derivative of SHAKE retraction at the accepted Newton geometry:
   dq = [I-M G0^T (G M G0^T)^-1 G] du. Freeze once per Newton correction;
   Krylov probes then use one cached linear solve, not nested nonlinear SHAKE.
   Only derivative probes use this tangent; accepted trials project fully. */
int filLengthFreeze(simptr sim,struct filamentconstraintstruct *p,const double *diagonal) {
 if(!p || !p->n) return 1;
 if(filLengthMeasure(sim,p) || p->maxratio>1) return 1;
 p->diagonal=diagonal;
 p->factored=p->forest && !filLengthFactor(sim,p,0);
 p->pfactored=p->forest && !filLengthFactor(sim,p,1);p->frozen=1;return 0;
}

int filLengthTangent(simptr sim,struct filamentconstraintstruct *p,const double *in,double *out) {
 int i,d,dim=sim->dim,n=sim->filss->steric->nnode*(dim==3?4:2);
 memcpy(out,in,(size_t)n*sizeof(double));
 if(!p || !p->n) return 0;
 if(!p->frozen) return 1;
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[i];row->error=0;
  for(d=0;d<dim;d++) row->error+=row->g[d]*((p->node[row->b].mobility>0?in[dim*row->b+d]:0)-(p->node[row->a].mobility>0?in[dim*row->a+d]:0));
 }
 if(p->factored) filLengthTreeSolve(sim,p,0);else if(filLengthBiCG(sim,p)) return 1;
 for(i=0;i<p->nnode;i++) if(p->node[i].mobility>0) for(d=0;d<dim;d++) out[dim*i+d]-=p->node[i].mobility*p->node[i].dual[d];
 return 0;
}

/* Inverse of B P+(I-P) for diagonal B, where P is the SHAKE tangent.
   The normal free-proposal directions have identity response, not B.
   For optional banded B^-1 use the same Schur approximation and two primal
   applications. Thus this preconditions constrained mechanics, not soft rods. */
int filLengthPrecondition(simptr sim,struct filamentconstraintstruct *p,struct filamentprecondstruct *band,const double *in,double *out) {
 int i,d,dim=sim->dim,n=p->nnode*(dim==3?4:2);double *q,*bq;
 if(band) filPrecondApply(sim,band,in,out);else for(i=0;i<n;i++) out[i]=in[i]/p->diagonal[i];
 if(!p->pfactored) return 0;
 if(band && (size_t)n>p->maxprec) {
  double *buffer;if((size_t)n>SIZE_MAX/(2*sizeof(double))) return 1;
  buffer=(double*)realloc(p->precwork,(size_t)n*2*sizeof(double));if(!buffer) return 1;p->precwork=buffer;p->maxprec=n;
 }
 for(i=0;i<p->n;i++) {
  FilLengthEdge *row=&p->edge[i];row->error=0;
  for(d=0;d<dim;d++) row->error+=row->g[d]*((p->node[row->b].mobility>0?out[dim*row->b+d]:0)-(p->node[row->a].mobility>0?out[dim*row->a+d]:0));
 }
 filLengthTreeSolve(sim,p,1);
 if(!band) {
  for(i=0;i<p->nnode;i++) for(d=0;d<dim;d++) out[dim*i+d]+=(1-1/p->diagonal[dim*i+d])*p->node[i].mobility*p->node[i].dual[d];
 } else {
  q=p->precwork;bq=q+n;for(i=0;i<n;i++) q[i]=0;
  for(i=0;i<p->nnode;i++) for(d=0;d<dim;d++) q[dim*i+d]=p->node[i].mobility*p->node[i].dual[d];
  filPrecondApply(sim,band,q,bq);for(i=0;i<n;i++) out[i]+=q[i]-bq[i];
 }
 return 0;
}

void filLengthReport(simptr sim,struct filamentconstraintstruct *p) {
 if(!p || !p->n) return;
 filLengthMeasure(sim,p);
 simLog(sim,2,"  fixed lengths: %i segments, %llu trial projections, %llu geometric corrections, %llu topology builds; max relative error %g (tolerance ratio %g)\n",p->n,p->projections,p->corrections,p->builds,p->maxrelative,p->maxratio);
 simLog(sim,2,"  constraint linear solves: %s, %llu BiCGSTAB solves, %llu singular-pivot fallbacks, %llu Krylov iterations\n",p->forest?"forest elimination":"general sparse",p->cyclesolves,p->fallbacks,p->lineariterations);
}
