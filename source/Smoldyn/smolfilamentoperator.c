/* Optional cached force derivative operator. LGPL as the surrounding sources.
   Linearizes the ORIGINAL finite-angle/material-frame forces. Frame derivatives
   propagate along each filament; no assumption of a globally banded Jacobian.
   Contact features and wall active sets are frozen only within one GMRES solve.
   Nonlinear forces/constraints/clamp acceptance remain in smolfilamentimplicit.c. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <limits.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "Sphere.h"
#ifdef _OPENMP
#include <omp.h>
#endif

void filBendTorque(const filamentptr fil,int node,double *torque);
int filJunctionGeometry(const filamentptr mother,int spot,const filamentptr daughter,int dim,double *tm,double *td,double *lm,double *ld,double *c,double *s);
int filBranchDazim(const filamentptr mother,int br,double *dphi);

typedef struct filopsegment {
 double t[3],y[3],z[3],gyaw[3],gpitch[3],paxis[3],tau[3],dtau[9],invlen;
} FilOpSegment;
typedef struct filopnode {double force[3],omega[3],delta[3],torque;int parentbranch;} FilOpNode;
typedef struct filopbranch {
 int mother,daughter,valid;
 double c,s,sd,sm,cd,cm,az,ap,ac;
} FilOpBranch;
typedef struct filopcontact {
 int node[4],freeS,freeT;
 double u[3],v[3],r[3],normal[3],s,t,k,delta,dist,a,b,c;
 double ds,dt,df[3];
} FilOpContact;
typedef struct filopwall {int node,axis,clamp;double k;} FilOpWall;
typedef struct filopclamp {panelptr panel;double k,delta;} FilOpClamp;
struct filamentoperatorstruct {
 FilOpSegment *segment;FilOpNode *node;FilOpBranch *branch;
 FilOpContact *contact;FilOpWall *wall;FilOpClamp *clamp;
 size_t maxsegment,maxnode,maxbranch,maxcontact,maxwall,maxclamp;
 int nnode,nbranch,ncontact,nwall,nclamp,workers;
 unsigned long long builds,products;
};

static double opDot(const double *a,const double *b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void opCross(const double *a,const double *b,double *c) {
 c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];
}
static void *opGrow(void *buffer,size_t *capacity,size_t count,size_t item) {
 size_t next;void *p;if(count<=*capacity) return buffer;
 if(count>SIZE_MAX-count/2-16) return NULL;
 next=count+count/2+16;if(next>SIZE_MAX/item) return NULL;
 p=realloc(buffer,next*item);if(!p) return NULL;*capacity=next;return p;
}
/* Assign through the typed owner, without type-punning a T** as void**. */
#define FIL_OP_GROW(buffer,capacity,count) do { \
 size_t requested=(size_t)(count);void *grown=opGrow(buffer,&(capacity),requested,sizeof(*(buffer))); \
 if(requested && !grown) return 1; \
 (buffer)=grown; \
} while(0)
void filOperatorFree(struct filamentoperatorstruct *p) {
 if(!p) return;
 free(p->segment);free(p->node);free(p->branch);free(p->contact);free(p->wall);free(p->clamp);free(p);
}
static void opDirection(simptr sim,const double *v,int node,double *out) {
 struct filamentstericstruct *w=sim->filss->steric;int d,root=w->nodes[node].parent;
 for(d=0;d<3;d++) out[d]=d<sim->dim && !w->nodes[root].fixed?v[sim->dim*root+d]:0;
}
static void opEdgeDirection(struct filamentoperatorstruct *p,int node,double *out) {
 int d;for(d=0;d<3;d++) out[d]=p->node[node+1].delta[d]-p->node[node].delta[d];
}
static void opUnitDerivative(const FilOpSegment *s,const double *v,double *out) {
 int d;double dl=opDot(s->t,v);for(d=0;d<3;d++) out[d]=s->invlen*(v[d]-s->t[d]*dl);
}

/* Cache geometric derivatives and the exact derivative of filBendTorque.
   gyaw/gpitch map motion relative to the previous material frame into Euler
   increments. Propagating angular variations avoids differentiating a dense
   chain of quaternions or rebuilding any physical geometry during products. */
