/* Mechanical oracles for the optional banded preconditioner. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "filament_test_compat.h"
#include "random2.h"
#undef CHECK
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL preconditioner line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
#define NEAR(a,b,t) CHECK(fabs((a)-(b))<(t))
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr type,const char *word,char *line);
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char end);
void filComputeForces(filamentptr fil,int lo,int hi);
void filNodes2Angles(filamentptr fil,int lo,int hi);
static simptr scene(int dim) {
 simptr sim=simalloc("");int d;char line[64];CHECK(sim);CHECK(!simsetdim(sim,dim));Simsetrandseed(sim,7);
 for(d=0;d<dim;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-4;return sim;
}
static filamenttypeptr type(simptr sim,const char *name) {
 filamenttypeptr t=filAddFilamentType(sim,name);CHECK(t);t->dynamics=FDimplicitnetwork;t->implicitpreconditioner=1;
 t->stdlen=0.01;t->klen=10000;t->kypr[0]=0.7;t->kypr[1]=0.4;t->kypr[2]=0;
 t->kT=0;t->brownian=0;t->mobility=1;t->implicittol=1e-10;t->implicititerations=30;t->implicitkrylov=100;return t;
}
static filamentptr chain(filamenttypeptr t,int n,double y) {
 filamentptr fil=filAddFilament(t,NULL);double pos[3]={0,y,0},angle[3]={0,0,0};int j;CHECK(fil);
 for(j=0;j<n;j++) CHECK(!filAddSegment(fil,j?NULL:pos,t->stdlen,angle,1,'b'));
 filNodes2Angles(fil,-1,-1);return fil;
}
static void prepare(simptr sim) {
 CHECK(!filStericPrepare(sim));CHECK(!filStericPrepareMovement(sim));filStericWorkers(sim);
}
/* At a straight, unstressed filament the retained elastic Hessian agrees with
   the actual force derivative. Verify the inverse by an independent central
   finite difference, including heterogeneous mobility and fixed variables. */
