/* Native checks for panel-owned ideal filament force clamps. */
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
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL force clamp line %d: %s\n",__LINE__,#c);exit(1);}} while(0)
#define NEAR(a,b,t) CHECK(fabs((a)-(b))<(t))
static int banded=0;
static int fixedlength=0;
static int projected=0;
static int cached=0;
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
int filAddSegment(filamentptr fil,const double *pos,double len,const double *angle,double thick,char end);
void filNodes2Angles(filamentptr fil,int lo,int hi);
void filComputeForces(filamentptr fil,int lo,int hi);
int panelinbox(simptr sim,panelptr pnl,boxptr box);
int expandboxpanels(boxptr box,int n);
static simptr scene(int dim) {
 simptr sim=simalloc("");int d;char line[64];CHECK(sim);CHECK(!simsetdim(sim,dim));Simsetrandseed(sim,1);
 for(d=0;d<dim;d++) {sprintf(line,"%c -1 1 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-4;return sim;
}
static filamenttypeptr type(simptr sim) {
 filamenttypeptr t=filAddFilamentType(sim,"actin");CHECK(t);
 t->dynamics=FDimplicitnetwork;t->kT=0;t->brownian=0;t->stdlen=0.01;t->mobility=1;t->klen=10000;
 t->implicitpreconditioner=banded;
 if(fixedlength) t->klen=-1;
 t->implicitmethod=projected;
 t->implicitjacobian=cached;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=0;t->implicittol=1e-12;t->implicititerations=20;t->implicitkrylov=80;
 t->confineforce=100;t->confineface=PFfront;t->sterick=4000;t->stericskin=0.002;return t;
}
static filamentptr rod(filamenttypeptr t,int axis,double height,double offset) {
 int d;double a[3]={0,0,0},angle[3]={0,0,0};filamentptr fil=filAddFilament(t,NULL);CHECK(fil);
 a[axis]=height;a[(axis+1)%t->filss->sim->dim]=offset;
 CHECK(!filAddSegment(fil,a,t->stdlen,angle,1,'b'));
 for(d=0;d<t->filss->sim->dim;d++) fil->nodes[1][d]=a[d];
 fil->nodes[1][(axis+1)%t->filss->sim->dim]+=t->stdlen;
 filNodes2Angles(fil,-1,-1);return fil;
}
static panelptr panel(simptr sim,int axis,int sign,double height) {
 surfaceptr surf=surfaddsurface(sim,"chamber");double pars[5];char direction[8];int d,n=0;CHECK(surf);
 for(d=0;d<sim->dim;d++) pars[n++]=d==axis?height:-0.5;
 for(d=1;d<sim->dim;d++) pars[n++]=1;
 sprintf(direction,"%c%d",sign>0?'+':'-',axis);
 CHECK(!surfaddpanel(surf,sim->dim,PSrect,direction,pars,"piston"));return surf->panels[PSrect][0];
}
static void membership(simptr sim) {
 int b,s,p,ps,j;CHECK(sim->boxs && sim->boxs->blist);
 for(b=0;b<sim->boxs->nbox;b++) {
  boxptr box=sim->boxs->blist[b];int order=0;
  for(s=0;sim->srfss && s<sim->srfss->nsrf;s++) for(ps=0;ps<PSMAX;ps++) for(p=0;p<sim->srfss->srflist[s]->npanel[ps];p++) {
   panelptr pnl=sim->srfss->srflist[s]->panels[ps][p];int count=0;
   for(j=0;j<box->npanel;j++) if(box->panel[j]==pnl) count++;
   CHECK(count==!!panelinbox(sim,pnl,box));
   if(count) {CHECK(order<box->npanel);CHECK(box->panel[order++]==pnl);}
  }
  CHECK(order==box->npanel);
 }
}
static void geometry_and_lifecycle(void) {
 int dim,axis,sign,r,move;
 for(dim=2;dim<=3;dim++) for(axis=0;axis<dim;axis++) for(sign=-1;sign<=1;sign+=2) for(r=0;r<2;r++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,axis,sign,sign<0?0.8:-0.8);
  filamentptr fil=rod(t,axis,0.2,0);double expected;
  t->confinesrf=pnl->srf;t->stericradius=r?0.0035:0;
  CHECK(!boxsetsize(sim,"boxsize",0.25));CHECK(!boxesupdate(sim));membership(sim);
  CHECK(!surfsetforceclamp(pnl,1,4,1e-10,-0.9,0.9));CHECK(sim->srfss->nforceclamp==1);
  CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));
  expected=0.2-sign*t->stericradius+sign*4/(2*t->confineforce);
  NEAR(pnl->forceclamp->trialposition,expected,1e-12);NEAR(pnl->point[0][axis],sign<0?0.8:-0.8,1e-14);
  filComputeForces(fil,-1,-1);
  NEAR(sign*(fil->filwork->forces[0][axis]+fil->filwork->forces[1][axis]),4,1e-9);
  CHECK(!surfforceclampend(sim,1));NEAR(pnl->point[0][axis],expected,1e-12);membership(sim);
  /* Cross several original boxes, both directions and exactly on a boundary. */
  for(move=0;move<3;move++) {double old=pnl->point[0][axis],shift[3]={0,0,0};shift[axis]=(move==0?0.5:move==1?0:-0.5)-old;surftranslatepanel(pnl,dim,shift);CHECK(!boxesupdatepanel(sim,pnl,old));membership(sim);}
  /* Leave an exact face, remain inside its next cell, then cross back. */
  for(move=0;move<3;move++) {double old=pnl->point[0][axis],shift[3]={0,0,0};shift[axis]=(move==0?-0.499:move==1?-0.498:-0.501)-old;surftranslatepanel(pnl,dim,shift);CHECK(!boxesupdatepanel(sim,pnl,old));membership(sim);}
  CHECK(!surfsetforceclamp(pnl,0,0,0,0,0));CHECK(!sim->srfss->nforceclamp);CHECK(!pnl->forceclamp);
  simfree(sim);
 }
 puts("PASS force clamp 2D/3D axes/front normals/radii, action-reaction, trial isolation and original-box membership/order");
}
static void dynamics_and_rollback(void) {
 int dim,sub;
 for(dim=2;dim<=3;dim++) for(sub=1;sub<=4;sub*=4) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,dim-1,-1,0.8);filamentptr fil=rod(t,dim-1,0.2,0);
  t->confinesrf=pnl->srf;t->stericradius=0.0035;t->stericsubsteps=sub;
  CHECK(!surfsetforceclamp(pnl,1,4,1e-10,-0.9,0.9));
  CHECK(!filDynamics(sim));NEAR(fil->nodes[0][dim-1],0.2-sim->dt*2,1e-10);
  NEAR(fil->nodes[1][dim-1],fil->nodes[0][dim-1],1e-11);
  NEAR(pnl->point[0][dim-1],fil->nodes[0][dim-1]+0.0035-0.02,1e-10);
  NEAR(pnl->forceclamp->reaction,4,1e-8);CHECK(!pnl->forceclamp->intrial);simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.8);filamentptr fil=rod(t,2,0.2,0);
  t->confinesrf=pnl->srf;t->stericradius=0;t->stericsubsteps=2;sim->dt=2e-4;
  CHECK(!surfsetforceclamp(pnl,1,4,1e-10,0.1797,0.9));
  CHECK(filDynamics(sim));NEAR(fil->nodes[0][2],0.2,1e-14);NEAR(pnl->point[0][2],0.8,1e-14);CHECK(!pnl->forceclamp->intrial);simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.8);t->confinesrf=pnl->srf;
  CHECK(!surfsetforceclamp(pnl,1,4,1e-10,-0.9,0.9));CHECK(filDynamics(sim));NEAR(pnl->point[0][2],0.8,1e-14);simfree(sim);
 }
 puts("PASS force clamp analytic loaded motion, substeps, later-substep full rollback and unsupported empty load");
}
static void parser_and_validation(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.8);surfaceptr surf=pnl->srf;FILE *file;char buffer[16384];size_t n;
 char valid[]="rect piston 4 1e-8 -0.9 0.9",bad[]="rect piston -4 1e-8 -0.9 0.9",off[]="rect piston off";
 t->confinesrf=surf;rod(t,2,0.2,0);
 CHECK(surfreadstring(sim,NULL,surf,"force_clamp",valid)==surf);NEAR(pnl->forceclamp->force,4,1e-14);
 CHECK(!surfreadstring(sim,NULL,surf,"force_clamp",bad));NEAR(pnl->forceclamp->force,4,1e-14);
 CHECK(!surfforceclampvalidate(sim));t->dynamics=FDeuler;CHECK(surfforceclampvalidate(sim));CHECK(filDynamics(sim));t->dynamics=FDimplicitnetwork;
 t->confineface=PFback;CHECK(surfforceclampvalidate(sim));t->confineface=PFfront;
 file=tmpfile();CHECK(file);writesurfaces(sim,file);rewind(file);n=fread(buffer,1,sizeof(buffer)-1,file);buffer[n]=0;fclose(file);
 CHECK(strstr(buffer,"force_clamp rect piston 4 "));CHECK(surfreadstring(sim,NULL,surf,"force_clamp",off)==surf);CHECK(!pnl->forceclamp);simfree(sim);
 puts("PASS force clamp parser, atomic invalid-setting rejection, dynamics/front validation, serialization and disable");
}
static void parallel_and_resize(void) {
 simptr sim[2];int run,i,j,d;double radius=0;
 for(run=0;run<2;run++) {
  filamenttypeptr t;panelptr pnl;sim[run]=scene(3);t=type(sim[run]);pnl=panel(sim[run],2,-1,0.8);t->confinesrf=pnl->srf;t->stericradius=radius;
#ifdef _OPENMP
  CHECK(!filSetWorkers(sim[run],run?4:1));
#else
  CHECK(!filSetWorkers(sim[run],1));
#endif
  for(i=0;i<600;i++) rod(t,2,0.2,-0.3+0.001*i);
  CHECK(!boxsetsize(sim[run],"boxsize",0.25));CHECK(!boxesupdate(sim[run]));
  for(i=0;i<sim[run]->boxs->nbox;i++) if(sim[run]->boxs->blist[i]->nfilseg) {
   boxptr box=sim[run]->boxs->blist[i];segmentptr *saved=box->filseg;segmentptr first=saved[0];
   CHECK(!expandboxpanels(box,5));CHECK(box->filseg==saved && box->filseg[0]==first);break;
  }
  CHECK(i<sim[run]->boxs->nbox);CHECK(!surfsetforceclamp(pnl,1,2400,1e-7,-0.9,0.9));CHECK(!filDynamics(sim[run]));membership(sim[run]);
#ifdef _OPENMP
  if(run) CHECK(sim[run]->filss->steric->workersused>1);
#endif
 }
 for(i=0;i<600;i++) for(j=0;j<2;j++) for(d=0;d<3;d++)
  CHECK(sim[0]->filss->filtypes[0]->fillist[i]->nodes[j][d]==sim[1]->filss->filtypes[0]->fillist[i]->nodes[j][d]);
 CHECK(sim[0]->srfss->srflist[0]->panels[PSrect][0]->point[0][2]==sim[1]->srfss->srflist[0]->panels[PSrect][0]->point[0][2]);
 simfree(sim[0]);simfree(sim[1]);puts("PASS force clamp 1/4-worker exact states and independent panel/segment storage lifecycle");
}
static void active_sets_and_fixed_support(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim),fixed=filAddFilamentType(sim,"fixed");
 panelptr pnl=panel(sim,2,-1,0.8);filamentptr upper;
 CHECK(fixed);t->confinesrf=pnl->srf;t->stericradius=0;
 fixed->dynamics=FDnone;fixed->kT=0;fixed->stdlen=0.01;fixed->klen=10000;
 fixed->confinesrf=pnl->srf;fixed->confineface=PFfront;fixed->confineforce=200;fixed->stericradius=0;
 upper=rod(t,2,0.2,0);rod(t,2,0.1,0.1);rod(fixed,2,0.2,0.2);
 CHECK(!surfsetforceclamp(pnl,1,4,1e-10,-0.9,0.9));
 CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));
 NEAR(pnl->forceclamp->trialposition,0.2-4.0/600,1e-12);CHECK(!surfforceclampend(sim,1));
 CHECK(!surfsetforceclamp(pnl,1,100,1e-10,-0.9,0.9));
 CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));
 NEAR(pnl->forceclamp->trialposition,0.05,1e-12);CHECK(!surfforceclampend(sim,1));
 CHECK(!surfsetforceclamp(pnl,1,0,1e-10,-0.9,0.9));
 CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));CHECK(!surfforceclampend(sim,1));
 NEAR(pnl->point[0][2],0.2,1e-12);
 CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));CHECK(!surfforceclampend(sim,1));
 NEAR(pnl->oldpoint[0][2],pnl->point[0][2],1e-14);
 /* A growing protrusion makes a loaded piston retreat, without a speed command. */
 upper->nodes[0][2]+=0.1;upper->nodes[1][2]+=0.1;filNodes2Angles(upper,-1,-1);
 CHECK(!surfsetforceclamp(pnl,1,4,1e-10,-0.9,0.9));
 CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampsolve(sim));
 NEAR(pnl->forceclamp->trialposition,0.28,1e-12);CHECK(!surfforceclampend(sim,1));
 simfree(sim);puts("PASS contact active-set changes, mixed stiffness/fixed support, zero load, old geometry and growth-driven retreat");
}

