/* Native checks for geometry, spatial indexing and coupled filament mechanics. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "Geometry.h"
filamenttypeptr filAddFilamentType(simptr sim,const char *name);
int filtypeSetParam(filamenttypeptr type,const char *param,int index,double value);
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thick,char end);
filamentptr filAddBranch(simptr sim,filamentptr mother,int seg,const double *angle,double thick,const char *name);
void filCopyRollToYpr2(filamentptr fil);
void filCopyYpr2ToRoll(filamentptr fil);
void filComputeForces(filamentptr fil,int lo,int hi);
void filNodes2Angles(filamentptr fil,int lo,int hi);
int filElongate(simptr sim,filamentptr fil,double length);
int filRemoveSegment(filamentptr fil,char end);
#undef CHECK
#define CHECK(c) do {if(!(c)) {fprintf(stderr,"FAIL line %i: %s\n",__LINE__,#c);exit(1);}} while(0)
#define NEAR(a,b,tol) CHECK(fabs((a)-(b))<(tol))
static unsigned int state=123;
static double uniform(void) {state=1664525u*state+1013904223u;return (state>>8)/16777216.0;}
static simptr scene(int dim) {
 simptr sim=simalloc("");char line[64];int d;
 CHECK(sim);CHECK(!simsetdim(sim,dim));
 Simsetrandseed(sim,1);
 for(d=0;d<dim;d++) {sprintf(line,"%c -2 2 r","xyz"[d]);CHECK(!simreadstring(sim,NULL,"boundaries",line));}
 sim->dt=1e-5;sim->time=0;return sim;
}
static filamenttypeptr type(simptr sim,const char *name,int fixed) {
 filamenttypeptr t=filAddFilamentType(sim,name);CHECK(t);
 t->dynamics=fixed?FDnone:FDeuler;t->kT=0;t->stdlen=0.01;t->klen=10000;t->kypr[0]=t->kypr[1]=t->kypr[2]=0;
 t->stericradius=0.0035;t->sterick=4000;t->stericskin=0.002;return t;
}
static filamentptr rod(filamenttypeptr t,const double *a,const double *b) {
 double angle[3]={0,0,0},len=0;int d;filamentptr f=filAddFilament(t,NULL);CHECK(f);
 for(d=0;d<3;d++) len+=(b[d]-a[d])*(b[d]-a[d]);
 CHECK(!filAddSegment(f,a,sqrt(len),angle,1,'b'));
 for(d=0;d<3;d++) f->nodes[1][d]=b[d];
 if(fabs(b[2]-a[2])>0.95*sqrt(len)) {f->seg0up[0]=1;f->seg0up[2]=0;}
 filNodes2Angles(f,-1,-1);return f;
}
static double distance(segmentptr a,segmentptr b) {
 double s,t,n[3];return Geo_ClosestSeg2Seg(a->xyzfront,a->xyzback,b->xyzfront,b->xyzback,3,&s,&t,n);
}
static void geometry(void) {
 double a[3]={0,0,0},b[3]={0.01,0,0},c[3]={0.002,0.006,0},d[3]={0.008,0.006,0},s,t,n[3];
 NEAR(Geo_ClosestSeg2Seg(a,b,c,d,3,&s,&t,n),0.006,1e-12);NEAR(s,0.5,1e-12);NEAR(t,0.5,1e-12);
 c[0]=d[0]=0.005;c[1]=-0.005;d[1]=0.005;c[2]=d[2]=0.006;
 NEAR(Geo_ClosestSeg2Seg(a,b,c,d,3,&s,&t,n),0.006,1e-12);NEAR(s,0.5,1e-12);NEAR(t,0.5,1e-12);
 c[2]=d[2]=0;NEAR(Geo_ClosestSeg2Seg(a,b,c,d,3,&s,&t,n),0,1e-12);NEAR(fabs(n[2]),1,1e-12);
 NEAR(Geo_ClosestSeg2Seg(a,a,c,d,3,&s,&t,n),0.005,1e-12);
 c[0]=c[1]=c[2]=0;NEAR(Geo_ClosestSeg2Seg(a,a,c,c,3,&s,&t,n),0,1e-12);CHECK(isfinite(n[0]));
 puts("PASS closest-point geometry (nm scale, parallel, crossing, degenerate)");
}
static void indexing(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);int i,j,p,found;unsigned long long rebuild;
 for(i=0;i<250;i++) {double a[3],b[3];for(j=0;j<3;j++) {a[j]=(uniform()-0.5)*0.1;b[j]=a[j]+(uniform()-0.5)*0.025;}rod(t,a,b);}
 CHECK(!filStericPrepare(sim));
 for(i=0;i<250;i++) for(j=i+1;j<250;j++) if(distance(t->fillist[i]->segments[0],t->fillist[j]->segments[0])<=0.009) {
  found=0;for(p=0;p<sim->filss->steric->npair;p++) if(sim->filss->steric->pairs[p].a==i && sim->filss->steric->pairs[p].b==j) found++;CHECK(found==1);
 }
 rebuild=sim->filss->steric->rebuilds;CHECK(!filStericPrepare(sim));CHECK(sim->filss->steric->rebuilds==rebuild);
 t->fillist[0]->nodes[0][0]+=0.002;CHECK(!filStericPrepare(sim));CHECK(sim->filss->steric->rebuilds>rebuild);
 rebuild=sim->filss->steric->rebuilds;{double a[3]={0,0,0},b[3]={0.01,0,0};rod(t,a,b);}
 CHECK(!filStericPrepare(sim));CHECK(sim->filss->steric->rebuilds>rebuild);
 simfree(sim);puts("PASS boxes versus exhaustive oracle, unique pairs, movement/topology invalidation");
}
static void shared_grids(void) {
 int dim,i,d;BoxGridCell cell;
 double p[3]={-0.015,0.035,0},a[3]={-0.02,0.03,0},b[3]={-0.01,0.04,0};
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);CHECK(!boxsetsize(sim,"boxsize",0.5));CHECK(!boxesupdate(sim));
  CHECK(sim->boxs->grid.dense==sim->boxs);
  for(i=0;i<100;i++) {
   for(d=0;d<dim;d++) p[d]=(uniform()-0.5)*6;
   CHECK(boxGridFindPoint(&sim->boxs->grid,p,&cell));CHECK(cell.box==pos2box(sim,p));
   CHECK(cell.box==pos2boxInGrid(sim->boxs,dim,p));
   box2posInGrid(sim->boxs,dim,cell.box,a,b);
   for(d=0;d<dim;d++) {
    int expected=(int)floor((p[d]+2)/0.5);if(expected<0) expected=0;if(expected>7) expected=7;
    CHECK(cell.index[d]==expected);NEAR(a[d],-2+0.5*expected,1e-12);NEAR(b[d],a[d]+0.5,1e-12);
   }
  }
  CHECK(!boxsetsize(sim,"boxsize",1));CHECK(!boxesupdate(sim));
  NEAR(sim->boxs->grid.width[0],1,1e-12);simfree(sim);
 }
 puts("PASS original shared grid, boundary clamping and explicit resize lifecycle");
}

static double query_oracle(simptr sim,const double *a,const double *b,double radius,segmentptr *nearest) {
 int ft,f,i;double best=DBL_MAX,s,t,n[3];*nearest=NULL;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr type=sim->filss->filtypes[ft];if(type->stericradius<=0) continue;
  for(f=0;f<type->nfil;f++) for(i=0;i<type->fillist[f]->nseg;i++) {
   segmentptr seg=type->fillist[f]->segments[i];
   double gap=Geo_ClosestSeg2Seg(a,b,seg->xyzfront,seg->xyzback,sim->dim,&s,&t,n)-radius-type->stericradius;
   if(gap<best) {best=gap;*nearest=seg;}
  }
 }
 return best;
}

static void unified_boxes(void) {
 int dim,d,i,seen,warn;double a[3]={-0.025,0,0},b[3]={0.025,0,0},p[3]={0,0.003,0};
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",1);filamentptr f=rod(t,a,b);
  boxptr cell;unsigned long long generation,rebuilds;int count;
  CHECK(!simreadstring(sim,NULL,"filament_box_density","50000"));
  CHECK(!boxsetsize(sim,"boxsize",0.1));CHECK(!boxesupdate(sim));
  cell=pos2box(sim,p);seen=0;
  for(i=0;i<cell->nsegment;i++) if(cell->segment[i]==f->segments[0]) seen++;
  CHECK(seen==1);CHECK(cell->maxsegment>=cell->nsegment);
  /* Axis-aligned centerline on a box face and physical capsule just across it. */
  CHECK(segmentinbox(sim,f->segments[0],pos2box(sim,a)));
  CHECK(filPointInFilament(sim,p,NULL,NULL)==f->segments[0]);
  CHECK(!filStericPrepare(sim));generation=sim->boxs->segmentgeneration;
  rebuilds=sim->filss->steric->rebuilds;count=sim->boxs->nbox;
  for(i=0;i<60;i++) {double aa[3]={0,0.05,0},bb[3]={0.01,0.05,0};aa[2]=bb[2]=dim==3?0.02:0;rod(t,aa,bb);}
  CHECK(!filStericPrepare(sim));CHECK(sim->boxs->nbox==count);
  CHECK(sim->boxs->segmentgeneration>generation && sim->filss->steric->rebuilds>rebuilds);
  /* Replacing box arrays invalidates the contact cache, even with unchanged topology. */
  CHECK(!boxsetsize(sim,"boxsize",0.08));CHECK(!boxesupdate(sim));
  CHECK(filPointInFilament(sim,p,NULL,NULL)==f->segments[0]);
  CHECK(!checkboxparams(sim,&warn));
  /* Removal clears old payload on the next cache rebuild. */
  f->nseg=0;CHECK(!filStericPrepare(sim));
  CHECK(!filPointInFilament(sim,p,NULL,NULL));
  for(i=0;i<sim->boxs->nsegmentbox;i++) {
   boxptr box=sim->boxs->segmentbox[i];int s;
   for(s=0;s<box->nsegment;s++) CHECK(box->segment[s]!=f->segments[0]);
  }
  simfree(sim);
 }
 /* Initialization uses anticipated density rather than the seed count. */
 {simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",1);rod(t,a,b);
  CHECK(!simreadstring(sim,NULL,"filament_box_density","100000"));CHECK(!boxesupdate(sim));
  CHECK(sim->boxs->nbox>1 && sim->boxs->nbox<=131072);
  for(d=0;d<3;d++) CHECK(sim->boxs->size[d]>0);
  {int old=sim->boxs->nbox;boxsetcondition(sim->boxs,SClists,0);CHECK(!boxesupdate(sim));CHECK(sim->boxs->nbox==old);}
  simfree(sim);
 }
 /* Disabled sterics still records finite centerlines in the original boxes. */
 {simptr sim=scene(2);filamenttypeptr t=type(sim,"plain",1);filamentptr f=rod(t,a,b);boxptr box;
  t->stericradius=0;CHECK(!boxsetsize(sim,"boxsize",0.1));CHECK(!boxesupdate(sim));
  box=pos2box(sim,a);seen=0;for(i=0;i<box->nsegment;i++) seen+=box->segment[i]==f->segments[0];CHECK(seen==1);
  f->nodes[0][1]=f->nodes[1][1]=0.5;CHECK(!filDynamics(sim));
  CHECK(box->nsegment==0);CHECK(pos2box(sim,f->nodes[0])->nsegment==1);simfree(sim);
 }
 /* Molecules, panels and segments share the very same box; rebuilding contact
    payload must leave molecule ownership and static panel storage untouched. */
 {simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",1);surfaceptr floor=surfaddsurface(sim,"floor");
  double panel[5]={-1,-1,0,2,2};int id=moladdspecies(sim,"monomer"),ll;boxptr box;moleculeptr mol=NULL;panelptr originalpanel;
  rod(t,a,b);CHECK(floor && id>0);CHECK(!surfaddpanel(floor,3,PSrect,"+2",panel,"base"));
  CHECK(!molsetmaxmol(sim,10));CHECK(!addmol(sim,1,id,p,p,0));CHECK(!boxsetsize(sim,"boxsize",0.1));CHECK(!simupdate(sim));
  for(ll=0;ll<sim->mols->nlist;ll++) for(i=0;i<sim->mols->nl[ll];i++) if(sim->mols->live[ll][i]->ident==id) mol=sim->mols->live[ll][i];
  CHECK(mol);box=mol->box;CHECK(box==pos2box(sim,p));CHECK(box->npanel>0 && box->nsegment>0);originalpanel=box->panel[0];
  CHECK(!filStericPrepare(sim));rod(t,a,b);CHECK(!filStericPrepare(sim));
  CHECK(mol->box==box && box->npanel>0 && box->panel[0]==originalpanel);
  CHECK(box->nmol[mol->list]==1);simfree(sim);
 }
 puts("PASS unified box payload, cross-face capsule query, removal, cache invalidation and fixed anticipated-density resolution");
}

