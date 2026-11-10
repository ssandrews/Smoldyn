/* Force generation, chemical work, refinement and compatibility regressions. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "filament_test_compat.h"
#undef CHECK
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL growth line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
#define NEAR(a,b,t) CHECK(fabs((a)-(b))<(t))
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
int filAddSegment(filamentptr fil,const double *pos,double len,const double *angle,double thick,char end);
int filElongate(simptr sim,filamentptr fil,double length);
void filNodes2Angles(filamentptr fil,int lo,int hi);
double filStretchEnergy(const filamentptr fil,int lo,int hi);

static simptr scene(void) {
 simptr sim=simalloc("");int d;char line[64];CHECK(sim);CHECK(!simsetdim(sim,3));Simsetrandseed(sim,1);
 for(d=0;d<3;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-4;return sim;
}
static filamenttypeptr type(simptr sim,const char *name,int growth) {
 filamenttypeptr t=filAddFilamentType(sim,name);CHECK(t);
 t->dynamics=FDimplicitnetwork;t->stdlen=.01;t->klen=10000;t->mobility=1;
 t->kT=.001;t->brownian=0;t->kypr[0]=t->kypr[1]=t->kypr[2]=0;
 t->implicittol=1e-10;t->implicititerations=20;t->implicitkrylov=80;t->implicitpreconditioner=1;
 t->elongmodel=growth;t->elongstep=.0027;t->elongenergy=.00318;
 t->sterick=4000;t->stericskin=.002;return t;
}
static filamentptr rod(filamenttypeptr t,int n,double z) {
 double pos[3]={0,0,0},angle[3]={0,0,0};int i;
 filamentptr f=filAddFilament(t,NULL);CHECK(f);pos[2]=z;
 for(i=0;i<n;i++) CHECK(!filAddSegment(f,i?NULL:pos,.01,angle,1,'b'));
 for(i=0;i<=n;i++) {f->nodes[i][0]=f->nodes[i][1]=0;f->nodes[i][2]=z+i*.01;}
 f->seg0up[0]=1;f->seg0up[1]=f->seg0up[2]=0;
 filNodes2Angles(f,-1,-1);return f;
}
static panelptr ceiling(simptr sim,double z) {
 surfaceptr surf=surfaddsurface(sim,"chamber");double pars[5]={-.5,-.5,0,1,1};CHECK(surf);
 pars[2]=0;CHECK(!surfaddpanel(surf,3,PSrect,"+2",pars,"floor"));
 pars[2]=z;CHECK(!surfaddpanel(surf,3,PSrect,"-2",pars,"ceiling"));return surf->panels[PSrect][1];
}
static void kinetics_and_energy(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr f=rod(t,1,0);
 double old=f->nodes[1][2],length=1e-6,stall=t->elongenergy/t->elongstep,EA=t->klen*t->stdlen,g=.25*stall;
 CHECK(!filGrowthValidate(t));CHECK(filElongate(sim,f,length)==0);
 NEAR(f->lenbank[1],length,1e-15);NEAR(f->nodes[1][2],old,1e-15);
 CHECK(filStretchEnergy(f,-1,-1)<=stall*length+1e-14);
 f->lenbank[1]=0;f->nodes[1][2]=.01*sqrt(1-2*g/EA);filNodes2Angles(f,-1,-1);
 CHECK(filElongate(sim,f,1e-9)==0);
 NEAR(f->lenbank[1]/1e-9,(exp(-g*t->elongstep/t->kT)-exp(-t->elongenergy/t->kT))/(1-exp(-t->elongenergy/t->kT)),1e-12);
 f->lenbank[1]=0;f->nodes[1][2]=.01*sqrt(1-3*stall/EA);filNodes2Angles(f,-1,-1);
 CHECK(filElongate(sim,f,length)==0);NEAR(f->lenbank[1],0,1e-15);
 f->nodes[1][2]=.01;filNodes2Angles(f,-1,-1);f->endstate[1]=1;
 CHECK(filElongate(sim,f,length)==0);NEAR(f->lenbank[1],0,1e-15);
 f->endstate[1]=0;t->elongmaxlen=.0100005;CHECK(filElongate(sim,f,length)==0);NEAR(filNominalLength(f),.0100005,1e-15);
 t->klen=-1;CHECK(filGrowthValidate(t));t->klen=10000;t->plusend='m';CHECK(filGrowthValidate(t));
 simfree(sim);puts("PASS growth kinetics, finite work, stall, cap, length ceiling and mode validation");
}
static void refinement(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr f=rod(t,1,0),child;
 double angle[3]={1,0,0},end=.0199,EA=t->klen*t->stdlen,expected;
 f->nodes[1][2]=end;filNodes2Angles(f,-1,-1);f->lenbank[1]=.0099;
 child=filAddBranch(sim,f,0,angle,1,NULL);CHECK(child);CHECK(f->juncts[0]->node==1);
 f->nodemobility[1]=0;
 CHECK(filElongate(sim,f,.0001)==1);CHECK(f->nseg==2);NEAR(filNominalLength(f),.02,1e-15);
 NEAR(f->nodes[2][2],end,1e-15);NEAR(f->nodes[1][2],end/2,1e-15);
 CHECK(f->juncts[0]->node==2);NEAR(child->nodes[0][2],f->nodes[2][2],1e-15);
 NEAR(f->nodemobility[2],0,1e-15);NEAR(f->nodemobility[1],1,1e-15);
 expected=.5*EA*(end-.02)*(end-.02)/.02;
 NEAR(filStretchEnergy(f,-1,-1),expected,1e-14);
 CHECK(t->elongwork<=t->elongaccepted*t->elongenergy/t->elongstep+1e-14);
 simfree(sim);puts("PASS geometry/compliance/energy preserving refinement and shared branch-tip remap");
}
static void force_energy_consistency(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr f=rod(t,1,0);
 double h=1e-8,plus,minus,force;
 f->lenbank[1]=.003;f->segments[0]->thk=2;f->nodes[1][2]=.012;filNodes2Angles(f,-1,-1);
 filComputeForces(f,-1,-1);force=f->filwork->forces[1][2];
 NEAR(force+f->filwork->forces[0][2],0,1e-12);
 f->nodes[1][2]+=(h);filNodes2Angles(f,-1,-1);plus=filStretchEnergy(f,-1,-1);
 f->nodes[1][2]-=2*h;filNodes2Angles(f,-1,-1);minus=filStretchEnergy(f,-1,-1);
 NEAR(force,-(plus-minus)/(2*h),1e-7);
 simfree(sim);puts("PASS variable-rest-length/thickness energy gradient and equal-opposite axial forces");
}
static double loaded_growth(double load,int growth,int cached,double dt) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",growth);filamentptr f=rod(t,2,.0035);
 panelptr plate=ceiling(sim,.05);double start,accepted;int i;
 sim->dt=dt;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=1;
 f->nodemobility[0]=0;t->stericradius=.0035;t->confinesrf=plate->srf;t->confineforce=400;
 t->implicitjacobian=cached;CHECK(!surfsetforceclamp(plate,1,load,1e-9,.005,.9));
 for(i=0;i<(int)ceil(.03/dt);i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
 start=plate->point[0][2];t->endrate[1][FESopen]=.06;
 for(i=0;i<(int)ceil(.4/dt);i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
 NEAR(plate->forceclamp->reaction,load,1e-8);
 accepted=t->elongaccepted;
 if(growth && load<1) {
  CHECK(plate->point[0][2]>start+.001);
  CHECK(t->elongwork<=accepted*t->elongenergy/t->elongstep+1e-10);
 } else {
  printf("CONTROL displacement=%g segments=%d\n",plate->point[0][2]-start,f->nseg);
  NEAR(plate->point[0][2],start,1e-8);
 }
 printf("MEASURE load=%g growth=%d cached=%d dt=%g accepted_length=%g plate_displacement=%g\n",load,growth,cached,dt,accepted,plate->point[0][2]-start);
 simfree(sim);return accepted;
}
static void steric_obstacle(int hybrid) {
 int i;double pos[3]={-.02,0,.034},angle[3]={0,0,0};
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1),other=type(sim,"obstacle",0);
 filamentptr f=rod(t,2,.0035),bar=filAddFilament(other,NULL);CHECK(bar);
 if(hybrid) {t->klen=-1;t->elongstiffness=10000;t->constrainttol=1e-8;t->constraintiterations=30;}
 t->stericradius=other->stericradius=.0035;f->nodemobility[0]=0;t->endrate[1][FESopen]=.06;
 CHECK(!filAddSegment(bar,pos,.04,angle,1,'b'));other->stdlen=.04;
 for(i=0;i<3000;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
 CHECK(bar->nodes[0][2]>.034+.001);CHECK(bar->nodes[1][2]>.034+.001);
 CHECK(sim->filss->steric->contacts>0);CHECK(t->elongaccepted>.003);
 printf("MEASURE mobile capsule obstacle moved %g um without insertion\n",bar->nodes[0][2]-.034);
 simfree(sim);puts("PASS growing tip pushes a separate filament through existing capsule contact forces");
}
static void parser(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",0);
 char model[]="mechanochemical",overlap[]="overlap",legacy[]="legacy",invalid[]="invalid",step[]="0.0027",energy[]="0.00318",bad[]="-1";
 CHECK(filtypeReadString(sim,NULL,t,"elongation_model",model)==t);
 CHECK(filtypeReadString(sim,NULL,t,"elongation_step",step)==t);
 CHECK(filtypeReadString(sim,NULL,t,"elongation_energy",energy)==t);
 CHECK(!filtypeReadString(sim,NULL,t,"elongation_energy",bad));CHECK(!filGrowthValidate(t));
 CHECK(filtypeReadString(sim,NULL,t,"elongation_model",overlap)==t);CHECK(!t->elongmodel && t->elongoverlap);
 CHECK(filtypeReadString(sim,NULL,t,"elongation_model",legacy)==t);CHECK(!t->elongmodel && !t->elongoverlap);
 CHECK(!filtypeReadString(sim,NULL,t,"elongation_model",invalid));CHECK(!t->elongmodel && !t->elongoverlap);
 rod(t,1,0);CHECK(!filtypeReadString(sim,NULL,t,"elongation_model",model));CHECK(!t->elongmodel && !t->elongoverlap);
 simfree(sim);puts("PASS explicit optional parser and invalid parameter rejection");
}
/* Collision rejection and force relaxation are separate: overlap mode must
   accept the same geometry that legacy vetoes, then retain physical forces. */