static int opBuildFilament(simptr sim,struct filamentoperatorstruct *p,int f) {
 struct filamentstericstruct *w=sim->filss->steric;FilStericFilament *record=&w->filaments[f];
 filamentptr fil=record->fil;filamenttypeptr type=fil->filtype;int j,d,a,off=record->nodeoffset,dim=sim->dim;
 if(off<0) return 0;
 for(j=0;j<fil->nseg;j++) {
  FilOpSegment *s=&p->segment[off+j];double len=0,unit[3],cp,sp,ct,st,D1,D2;
  memset(s,0,sizeof(*s));
  for(d=0;d<dim;d++) {s->t[d]=fil->nodes[j+1][d]-fil->nodes[j][d];len+=s->t[d]*s->t[d];}
  if(!(len>0) || !isfinite(len)) return 1;
  len=sqrt(len);s->invlen=1/len;for(d=0;d<3;d++) s->t[d]/=len;
  if(dim==2) {s->y[0]=-s->t[1];s->y[1]=s->t[0];s->z[2]=1;for(d=0;d<2;d++) s->gyaw[d]=s->y[d]/len;}
  else {
   for(a=1;a<3;a++) {unit[0]=unit[1]=unit[2]=0;unit[a]=1;Sph_QtniRotate(fil->segments[j]->qabs,unit,a==1?s->y:s->z);}
   if(j) {
    FilOpSegment *prev=s-1;double x=opDot(s->t,prev->t),y=opDot(s->t,prev->y),z=opDot(s->t,prev->z),h2=x*x+y*y;
    if(h2<1e-16) return 1; /* Euler pole: retain residual-difference fallback. */
    cp=cos(fil->segments[j]->ypr[0]);sp=sin(fil->segments[j]->ypr[0]);
    for(d=0;d<3;d++) {
     s->gyaw[d]=(-y*prev->t[d]+x*prev->y[d])/(len*h2);
     s->gpitch[d]=-(prev->z[d]-z*s->t[d])/(len*sqrt(h2));
     s->paxis[d]=-sp*prev->t[d]+cp*prev->y[d];
    }
   } else if(type->dynamics!=FDnone) {
    double axis[3],norm=0,b=opDot(fil->seg0up,s->z);
    if(fabs(b)<1e-10) return 1;
    s->gyaw[0]=opDot(fil->seg0up,s->t)/b;s->gyaw[1]=1/b;
    for(d=0;d<3;d++) {axis[d]=fil->filwork->wnodes[1][1][d]-fil->filwork->wnodes[1][0][d];norm+=axis[d]*axis[d];s->paxis[d]=fil->seg0up[d];}
    if(!(norm>0)) return 1;
    for(d=0;d<3;d++) s->gpitch[d]=axis[d]/sqrt(norm);
   }
  }
  if(!j || type->dynamics==FDnone) continue;
  filBendTorque(fil,j,s->tau);
  if(dim==2) {s->dtau[6]=-type->kypr[0];continue;}
  cp=cos(fil->segments[j]->ypr[0]);sp=sin(fil->segments[j]->ypr[0]);
  ct=cos(fil->segments[j]->ypr[1]);st=sin(fil->segments[j]->ypr[1]);
  D1=type->kypr[1]*(fil->segments[j]->ypr[1]-type->stdypr[1]);D2=type->kypr[2]*(fil->segments[j]->ypr[2]-type->stdypr[2]);
  s->dtau[0]=D2*ct*sp+D1*cp;s->dtau[1]=D2*st*cp+type->kypr[1]*sp;s->dtau[2]=-type->kypr[2]*ct*cp;
  s->dtau[3]=D1*sp-D2*cp;s->dtau[4]=-type->kypr[1]*cp;s->dtau[5]=-type->kypr[2]*sp;
  s->dtau[6]=-type->kypr[0]-D2*sp*st;s->dtau[7]=D2*cp*ct;s->dtau[8]=type->kypr[2]*cp*st;
  {double local[9];FilOpSegment *prev=s-1;memcpy(local,s->dtau,sizeof(local));
   for(d=0;d<3;d++) for(a=0;a<3;a++) s->dtau[3*d+a]=prev->t[d]*local[a]+prev->y[d]*local[3+a]+prev->z[d]*local[6+a];
  }
 }
 return 0;
}
static int opBuildBranches(simptr sim,struct filamentoperatorstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int f,br;
 for(f=0;f<w->nfilament;f++) {
  FilStericFilament *record=&w->filaments[f];filamentptr mother=record->fil;
  for(br=0;br<mother->nbranch;br++) {
   FilOpBranch *b=&p->branch[record->branchstart+br];filamentptr daughter=mother->branches[br];double tm[3],td[3],lm,ld,angle,phi;
   int spot=mother->branchspots[br];memset(b,0,sizeof(*b));
   if(!daughter || daughter->frontend!=mother || !daughter->nseg || spot<0 || spot>=mother->nseg) continue;
   if(!filJunctionGeometry(mother,spot,daughter,sim->dim,tm,td,&lm,&ld,&b->c,&b->s)) continue;
   b->valid=1;b->mother=record->nodeoffset+spot;b->daughter=daughter->stericnodeoffset;
   if(b->daughter<0) return 1;
   p->node[b->daughter].parentbranch=record->branchstart+br;angle=acos(b->c);
   b->sd=daughter->filtype->branchforceangle*(angle-daughter->filtype->branchangle)/(ld*b->s);
   b->sm=mother->filtype->branchforceangle*(angle-mother->filtype->branchangle)/(lm*b->s);
   b->cd=(-daughter->filtype->branchforceangle/ld+b->sd*b->c)/(b->s*b->s);
   b->cm=(-mother->filtype->branchforceangle/lm+b->sm*b->c)/(b->s*b->s);
   if(daughter->filtype->branchforceazimuth>0 && sim->dim==3 && filBranchDazim(mother,br,&phi)) {
    double sb=fmax(b->s,0.1);
    b->ap=-daughter->filtype->branchforceazimuth/(ld*sb*sb);b->az=b->ap*phi;
    b->ac=b->s>0.1?2*b->az*b->c/(b->s*b->s):0;
   }
  }
 }
 return 0;
}
static int opBuildContacts(simptr sim,struct filamentoperatorstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int i,d;
 p->ncontact=0;if(!filStericEnabled(sim)) return 0;
 for(i=0;i<w->npair;i++) {
  FilStericSegment *a=&w->segments[w->pairs[i].a],*b=&w->segments[w->pairs[i].b];
  FilOpContact c;double trimA,trimB,normal[3]={0,0,0},s,t,dist;
  dist=filStericGeometry(a->segment,b->segment,&s,&t,normal);
  if(!(dist<a->radius+b->radius)) continue;
  if(!(dist>1e-12)) return 1;
  trimA=filStericTrim(a->segment,b->segment);trimB=filStericTrim(b->segment,a->segment);
  /* A moving trim bound depends on contour lengths outside the contact.
     Fixed-length tangents make those derivatives zero. For elastic trimmed
     contacts use the complete reference operator rather than omit them. */
  if((trimA>0 || trimB>0) && (a->fil->filtype->klen>=0 || b->fil->filtype->klen>=0)) return 1;
  memset(&c,0,sizeof(c));c.s=s;c.t=t;c.dist=dist;c.delta=a->radius+b->radius-dist;c.k=2/(1/a->stiffness+1/b->stiffness);
  c.node[0]=a->fil->stericnodeoffset+a->index;c.node[1]=c.node[0]+1;c.node[2]=b->fil->stericnodeoffset+b->index;c.node[3]=c.node[2]+1;
  c.freeS=s>trimA+1e-8 && s<1-1e-8;c.freeT=t>trimB+1e-8 && t<1-1e-8;
  for(d=0;d<3;d++) {c.u[d]=a->segment->xyzback[d]-a->segment->xyzfront[d];c.v[d]=b->segment->xyzback[d]-b->segment->xyzfront[d];c.normal[d]=normal[d];c.r[d]=normal[d]*dist;}
  c.a=opDot(c.u,c.u);c.b=opDot(c.u,c.v);c.c=opDot(c.v,c.v);
  if(c.freeS && c.freeT && c.a*c.c-c.b*c.b<1e-12*c.a*c.c) c.freeS=c.freeT=0; /* parallel closest-point gauge; quasi-Newton block */
  FIL_OP_GROW(p->contact,p->maxcontact,(size_t)p->ncontact+1);
  p->contact[p->ncontact++]=c;
 }
 return 0;
}
static int opBuildWalls(simptr sim,struct filamentoperatorstruct *p) {
 struct filamentstericstruct *w=sim->filss->steric;int f,ps,b,j,c,d; p->nwall=p->nclamp=0;
 for(f=0;f<w->nfilament;f++) {
  FilStericFilament *r=&w->filaments[f];filamentptr fil=r->fil;filamenttypeptr t=fil->filtype;
  if(r->nodeoffset<0 || !t->confinesrf || t->confineforce<=0) continue;
  for(ps=0;ps<PSMAX;ps++) for(b=0;b<t->confinesrf->npanel[ps];b++) {
   panelptr pnl=t->confinesrf->panels[ps][b];double sign,position;
   if(ps!=PSrect) return 1;
   d=(int)pnl->front[1];sign=(t->confineface==PFfront?1:-1)*pnl->front[0];
   position=pnl->forceclamp && pnl->forceclamp->intrial?pnl->forceclamp->trialposition:pnl->point[0][d];c=-1;
   if(pnl->forceclamp && !pnl->forceclamp->approaching && !pnl->forceclamp->atstop) {
    for(c=0;c<p->nclamp && p->clamp[c].panel!=pnl;c++) {}
    if(c==p->nclamp) {
     FIL_OP_GROW(p->clamp,p->maxclamp,(size_t)c+1);
     p->clamp[c].panel=pnl;p->clamp[c].k=p->clamp[c].delta=0;p->nclamp++;
    }
   }
   for(j=0;j<=fil->nseg;j++) if(sign*(fil->nodes[j][d]-position)<t->stericradius) {
    FilOpWall *wall;if(p->nwall==INT_MAX) return 1;
    FIL_OP_GROW(p->wall,p->maxwall,(size_t)p->nwall+1);
    wall=&p->wall[p->nwall++];wall->node=r->nodeoffset+j;wall->axis=d;wall->k=t->confineforce;wall->clamp=c;
    if(c>=0) p->clamp[c].k+=wall->k; /* include fixed supports in height response */
   }
  }
 }
 return 0;
}

