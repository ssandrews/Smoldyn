/* Physical and dense-matrix oracles for fixed-length shared-node mechanics. */
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
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL fixed length line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
#define NEAR(a,b,t) CHECK(fabs((a)-(b))<(t))
static int projected=0;
static int cached=0;
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr type,const char *word,char *line);
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char end);
void filNodes2Angles(filamentptr fil,int lo,int hi);
double filRandomLength(const filamenttypeptr type,double thickness,double sigma);
int filElongate(simptr sim,filamentptr fil,double length);
int filRemoveSegment(filamentptr fil,char end);
static simptr scene(int dim) {
 simptr sim=simalloc("");char line[64];int d;CHECK(sim);CHECK(!simsetdim(sim,dim));Simsetrandseed(sim,9);
 for(d=0;d<dim;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-4;return sim;
}
static filamenttypeptr type(simptr sim) {
 filamenttypeptr t=filAddFilamentType(sim,"actin");CHECK(t);t->klen=-1;t->stdlen=0.01;t->mobility=1;t->dynamics=FDimplicitnetwork;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=0;t->kT=0;t->brownian=0;t->implicittol=1e-11;t->implicititerations=30;t->implicitkrylov=100;
 t->constrainttol=1e-10;t->constraintiterations=30;t->implicitmethod=projected;t->implicitjacobian=cached;return t;
}
static filamentptr chain(filamenttypeptr t,int n) {
 filamentptr fil=filAddFilament(t,NULL);double x[3]={0,0,0},angle[3]={0.1,0,0};int j;CHECK(fil);
 for(j=0;j<n;j++) CHECK(!filAddSegment(fil,j?NULL:x,t->stdlen,angle,1,'b'));
 filNodes2Angles(fil,-1,-1);return fil;
}
static void lengths(simptr sim,double tol) {
 int ft,f,j,d;for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];if(t->klen>=0 || t->dynamics==FDnone) continue;
  for(f=0;f<t->nfil;f++) for(j=0;j<t->fillist[f]->nseg;j++) {
   double len=0;filamentptr fil=t->fillist[f];
   for(d=0;d<sim->dim;d++) {double dx=fil->nodes[j+1][d]-fil->nodes[j][d];len+=dx*dx;}
   CHECK(fabs(sqrt(len)/t->stdlen-1)<tol);
  }
 }
}
static void prepare(simptr sim) {CHECK(!filStericPrepare(sim));CHECK(!filStericPrepareMovement(sim));filStericWorkers(sim);}
static void dense_solve(double a[32][32],double *b,int n) {
 int i,j,k,p;for(i=0;i<n;i++) {
  p=i;for(j=i+1;j<n;j++) if(fabs(a[j][i])>fabs(a[p][i])) p=j;CHECK(fabs(a[p][i])>1e-15);
  if(p!=i) {double x=b[i];b[i]=b[p];b[p]=x;for(k=i;k<n;k++) {x=a[i][k];a[i][k]=a[p][k];a[p][k]=x;}}
  for(j=i+1;j<n;j++) {double x=a[j][i]/a[i][i];for(k=i;k<n;k++) a[j][k]-=x*a[i][k];b[j]-=x*b[i];}
 }
 for(i=n-1;i>=0;i--) {for(j=i+1;j<n;j++) b[i]-=a[i][j]*b[j];b[i]/=a[i][i];}
}
/* Independent dense SHAKE reference, with current left/start right gradients.
   Compare a branched tree, uneven mobilities, a fixed anchor, and a loop. */