static void force_derivative_oracle(void) {
 int dim,fixed,node,d,i,j,n;
 for(dim=2;dim<=3;dim++) for(fixed=0;fixed<2;fixed++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin");filamentptr fil=chain(t,5,0);
  struct filamentprecondstruct *p=NULL;double diagonal[64],rhs[64],answer[64],matrix[32][32],fp[32],fm[32];
  double eps=1e-8;
  n=dim*6;
  for(node=0;node<6;node++) fil->nodemobility[node]=fixed && node==2?0:0.3+node*0.2;
  prepare(sim);
  for(i=0;i<64;i++) {diagonal[i]=1;rhs[i]=sin(i+0.25);}
  CHECK(!filPrecondBuild(sim,&p,diagonal));filPrecondApply(sim,p,rhs,answer);
  for(j=0;j<n;j++) {
   node=j/dim;d=j%dim;
   for(i=0;i<n;i++) matrix[i][j]=i==j?1:0;
   if(fil->nodemobility[node]==0) continue;
   fil->nodes[node][d]+=eps;filNodes2Angles(fil,-1,-1);filComputeForces(fil,-1,-1);
   for(i=0;i<n;i++) fp[i]=fil->filwork->forces[i/dim][i%dim];
   fil->nodes[node][d]-=2*eps;filNodes2Angles(fil,-1,-1);filComputeForces(fil,-1,-1);
   for(i=0;i<n;i++) fm[i]=fil->filwork->forces[i/dim][i%dim];
   fil->nodes[node][d]+=eps;filNodes2Angles(fil,-1,-1);
   for(i=0;i<n;i++) if(fil->nodemobility[i/dim]>0)
    matrix[i][j]-=sim->dt*t->mobility*fil->nodemobility[i/dim]*(fp[i]-fm[i])/(2*eps);
  }
  for(i=0;i<n;i++) {double value=0;for(j=0;j<n;j++) value+=matrix[i][j]*answer[j];NEAR(value,rhs[i],2e-6);}
  filPrecondFree(p);simfree(sim);
 }
 puts("PASS banded inverse versus physical force derivative, 2D/3D bending/stretching, heterogeneous drag and fixed nodes");
}
static void shared_identity_and_lifecycle(void) {
 int dim,j,d,i,n;
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr a=type(sim,"mother"),b=type(sim,"other");
  filamentptr mother=chain(a,3,0),daughter;double angle[3]={1,0,0};
  struct filamentprecondstruct *p=NULL;double diagonal[128],rhs[128],out[128];
  a->branchsegments=2;daughter=filAddBranch(sim,mother,1,angle,1,NULL);CHECK(daughter);
  daughter->nodemobility[0]=0.2;mother->nodemobility[2]=1.7;
  a->klen=b->klen=0;for(d=0;d<3;d++) a->kypr[d]=b->kypr[d]=0;
  chain(b,2,0.1);prepare(sim);
  for(i=0;i<128;i++) {diagonal[i]=1;rhs[i]=cos(i*0.3);}
  CHECK(!filPrecondBuild(sim,&p,diagonal));filPrecondApply(sim,p,rhs,out);
  n=sim->filss->steric->nnode*(dim==3?4:2);for(i=0;i<n;i++) NEAR(out[i],rhs[i],2e-14);
  /* Reuse allocations after elongation changes offsets and branch membership. */
  CHECK(!filAddSegment(mother,NULL,0.01,angle,1,'b'));prepare(sim);
  CHECK(!filPrecondBuild(sim,&p,diagonal));filPrecondApply(sim,p,rhs,out);
  n=sim->filss->steric->nnode*(dim==3?4:2);for(i=0;i<n;i++) NEAR(out[i],rhs[i],2e-14);
  a->klen=1e4;a->kypr[0]=a->kypr[1]=0.1;mother->nodemobility[2]=0;prepare(sim);
  CHECK(!filPrecondBuild(sim,&p,diagonal));filPrecondApply(sim,p,rhs,out);
  for(j=0;j<sim->filss->steric->nnode;j++) if(sim->filss->steric->nodes[j].fixed)
   for(d=0;d<dim;d++) NEAR(out[dim*j+d],rhs[dim*j+d],1e-14);
  filPrecondFree(p);simfree(sim);
 }
 puts("PASS overlapping shared-node partition of unity, combined drag, fixed branches and topology growth/reuse");
}
static simptr bent_scene(int dim,int mode,int workers) {
 simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin");filamentptr mother=chain(t,8,0),daughter;
 double angle[3]={1,0.1,0.2};int i;t->implicitpreconditioner=mode;t->branchsegments=3;
 mother->nodes[4][1]=0.001;filNodes2Angles(mother,-1,-1);
 daughter=filAddBranch(sim,mother,3,angle,1,NULL);CHECK(daughter);t->branchforceangle=0.02;t->branchforceazimuth=0.03;
 /* Exceed the mechanical worker threshold without contacts between chains. */
 if(workers>1) for(i=0;i<160;i++) {filamentptr fil=chain(t,8,0.1+i*0.004);fil->nodes[3][1]+=0.0003;filNodes2Angles(fil,-1,-1);}
 CHECK(!filSetWorkers(sim,workers));return sim;
}
static void nonlinear_and_workers(void) {
 int dim,i,f,j,d;
 for(dim=2;dim<=3;dim++) {
  simptr a=bent_scene(dim,0,1),b=bent_scene(dim,1,1);
  for(i=0;i<4;i++) {CHECK(!filDynamics(a));CHECK(!filDynamics(b));a->time+=a->dt;b->time+=b->dt;}
  for(f=0;f<a->filss->filtypes[0]->nfil;f++) for(j=0;j<=a->filss->filtypes[0]->fillist[f]->nseg;j++) for(d=0;d<dim;d++)
   NEAR(a->filss->filtypes[0]->fillist[f]->nodes[j][d],b->filss->filtypes[0]->fillist[f]->nodes[j][d],2e-9);
  simfree(a);simfree(b);
 }
 if(filWorkersSupported()) {
  simptr a=bent_scene(3,1,4),b=bent_scene(3,1,4);CHECK(!filSetWorkers(a,1));
  CHECK(!filDynamics(a));CHECK(!filDynamics(b));CHECK(b->filss->steric->workersused==4);
  for(f=0;f<a->filss->filtypes[0]->nfil;f++) for(j=0;j<=a->filss->filtypes[0]->fillist[f]->nseg;j++)
   CHECK(!memcmp(a->filss->filtypes[0]->fillist[f]->nodes[j],b->filss->filtypes[0]->fillist[f]->nodes[j],3*sizeof(double)));
  simfree(a);simfree(b);
 }
 puts("PASS nonlinear branched trajectories versus diagonal and exact 1/4-worker banded reproducibility");
}
static void parser_and_validation(void) {
 simptr sim=scene(3);filamenttypeptr a=type(sim,"a"),b=type(sim,"b");char value[64];
 strcpy(value,"diagonal");CHECK(filtypeReadString(sim,NULL,a,"implicit_preconditioner",value));CHECK(!a->implicitpreconditioner);
 CHECK(filStericValidate(sim));b->implicitpreconditioner=0;CHECK(!filStericValidate(sim));
 strcpy(value,"banded");CHECK(filtypeReadString(sim,NULL,a,"implicit_preconditioner",value));CHECK(a->implicitpreconditioner==1);
 strcpy(value,"diagonal extra");CHECK(!filtypeReadString(sim,NULL,a,"implicit_preconditioner",value));CHECK(a->implicitpreconditioner==1);
 strcpy(value,"unknown");CHECK(!filtypeReadString(sim,NULL,a,"implicit_preconditioner",value));CHECK(a->implicitpreconditioner==1);
 b->dynamics=FDnone;CHECK(!filStericValidate(sim));simfree(sim);
 puts("PASS opt-in parser, atomic invalid settings and consistent mobile-type selection");
}
int main(void) {
 force_derivative_oracle();shared_identity_and_lifecycle();nonlinear_and_workers();parser_and_validation();
 puts("All banded preconditioner checks passed.");return 0;
}