static void queries(void) {
 int dim,i,d;double a[3]={0,0,0},b[3]={0.01,0,0},p[3],dist,expected;
 segmentptr nearest,want,hit;filamentptr fptr;
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);
  for(i=0;i<100;i++) {
   for(d=0;d<3;d++) a[d]=b[d]=0;
   for(d=0;d<dim;d++) {a[d]=(uniform()-0.5)*0.15;b[d]=a[d]+(uniform()-0.5)*0.02;}
   rod(t,a,b);
  }
  for(i=0;i<300;i++) {
   for(d=0;d<3;d++) p[d]=a[d]=b[d]=0;
   for(d=0;d<dim;d++) {p[d]=(uniform()-0.5)*0.2;a[d]=p[d];b[d]=a[d]+(uniform()-0.5)*0.03;}
   expected=query_oracle(sim,p,p,0,&want);
   hit=filPointInFilament(sim,p,&dist,&nearest);NEAR(dist,expected,1e-12);CHECK(nearest==want);
   CHECK((hit!=NULL)==(expected<=0));CHECK((filPointInFilament(sim,p,NULL,NULL)!=NULL)==(expected<=0));
   expected=query_oracle(sim,a,b,0.002,&want);
    hit=filLineXFilament(sim,a,b,0.002,&dist,&nearest);NEAR(dist,expected,1e-12);CHECK(nearest);
    /* Several 2D crossings can be equally nearest; any tied segment is valid. */
    {double s,tt,n[3];NEAR(Geo_ClosestSeg2Seg(a,b,nearest->xyzfront,nearest->xyzback,dim,&s,&tt,n)-0.002-nearest->fil->filtype->stericradius,expected,1e-12);}
   CHECK((hit!=NULL)==(expected<=0));CHECK((filLineXFilament(sim,a,b,0.002,NULL,NULL)!=NULL)==(expected<=0));
  }
  /* A nearest query far outside occupied cells still finds the global answer. */
  p[0]=10;p[1]=11;p[2]=dim==3?12:0;expected=query_oracle(sim,p,p,0,&want);
  CHECK(!filPointInFilament(sim,p,&dist,&nearest));NEAR(dist,expected,1e-12);CHECK(nearest==want);
  /* Very long diagonal lines use an occupied-object pass, not a huge cell cube. */
  for(d=0;d<dim;d++) {a[d]=-100;b[d]=100;}
  expected=query_oracle(sim,a,b,0,&want);
  CHECK((filLineXFilament(sim,a,b,0,NULL,NULL)!=NULL)==(expected<=0));
  CHECK(!filStericChemistry(sim,1));
  for(d=0;d<3;d++) a[d]=b[d]=p[d]=0;
  a[0]=b[0]=p[0]=0.4;b[1]=0.01;p[1]=0.005;
  {filamentptr fresh=rod(t,a,b);int before;
   CHECK(!filStericRegisterSegment(sim,fresh->segments[0]));before=sim->filss->steric->npending;
   CHECK(filPointInFilament(sim,p,NULL,NULL)==fresh->segments[0]);CHECK(sim->filss->steric->npending==before);
   fresh->nseg=0;CHECK(!filPointInFilament(sim,p,NULL,NULL));
  }
  CHECK(!filStericChemistry(sim,0));
  p[0]=NAN;CHECK(!filPointInFilament(sim,p,&dist,&nearest));CHECK(isnan(dist) && !nearest);
  simfree(sim);
 }
 /* Capsule query at a shaft/endcap and legacy segment API use physical radius. */
 {simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr rod1,rod2;
  a[0]=a[1]=a[2]=0;b[0]=0.01;b[1]=b[2]=0;rod1=rod(t,a,b);
  p[0]=-0.002;p[1]=p[2]=0;CHECK(filPointInFilament(sim,p,&dist,&nearest)==rod1->segments[0]);NEAR(dist,-0.0015,1e-12);
  a[0]=b[0]=0.005;a[1]=-0.005;b[1]=0.005;a[2]=b[2]=0.006;rod2=rod(t,a,b);
  CHECK(filSegmentXFilament(sim,rod2->segments[0],&fptr));CHECK(fptr==rod1);
  a[2]=b[2]=0.1;rod2=rod(t,a,b);CHECK(!filSegmentXFilament(sim,rod2->segments[0],&fptr));CHECK(!fptr);
  {filamenttypeptr wide=type(sim,"wide",1);filamentptr larger;
   wide->stericradius=0.02;a[0]=0;b[0]=0.01;a[1]=b[1]=0.015;a[2]=b[2]=0;larger=rod(wide,a,b);
   p[0]=0.005;p[1]=0.005;p[2]=0;
   CHECK(filPointInFilament(sim,p,&dist,&nearest)==larger->segments[0]);NEAR(dist,-0.01,1e-12);
  }
  simfree(sim);
 }
 {simptr sim=scene(2);type(sim,"empty",0);p[0]=p[1]=0;
  CHECK(!filPointInFilament(sim,p,&dist,&nearest));CHECK(dist==DBL_MAX && !nearest);simfree(sim);
 }
 puts("PASS point/line/segment capsule queries versus exhaustive oracle, exact global nearest and chemistry visibility");
}