int filOperatorBuild(simptr sim,struct filamentoperatorstruct **work,int workers) {
 struct filamentstericstruct *w=sim->filss->steric;struct filamentoperatorstruct *p=*work;int i,slot,er=0;
 if(workers<1 || workers>256) return 1;
 if(!p) {p=(struct filamentoperatorstruct*)calloc(1,sizeof(*p));if(!p) return 1;*work=p;}
 FIL_OP_GROW(p->segment,p->maxsegment,w->nnode);
 FIL_OP_GROW(p->node,p->maxnode,w->nnode);
 FIL_OP_GROW(p->branch,p->maxbranch,w->nbranch);
 p->nnode=w->nnode;p->nbranch=w->nbranch;p->workers=workers;p->builds++;
 filWorkSchedule(sim,workers);
 for(i=0;i<w->nnode;i++) p->node[i].parentbranch=-1;
#ifdef _OPENMP
#pragma omp parallel for num_threads(workers) schedule(static,1) reduction(|:er) if(workers>1)
#endif
 for(slot=0;slot<workers;slot++) for(int f=w->workfirst[slot];f>=0;f=w->filaments[f].worknext) er|=opBuildFilament(sim,p,f);
 if(er || opBuildBranches(sim,p) || opBuildContacts(sim,p) || opBuildWalls(sim,p)) return 1;
 return 0;
}