static void approach_and_feedback(void) {
 int dim,axis,sign,sub,i;
 for(dim=2;dim<=3;dim++) for(axis=0;axis<dim;axis++) for(sign=-1;sign<=1;sign+=2) for(sub=1;sub<=4;sub*=4) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim);double start=sign<0?0.8:-0.8;
  panelptr pnl=panel(sim,axis,sign,start);filamentptr fil=rod(t,axis,0.2,0);
  /* Avoid the material-frame pole of the generic rod helper (+z tangent). */
  if(dim==3 && axis==1) {fil->nodes[1][2]=fil->nodes[0][2];fil->nodes[1][0]=fil->nodes[0][0]+t->stdlen;filNodes2Angles(fil,-1,-1);}
  t->confinesrf=pnl->srf;t->stericradius=0.0035;t->stericsubsteps=sub;
  CHECK(!surfsetforceclamp(pnl,1,0.1,1e-10,-0.9,0.9));CHECK(!surfsetforceclampapproach(pnl,0.5,0));
  for(i=0;i<3;i++) {CHECK(!filDynamics(sim));NEAR(pnl->point[0][axis],start+sign*(i+1)*0.5*sim->dt,1e-12);NEAR(fil->nodes[0][axis],0.2,1e-12);CHECK(!pnl->forceclamp->engaged);}
  CHECK(!surfforceclampbegin(sim));CHECK(!surfforceclampstep(sim));
  {unsigned long long scans=pnl->forceclamp->evaluations;double frozen=pnl->forceclamp->trialposition;
   for(i=0;i<100;i++) CHECK(!surfforceclampsolve(sim));
   CHECK(scans==pnl->forceclamp->evaluations);NEAR(pnl->forceclamp->trialposition,frozen,1e-14);
  }
  CHECK(!surfforceclampend(sim,0));simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.205);filamentptr fil=rod(t,2,0.2,0);
  double loaded;sim->dt=0.001;t->confinesrf=pnl->srf;t->stericradius=0.0035;t->mobility=0;
  CHECK(!surfsetforceclamp(pnl,1,0.1,1e-10,-0.9,0.9));CHECK(!surfsetforceclampapproach(pnl,10,0));
  CHECK(!filDynamics(sim));CHECK(pnl->forceclamp->engaged);loaded=pnl->point[0][2];NEAR(loaded,0.203,1e-10);
  CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],loaded,1e-12);NEAR(pnl->forceclamp->reaction,0.1,1e-9);
  for(i=0;i<2;i++) fil->nodes[i][2]+=0.05;
  filNodes2Angles(fil,-1,-1);CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],loaded+0.05,1e-10);NEAR(pnl->forceclamp->reaction,0.1,1e-9);
  for(i=0;i<2;i++) fil->nodes[i][2]-=0.02;
  filNodes2Angles(fil,-1,-1);CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],loaded+0.03,1e-10);NEAR(pnl->forceclamp->reaction,0.1,1e-9);
  simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,-0.899);t->confinesrf=pnl->srf;sim->dt=0.01;
  CHECK(!surfsetforceclamp(pnl,1,0.1,1e-10,-0.9,0.9));CHECK(!surfsetforceclampapproach(pnl,1,0));
  CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],-0.9,1e-12);CHECK(!pnl->forceclamp->engaged);
  CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],-0.9,1e-12);simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.204);filamentptr fil=rod(t,2,0.2,0);
  t->confinesrf=pnl->srf;t->stericradius=0.0035;t->stericsubsteps=4;sim->dt=0.002;
  CHECK(!surfsetforceclamp(pnl,1,1000,1e-10,-0.9,0.9));CHECK(!surfsetforceclampapproach(pnl,1,0));
  /* Phase selected on a later substep must disappear when the outer interval
     is rejected, just like the already-tested filament/plate rollback. */
  CHECK(!surfforceclampbegin(sim));sim->dt/=4;
  CHECK(!surfforceclampstep(sim));CHECK(!surfforceclampsolve(sim));CHECK(!pnl->forceclamp->trialengaged);
  CHECK(!surfforceclampstep(sim));CHECK(!surfforceclampsolve(sim));CHECK(pnl->forceclamp->trialengaged);
  CHECK(!surfforceclampend(sim,0));NEAR(pnl->point[0][2],0.204,1e-14);NEAR(fil->nodes[0][2],0.2,1e-14);
  CHECK(!pnl->forceclamp->engaged && !pnl->forceclamp->trialengaged && !pnl->forceclamp->intrial);simfree(sim);
 }
 {simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.205);filamentptr fil=rod(t,2,0.2,0);
  t->confinesrf=pnl->srf;t->stericradius=0.0035;t->mobility=0;sim->dt=0.001;
  CHECK(!surfsetforceclamp(pnl,1,4,1e-10,0.202,0.9));CHECK(!surfsetforceclampapproach(pnl,10,0));
  CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],0.202,1e-12);NEAR(pnl->forceclamp->reaction,0.3,1e-10);
  for(i=0;i<2;i++) fil->nodes[i][2]+=0.1;
  filNodes2Angles(fil,-1,-1);CHECK(!filDynamics(sim));NEAR(pnl->point[0][2],0.2835,1e-10);NEAR(pnl->forceclamp->reaction,4,1e-9);simfree(sim);
 }
 puts("PASS finite approach: axes/normals/substeps, frozen trials without repeated scans, contact/hold/up/down, empty stop and phase rollback");
}