static void growth_switch(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),ob=type(sim,"obstacle",1);
 double a[3]={0,0,0},b[3]={0.01,0,0},c[3]={0.015,-0.005,0},d[3]={0.015,0.005,0},p[3]={0.016,0,0};
 filamentptr f=rod(t,a,b);char off[]="0",on[]="1",bad[]="2";
 rod(ob,c,d);t->kypr[0]=t->kypr[1]=t->kypr[2]=1;
 CHECK(filtypeReadString(sim,NULL,t,"steric_growth_check",bad)==NULL);
 CHECK(filtypeReadString(sim,NULL,t,"steric_growth_check",off)==t);CHECK(!t->stericgrowth);
 CHECK(!filStericChemistry(sim,1));CHECK(filElongate(sim,f,0.011)==1);CHECK(f->nseg==2);
 CHECK(sim->filss->steric->npending==1);CHECK(filPointInFilament(sim,p,NULL,NULL)!=NULL);
 CHECK(filtypeReadString(sim,NULL,t,"steric_growth_check",on)==t);CHECK(t->stericgrowth);
 CHECK(!filStericChemistry(sim,0));
 filComputeForces(f,-1,-1);CHECK(!filStericForces(sim));CHECK(sim->filss->steric->contacts>0);
 simfree(sim);puts("PASS independent growth overlap control, unchecked insertions remain indexed and forces stay active");
}