/* Differentiate tau cross t / length, including the changing unit tangent. */
static void opBendForce(const FilOpSegment *s,const double *tau,const double *dtau,const double *ds,double *out) {
 double a[3],b[3],c[3],dl=opDot(s->t,ds);int d;opCross(dtau,s->t,a);opCross(tau,ds,b);opCross(tau,s->t,c);
 for(d=0;d<3;d++) out[d]=s->invlen*a[d]+s->invlen*s->invlen*(b[d]-2*dl*c[d]);
}
static void opApplyFilament(simptr sim,struct filamentoperatorstruct *p,int f,const double *v) {
 struct filamentstericstruct *w=sim->filss->steric;FilStericFilament *r=&w->filaments[f];filamentptr fil=r->fil;filamenttypeptr t=fil->filtype;
 int off=r->nodeoffset,j,d,a,dim=sim->dim;double previous[3]={0,0,0};
 if(off<0) return;
 for(j=0;j<=fil->nseg;j++) {memset(p->node[off+j].force,0,3*sizeof(double));memset(p->node[off+j].omega,0,3*sizeof(double));p->node[off+j].torque=0;opDirection(sim,v,off+j,p->node[off+j].delta);}
 if(t->dynamics==FDnone) return;
 for(j=0;j<fil->nseg;j++) {
  FilOpSegment *s=&p->segment[off+j];FilOpNode *node=&p->node[off+j];
  double ds[3],dt[3],angles[3]={0,0,0},adjusted[3],rot[3],tau[3],fm[3],fp[3],dsm[3],dtm[3];
  opEdgeDirection(p,off+j,ds);opUnitDerivative(s,ds,dt);
  if(!j) {
   if(dim==2) node->omega[2]=opDot(s->gyaw,ds);
   else {
    double du[3],scale;opCross(s->gpitch,s->paxis,du);scale=v[dim*w->nnode+off]/t->stdlen;
    scale=s->gyaw[0]*opDot(s->y,dt)-s->gyaw[1]*opDot(s->y,du)*scale;
    opCross(s->t,dt,node->omega);for(d=0;d<3;d++) node->omega[d]+=s->t[d]*scale;
   }
  } else {
   FilOpSegment *prev=s-1;
   if(dim==2) {angles[0]=opDot(s->gyaw,ds)-previous[2];node->omega[2]=previous[2]+angles[0];}
   else {
    for(d=0;d<3;d++) adjusted[d]=s->t[d]/s->invlen;
    opCross(previous,adjusted,rot);for(d=0;d<3;d++) adjusted[d]=ds[d]-rot[d];
    angles[0]=opDot(s->gyaw,adjusted);angles[1]=opDot(s->gpitch,adjusted);angles[2]=v[dim*w->nnode+off+j]/t->stdlen;
    for(d=0;d<3;d++) node->omega[d]=previous[d]+prev->z[d]*angles[0]+s->paxis[d]*angles[1]+s->t[d]*angles[2];
   }
   opCross(previous,s->tau,rot);
   for(d=0;d<3;d++) {tau[d]=rot[d];for(a=0;a<3;a++) tau[d]+=s->dtau[3*d+a]*angles[a];}
   opEdgeDirection(p,off+j-1,dsm);opUnitDerivative(prev,dsm,dtm);
   opBendForce(prev,s->tau,tau,dsm,fm);opBendForce(s,s->tau,tau,ds,fp);
   for(d=0;d<3;d++) {p->node[off+j-1].force[d]+=fm[d];node->force[d]-=fm[d]+fp[d];p->node[off+j+1].force[d]+=fp[d];}
   if(dim==3) {node->torque+=opDot(tau,s->t)+opDot(s->tau,dt);p->node[off+j-1].torque-=opDot(tau,prev->t)+opDot(s->tau,dtm);}
  }
  if(t->klen>0 || (t->elongmodel && j==fil->nseg-1)) {
   double axial=opDot(s->t,ds),ratio=(t->elongmodel?filSegmentRestLength(fil,j):t->stdlen)*s->invlen,k=t->elongmodel?filSegmentStiffness(fil,j):t->klen;
   for(d=0;d<dim;d++) {double df=-k*((1-ratio)*ds[d]+ratio*axial*s->t[d]);node->force[d]-=df;p->node[off+j+1].force[d]+=df;}
  }
  memcpy(previous,node->omega,3*sizeof(double));
 }
}
/* Own-filament writes, as in filAddJunctionForces. Both passes read completed
   frame variations, so the barrier after opApplyFilament is necessary. */