static void overlap_wall(void) {
 int overlap,fixed,front,i,j;
 for(overlap=0;overlap<2;overlap++) for(fixed=0;fixed<2;fixed++) for(front=0;front<2;front++) {
  simptr sim=scene();filamenttypeptr t=type(sim,"actin",0);filamentptr f=rod(t,1,.005);
  panelptr wall=ceiling(sim,.02);double before;
  t->elongoverlap=overlap;t->klen=fixed?-1:10000;t->kT=0;
  t->kypr[0]=t->kypr[1]=t->kypr[2]=.05;t->plusend=front?'m':'p';
  t->confinesrf=wall->srf;t->confineforce=400;
  t->constrainttol=1e-8;t->constraintiterations=30;
  CHECK(filElongate(sim,f,.01)==overlap);CHECK(f->nseg==1+overlap);
  if(overlap) {
   int end=front?0:f->nseg;before=f->nodes[end][2];
   CHECK(front?before<0:before>.02);
   filComputeForces(f,-1,-1);CHECK(front?f->filwork->forces[end][2]>0:f->filwork->forces[end][2]<0);
   for(i=0;i<20;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
   CHECK(front?f->nodes[end][2]>before:f->nodes[end][2]<before);
   if(fixed) for(j=0;j<f->nseg;j++) CHECK(fabs(f->segments[j]->len/t->stdlen-1)<1.01*t->constrainttol);
   f->endstate[front?0:1]=FEScapped;CHECK(filElongate(sim,f,.01)==0);CHECK(f->nseg==2);
   f->endstate[front?0:1]=FESopen;t->elongmaxlen=.015;CHECK(filElongate(sim,f,.01)==0);CHECK(f->nseg==2);
  }
  simfree(sim);
 }
 puts("PASS overlap wall insertion, both plus ends, cap/length limit, elastic/fixed lengths and restoring wall forces; legacy still rejects");
}
static void overlap_capsule(void) {
 int overlap,fixed,i;
 for(overlap=0;overlap<2;overlap++) for(fixed=0;fixed<2;fixed++) {
  simptr sim=scene();filamenttypeptr t=type(sim,"actin",0),other=type(sim,"obstacle",0);
  filamentptr f=rod(t,1,0),bar=filAddFilament(other,NULL);double pos[3]={-.02,.003,.018},angle[3]={0,0,0};
  double initial,final;segmentptr nearest;
  CHECK(bar);CHECK(!filAddSegment(bar,pos,.04,angle,1,'b'));
  other->stdlen=.04;other->dynamics=FDnone;
  t->elongoverlap=overlap;t->klen=fixed?-1:10000;t->kT=0;
  t->kypr[0]=t->kypr[1]=t->kypr[2]=.05;t->stericradius=other->stericradius=.0035;
  t->constrainttol=1e-8;t->constraintiterations=30;
  CHECK(t->stericgrowth);CHECK(!filStericChemistry(sim,1));
  CHECK(filElongate(sim,f,.004)==0);CHECK(filElongate(sim,f,.006)==overlap);CHECK(f->nseg==1+overlap);
  if(overlap) {
   CHECK(sim->filss->steric->npending==1 && sim->filss->steric->pending[0]==f->segments[1]);
   CHECK(filStericQuery(sim,f->nodes[1],f->nodes[2],t->stericradius,f->segments[1],&initial,&nearest));
   CHECK(initial<0 && nearest==bar->segments[0]);
   CHECK(!sim->filss->steric->blockedgrowth);
  } else CHECK(sim->filss->steric->blockedgrowth==1);
  CHECK(!filStericChemistry(sim,0));
  if(overlap) {
   for(i=0;i<40;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
   filStericQuery(sim,f->nodes[1],f->nodes[2],t->stericradius,f->segments[1],&final,&nearest);
   CHECK(final>initial);CHECK(nearest==bar->segments[0]);NEAR(bar->nodes[0][1],.003,1e-15);
   if(fixed) for(i=0;i<f->nseg;i++) CHECK(fabs(f->segments[i]->len/t->stdlen-1)<1.01*t->constrainttol);
  }
  simfree(sim);
 }
 puts("PASS overlap capsule insertion and pending registration; contact forces reduce penetration with elastic/fixed segments; legacy still rejects");
}
static void hybrid_brownian(void) {
 int mode,n,i,j,refinements=0;double maxerror=0;
 for(mode=0;mode<2;mode++) for(n=1;n<=3;n+=2) {
  simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr f=rod(t,n,0);
  t->klen=-1;t->elongstiffness=10000;t->brownian=1;t->implicitjacobian=mode;
  t->endrate[1][FESopen]=.15;t->constrainttol=1e-8;t->constraintiterations=30;
  t->implicittol=1e-8;t->implicititerations=40;t->implicitkrylov=100;
  t->kypr[0]=t->kypr[1]=t->kypr[2]=.05;sim->dt=1e-4;
  CHECK(!filGrowthValidate(t));
  for(i=0;i<1500;i++) {
   CHECK(!filDynamics(sim));sim->time+=sim->dt;
   for(j=0;j<f->nseg-1;j++) {
    double err=fabs(f->segments[j]->len/t->stdlen-1);maxerror=fmax(maxerror,err);
    CHECK(err<=t->constrainttol*1.001);NEAR(filSegmentStiffness(f,j),0,1e-15);
   }
   CHECK(filSegmentStiffness(f,f->nseg-1)>0);
  }
  CHECK(t->elongaccepted>.01);CHECK(t->elongremesh>0);refinements+=(int)t->elongremesh;
  simfree(sim);
 }
 printf("PASS hybrid fixed interiors/compliant tip, Brownian noise, one/multiple initial segments, cached/reference derivatives; %d refinements, max relative error %g\n",refinements,maxerror);
}
static void hybrid_cap_transitions(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr a=rod(t,1,0),b=rod(t,1,.1);
 int i,swap;double length[2];
 t->klen=-1;t->elongstiffness=10000;t->endrate[1][FESopen]=.15;t->implicittol=1e-9;
 t->constraintiterations=30;t->constrainttol=1e-8;
 a->lenbank[1]=.003;b->lenbank[1]=.002;
 for(swap=0;swap<4;swap++) {
  a->endstate[1]=swap%2==0;b->endstate[1]=!a->endstate[1];
  length[0]=filNominalLength(a);length[1]=filNominalLength(b);
  CHECK(filSegmentIsConstrained(a,0)==a->endstate[1]);CHECK(filSegmentIsConstrained(b,0)==b->endstate[1]);
  for(i=0;i<100;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
  if(a->endstate[1]) {NEAR(a->segments[0]->len,length[0],1e-9);NEAR(filNominalLength(a),length[0],1e-15);}
  else {NEAR(b->segments[0]->len,length[1],1e-9);NEAR(filNominalLength(b),length[1],1e-15);}
 }
 simfree(sim);puts("PASS capping freezes partial terminal length; uncapping restores compliant growth with unchanged constraint count");
}
static void hybrid_branched(void) {
 simptr sim=scene();filamenttypeptr t=type(sim,"actin",1);filamentptr mother=rod(t,3,0),daughter;
 double angle[3]={1,0,0};int i,f,j;
 t->klen=-1;t->elongstiffness=10000;t->endrate[1][FESopen]=.15;t->brownian=1;t->implicitjacobian=1;
 t->implicittol=1e-8;t->implicititerations=40;t->constrainttol=1e-8;t->constraintiterations=30;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=.05;
 daughter=filAddBranch(sim,mother,2,angle,1,NULL);CHECK(daughter);
 for(i=0;i<2000;i++) {
  CHECK(!filDynamics(sim));sim->time+=sim->dt;
  for(f=0;f<t->nfil;f++) {
   filamentptr fil=t->fillist[f];
   for(j=0;j<fil->nseg;j++) if(filSegmentIsConstrained(fil,j))
    CHECK(fabs(fil->segments[j]->len/filSegmentRestLength(fil,j)-1)<1.01*t->constrainttol);
  }
  for(j=0;j<3;j++) NEAR(daughter->nodes[0][j],mother->nodes[mother->juncts[0]->node][j],1e-12);
 }
 CHECK(t->elongremesh>0);simfree(sim);puts("PASS hybrid shared branch node remains attached through Brownian movement and tip refinement");
}
int main(void) {
 double low,high,reference;
 parser();overlap_wall();overlap_capsule();kinetics_and_energy();refinement();force_energy_consistency();hybrid_brownian();hybrid_cap_transitions();hybrid_branched();
 low=loaded_growth(.1,1,0,1e-4);high=loaded_growth(.5,1,0,1e-4);CHECK(low>high && high>0);
 reference=loaded_growth(.1,1,1,1e-4);NEAR(reference,low,1e-6);
 reference=loaded_growth(.1,1,0,5e-5);CHECK(fabs(reference-low)/reference<.02);
 loaded_growth(1.5,1,0,1e-4);loaded_growth(.1,0,0,1e-4);steric_obstacle(0);steric_obstacle(1);
 puts("PASS force generation, load-dependent velocity, superstall arrest, legacy control and cached/banded solvers");return 0;
}