static void forces(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr a,b;int i,node,d;
 double a0[3]={-0.005,0,0},a1[3]={0.005,0,0},b0[3]={0,-0.005,0.006},b1[3]={0,0.005,0.006};
 double total[3]={0,0,0},torque[3]={0,0,0},fref,plus,minus,h=1e-8;
 a=rod(t,a0,a1);b=rod(t,b0,b1);filComputeForces(a,-1,-1);filComputeForces(b,-1,-1);CHECK(!filStericForces(sim));CHECK(sim->filss->steric->contacts==1);
 for(i=0;i<2;i++) for(node=0;node<2;node++) {
  filamentptr fil=t->fillist[i];double *x=fil->nodes[node],*f=fil->filwork->forces[node];
  for(d=0;d<3;d++) total[d]+=f[d];
  torque[0]+=x[1]*f[2]-x[2]*f[1];torque[1]+=x[2]*f[0]-x[0]*f[2];torque[2]+=x[0]*f[1]-x[1]*f[0];
 }
 for(d=0;d<3;d++) {NEAR(total[d],0,1e-10);NEAR(torque[d],0,1e-10);}
 fref=a->filwork->forces[0][2];a->nodes[0][2]+=h;plus=0.5*t->sterick*pow(0.007-distance(a->segments[0],b->segments[0]),2);
 a->nodes[0][2]-=2*h;minus=0.5*t->sterick*pow(0.007-distance(a->segments[0],b->segments[0]),2);a->nodes[0][2]+=h;NEAR(fref,-(plus-minus)/(2*h),1e-5);
 for(i=0;i<250;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}CHECK(distance(a->segments[0],b->segments[0])>0.00699);
 simfree(sim);puts("PASS contact gradient, force/torque balance and overlap relaxation");
}
static void branch(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),ob=type(sim,"obstacle",1);
 double a[3]={0,0,0},b[3]={0.01,0,0},angle[3]={0,0,0},c[3]={0.016,0.004,-0.005},d[3]={0.016,0.004,0.005};
 filamentptr mother=rod(t,a,b),daughter;int i;
 t->branchsegments=3;daughter=filAddBranch(sim,mother,0,angle,1,NULL);CHECK(daughter);
 for(i=0;i<=3;i++) {daughter->nodes[i][0]=0.01;daughter->nodes[i][1]=0.01*i;daughter->nodes[i][2]=0;}
 filNodes2Angles(daughter,-1,-1);rod(ob,c,d);
 CHECK(!filDynamics(sim));CHECK(mother->nodes[1][0]<0.01);
 for(i=0;i<3;i++) NEAR(mother->nodes[1][i],daughter->nodes[0][i],1e-14);
 for(i=0;i<100;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
 for(i=0;i<3;i++) NEAR(mother->nodes[1][i],daughter->nodes[0][i],1e-14);
 CHECK(filStericGeometry(daughter->segments[0],mother->segments[0],&c[0],&c[1],d)==DBL_MAX);
 simfree(sim);puts("PASS contact load reaches mother, junction remains attached, local exemption");
}
static void growth(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),ob=type(sim,"obstacle",1);
 double a[3]={0,0,0},b[3]={0.01,0,0},c[3]={0.015,-0.005,0},d[3]={0.015,0.005,0};filamentptr f=rod(t,a,b);rod(ob,c,d);
 CHECK(filElongate(sim,f,0.011)==0);CHECK(f->nseg==1);CHECK(sim->filss->steric && sim->filss->steric->blockedgrowth==1);
 simfree(sim);puts("PASS new growth segment cannot jump through an obstacle");
}
static int trial_oracle(simptr sim,segmentptr trial) {
 int ft,f,i,d;double s,t,n[3],extent;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr type=sim->filss->filtypes[ft];if(type->stericradius<=0) continue;
  for(f=0;f<type->nfil;f++) for(i=0;i<type->fillist[f]->nseg;i++) {
   segmentptr other=type->fillist[f]->segments[i];if(filStericExcluded(trial,other)) continue;
   extent=trial->fil->filtype->stericradius+type->stericradius;
   for(d=0;d<sim->dim;d++)
    if(fmin(trial->xyzfront[d],trial->xyzback[d])>fmax(other->xyzfront[d],other->xyzback[d])+extent || fmin(other->xyzfront[d],other->xyzback[d])>fmax(trial->xyzfront[d],trial->xyzback[d])+extent) break;
   if(d==sim->dim && filStericGeometry(trial,other,&s,&t,n)+1e-12<extent) return 1;
  }
 }
 return 0;
}
static void growth_index(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);int i,j,expected;filamentptr trial,pending,front;
 double a[3],b[3];
 for(i=0;i<150;i++) {for(j=0;j<3;j++) {a[j]=(uniform()-0.5)*0.1;b[j]=a[j]+(uniform()-0.5)*0.02;}rod(t,a,b);}
 a[0]=-0.3;b[0]=-0.29;a[1]=b[1]=0.2;a[2]=b[2]=0;front=rod(t,a,b);
 CHECK(!filStericChemistry(sim,1));
 /* Endpoint displacements smaller than the skin margin remain covered. */
 for(i=0;i<150;i++) for(j=0;j<2;j++) t->fillist[i]->nodes[j][0]+=0.0004;
 a[0]=0.2;a[1]=a[2]=0;b[0]=0.21;b[1]=b[2]=0;pending=rod(t,a,b);
 CHECK(!filStericSegmentBlocked(sim,pending->segments[0]));CHECK(sim->filss->steric->npending==1);
 a[0]=b[0]=0.205;a[1]=-0.005;b[1]=0.005;trial=rod(t,a,b);
 CHECK(filStericSegmentBlocked(sim,trial->segments[0]));trial->nseg=0;t->nfil--;
 pending->nseg=0;trial=rod(t,a,b);CHECK(!filStericSegmentBlocked(sim,trial->segments[0]));
 for(i=0;i<400;i++) {
  for(j=0;j<3;j++) {a[j]=(uniform()-0.5)*0.1;b[j]=a[j]+(uniform()-0.5)*0.02;}
  trial=rod(t,a,b);expected=trial_oracle(sim,trial->segments[0]);CHECK(filStericSegmentBlocked(sim,trial->segments[0])==expected);
  if(expected) {trial->nseg=0;t->nfil--;}
 }
 /* Zero bending stiffness deliberately samples arbitrary birth angles even at
    kT=0. Use a straight deterministic insertion for this location assertion. */
 t->kypr[0]=t->kypr[1]=t->kypr[2]=1;
 t->plusend='f';CHECK(filElongate(sim,front,0.011)==1);CHECK(front->nseg==2);
 a[0]=b[0]=-0.309;a[1]=0.195;b[1]=0.205;a[2]=b[2]=0;trial=rod(t,a,b);
 CHECK(filStericSegmentBlocked(sim,trial->segments[0]));trial->nseg=0;t->nfil--;
 CHECK(!filRemoveSegment(front,'f'));trial=rod(t,a,b);CHECK(!filStericSegmentBlocked(sim,trial->segments[0]));
 CHECK(!filStericChemistry(sim,0));CHECK(!filStericPrepare(sim));simfree(sim);
 puts("PASS growth boxes versus exhaustive oracle, skin movement, pending insertions and removed/reused slots");
}
static void branch_rollback(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),ob=type(sim,"obstacle",1);
 double a[3]={0,0,0},b[3]={0.01,0,0},c[3]={0.006,0.005,0.003},d[3]={0.016,0.005,0.003},angle[3]={1.5707963267948966,0,0};
 filamentptr mother=rod(t,a,b),obstacle=rod(ob,c,d),daughter;
 CHECK(!filAddBranch(sim,mother,0,angle,1,NULL));CHECK(t->nfil==1);CHECK(mother->nbranch==0);
 CHECK(sim->filss->steric && sim->filss->steric->blockedbranches==1);
 obstacle->nseg=0;daughter=filAddBranch(sim,mother,0,angle,1,"daughter");CHECK(daughter);CHECK(t->nfil==2);
 CHECK(!filAddBranch(sim,mother,0,angle,1,"daughter"));CHECK(t->nfil==2 && mother->nbranch==1 && daughter->frontend==mother);
 CHECK(!filDynamics(sim));simfree(sim);puts("PASS rejected branch rollback and duplicate-name safety");
}
static void diffusion(void) {
 int sub,i;double a[3]={0,0,0},b[3]={0.01,0,0};
 for(sub=1;sub<=4;sub*=4) {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr f=rod(t,a,b);double sum=0,mean=0;t->kT=0.001;t->stericsubsteps=sub;
  for(i=0;i<12000;i++) {double before=(f->nodes[0][0]+f->nodes[1][0])/2,dx;CHECK(!filDynamics(sim));sim->time+=sim->dt;dx=(f->nodes[0][0]+f->nodes[1][0])/2-before;sum+=dx*dx;mean+=dx;}
  CHECK(fabs(sum/12000/(t->kT*t->mobility*sim->dt)-1)<0.06);CHECK(fabs(mean/12000)<5*sqrt(t->kT*t->mobility*sim->dt/12000));simfree(sim);
 }
 puts("PASS Brownian diffusion with 1 and 4 mechanical substeps");
}
static void walls_and_guards(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);surfaceptr wall=surfaddsurface(sim,"chamber");
 double p[5]={-1,-1,0,2,2},a[3]={0,0,0.0025},b[3]={0.01,0,0.0025},old;
 filamentptr f=rod(t,a,b);CHECK(wall);CHECK(!surfaddpanel(wall,3,PSrect,"+2",p,"floor"));
 t->confinesrf=wall;t->confineforce=400;
 filComputeForces(f,-1,-1);NEAR(f->filwork->forces[0][2],0.4,1e-10);
 wall->panels[PSrect][0]->front[0]=-1;wall->panels[PSrect][0]->point[0][2]=0.005;
 filComputeForces(f,-1,-1);NEAR(f->filwork->forces[0][2],-0.4,1e-10);
 t->confinesrf=NULL;{double c[3]={0,-0.005,0.006},d[3]={0,0.005,0.006};rod(t,c,d);}
 t->sterick=1e7;old=f->nodes[0][2];CHECK(filDynamics(sim)!=0);NEAR(f->nodes[0][2],old,1e-15);NEAR(sim->dt,1e-5,1e-15);
 t->sterick=4000;t->dynamics=FDRK2;CHECK(filStericValidate(sim)!=0);t->dynamics=FDeuler;
 sim->wlist[0]->type='p';CHECK(filStericValidate(sim)!=0);sim->wlist[0]->type='r';
 simfree(sim);puts("PASS radius-aware floor/ceiling, unsafe contact guard, unsupported settings rejected");
}
static void rectangular_wall_normals(void) {
 int dim,axis,sign,face,r,j,node,d;
 /* Sample both sides of contact outside a finite panel's lateral extent.
    The force must remain normal and continuous, including at radius zero. */
 for(dim=2;dim<=3;dim++) for(axis=0;axis<dim;axis++) for(sign=-1;sign<=1;sign+=2) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);surfaceptr wall=surfaddsurface(sim,"wall");
  double panel[5]={0},a[3]={0},b[3]={0},inward,dist,expected;char normal[3];int tangent=(axis+1)%dim;
  for(d=0;d<dim;d++) panel[d]=-0.5;
  panel[axis]=0;for(d=dim;d<2*dim-1;d++) panel[d]=1;
  sprintf(normal,"%c%d",sign>0?'+':'-',axis);CHECK(wall);CHECK(!surfaddpanel(wall,dim,PSrect,normal,panel,"panel"));
  a[tangent]=0.500622;b[tangent]=0.510622;
  {filamentptr f=rod(t,a,b);t->klen=0;t->confinesrf=wall;t->confineforce=400;
   for(face=0;face<2;face++) for(r=0;r<2;r++) for(j=0;j<4;j++) {
    t->confineface=face?PFback:PFfront;t->stericradius=r?0.0035:0;inward=sign*(face?-1:1);
    dist=j==3?-0.002:t->stericradius+(1-j)*1e-9;
    for(node=0;node<2;node++) f->nodes[node][axis]=inward*dist;
    filNodes2Angles(f,-1,-1);filComputeForces(f,-1,-1);
    expected=inward*400*fmax(t->stericradius-dist,0);
    for(node=0;node<2;node++) for(d=0;d<dim;d++) NEAR(f->filwork->forces[node][d],d==axis?expected:0,1e-11);
   }
  }
  simfree(sim);
 }
 /* Two simultaneous wall contacts have independent backward Euler solutions. */
 {simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);surfaceptr wall=surfaddsurface(sim,"corner");
  double floor[5]={-0.5,-0.5,0,1,1},side[5]={0.5,-0.5,0,1,1};
  double a[3]={0.500622,0,-2.7e-7},b[3]={0.500622,0.01,-2.7e-7};filamentptr f=rod(t,a,b);
  CHECK(wall);CHECK(!surfaddpanel(wall,3,PSrect,"+2",floor,"floor"));CHECK(!surfaddpanel(wall,3,PSrect,"-0",side,"side"));
  t->stericradius=0;t->dynamics=FDimplicitnetwork;t->confinesrf=wall;t->confineforce=1e6;t->implicittol=1e-10;sim->dt=1e-4;
  CHECK(!filDynamics(sim));
  for(node=0;node<2;node++) {
   NEAR(f->nodes[node][0],0.5+0.000622/101,2e-10);NEAR(f->nodes[node][2],-2.7e-7/101,2e-10);
   NEAR(f->nodes[node][1],node?0.01:0,2e-10);
  }
  simfree(sim);
 }
 puts("PASS 2D/3D rectangular wall normals, both faces/radii, edge continuity and implicit corner relaxation");
}
static void convergence_and_2d(void) {
 int dim,level,i,steps;double previous=1,err;
 for(dim=2;dim<=3;dim++) for(level=0;level<3;level++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);
  double a[3]={-0.005,0,0},b[3]={0.005,0,0},c[3]={-0.005,0.006,0},d[3]={0.005,0.006,0},s,u,n[3],dist;
  filamentptr first=rod(t,a,b),second=rod(t,c,d);steps=10*(1<<level);sim->dt=0.0001/steps;
  for(i=0;i<steps;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
  dist=Geo_ClosestSeg2Seg(first->nodes[0],first->nodes[1],second->nodes[0],second->nodes[1],dim,&s,&u,n);
  err=fabs(dist-(0.007-0.001*exp(-4000*0.0001)));
  CHECK(err<6e-6);if(level) CHECK(err<0.55*previous);previous=err;simfree(sim);
 }
 puts("PASS 2D/3D contact relaxation converges to analytic solution as timestep halves");
}
static void benchmark(int dense,int thermal) {
 int counts[3]={1000,5000,15000},c,i,j,enabled,steps=thermal?100:300;
 puts("segments,density,thermal,contacts_enabled,steps,seconds,neighbor_pairs,rebuilds");
 for(c=0;c<3;c++) for(enabled=0;enabled<=1;enabled++) {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);clock_t start;double a[3],b[3],spacing=dense?0.006:0.02;
  t->stericradius=enabled?0.0035:0;t->sterick=1000;t->stericsubsteps=1;
  if(thermal) {t->kT=0.001;t->kypr[0]=t->kypr[1]=t->kypr[2]=1;}
  for(i=0;i<counts[c]/10;i++) {
   filamentptr f;a[0]=(i%25)*0.12;a[1]=((i/25)%25)*spacing;a[2]=(i/625)*spacing;memcpy(b,a,sizeof(b));b[0]+=0.01;f=rod(t,a,b);
   for(j=1;j<10;j++) {double angle[3]={0,0,0};CHECK(!filAddSegment(f,NULL,0.01,angle,1,'b'));}
  }
  start=clock();for(i=0;i<steps;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
  printf("%i,%s,%i,%i,%i,%.6f,%i,%llu\n",counts[c],dense?"dense":"sparse",thermal,enabled,steps,(double)(clock()-start)/CLOCKS_PER_SEC,sim->filss->steric?sim->filss->steric->npair:0,sim->filss->steric?sim->filss->steric->rebuilds:0);simfree(sim);
 }
}
static void growth_benchmark(void) {
 int counts[3]={1000,5000,15000},c,i,j,mode,queries=10000;puts("segments,query_method,queries,seconds");
 for(c=0;c<3;c++) {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr trial;double a[3]={10,0,0},b[3]={10,0.01,0};clock_t start;
  for(i=0;i<counts[c];i++) {a[0]=(i%25)*0.02;a[1]=((i/25)%25)*0.02;a[2]=(i/625)*0.02;memcpy(b,a,sizeof(b));b[0]+=0.01;rod(t,a,b);}
  a[0]=b[0]=10;a[1]=0;b[1]=0.01;a[2]=b[2]=0;trial=rod(t,a,b);trial->nseg=0;
  CHECK(!filStericChemistry(sim,1));trial->nseg=1;
  for(mode=0;mode<2;mode++) {
   start=clock();for(i=0;i<queries;i++) {
    filamentptr target=t->fillist[i%counts[c]];
    for(j=0;j<3;j++) trial->nodes[0][j]=trial->nodes[1][j]=target->nodes[0][j];
    trial->nodes[0][0]+=0.005;trial->nodes[1][0]+=0.005;
    trial->nodes[0][1]-=0.005;trial->nodes[1][1]+=0.005;trial->nodes[0][2]+=0.003;trial->nodes[1][2]+=0.003;
    CHECK(mode?trial_oracle(sim,trial->segments[0]):filStericSegmentBlocked(sim,trial->segments[0]));
   }
   printf("%i,%s,%i,%.6f\n",counts[c],mode?"exhaustive":"boxes",queries,(double)(clock()-start)/CLOCKS_PER_SEC);
  }
  CHECK(!filStericChemistry(sim,0));simfree(sim);
 }
}
static void brownian_switch(void) {
 int dim,i,j,d;char off[]="0",on[]="1",invalid[]="0.5",extra[]="0 extra";
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);filamentptr fil;
  double a[3]={0,0,0},b[3]={0.01,0,0},oldlength,noise=0;
  CHECK(t->brownian==1);t->kT=0.001;t->branchspread=0.2;t->stericsubsteps=4;fil=rod(t,a,b);
  CHECK(!filtypeReadString(sim,NULL,t,"brownian_motion",invalid));CHECK(t->brownian==1);
  CHECK(!filtypeReadString(sim,NULL,t,"brownian_motion",extra));CHECK(t->brownian==1);
  CHECK(filtypeReadString(sim,NULL,t,"brownian_motion",off)==t);CHECK(!t->brownian);
  NEAR(t->kT,0.001,1e-15);NEAR(t->branchspread,0.2,1e-15);
  for(i=0;i<20;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}
  for(j=0;j<2;j++) for(d=0;d<dim;d++) {
   NEAR(fil->nodes[j][d],j?b[d]:a[d],1e-15);CHECK(fil->filwork->thermforce[j][d]==0);
  }
  /* Turning off thermal noise must leave elastic relaxation active. */
  fil->nodes[1][0]+=0.002;filNodes2Angles(fil,-1,-1);oldlength=fil->segments[0]->len;
  CHECK(!filDynamics(sim));sim->time+=sim->dt;CHECK(fil->segments[0]->len<oldlength);
  CHECK(filtypeReadString(sim,NULL,t,"brownian_motion",on)==t);filComputeForces(fil,-1,-1);
  for(j=0;j<2;j++) for(d=0;d<dim;d++) noise+=fabs(fil->filwork->thermforce[j][d]);
  CHECK(noise>0);
  /* Runtime toggles invalidate cached noise even at unchanged simulation time. */
  CHECK(!filtypeSetParam(t,"brownian",0,0));filComputeForces(fil,-1,-1);
  for(j=0;j<2;j++) for(d=0;d<dim;d++) CHECK(fil->filwork->thermforce[j][d]==0);
  simfree(sim);
 }
 puts("PASS 2D/3D Brownian motion switch, runtime cache invalidation, retained kT/spread and elastic relaxation");
}
static void movement_cache(void) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr mother,daughter;
 double a[3]={0,0,0},b[3]={0.01,0,0},angle[3]={1,0,0};int i,root;unsigned long long rebuild;
 struct filamentstericstruct *w;
 mother=rod(t,a,b);CHECK(!filAddSegment(mother,NULL,0.01,angle,1,'b'));
 t->stericradius=0;t->branchsegments=2;daughter=filAddBranch(sim,mother,0,angle,1,NULL);CHECK(daughter);t->stericradius=0.0035;
 t->stericsubsteps=4;CHECK(!filDynamics(sim));sim->time+=sim->dt;
 w=sim->filss->steric;rebuild=w->movementrebuilds;CHECK(rebuild==1);
 for(i=0;i<8;i++) {CHECK(!filDynamics(sim));sim->time+=sim->dt;}CHECK(w->movementrebuilds==rebuild);
 /* Mobility changes refresh the group drag, without rebuilding its identity. */
 mother->nodemobility[1]=0;CHECK(!filDynamics(sim));sim->time+=sim->dt;
 root=w->nodes[daughter->stericnodeoffset].parent;CHECK(w->nodes[root].fixed);
 CHECK(w->movementrebuilds==rebuild);mother->nodemobility[1]=1;
 CHECK(!filDynamics(sim));sim->time+=sim->dt;CHECK(!w->nodes[root].fixed);
 /* A same-count branch-spot edit must invalidate the cache. */
 mother->branchspots[0]=1;memcpy(daughter->nodes[0],mother->nodes[2],3*sizeof(double));filNodes2Angles(daughter,-1,-1);
 CHECK(!filDynamics(sim));sim->time+=sim->dt;CHECK(w->movementrebuilds==++rebuild);
 root=w->nodes[daughter->stericnodeoffset].parent;
 CHECK(root==mother->stericnodeoffset+2);
 CHECK(!filRemoveSegment(daughter,'b'));CHECK(!filDynamics(sim));sim->time+=sim->dt;
 CHECK(w->movementrebuilds==++rebuild);
 daughter->frontend=NULL;CHECK(!filDynamics(sim));sim->time+=sim->dt;CHECK(w->movementrebuilds==++rebuild);
 CHECK(w->nodes[daughter->stericnodeoffset].parent==daughter->stericnodeoffset);
 simfree(sim);puts("PASS movement topology reuse, mobility refresh, branch-spot edits, removal and detachment");
}
static simptr worker_scene(int workers) {
 simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);int i,j;
 double a[3],b[3],angle[3]={0.01,0.005,0};
 t->sterick=1000;t->stericsubsteps=4;t->kypr[0]=t->kypr[1]=t->kypr[2]=0.02;
 t->branchforceangle=0.018;t->branchforceazimuth=0.02;t->branchangle=1.0;t->branchsegments=2;t->stericradius=0;
 CHECK(!filSetWorkers(sim,workers));
 for(i=0;i<220;i++) {
  filamentptr fil;a[0]=(i%10)*0.08;a[1]=(i/10)*0.012;a[2]=(i%2)*0.006;
  memcpy(b,a,sizeof(b));b[0]+=0.01;fil=rod(t,a,b);
  for(j=0;j<4;j++) CHECK(!filAddSegment(fil,NULL,0.01,angle,1,'b'));
  if(i%20==0) {double branchangle[3]={1,0,0};CHECK(filAddBranch(sim,fil,2,branchangle,1,NULL));}
 }
 t->stericradius=0.0035;return sim;
}
static void workers(void) {
 simptr serial,parallel;int i,f,j;filamenttypeptr a,b;
 simptr probe=scene(3);char bad[]="-1",extra[]="2 extra",one[]="1",automatic[]="0";
 CHECK(simreadstring(probe,NULL,"filament_workers",bad)!=0);
 CHECK(simreadstring(probe,NULL,"filament_workers",extra)!=0);
 CHECK(!simreadstring(probe,NULL,"filament_workers",one));CHECK(probe->filss->workers==1);
 CHECK(!simreadstring(probe,NULL,"filament_workers",automatic));CHECK(probe->filss->workers==0);simfree(probe);
 if(!filWorkersSupported()) {
  probe=worker_scene(1);CHECK(filSetWorkers(probe,4)==2);CHECK(probe->filss->workers==1);
  CHECK(!filSetWorkers(probe,0));CHECK(!filDynamics(probe));CHECK(probe->filss->steric->workersused==1);simfree(probe);
  puts("PASS serial-only worker setting, automatic fallback and unavailable-worker rejection");return;
 }
 serial=worker_scene(1);parallel=worker_scene(4);a=serial->filss->filtypes[0];b=parallel->filss->filtypes[0];
 for(i=0;i<20;i++) {
  CHECK(!filDynamics(serial));CHECK(!filDynamics(parallel));serial->time+=serial->dt;parallel->time+=parallel->dt;
  CHECK(a->nfil==b->nfil);
  for(f=0;f<a->nfil;f++) {
   filamentptr x=a->fillist[f],y=b->fillist[f];CHECK(x->nseg==y->nseg);
   CHECK(!memcmp(x->seg0up,y->seg0up,3*sizeof(double)));
   for(j=0;j<=x->nseg;j++) CHECK(!memcmp(x->nodes[j],y->nodes[j],3*sizeof(double)));
   for(j=0;j<x->nseg;j++) {
    CHECK(!memcmp(x->segments[j]->qabs,y->segments[j]->qabs,4*sizeof(double)));
    CHECK(!memcmp(x->segments[j]->ypr,y->segments[j]->ypr,3*sizeof(double)));
   }
  }
 }
 CHECK(parallel->filss->steric->workersused==4);CHECK(serial->filss->steric->movementrebuilds==1);
 CHECK(parallel->filss->steric->npair==serial->filss->steric->npair);
 simfree(serial);simfree(parallel);puts("PASS exact 1/4-worker deterministic mechanics, material frames and branch geometry");
}