static void dense_oracle(void) {
 int dim,loop,i,j,k,d,n,a[32],b[32];
 for(dim=2;dim<=3;dim++) for(loop=0;loop<2;loop++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);filamentptr fil=chain(t,loop?4:5);struct filamentstericstruct *w;
  struct filamentconstraintstruct *work=NULL;double expected[64][3],g0[32][3],g[32][3],matrix[32][32],rhs[32];
  if(loop) {double points[5][3]={{0,0,0},{0.01,0,0},{0.01,0.01,0},{0,0.01,0},{0,0,0}};for(i=0;i<5;i++) memcpy(fil->nodes[i],points[i],3*sizeof(double));}
  else {double angle[3]={1,0.1,0};filamentptr daughter;t->branchsegments=3;daughter=filAddBranch(sim,fil,2,angle,1,NULL);CHECK(daughter);CHECK(filAddBranch(sim,fil,1,angle,1,NULL));CHECK(filAddBranch(sim,daughter,0,angle,1,NULL));fil->nodemobility[0]=0;fil->nodemobility[3]=0.2;}
  prepare(sim);w=sim->filss->steric;
  if(loop) {w->nodes[4].parent=0;w->nodes[0].drag+=w->nodes[4].drag;}
  CHECK(!filLengthPrepare(sim,&work));
  n=0;for(i=0;i<w->nfilament;i++) for(j=0;j<w->filaments[i].fil->nseg;j++) {
   int off=w->filaments[i].nodeoffset;double norm=0;a[n]=w->nodes[off+j].parent;b[n]=w->nodes[off+j+1].parent;
   for(d=0;d<dim;d++) {g0[n][d]=w->nodes[b[n]].fil->nodes[w->nodes[b[n]].node][d]-w->nodes[a[n]].fil->nodes[w->nodes[a[n]].node][d];norm+=g0[n][d]*g0[n][d];}
   for(d=0;d<dim;d++) g0[n][d]/=sqrt(norm);n++;
  }
  for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) {
   expected[i][d]=w->nodes[i].fil->nodes[w->nodes[i].node][d];
   if(w->nodes[i].parent==i && !w->nodes[i].fixed) expected[i][d]+=0.00005*sin(2*i+d);
  }
  for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) w->nodes[i].fil->nodes[w->nodes[i].node][d]=expected[w->nodes[i].parent][d];
  for(k=0;k<20;k++) {
   double maximum=0;
   for(i=0;i<n;i++) {double len=0;for(d=0;d<dim;d++) {g[i][d]=expected[b[i]][d]-expected[a[i]][d];len+=g[i][d]*g[i][d];}
    len=sqrt(len);rhs[i]=len-t->stdlen;maximum=fmax(maximum,fabs(rhs[i]));for(d=0;d<dim;d++) g[i][d]/=len;}
   if(maximum<t->stdlen*t->constrainttol) break;
   for(i=0;i<n;i++) for(j=0;j<n;j++) {
    matrix[i][j]=0;
    for(d=0;d<dim;d++) {
     int u[2]={a[i],b[i]},v[2]={a[j],b[j]},s,z;
     for(s=0;s<2;s++) for(z=0;z<2;z++) if(u[s]==v[z] && !w->nodes[u[s]].fixed)
      matrix[i][j]+=(s==z?1:-1)*g[i][d]*g0[j][d]/w->nodes[u[s]].drag;
    }
   }
   dense_solve(matrix,rhs,n);
   for(i=0;i<n;i++) for(d=0;d<dim;d++) {
    if(!w->nodes[a[i]].fixed) expected[a[i]][d]+=g0[i][d]*rhs[i]/w->nodes[a[i]].drag;
    if(!w->nodes[b[i]].fixed) expected[b[i]][d]-=g0[i][d]*rhs[i]/w->nodes[b[i]].drag;
   }
  }
  CHECK(k<20);CHECK(!filLengthProject(sim,work));
  for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) NEAR(w->nodes[i].fil->nodes[w->nodes[i].node][d],expected[w->nodes[i].parent][d],2e-12);
  lengths(sim,2e-10);
  {double v[256]={0},tangent[256],base[64][3],plus[64][3],eps=1e-6;int sign;
   for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) {
    base[i][d]=w->nodes[i].fil->nodes[w->nodes[i].node][d];
    if(w->nodes[i].parent==i && !w->nodes[i].fixed) v[dim*i+d]=0.1*sin(3*i+d);
   }
   {double diagonal[256],z[256],pz[256];
    for(i=0;i<256;i++) diagonal[i]=1;
    for(i=0;i<w->nnode;i++) if(w->nodes[i].parent==i && !w->nodes[i].fixed) for(d=0;d<dim;d++) diagonal[dim*i+d]=2+0.1*(i%7);
    CHECK(!filLengthFreeze(sim,work,diagonal));CHECK(!filLengthTangent(sim,work,v,tangent));
    if(!loop) {
     CHECK(!filLengthPrecondition(sim,work,NULL,v,z));CHECK(!filLengthTangent(sim,work,z,pz));
     for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) NEAR(diagonal[dim*i+d]*pz[dim*i+d]+z[dim*i+d]-pz[dim*i+d],v[dim*i+d],2e-10);
    }
   }
   for(sign=1;sign>=-1;sign-=2) {
    for(i=0;i<w->nnode;i++) for(d=0;d<dim;d++) {int root=w->nodes[i].parent;w->nodes[i].fil->nodes[w->nodes[i].node][d]=base[root][d]+sign*eps*v[dim*root+d];}
    CHECK(!filLengthProject(sim,work));
    for(i=0;i<w->nnode;i++) if(w->nodes[i].parent==i) for(d=0;d<dim;d++) {
     double value=w->nodes[i].fil->nodes[w->nodes[i].node][d];
     if(sign==1) plus[i][d]=value;else NEAR((plus[i][d]-value)/(2*eps),tangent[dim*i+d],2e-5);
    }
   }
  }
  filLengthFree(work);simfree(sim);
 }
 puts("PASS forest/cyclic SHAKE versus dense oracle and analytic tangent versus finite differences, 2D/3D nested branches and unequal drag");
}
static void analytic_motion(void) {
 int dim,mode,d,j;for(dim=2;dim<=3;dim++) for(mode=0;mode<2;mode++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);filamentptr fil=chain(t,1);surfaceptr wall=surfaddsurface(sim,"wall");
  double panel[5]={0,-0.5,-0.5,1,1},mu0=0.2,mu1=1.7,effective=1/(1/mu0+1/mu1),expected;
  if(dim==2) {panel[0]=0;panel[1]=-0.5;panel[2]=1;}
  for(j=0;j<2;j++) for(d=0;d<dim;d++) fil->nodes[j][d]=d==0?-0.001+j*0.01:0;
  filNodes2Angles(fil,-1,-1);fil->nodemobility[0]=mu0;fil->nodemobility[1]=mu1;t->implicitpreconditioner=mode;
  t->confinesrf=wall;t->confineforce=1e5;CHECK(!surfaddpanel(wall,dim,PSrect,"+0",panel,"floor"));
  expected=-0.001/(1+sim->dt*t->confineforce*effective);
  CHECK(!filDynamics(sim));lengths(sim,2e-10);NEAR(fil->nodes[0][0],expected,2e-10);NEAR(fil->nodes[1][0],expected+0.01,2e-10);
  simfree(sim);
 }
 puts("PASS analytic constrained wall-driven translation, axial load transmission and total drag, both preconditioners");
}
static void growth_and_brownian(void) {
 int dim,i,d,mode;
 for(dim=2;dim<=3;dim++) for(mode=0;mode<2;mode++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);filamentptr fil=chain(t,4),daughter;double angle[3]={1.1,0.1,0.2};
  t->implicitpreconditioner=mode;t->kypr[0]=t->kypr[1]=0.05;t->branchsegments=2;t->branchforceangle=0.02;t->branchforceazimuth=0.03;
  daughter=filAddBranch(sim,fil,2,angle,1,NULL);CHECK(daughter);fil->nodemobility[0]=0;
  for(i=0;i<3;i++) {CHECK(!filDynamics(sim));lengths(sim,2e-10);for(d=0;d<dim;d++) CHECK(daughter->nodes[0][d]==fil->nodes[3][d]);sim->time+=sim->dt;}
  CHECK(filElongate(sim,daughter,0.02)==2);CHECK(!filDynamics(sim));lengths(sim,2e-10);
  CHECK(!filRemoveSegment(daughter,'b'));CHECK(!filDynamics(sim));lengths(sim,2e-10);
  daughter->endstate[1]=1;CHECK(!filElongate(sim,daughter,0.02));simfree(sim);
 }
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);filamentptr fil=chain(t,1);double before[3];
  t->kT=0.001;t->brownian=1;
  NEAR(filRandomLength(t,1,1),0.01,1e-15);
  for(i=0;i<20;i++) {
   for(d=0;d<dim;d++) before[d]=(fil->nodes[0][d]+fil->nodes[1][d])*0.5;
   CHECK(!filDynamics(sim));lengths(sim,2e-10);
   for(d=0;d<dim;d++) NEAR((fil->nodes[0][d]+fil->nodes[1][d])*0.5-before[d],sim->dt*(fil->filwork->thermforce[0][d]+fil->filwork->thermforce[1][d])*0.5,2e-10);
   sim->time+=sim->dt;
  }
  simfree(sim);
 }
 puts("PASS constrained bending/branches/growth/removal/capping and projected Brownian center-of-mass increments");
}
static void contacts(void) {
 int dim,mode,d,j;for(dim=2;dim<=3;dim++) for(mode=0;mode<2;mode++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim),fixed=filAddFilamentType(sim,"obstacle");filamentptr fil,obstacle;
  CHECK(fixed);fixed->dynamics=FDnone;fixed->stdlen=0.01;fixed->kT=0;
  t->stericradius=fixed->stericradius=0.0035;t->sterick=fixed->sterick=4000;t->stericskin=fixed->stericskin=0.002;
  t->implicitpreconditioner=mode;fil=chain(t,1);obstacle=chain(fixed,1);
  for(j=0;j<2;j++) for(d=0;d<dim;d++) {fil->nodes[j][d]=d==0?j*0.01:0;obstacle->nodes[j][d]=d==0?j*0.01:d==1?0.006:0;}
  filNodes2Angles(fil,-1,-1);filNodes2Angles(obstacle,-1,-1);
  CHECK(!filDynamics(sim));lengths(sim,2e-10);
  NEAR(fil->nodes[0][1],-0.001*(1-1/(1+sim->dt*t->sterick/2)),2e-10);NEAR(obstacle->nodes[0][1],0.006,1e-15);
  simfree(sim);
 }
 puts("PASS fixed-length capsule contact against immobile obstacles, 2D/3D and both preconditioners");
}
static void failure_and_parser(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim);filamentptr fil=chain(t,1);char value[64];double saved[2][3];
 fil->nodes[1][0]=fil->nodes[0][0]+0.012;fil->nodes[1][1]=fil->nodes[0][1];fil->nodemobility[0]=fil->nodemobility[1]=0;
 memcpy(saved[0],fil->nodes[0],3*sizeof(double));memcpy(saved[1],fil->nodes[1],3*sizeof(double));CHECK(filDynamics(sim));
 CHECK(!memcmp(saved[0],fil->nodes[0],3*sizeof(double)));CHECK(!memcmp(saved[1],fil->nodes[1],3*sizeof(double)));
 strcpy(value,"1e-9");CHECK(filtypeReadString(sim,NULL,t,"constraint_tolerance",value));NEAR(t->constrainttol,1e-9,1e-20);
 strcpy(value,"0");CHECK(!filtypeReadString(sim,NULL,t,"constraint_tolerance",value));NEAR(t->constrainttol,1e-9,1e-20);
 strcpy(value,"25");CHECK(filtypeReadString(sim,NULL,t,"constraint_iterations",value));CHECK(t->constraintiterations==25);
 t->dynamics=FDeuler;CHECK((filStericValidate(sim)!=0)==(t->implicitmethod!=0 || t->implicitjacobian!=0));simfree(sim);
 puts("PASS infeasible fixed-anchor failure restores mechanics; constraint parsing and unsupported-method rejection");
}
static void workers(void) {
 simptr sims[2];int run,f,j; if(!filWorkersSupported()) return;
 for(run=0;run<2;run++) {
  filamenttypeptr t;sims[run]=scene(3);t=type(sims[run]);t->kypr[0]=t->kypr[1]=0.05;
  CHECK(!filSetWorkers(sims[run],run?4:1));
  for(f=0;f<300;f++) chain(t,8);
  CHECK(!filDynamics(sims[run]));lengths(sims[run],2e-10);
 }
 CHECK(sims[1]->filss->steric->workersused==4);
 for(f=0;f<300;f++) for(j=0;j<=8;j++) CHECK(!memcmp(sims[0]->filss->filtypes[0]->fillist[f]->nodes[j],sims[1]->filss->filtypes[0]->fillist[f]->nodes[j],3*sizeof(double)));
 simfree(sims[0]);simfree(sims[1]);puts("PASS exact 1/4-worker constrained bending and cached tangent reproducibility");
}
static int trajectory(int method,double h,int steps,double *out) {
 simptr sim=scene(3);filamenttypeptr t=type(sim);filamentptr fil=chain(t,8);int i,f,j,d,n=0;
 double angle[3]={1.1,0.2,0};
 sim->dt=h;t->implicitmethod=method;t->projectedtol=1e-4;t->implicittol=1e-12;
 t->kypr[0]=t->kypr[1]=0.02;t->branchforceangle=0.02;t->branchforceazimuth=0.01;t->branchsegments=3;
 CHECK(filAddBranch(sim,fil,3,angle,1,NULL));fil->nodemobility[0]=0;
 for(i=0;i<steps;i++) {CHECK(!filDynamics(sim));lengths(sim,2e-10);sim->time+=sim->dt;}
 for(f=0;f<t->nfil;f++) for(j=0;j<=t->fillist[f]->nseg;j++) for(d=0;d<3;d++) out[n++]=t->fillist[f]->nodes[j][d];
 if(method) filImplicitReport(sim);
 simfree(sim);return n;
}
static void projected_checks(void) {
 double ref[128],coarse[128],fine[128],finer[128],e[3]={0,0,0};int n,i,j;
 simptr sims[2];
 n=trajectory(0,1.25e-5,160,ref);CHECK(n==trajectory(1,1e-4,20,coarse));CHECK(n==trajectory(1,5e-5,40,fine));CHECK(n==trajectory(1,2.5e-5,80,finer));
 for(i=0;i<n;i++) {e[0]+=pow(coarse[i]-ref[i],2);e[1]+=pow(fine[i]-ref[i],2);e[2]+=pow(finer[i]-ref[i],2);}
 for(i=0;i<3;i++) e[i]=sqrt(e[i]/n);
 printf("Projected branched bending RMS errors: %.9g %.9g %.9g length units\n",e[0],e[1],e[2]);
 CHECK(e[1]<0.8*e[0] && e[2]<0.8*e[1]);CHECK(e[0]<1e-5);
 /* A rejected predictor must converge to the reference Newton result with
    identical cached Brownian forces, rather than redraw or double-step. */
 for(i=0;i<2;i++) {
  filamenttypeptr t;sims[i]=scene(3);t=type(sims[i]);t->implicitmethod=i;t->projectedtol=1e-25;
  t->kypr[0]=t->kypr[1]=0.05;t->kT=0.001;t->brownian=1;chain(t,5);
  CHECK(!filDynamics(sims[i]));lengths(sims[i],2e-10);
 }
 for(j=0;j<=5;j++) {int d;for(d=0;d<3;d++) NEAR(sims[0]->filss->filtypes[0]->fillist[0]->nodes[j][d],sims[1]->filss->filtypes[0]->fillist[0]->nodes[j][d],2e-10);}
 CHECK(!memcmp(sims[0]->filss->filtypes[0]->fillist[0]->filwork->thermforce[0],sims[1]->filss->filtypes[0]->fillist[0]->filwork->thermforce[0],3*sizeof(double)));
 filImplicitReport(sims[1]);simfree(sims[0]);simfree(sims[1]);
 /* Exhaust the predictor's tighter inner target while ordinary inexact
    Newton can still converge with one Krylov vector per correction. */
 for(i=0;i<2;i++) {
  filamenttypeptr t;sims[i]=scene(3);t=type(sims[i]);t->implicitmethod=i;t->projectedtol=1e-2;
  t->implicitkrylov=1;t->kypr[0]=t->kypr[1]=0.001;chain(t,5);
  CHECK(!filDynamics(sims[i]));lengths(sims[i],2e-10);
 }
 for(j=0;j<=5;j++) {int d;for(d=0;d<3;d++) NEAR(sims[0]->filss->filtypes[0]->fillist[0]->nodes[j][d],sims[1]->filss->filtypes[0]->fillist[0]->nodes[j][d],2e-10);}
 puts("Krylov-budget fallback check:");filImplicitReport(sims[1]);simfree(sims[0]);simfree(sims[1]);
 {simptr sim=scene(3);filamenttypeptr t=type(sim);char value[64];
  strcpy(value,"projected");CHECK(filtypeReadString(sim,NULL,t,"implicit_method",value));CHECK(t->implicitmethod==1);
  strcpy(value,"1e-5");CHECK(filtypeReadString(sim,NULL,t,"projected_tolerance",value));NEAR(t->projectedtol,1e-5,1e-16);
  strcpy(value,"nan");CHECK(!filtypeReadString(sim,NULL,t,"projected_tolerance",value));
  strcpy(value,"invalid");CHECK(!filtypeReadString(sim,NULL,t,"implicit_method",value));
  t->klen=10000;CHECK(filStericValidate(sim));
  t->dynamics=FDeuler;CHECK((filStericValidate(sim)!=0)==(t->implicitmethod!=0 || t->implicitjacobian!=0));
  t->implicitmethod=0;t->implicitjacobian=0;CHECK(!filStericValidate(sim));simfree(sim);
 }
 puts("PASS projected timestep convergence, same-noise Newton fallback and parser/model validation");
}
int main(int argc,char **argv) {cached=argc>1 && (!strcmp(argv[1],"--cached") || !strcmp(argv[1],"--cached-projected"));projected=argc>1 && (!strcmp(argv[1],"--projected") || !strcmp(argv[1],"--cached-projected"));dense_oracle();analytic_motion();growth_and_brownian();contacts();failure_and_parser();workers();if(projected) projected_checks();puts("All fixed-length constraint checks passed.");return 0;}