static void opBranchForce(simptr sim,struct filamentoperatorstruct *p,const FilOpBranch *b,int daughter) {
 FilOpSegment *m=&p->segment[b->mother],*s=&p->segment[b->daughter];
 double dm[3],dd[3],tm[3],td[3],force[3],dscale,dc,dl;int d,node=daughter?b->daughter:b->mother;
 opEdgeDirection(p,b->mother,dm);opEdgeDirection(p,b->daughter,dd);opUnitDerivative(m,dm,tm);opUnitDerivative(s,dd,td);
 dc=opDot(tm,s->t)+opDot(m->t,td);dl=opDot(daughter?s->t:m->t,daughter?dd:dm)*(daughter?s->invlen:m->invlen);
 dscale=(daughter?b->cd:b->cm)*dc-(daughter?b->sd:b->sm)*dl;
 for(d=0;d<3;d++) {
  double vec=daughter?m->t[d]-b->c*s->t[d]:s->t[d]-b->c*m->t[d];
  double dv=daughter?tm[d]-dc*s->t[d]-b->c*td[d]:td[d]-dc*m->t[d]-b->c*tm[d];
  force[d]=dscale*vec+(daughter?b->sd:b->sm)*dv;
 }
 if(daughter && b->ap!=0) {
  double cross[3],a[3],z[3],relative[3],y0=opDot(m->y,s->t),z0=opDot(m->z,s->t),phi;
  opCross(p->node[b->mother].omega,s->t,relative);for(d=0;d<3;d++) relative[d]=td[d]-relative[d];
  phi=(y0*opDot(m->z,relative)-z0*opDot(m->y,relative))/(y0*y0+z0*z0);
  dscale=b->ap*phi+b->ac*dc-b->az*dl;
  opCross(m->t,s->t,cross);opCross(tm,s->t,a);opCross(m->t,td,z);
  for(d=0;d<3;d++) force[d]+=dscale*cross[d]+b->az*(a[d]+z[d]);
 }
 for(d=0;d<sim->dim;d++) {p->node[node].force[d]-=force[d];p->node[node+1].force[d]+=force[d];}
}
static void opApplyBranches(simptr sim,struct filamentoperatorstruct *p,int f) {
 struct filamentstericstruct *w=sim->filss->steric;FilStericFilament *r=&w->filaments[f];int j,b;
 if(r->nodeoffset<0 || r->fil->filtype->dynamics==FDnone) return;
 b=p->node[r->nodeoffset].parentbranch;if(b>=0) opBranchForce(sim,p,&p->branch[b],1);
 for(j=0;j<r->fil->nbranch;j++) {b=r->branchstart+j;if(p->branch[b].valid) opBranchForce(sim,p,&p->branch[b],0);}
}
static void opContactForce(struct filamentoperatorstruct *p,FilOpContact *c) {
 double a[3],b[3],du[3],dv[3],r[3],dr[3],ds,dt,dd,rs,rt;int d;
 memcpy(a,p->node[c->node[0]].delta,sizeof(a));memcpy(b,p->node[c->node[2]].delta,sizeof(b));opEdgeDirection(p,c->node[0],du);opEdgeDirection(p,c->node[2],dv);
 for(d=0;d<3;d++) r[d]=a[d]+c->s*du[d]-b[d]-c->t*dv[d];
 rs=-opDot(c->u,r)-opDot(du,c->r);rt=opDot(c->v,r)+opDot(dv,c->r);
 ds=dt=0;
 if(c->freeS && c->freeT) {double det=c->a*c->c-c->b*c->b;ds=(c->c*rs+c->b*rt)/det;dt=(c->b*rs+c->a*rt)/det;}
 else if(c->freeS) ds=rs/c->a;else if(c->freeT) dt=rt/c->c;
 for(d=0;d<3;d++) dr[d]=r[d]+c->u[d]*ds-c->v[d]*dt;
 dd=opDot(c->normal,dr);c->ds=ds;c->dt=dt;
 for(d=0;d<3;d++) c->df[d]=c->k*(-dd*c->normal[d]+c->delta*(dr[d]-c->normal[d]*dd)/c->dist);
}