static void implicit_network(void) {
 int dim,i,j,d;
 /* Single spring: backward Euler has a known exact discrete solution even
    when an explicit step would diverge. Test a stiffness/timestep sweep. */
 for(dim=2;dim<=3;dim++) for(i=0;i<5;i++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);filamentptr fil;
  double a[3]={0,0,0},b[3]={0.012,0,0},h=1e-4*pow(10,i),expected;
  t->dynamics=FDimplicitnetwork;t->stericradius=0;sim->dt=h;
  fil=rod(t,a,b);expected=0.01+0.002/(1+2*h*t->mobility*t->klen);
  CHECK(!filDynamics(sim));NEAR(fil->segments[0]->len,expected,2e-8);
  NEAR((fil->nodes[0][0]+fil->nodes[1][0])*0.5,0.006,1e-12);
  simfree(sim);
 }
 /* Existing contacts and new contacts encountered by a Newton trial must
    participate in the full coupled residual, including immobile obstacles. */
 for(i=0;i<3;i++) {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0),fixed=type(sim,"fixed",1);
  double a[3]={0,0,0},b[3]={0.01,0,0},c[3]={0,0.006,0},e[3]={0.01,0.006,0};
  filamentptr f=rod(t,a,b);filamentptr obstacle=rod(fixed,c,e);
  t->dynamics=FDimplicitnetwork;sim->dt=1e-4*pow(10,i);
  CHECK(!filDynamics(sim));
  /* Fixed obstacle, force split evenly over both endpoints: rate k/2. */
  NEAR(f->nodes[0][1],-0.001*(1-1/(1+sim->dt*t->sterick/2)),3e-8);
  NEAR(obstacle->nodes[0][1],0.006,1e-15);CHECK(sim->filss->steric->contactbound>0.5 || i==0);
  simfree(sim);
 }
 /* Shared branch coordinates stay exactly equal and fixed roots remain fixed.
    A deliberately impossible nonlinear tolerance must report failure and restore. */
 {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);filamentptr mother,daughter;
  double a[3]={0,0,0},b[3]={0.01,0,0},angle[3]={1,0.1,0.2},saved[3];
  t->stericradius=0;t->branchsegments=2;mother=rod(t,a,b);
  daughter=filAddBranch(sim,mother,0,angle,1,NULL);CHECK(daughter);
  t->dynamics=FDimplicitnetwork;t->kypr[0]=t->kypr[1]=t->kypr[2]=0.02;
  t->branchforceangle=0.02;t->branchangle=1.2;t->branchforceazimuth=0.03;
  mother->nodemobility[1]=0;sim->dt=1e-4;
  memcpy(saved,mother->nodes[1],sizeof(saved));CHECK(!filDynamics(sim));
  for(d=0;d<3;d++) {NEAR(mother->nodes[1][d],saved[d],1e-15);NEAR(daughter->nodes[0][d],saved[d],1e-15);}
  /* Nonlinear finite-difference roundoff prevents a 1e-30 residual. */
  t->implicittol=1e-30;t->implicititerations=1;
  for(j=0;j<=daughter->nseg;j++) memcpy(daughter->nodesx[j],daughter->nodes[j],3*sizeof(double));
  CHECK(filDynamics(sim)!=0);
  for(j=0;j<=daughter->nseg;j++) for(d=0;d<3;d++) NEAR(daughter->nodes[j][d],daughter->nodesx[j][d],1e-15);
  simfree(sim);
 }

 /* Radius-aware wall relaxation also has a known backward Euler solution. */
 {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);surfaceptr wall=surfaddsurface(sim,"chamber");
  double panel[5]={-1,-1,0,2,2},a[3]={0,0,0.0025},b[3]={0.01,0,0.0025};
  filamentptr f=rod(t,a,b);CHECK(wall);CHECK(!surfaddpanel(wall,3,PSrect,"+2",panel,"floor"));
  t->dynamics=FDimplicitnetwork;t->confinesrf=wall;t->confineforce=1e6;sim->dt=1e-4;
  CHECK(!filDynamics(sim));NEAR(f->nodes[0][2],0.0035-0.001/101,2e-8);
  simfree(sim);
 }
 /* Relative material roll is an implicit unknown, not an explicit after-step.
    This uses the author's local torque/mobility convention. */
 {
  simptr sim=scene(3);filamenttypeptr t=type(sim,"actin",0);double a[3]={0,0,0},b[3]={0.01,0,0},angle[3]={0,0,0.4};
  filamentptr f=rod(t,a,b);CHECK(!filAddSegment(f,NULL,0.01,angle,1,'b'));
  filCopyYpr2ToRoll(f);NEAR(f->roll[1],0.4,1e-15);
  f->roll[1]=0.3;filCopyRollToYpr2(f);NEAR(f->segments[1]->ypr[2],0.3,1e-15);
  f->segments[1]->ypr[2]=0.4;filNodes2Angles(f,-1,-1);
  t->stericradius=0;t->kypr[2]=2;sim->dt=0.01;
  CHECK(!filDynamics(sim));NEAR(f->segments[1]->ypr[2],0.4*(1-0.02),2e-8);
  f->segments[1]->ypr[2]=0.4;filNodes2Angles(f,-1,-1);
  t->dynamics=FDimplicitnetwork;sim->dt=0.1;
  CHECK(!filDynamics(sim));NEAR(f->segments[1]->ypr[2],0.4/1.2,2e-7);
  NEAR(f->roll[1],f->segments[1]->ypr[2],1e-15);simfree(sim);
 }
 /* With no elastic forces, implicit Brownian displacement is exactly h*mu*F.
    A stiff deterministic solve with noise disabled is covered above. */
 for(dim=2;dim<=3;dim++) {
  simptr sim=scene(dim);filamenttypeptr t=type(sim,"actin",0);double a[3]={0,0,0},b[3]={0.01,0,0};
  filamentptr f=rod(t,a,b);t->dynamics=FDimplicitnetwork;t->stericradius=0;t->klen=0;t->kT=0.001;sim->dt=1e-4;
  CHECK(!filDynamics(sim));
  for(j=0;j<=1;j++) for(d=0;d<dim;d++)
   NEAR(f->nodes[j][d]-(j?b[d]:a[d]),sim->dt*t->mobility*f->filwork->thermforce[j][d],2e-8);
  simfree(sim);
 }

 if(filWorkersSupported()) {
  simptr serial=worker_scene(1),parallel=worker_scene(4);
  serial->filss->filtypes[0]->dynamics=parallel->filss->filtypes[0]->dynamics=FDimplicitnetwork;
  for(i=0;i<3;i++) {
   CHECK(!filDynamics(serial));CHECK(!filDynamics(parallel));serial->time+=serial->dt;parallel->time+=parallel->dt;
   for(j=0;j<serial->filss->filtypes[0]->nfil;j++) {
    filamentptr a=serial->filss->filtypes[0]->fillist[j],b=parallel->filss->filtypes[0]->fillist[j];int node;
    CHECK(!memcmp(a->seg0up,b->seg0up,3*sizeof(double)));
    for(node=0;node<=a->nseg;node++) CHECK(!memcmp(a->nodes[node],b->nodes[node],3*sizeof(double)));
   }
  }
  simfree(serial);simfree(parallel);
 }
 puts("PASS coupled implicit 2D/3D analytic spring sweep, stiff contacts/walls, roll, Brownian increments, branch constraints, worker reproducibility and rollback");
}

int main(int argc,char **argv) {
 setvbuf(stdout,NULL,_IONBF,0);
 if(argc>1 && !strcmp(argv[1],"--implicit")) {implicit_network();return 0;}
 if(argc>1 && !strcmp(argv[1],"--benchmark")) {benchmark(0,0);benchmark(1,0);return 0;}
 if(argc>1 && !strcmp(argv[1],"--benchmark-thermal")) {benchmark(0,1);benchmark(1,1);return 0;}
 if(argc>1 && !strcmp(argv[1],"--benchmark-growth")) {growth_benchmark();return 0;}
 geometry();indexing();shared_grids();unified_boxes();queries();growth_switch();forces();branch();growth();growth_index();branch_rollback();diffusion();walls_and_guards();rectangular_wall_normals();convergence_and_2d();brownian_switch();movement_cache();workers();implicit_network();puts("All filament steric checks passed.");return 0;
}
