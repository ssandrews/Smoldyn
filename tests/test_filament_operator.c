/* Independent directional-difference checks of cached derivatives against the
   existing nonlinear force functions, including propagated material frames. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "Sphere.h"
#undef CHECK
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL operator line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
filamentworkptr filWorkAlloc(filamentptr fil,int n);
int filAddSegment(filamentptr fil,const double *x,double len,const double *angle,double thickness,char end);
filamentptr filAddBranch(simptr sim,filamentptr mother,int seg,const double *angle,double thickness,const char *name);
void filNodes2Angles(filamentptr fil,int lo,int hi);
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr type,const char *word,char *line);
static uint64_t physical_hash(simptr sim) {
 struct filamentstericstruct *w=sim->filss->steric;uint64_t h=1469598103934665603ULL;int f,j,k,a;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;if(!fil->nseg || fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<=fil->nseg;j++) for(a=0;a<2;a++) {
   const unsigned char *bytes=(const unsigned char*)(a?fil->filwork->forces[j]:fil->nodes[j]);
   for(k=0;k<(int)(3*sizeof(double));k++) {h^=bytes[k];h*=1099511628211ULL;}
  }
  for(j=0;j<fil->nseg;j++) {
   const unsigned char *bytes=(const unsigned char*)fil->segments[j]->qabs;
   for(k=0;k<(int)(4*sizeof(double));k++) {h^=bytes[k];h*=1099511628211ULL;}
  }
 }
 return h;
}
static simptr scene(int dim) {
 simptr sim=simalloc("");int d;char line[64];CHECK(sim);CHECK(!simsetdim(sim,dim));Simsetrandseed(sim,31);sim->dt=1e-4;
 for(d=0;d<dim;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 return sim;
}
static filamenttypeptr type(simptr sim,const char *name,int elastic) {
 filamenttypeptr t=filAddFilamentType(sim,name);CHECK(t);t->dynamics=FDimplicitnetwork;t->klen=elastic?10000:-1;
 t->stdlen=0.01;t->mobility=1;t->kT=0;t->brownian=0;t->kypr[0]=0.4;t->kypr[1]=0.7;t->kypr[2]=0.9;
 t->stdypr[0]=0.02;t->stdypr[1]=-0.04;t->stdypr[2]=0.03;return t;
}
static filamentptr chain(filamenttypeptr t,int n,int elastic) {
 filamentptr fil=filAddFilament(t,NULL);double x[3]={-0.1,0.2,0.01},a[3]={0.3,0.2,0.4};int j;CHECK(fil);
 for(j=0;j<n;j++) {
  if(j) {a[0]=0.15;a[1]=-0.1;a[2]=0.2;}
  if(t->filss->sim->dim==2) a[1]=a[2]=0;
  CHECK(!filAddSegment(fil,j?NULL:x,t->stdlen*(elastic?1+0.02*j:1),a,1,'b'));
 }
 filNodes2Angles(fil,-1,-1);return fil;
}
static void prepare(simptr sim) {
 struct filamentstericstruct *w;int f,j;CHECK(!filStericPrepare(sim));CHECK(!filStericPrepareMovement(sim));w=sim->filss->steric;
 for(f=0;f<w->nfilament;f++) {
  filamentptr fil=w->filaments[f].fil;if(!fil->nseg) continue;
  CHECK(filWorkAlloc(fil,fil->maxseg));
  for(j=0;j<=fil->nseg;j++) memcpy(fil->filwork->wnodes[1][j],fil->nodes[j],3*sizeof(double));
  memcpy(fil->filwork->wseg0up[1],fil->seg0up,3*sizeof(double));
  for(j=0;j<fil->nseg;j++) fil->filwork->wroll[1][j]=fil->segments[j]->ypr[2];
 }
 CHECK(!surfforceclampbegin(sim));
}
static void flow(simptr sim,double *out) {
 struct filamentstericstruct *w=sim->filss->steric;int i,d,dim=sim->dim;
 CHECK(!surfforceclampsolve(sim));filStericOrdinaryForces(sim,1,0);if(filStericEnabled(sim)) CHECK(!filStericForces(sim));
 memset(out,0,(size_t)w->nnode*(dim==3?4:2)*sizeof(double));
 for(i=0;i<w->nnode;i++) {
  FilStericNode *n=&w->nodes[i];filamentptr fil=n->fil;filamenttypeptr t=fil->filtype;filStericNodeForce(sim,i,1);
  if(n->parent==i && !n->fixed) for(d=0;d<dim;d++) out[dim*i+d]=sim->dt*n->force[d]/n->drag;
  if(dim==3 && n->node<fil->nseg && t->dynamics!=FDnone)
   out[dim*w->nnode+i]=sim->dt*t->stdlen*t->mobility*fil->nodemobility[n->node?1:0]*fil->filwork->torques[n->node];
 }
}
static void install(simptr sim,const double *v,double eps) {
 struct filamentstericstruct *w=sim->filss->steric;int f,j,d,dim=sim->dim;
 for(f=0;f<w->nfilament;f++) {
  FilStericFilament *r=&w->filaments[f];filamentptr fil=r->fil;double axis[3];if(r->nodeoffset<0 || fil->filtype->dynamics==FDnone) continue;
  for(j=0;j<=fil->nseg;j++) {
   int root=w->nodes[r->nodeoffset+j].parent;FilStericNode *base=&w->nodes[root];
   for(d=0;d<dim;d++) fil->nodes[j][d]=base->fil->filwork->wnodes[1][base->node][d]+eps*v[dim*root+d];
  }
  if(dim==3) {
   for(d=0;d<3;d++) axis[d]=fil->filwork->wnodes[1][1][d]-fil->filwork->wnodes[1][0][d];
   Sph_RotateVectorAxisAngle(fil->filwork->wseg0up[1],axis,eps*v[dim*w->nnode+r->nodeoffset]/fil->filtype->stdlen,fil->seg0up);
   for(j=1;j<fil->nseg;j++) fil->segments[j]->ypr[2]=fil->filwork->wroll[1][j]+eps*v[dim*w->nnode+r->nodeoffset+j]/fil->filtype->stdlen;
  }
 }
 for(f=0;f<w->nfilament;f++) if(w->filaments[f].fil->filtype->dynamics!=FDnone) filNodes2Angles(w->filaments[f].fil,-1,-1);
}
static void oracle(simptr sim,int project,const char *label) {
 struct filamentstericstruct *w;struct filamentoperatorstruct *op=NULL;struct filamentconstraintstruct *length=NULL;
 double *buf,*v,*got,*plus,*minus,*diagonal,*tmp,maximum=0,scale=0;int n,i,d,pass;
 prepare(sim);w=sim->filss->steric;n=w->nnode*(sim->dim==3?4:2);buf=(double*)calloc((size_t)n*7,sizeof(double));CHECK(buf);
 v=buf;got=v+n;plus=got+n;minus=plus+n;diagonal=minus+n;tmp=diagonal+n;
 flow(sim,plus);
 for(i=0;i<n;i++) diagonal[i]=1;
 if(project) {CHECK(!filLengthPrepare(sim,&length));CHECK(!filLengthFreeze(sim,length,diagonal));}
 CHECK(!filOperatorBuild(sim,&op,1));
 for(pass=0;pass<4;pass++) {
  double eps=1e-6;unsigned long long rebuilds=w->rebuilds,evaluations=w->evaluations;uint64_t physical=physical_hash(sim);
  for(i=0;i<n;i++) v[i]=0.02*sin(3*i+pass+1);
  for(i=0;i<w->nnode;i++) if(w->nodes[i].parent!=i || w->nodes[i].fixed) for(d=0;d<sim->dim;d++) v[sim->dim*i+d]=0;
  if(project) {CHECK(!filLengthTangent(sim,length,v,tmp));memcpy(v,tmp,(size_t)n*sizeof(double));}
  CHECK(!filOperatorApply(sim,op,v,got));CHECK(w->rebuilds==rebuilds && w->evaluations==evaluations);CHECK(physical_hash(sim)==physical);
  install(sim,v,eps);flow(sim,plus);install(sim,v,-eps);flow(sim,minus);
  for(i=0;i<n;i++) {double expected=(plus[i]-minus[i])/(2*eps);maximum=fmax(maximum,fabs(got[i]-expected));scale=fmax(scale,fabs(expected));}
  install(sim,v,0);
 }
 printf("%s dim %i: max derivative error %.9g, scale %.9g\n",label,sim->dim,maximum,scale);
 CHECK(maximum<1e-7+3e-5*scale);
 filOperatorFree(op);filLengthFree(length);free(buf);simfree(sim);
}
static void bent_network(int dim,int elastic,int clamp) {
 simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",elastic);filamentptr fil=chain(t,7,elastic),daughter;double angle[3]={1.1,0.3,0.6};
 if(dim==2) angle[1]=angle[2]=0;
 t->branchsegments=3;t->branchforceangle=0.12;t->branchforceazimuth=0.23;daughter=filAddBranch(sim,fil,3,angle,1,NULL);CHECK(daughter);
 daughter->nodes[1][1]+=0.0002;filNodes2Angles(daughter,-1,-1);fil->nodemobility[0]=0;fil->nodemobility[2]=0.3;
 if(!elastic) {
  /* Perturbed daughter length must remain feasible for the tangent oracle. */
  double delta[3],norm=0;int d;for(d=0;d<dim;d++) {delta[d]=daughter->nodes[1][d]-daughter->nodes[0][d];norm+=delta[d]*delta[d];}
  for(d=0;d<dim;d++) daughter->nodes[1][d]=daughter->nodes[0][d]+0.01*delta[d]/sqrt(norm);
  for(int j=1;j<daughter->nseg;j++) {norm=0;for(d=0;d<dim;d++) {delta[d]=daughter->nodes[j+1][d]-daughter->nodes[j][d];norm+=delta[d]*delta[d];}for(d=0;d<dim;d++) daughter->nodes[j+1][d]=daughter->nodes[j][d]+0.01*delta[d]/sqrt(norm);}
  filNodes2Angles(daughter,-1,-1);
 }
 if(clamp) {
  surfaceptr wall=surfaddsurface(sim,"wall");double pars[5]={-0.09,-0.5,-0.5,1,1};CHECK(wall);
  if(dim==2) {pars[0]=-0.09;pars[1]=-0.5;pars[2]=1;}
  CHECK(!surfaddpanel(wall,dim,PSrect,"+0",pars,"floor"));t->confinesrf=wall;t->confineforce=1000;
  if(dim==3) {
   double top[5]={-0.5,-0.5,clamp>=2?0.015:0.1,1,1};panelptr pnl;
   CHECK(!surfaddpanel(wall,dim,PSrect,"-2",top,"ceiling"));pnl=wall->panels[PSrect][1];
   CHECK(!surfsetforceclamp(pnl,1,clamp==3?1e6:10,1e-10,clamp==3?0.009:-0.9,0.9));
   if(clamp==2) CHECK(!surfsetforceclampapproach(pnl,0.1,0));
   if(clamp==3) CHECK(!surfsetforceclampapproach(pnl,0.1,1));
  }
 }
 oracle(sim,!elastic,clamp==3?"active travel stop wall":clamp==2?"frozen approach wall with trial contacts":clamp?"bending/roll/branch/wall/clamp":"bending/roll/branch/stretch");
}
static void contacts(int dim) {
 simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0),fixed=type(sim,"fixed",0);filamentptr a=chain(t,1,0),b=chain(fixed,1,0);int j,d;
 fixed->dynamics=FDnone;t->kypr[0]=t->kypr[1]=t->kypr[2]=0;t->stericradius=fixed->stericradius=0.0035;
 t->sterick=fixed->sterick=4000;t->stericskin=fixed->stericskin=0.002;
 for(j=0;j<2;j++) for(d=0;d<3;d++) {a->nodes[j][d]=d==0?j*0.01:0;b->nodes[j][d]=d==0?0.003:d==1?0.004+(dim==2?j*0.01:0):dim==3?-0.005+j*0.01:0;}
 filNodes2Angles(a,-1,-1);filNodes2Angles(b,-1,-1);oracle(sim,1,"capsule closest-feature/normal/weight derivatives");
}
static void trimmed_contact(int dim) {
 simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);filamentptr mother=chain(t,5,0);double angle[3]={0.05,0.03,0.2};int j,d;
 t->branchsegments=3;t->branchforceangle=0.12;t->branchforceazimuth=0.23;
 for(j=0;j<=5;j++) for(d=0;d<3;d++) mother->nodes[j][d]=d==0?j*0.01:0;
 filNodes2Angles(mother,-1,-1);if(dim==2) angle[1]=angle[2]=0;
 CHECK(filAddBranch(sim,mother,2,angle,1,NULL));
 t->stericradius=0.0035;t->sterick=4000;t->stericskin=0.002;
 oracle(sim,1,"trimmed mother/daughter capsule and azimuth fade");
}
static void parser(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),other;char value[64];
 strcpy(value,"cached");CHECK(filtypeReadString(sim,NULL,t,"implicit_jacobian",value));CHECK(t->implicitjacobian==1);
 strcpy(value,"invalid");CHECK(!filtypeReadString(sim,NULL,t,"implicit_jacobian",value));CHECK(t->implicitjacobian==1);
 other=type(sim,"other",0);CHECK(filStericValidate(sim));other->implicitjacobian=1;CHECK(!filStericValidate(sim));
 t->dynamics=FDeuler;CHECK(filStericValidate(sim));simfree(sim);puts("PASS cached option parsing and matching-method validation");
}
static void dynamics_and_fallback(void) {
 int fallback,run,f,j,d;for(fallback=0;fallback<2;fallback++) {
  simptr sims[2];
  for(run=0;run<2;run++) {
   simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",!fallback);filamentptr fil=chain(t,fallback?1:5,!fallback);double angle[3]={1.1,0.3,0.6};
   sims[run]=sim;sim->dt=1e-5;t->implicitjacobian=run;t->implicittol=1e-11;t->implicititerations=30;t->implicitkrylov=100;
   if(fallback) {
    surfaceptr wall=surfaddsurface(sim,"sphere");double pars[6]={0,0,0,0.5,16,16};
    t->kypr[0]=t->kypr[1]=t->kypr[2]=0;t->confinesrf=wall;t->confineforce=100;
    CHECK(!surfaddpanel(wall,3,PSsph,"",pars,"sphere"));
   } else {
    t->branchsegments=2;t->branchforceangle=0.12;t->branchforceazimuth=0.23;CHECK(filAddBranch(sim,fil,2,angle,1,NULL));fil->nodemobility[0]=0;
   }
   for(j=0;j<3;j++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
   if(run && fallback) {puts("Nonrectangular-wall reference fallback:");filImplicitReport(sim);}
  }
  for(f=0;f<sims[0]->filss->filtypes[0]->nfil;f++) {
   filamentptr a=sims[0]->filss->filtypes[0]->fillist[f],b=sims[1]->filss->filtypes[0]->fillist[f];
   for(j=0;j<=a->nseg;j++) for(d=0;d<3;d++) CHECK(fabs(a->nodes[j][d]-b->nodes[j][d])<2e-9);
  }
  simfree(sims[0]);simfree(sims[1]);
 }
 puts("PASS elastic cached Newton and automatic unsupported-surface reference fallback");
}
int main(void) {
 int dim,e;for(dim=2;dim<=3;dim++) {for(e=0;e<2;e++) {bent_network(dim,e,0);bent_network(dim,e,1);if(dim==3) {bent_network(dim,e,2);bent_network(dim,e,3);}}contacts(dim);trimmed_contact(dim);}parser();dynamics_and_fallback();
 puts("All cached force operator checks passed.");return 0;
}