static void approach_parser(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr pnl=panel(sim,2,-1,0.8);surfaceptr surf=pnl->srf;FILE *file;char buffer[16384];size_t n;
 char valid[]="rect piston 0.1 1e-8 -0.9 0.9 approach 0.5",bad[]="rect piston 8 1e-8 -0.9 0.9 approach -1";
 char extra[]="rect piston 8 1e-8 -0.9 0.9 approach 0.5 loaded extra",loaded[]="rect piston 0.1 1e-8 -0.9 0.9 approach 0.5 loaded";
 t->confinesrf=surf;rod(t,2,0.2,0);
 CHECK(surfreadstring(sim,NULL,surf,"force_clamp",valid)==surf);NEAR(pnl->forceclamp->approachspeed,0.5,1e-14);CHECK(!pnl->forceclamp->engaged);
 CHECK(!surfreadstring(sim,NULL,surf,"force_clamp",bad));CHECK(!surfreadstring(sim,NULL,surf,"force_clamp",extra));
 NEAR(pnl->forceclamp->force,0.1,1e-14);NEAR(pnl->forceclamp->approachspeed,0.5,1e-14);CHECK(!pnl->forceclamp->engaged);
 file=tmpfile();CHECK(file);writesurfaces(sim,file);rewind(file);n=fread(buffer,1,sizeof(buffer)-1,file);buffer[n]=0;fclose(file);CHECK(strstr(buffer," approach 0.5\n"));
 CHECK(surfreadstring(sim,NULL,surf,"force_clamp",loaded)==surf);CHECK(pnl->forceclamp->engaged);
 file=tmpfile();CHECK(file);writesurfaces(sim,file);rewind(file);n=fread(buffer,1,sizeof(buffer)-1,file);buffer[n]=0;fclose(file);CHECK(strstr(buffer," approach 0.5 loaded\n"));
 simfree(sim);puts("PASS approach parser, invalid-setting atomicity and serialized contact phase");
}