int filOperatorApply(simptr sim,struct filamentoperatorstruct *p,const double *direction,double *out) {
 struct filamentstericstruct *w=sim->filss->steric;int slot,i,j,d,dim=sim->dim,workers=p->workers;
 if(p->nnode!=w->nnode) return 1;
 p->products++;
#ifdef _OPENMP
#pragma omp parallel num_threads(workers) if(workers>1)
 {
#pragma omp for schedule(static,1)
  for(slot=0;slot<workers;slot++) for(int f=w->workfirst[slot];f>=0;f=w->filaments[f].worknext) opApplyFilament(sim,p,f,direction);
#pragma omp for schedule(static,1)
  for(slot=0;slot<workers;slot++) for(int f=w->workfirst[slot];f>=0;f=w->filaments[f].worknext) opApplyBranches(sim,p,f);
 }
#else
 (void)workers;(void)slot;for(int f=0;f<w->nfilament;f++) opApplyFilament(sim,p,f,direction);
 for(int f=0;f<w->nfilament;f++) opApplyBranches(sim,p,f);
#endif
 for(i=0;i<p->nclamp;i++) p->clamp[i].delta=0;
 for(i=0;i<p->nwall;i++) {FilOpWall *a=&p->wall[i];double *v=p->node[a->node].delta;if(a->clamp>=0) p->clamp[a->clamp].delta+=a->k*v[a->axis];}
 for(i=0;i<p->nclamp;i++) if(p->clamp[i].k>0) p->clamp[i].delta/=p->clamp[i].k;
 for(i=0;i<p->nwall;i++) {FilOpWall *a=&p->wall[i];double *v=p->node[a->node].delta;if(w->nodes[a->node].fil->filtype->dynamics!=FDnone) p->node[a->node].force[a->axis]-=a->k*(v[a->axis]-(a->clamp>=0?p->clamp[a->clamp].delta:0));}
#ifdef _OPENMP
#pragma omp parallel for num_threads(workers) schedule(static) if(workers>1 && p->ncontact>=4096)
#endif
 for(i=0;i<p->ncontact;i++) opContactForce(p,&p->contact[i]);
 /* Deterministic contact accumulation, matching the existing pair order. */
 for(i=0;i<p->ncontact;i++) {
  FilOpContact *c=&p->contact[i];double coef[4]={1-c->s,c->s,-(1-c->t),-c->t},delta[4]={-c->ds,c->ds,c->dt,-c->dt};
  for(j=0;j<4;j++) if(w->nodes[c->node[j]].fil->filtype->dynamics!=FDnone)
   for(d=0;d<dim;d++) p->node[c->node[j]].force[d]+=coef[j]*c->df[d]+delta[j]*c->k*c->delta*c->normal[d];
 }
 memset(out,0,(size_t)w->nnode*(dim==3?4:2)*sizeof(double));
#ifdef _OPENMP
#pragma omp parallel for num_threads(workers) schedule(static) if(workers>1)
#endif
 for(i=0;i<w->nnode;i++) {
  FilStericNode *n=&w->nodes[i];filamentptr fil=n->fil;
  if(n->parent==i && !n->fixed) {
   double force[3]={0,0,0};int m,k;
   for(m=w->memberfirst[i];m>=0;m=w->membernext[m]) if(w->nodes[m].fil->filtype->dynamics!=FDnone) for(k=0;k<dim;k++) force[k]+=p->node[m].force[k];
   for(k=0;k<dim;k++) out[dim*i+k]=sim->dt*force[k]/n->drag;
  }
  if(dim==3 && n->node<fil->nseg && fil->filtype->dynamics!=FDnone)
   out[dim*w->nnode+i]=sim->dt*fil->filtype->stdlen*fil->filtype->mobility*fil->nodemobility[n->node?1:0]*p->node[i].torque;
 }
 return 0;
}
void filOperatorReport(simptr sim,struct filamentoperatorstruct *p) {
 size_t bytes;if(!p) return;
 bytes=p->maxsegment*sizeof(*p->segment)+p->maxnode*sizeof(*p->node)+p->maxbranch*sizeof(*p->branch)+p->maxcontact*sizeof(*p->contact)+p->maxwall*sizeof(*p->wall)+p->maxclamp*sizeof(*p->clamp);
 simLog(sim,2,"  cached force operator: %llu setups, %llu products; last %i active contacts, %i wall entries; allocated %g MiB\n",p->builds,p->products,p->ncontact,p->nwall,(double)bytes/1048576);
}
