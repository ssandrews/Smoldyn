/* Compatibility checks at the boundary between upstream structures and the
   optional network extensions. All topology/state writes use native APIs. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "random2.h"
#undef CHECK
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL upstream compatibility line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
int filAddSegment(filamentptr fil,const double *pos,double length,const double *angle,double thick,char end);
filamentptr filAddBranch(simptr sim,filamentptr mother,int node,char end,double length,const double *angle,double thick,const char *name);
int filAttachEnd(filamentptr fil,char end,filamentptr target,int node);
int filBoxesCheck(simptr sim);
double filEndPushForce(const filamentptr fil,int end);
double filEndVelocity(const filamentptr fil,int end);
void filSurfReactionForces(simptr sim);
void filNodes2Angles(filamentptr fil,int lo,int hi);

static simptr scene(void) {
 simptr sim=simalloc("");char line[32];CHECK(sim);CHECK(!simsetdim(sim,3));Simsetrandseed(sim,17);
 for(int d=0;d<3;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-4;return sim;
}
static filamenttypeptr type(simptr sim) {
 filamenttypeptr t=filAddFilamentType(sim,"actin");CHECK(t);
 t->dynamics=FDimplicitnetwork;t->stdlen=.01;t->klen=-1;t->kT=0;t->brownian=0;t->mobility=1;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=.05;
 t->implicittol=1e-8;t->implicititerations=40;t->implicitkrylov=100;t->implicitjacobian=1;t->implicitpreconditioner=1;
 t->constrainttol=1e-8;t->constraintiterations=30;return t;
}
static filamentptr rod(filamenttypeptr t,int n) {
 double pos[3]={0,0,0},angle[3]={0,0,0};filamentptr fil=filAddFilament(t,NULL);CHECK(fil);
 for(int j=0;j<n;j++) CHECK(!filAddSegment(fil,j?NULL:pos,t->stdlen,angle,1,'p'));
 filNodes2Angles(fil,-1,-1);return fil;
}
static void gaussian_reseed(void) {
 simptr sim=scene();double first[3],again[3];
 for(int repeat=0;repeat<2;repeat++) {Simsetrandseed(sim,19);for(int j=0;j<3;j++) (repeat?again:first)[j]=gaussrandD();}
 CHECK(!memcmp(first,again,sizeof(first)));simfree(sim);
 puts("PASS reseeding after odd Gaussian draws clears paired cache");
}
static void native_end_states(void) {
 simptr sim=scene();filamenttypeptr t=type(sim);filamentptr fil=rod(t,1);
 t->dynamics=FDnone;sim->dt=.1;t->endrate[0][FESopen]=.2;t->endrate[1][FESopen]=.1;
 CHECK(!boxesupdate(sim));CHECK(sim->boxs->nbox==1); /* native automatic sizing */
 CHECK(!filDynamics(sim));CHECK(fil->nseg==4);
 t->endrate[1][FESshrink]=-.1;fil->endstate[0]=FEScapped;fil->endstate[1]=FESshrink;
 CHECK(!filDynamics(sim));CHECK(fil->nseg==3);CHECK(fil->endstate[0]==FEScapped);
 simfree(sim);puts("PASS native two-ended growth, capped minus end and plus-end shrink state");
}
static void plus_junction_and_loop(void) {
 simptr sim=scene();filamenttypeptr t=type(sim);filamentptr mother=rod(t,4),daughter;double angle[3]={1,0,0},before[3]={0,0,0};
 t->stericradius=.0035;t->sterick=4000;t->stericskin=.002;t->branchsegments=2;t->junctforce=100;
 daughter=filAddBranch(sim,mother,2,'p',0,angle,1,NULL);CHECK(daughter);
 CHECK(daughter->endjunct[1] && daughter->endjunct[1]->rigid);
 CHECK(!filStericPrepare(sim));CHECK(!filStericPrepareMovement(sim));
 CHECK(sim->filss->steric->nodes[daughter->stericnodeoffset+daughter->nseg].parent==sim->filss->steric->nodes[mother->stericnodeoffset+2].parent);
 CHECK(!filAttachEnd(daughter,'m',mother,4));CHECK(sim->filss->nspring==1);
 for(int f=0;f<t->nfil;f++) for(int j=0;j<=t->fillist[f]->nseg;j++) for(int d=0;d<3;d++) before[d]+=t->fillist[f]->nodes[j][d];
 for(int i=0;i<40;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
 for(int d=0;d<3;d++) CHECK(fabs(daughter->nodes[daughter->nseg][d]-mother->nodes[2][d])<1e-12);
 for(int f=0;f<t->nfil;f++) for(int j=0;j<t->fillist[f]->nseg;j++) CHECK(fabs(t->fillist[f]->segments[j]->len/t->stdlen-1)<1.01*t->constrainttol);
 /* Loop spring forces are internal; shared-node drag includes both members. */
 for(int d=0;d<3;d++) {double after=0;for(int f=0;f<t->nfil;f++) for(int j=0;j<=t->fillist[f]->nseg;j++) after+=t->fillist[f]->nodes[j][d];CHECK(fabs(after-before[d])<1e-7);}
 CHECK(!filBoxesCheck(sim));simfree(sim);
 puts("PASS plus-end branch, native loop-closing junction spring, shared node/length constraints, force balance and native box oracle");
}
static void legacy_force_bridge(void) {
 simptr sim=scene();filamenttypeptr t=type(sim);filamentptr fil=rod(t,1);surfaceptr wall=surfaddsurface(sim,"wall");
 double pars[5]={.005,-.5,-.5,1,1};CHECK(wall);CHECK(!surfaddpanel(wall,3,PSrect,"-0",pars,"wall"));
 t->klen=10000;t->confinesrf=wall;t->confineforce=100;
 fil->endstate[1]=FESbarbed;t->endrate[1][FESbarbed]=.1;t->barbedparameters[1][0]=2;
 CHECK(fabs(filEndPushForce(fil,1)-.5)<1e-12);
 CHECK(fabs(filEndVelocity(fil,1)-.1*exp(-1))<1e-12);
 t->elongoverlap=1;CHECK(fabs(filEndVelocity(fil,1)-.1)<1e-12);
 filSurfReactionForces(sim);CHECK(fabs(wall->force[0]-.5)<1e-12);
 CHECK(fabs(wall->panels[PSrect][0]->force[0]-.5)<1e-12);
 simfree(sim);puts("PASS legacy wall forces feed native barbed kinetics and surface reactions; overlap growth disables force feedback");
}
int main(void) {gaussian_reseed();native_end_states();plus_junction_and_loop();legacy_force_bridge();return 0;}