static void opposing_panels(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim);panelptr ceiling=panel(sim,2,-1,0.8),floor;
 double pars[5]={-0.5,-0.5,-0.8,1,1};filamentptr fil;
 CHECK(!surfaddpanel(ceiling->srf,3,PSrect,"+2",pars,"floor"));floor=ceiling->srf->panels[PSrect][1];
 t->confinesrf=ceiling->srf;t->stericradius=0.0035;fil=rod(t,2,0.2,0);
 CHECK(!boxsetsize(sim,"boxsize",0.25));CHECK(!boxesupdate(sim));
 CHECK(!surfsetforceclamp(ceiling,1,0.1,1e-10,-0.9,0.9));CHECK(!surfsetforceclamp(floor,1,0.1,1e-10,-0.9,0.9));
 CHECK(sim->srfss->nforceclamp==2);CHECK(!filDynamics(sim));
 NEAR(fil->nodes[0][2],0.2,1e-12);NEAR(ceiling->point[0][2],0.203,1e-12);NEAR(floor->point[0][2],0.197,1e-12);
 NEAR(ceiling->forceclamp->reaction,0.1,1e-10);NEAR(floor->forceclamp->reaction,0.1,1e-10);membership(sim);
 CHECK(!surfsetforceclamp(floor,0,0,0,0,0));CHECK(sim->srfss->nforceclamp==1);simfree(sim);
 puts("PASS simultaneous opposing clamps, independent panel ownership and cancelling loads");
}

int main(int argc,char **argv) {banded=argc>1 && !strcmp(argv[1],"--banded");cached=argc>1 && !strcmp(argv[1],"--cached");projected=argc>1 && !strcmp(argv[1],"--projected");fixedlength=cached || projected || (argc>1 && !strcmp(argv[1],"--fixed-length"));opposing_panels();geometry_and_lifecycle();active_sets_and_fixed_support();dynamics_and_rollback();parser_and_validation();parallel_and_resize();approach_and_feedback();approach_parser();puts("All force clamp checks passed.");return 0;}
