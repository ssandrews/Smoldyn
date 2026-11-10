/* Short computational replays of recorded geometry, not biological checkpoints.
   Usage: profile_filaments model snapshot steps warmup timers output_repeats
   Optional final argument --fixed-initial-lengths rebuilds a common feasible
   initial tree at standard lengths before timing either axial model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "SimCommand.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "smolprofile.h"

filamentptr filAlloc(filamentptr fil,int maxseg,int maxbranch,int maxsequence);
int filAddSegment(filamentptr fil,const double *pos,double length,const double *angle,double thick,char end);
void filNodes2Angles(filamentptr fil,int lo,int hi);
double filBranchAzimuth(const filamentptr mother,int spot,const filamentptr daughter);
enum CMDcode cmdprintFilaments(simptr sim,cmdptr cmd,char *line);
double gaussrandD(void);
#define REQUIRE(x) do {if(!(x)) {fprintf(stderr,"Profile failed at line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
typedef struct {filamentptr fil;char parent[STRCHAR];} Record;
static void counts(simptr sim,int *filaments,int *segments,int *growing) {
 int ft,f;*filaments=*segments=*growing=0;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];*filaments+=t->nfil;
  for(f=0;f<t->nfil;f++) {*segments+=t->fillist[f]->nseg;if(!t->fillist[f]->capped) (*growing)++;}
 }
}
/* Exact doubles, material frames and subsequent RNG draws: worker regression
   checks must detect differences that the rounded FIL text would conceal. */
static void writeState(simptr sim) {
 FILE *file=fopen("final_state.bin","wb");int ft,f,j;
 REQUIRE(file);REQUIRE(fwrite(&sim->time,sizeof(double),1,file)==1);
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr type=sim->filss->filtypes[ft];
  REQUIRE(fwrite(&type->nfil,sizeof(int),1,file)==1);
  for(f=0;f<type->nfil;f++) {
   filamentptr fil=type->fillist[f];
   REQUIRE(fwrite(fil->filname,1,strlen(fil->filname)+1,file)==strlen(fil->filname)+1);
   REQUIRE(fwrite(&fil->nseg,sizeof(int),1,file)==1);REQUIRE(fwrite(&fil->capped,sizeof(int),1,file)==1);
   REQUIRE(fwrite(&fil->growbank,sizeof(double),1,file)==1);REQUIRE(fwrite(fil->seg0up,sizeof(double),3,file)==3);
   for(j=0;j<=fil->nseg;j++) REQUIRE(fwrite(fil->nodes[j],sizeof(double),sim->dim,file)==(size_t)sim->dim);
   for(j=0;j<fil->nseg;j++) {
    REQUIRE(fwrite(&fil->segments[j]->len,sizeof(double),1,file)==1);
    REQUIRE(fwrite(fil->segments[j]->ypr,sizeof(double),3,file)==3);
    REQUIRE(fwrite(fil->segments[j]->qrel,sizeof(double),4,file)==4);
    REQUIRE(fwrite(fil->segments[j]->qabs,sizeof(double),4,file)==4);
    REQUIRE(fwrite(&fil->roll[j],sizeof(double),1,file)==1);
   }
  }
 }
 for(j=0;j<32;j++) {double noise=gaussrandD();REQUIRE(fwrite(&noise,sizeof(double),1,file)==1);}
 REQUIRE(!fclose(file));
}
static void snapshot(simptr sim,const char *path) {
 FILE *file=fopen(path,"r");Record *records=NULL;int n=0,capacity=0,i,j,d,ft,nseg,capped;
 char token[STRCHAR],name[STRCHAR],parent[STRCHAR],*colon;double time,bank,angle[3]={0,0,0};
 REQUIRE(file);
 while(fscanf(file,"%255s",token)==1) {
  filamenttypeptr t=NULL;filamentptr fil;double start[3]={0,0,0};
  REQUIRE(!strcmp(token,"FIL"));
  REQUIRE(fscanf(file,"%lf %255s %d %255s %d",&time,name,&nseg,parent,&capped)==5);
  REQUIRE(nseg>0);colon=strchr(name,':');REQUIRE(colon);*colon++=0;
  for(ft=0;ft<sim->filss->ntype;ft++) if(!strcmp(sim->filss->filtypes[ft]->ftname,name)) t=sim->filss->filtypes[ft];
  REQUIRE(t);fil=filAddFilament(t,colon);REQUIRE(fil);
  for(i=0;i<nseg;i++) REQUIRE(!filAddSegment(fil,start,t->stdlen,angle,1,'b'));
  for(i=0;i<=nseg;i++) for(d=0;d<sim->dim;d++) REQUIRE(fscanf(file,"%lf",&fil->nodes[i][d])==1);
  REQUIRE(fscanf(file,"%lf",&bank)==1);fil->growbank=bank;fil->capped=capped?1:0;
  /* Avoid a degenerate up vector for a nearly vertical first segment. */
  {double z=fil->nodes[1][2]-fil->nodes[0][2],len=0;
   for(d=0;d<sim->dim;d++) {double dx=fil->nodes[1][d]-fil->nodes[0][d];len+=dx*dx;}
   REQUIRE(len>0);if(fabs(z)>0.95*sqrt(len)) {fil->seg0up[0]=1;fil->seg0up[2]=0;}
  }
  filNodes2Angles(fil,-1,-1);sim->time=time;
  if(n==capacity) {capacity=capacity?capacity*2:64;records=(Record*)realloc(records,capacity*sizeof(*records));REQUIRE(records);}
  records[n].fil=fil;strcpy(records[n++].parent,parent);
  /* Auto names in subsequent branching must not collide with loaded names. */
  {int index;if(sscanf(colon,"actin.%d",&index)==1 && t->autonamenum<=index) t->autonamenum=index+1;}
 }
 REQUIRE(n>0);fclose(file);
 for(i=0;i<n;i++) if(strcmp(records[i].parent,"-")) {
  filamentptr mother=NULL,daughter=records[i].fil;int spot=-1,b;double best=1e100;
  for(j=0;j<n;j++) if(!strcmp(records[j].fil->filname,records[i].parent)) mother=records[j].fil;
  REQUIRE(mother);
  for(j=1;j<=mother->nseg;j++) {double dist=0;
   for(d=0;d<sim->dim;d++) {double dx=mother->nodes[j][d]-daughter->nodes[0][d];dist+=dx*dx;}
   if(dist<best) {best=dist;spot=j-1;}
  }
  REQUIRE(best<1e-10);b=mother->nbranch;REQUIRE(filAlloc(mother,0,b+1,0));
  mother->branchspots[b]=spot;mother->branches[b]=daughter;mother->nbranch++;
  daughter->frontend=mother;
  for(d=0;d<sim->dim;d++) daughter->nodes[0][d]=mother->nodes[spot+1][d];
  filNodes2Angles(daughter,-1,-1);mother->branchazim0[b]=filBranchAzimuth(mother,spot,daughter);
 }
 free(records);
}
int main(int argc,char **argv) {
 simptr sim=NULL;cmdptr cmd;FILE *metrics;int steps,warmup,timers,repeats,i,er;
 int f0,s0,g0,f1,s1,g1;double start,wall,initialization,outputwall;
 unsigned long long rebuilds,evaluations,blockedgrowth,blockedbranches;
 struct filamentstericstruct *w;
 REQUIRE(argc==7 || (argc==8 && !strcmp(argv[7],"--fixed-initial-lengths")));steps=atoi(argv[3]);warmup=atoi(argv[4]);timers=atoi(argv[5]);repeats=atoi(argv[6]);
 REQUIRE(steps>0 && warmup>=0 && repeats>0);
 start=smolProfileClock();REQUIRE(!simInitAndLoad("",argv[1],&sim,"tq",NULL));
 if(strcmp(argv[2],"-")) snapshot(sim,argv[2]);
 REQUIRE(!simUpdateAndDisplay(sim));REQUIRE(!scmdopenfiles(sim->cmds,1));
 if(argc==8) {
  int ft,f,j,d,done=0,progress;unsigned char *ready;FILE *initial;
  REQUIRE(!filStericPrepare(sim));REQUIRE(!filStericPrepareMovement(sim));w=sim->filss->steric;
  ready=(unsigned char*)calloc((size_t)w->nfilament,1);REQUIRE(ready);
  for(f=0;f<w->nfilament;f++) {
   filamentptr fil=w->filaments[f].fil;
   for(j=0;j<=fil->nseg;j++) memcpy(fil->nodesx[j],fil->nodes[j],3*sizeof(double));
  }
  /* The archived elastic snapshots have axial strain. Construct the SAME
     feasible comparison geometry for both models, preserving segment tangent
     directions and root seed positions, then anchoring daughters to mothers.
     This is benchmark initialization, never a dynamics projection or restart. */
  while(done<w->nfilament) {
   progress=0;
   for(f=0;f<w->nfilament;f++) if(!ready[f]) {
    filamentptr fil=w->filaments[f].fil,mother=fil->frontend;int parent=-1,br;
    if(mother) {
     for(parent=0;parent<w->nfilament && w->filaments[parent].fil!=mother;parent++);
     REQUIRE(parent<w->nfilament);if(!ready[parent]) continue;
    }
    if(fil->nseg && fil->filtype->dynamics!=FDnone) {
     if(mother) {
      for(br=0;br<mother->nbranch && mother->branches[br]!=fil;br++) {}
      REQUIRE(br<mother->nbranch);
      memcpy(fil->nodes[0],mother->nodes[mother->branchspots[br]+1],3*sizeof(double));
     }
     for(j=0;j<fil->nseg;j++) {
      double len=0,axis[3];REQUIRE(fil->nodemobility[j+1]>0);
      for(d=0;d<sim->dim;d++) {axis[d]=fil->nodesx[j+1][d]-fil->nodesx[j][d];len+=axis[d]*axis[d];}
      REQUIRE(len>0);len=sqrt(len);
      for(d=0;d<sim->dim;d++) fil->nodes[j+1][d]=fil->nodes[j][d]+fil->filtype->stdlen*axis[d]/len;
     }
     filNodes2Angles(fil,-1,-1);
    }
    ready[f]=1;done++;progress++;
   }
   REQUIRE(progress>0);
  }
  free(ready);
  initial=fopen("initial_coordinates.csv","w");REQUIRE(initial);fprintf(initial,"filament,node,x,y,z\n");
  for(ft=0;ft<sim->filss->ntype;ft++) for(f=0;f<sim->filss->filtypes[ft]->nfil;f++) {
   filamentptr fil=sim->filss->filtypes[ft]->fillist[f];
   for(j=0;j<=fil->nseg;j++) fprintf(initial,"%s,%d,%.17g,%.17g,%.17g\n",fil->filname,j,fil->nodes[j][0],fil->nodes[j][1],fil->nodes[j][2]);
  }
  REQUIRE(!fclose(initial));
 }
 initialization=smolProfileClock()-start;
 REQUIRE(!filStericPrepare(sim));
 for(i=0;i<warmup;i++) REQUIRE(!simulatetimestep(sim));
 w=sim->filss->steric;REQUIRE(w);counts(sim,&f0,&s0,&g0);
 rebuilds=w->rebuilds;evaluations=w->evaluations;blockedgrowth=w->blockedgrowth;blockedbranches=w->blockedbranches;
 smolProfileReset();smolProfileDetail=timers!=2;smolProfileActive=timers;start=smolProfileClock();
 for(i=0;i<steps;i++) {SMOL_PROFILE_RUN(SP_STEP,er=simulatetimestep(sim));if(er) {fprintf(stderr,"Replay stopped: code %d, time %.17g, measured step %d\n",er,sim->time,i);filStericReport(sim);}REQUIRE(!er);}
 wall=smolProfileClock()-start;smolProfileActive=0;counts(sim,&f1,&s1,&g1);
 /* Isolated formatting + file flush cost at the measured final geometry. */
 cmd=scmdalloc();REQUIRE(cmd);cmd->cmds=sim->cmds;start=smolProfileClock();smolProfileActive=timers;
 for(i=0;i<repeats;i++) {SMOL_PROFILE_RUN(SP_OUTPUT,er=cmdprintFilaments(sim,cmd,"profile_frames.txt"));REQUIRE(er==CMDok);}
 outputwall=smolProfileClock()-start;smolProfileActive=0;scmdfree(cmd);
 writeState(sim);
 {FILE *coords=fopen("coordinates.csv","w");int ft,f,j;REQUIRE(coords);
  fprintf(coords,"filament,node,x,y,z\n");
  for(ft=0;ft<sim->filss->ntype;ft++) for(f=0;f<sim->filss->filtypes[ft]->nfil;f++) {
   filamentptr fil=sim->filss->filtypes[ft]->fillist[f];
   for(j=0;j<=fil->nseg;j++) fprintf(coords,"%s,%d,%.17g,%.17g,%.17g\n",fil->filname,j,fil->nodes[j][0],fil->nodes[j][1],fil->nodes[j][2]);
  }
  fclose(coords);
 }
 filStericReport(sim);
 REQUIRE(!smolProfileWrite("timing.csv"));metrics=fopen("metrics.json","w");REQUIRE(metrics);
 fprintf(metrics,"{\n\"steps\":%d,\"warmup\":%d,\"timers\":%d,\"dt\":%.12g,\"end_time\":%.12g,\n",steps,warmup,timers,sim->dt,sim->time);
 fprintf(metrics,"\"wall_seconds\":%.9f,\"initialization_seconds\":%.9f,\"output_seconds_per_frame\":%.9f,\n",wall,initialization,outputwall/repeats);
 fprintf(metrics,"\"filaments_start\":%d,\"segments_start\":%d,\"growing_start\":%d,\"filaments_end\":%d,\"segments_end\":%d,\"growing_end\":%d,\n",f0,s0,g0,f1,s1,g1);
 fprintf(metrics,"\"rebuilds\":%llu,\"force_evaluations\":%llu,\"blocked_growth\":%llu,\"blocked_branches\":%llu,\n",w->rebuilds-rebuilds,w->evaluations-evaluations,w->blockedgrowth-blockedgrowth,w->blockedbranches-blockedbranches);
 fprintf(metrics,"\"movement_rebuilds_total\":%llu,\"workers_requested\":%d,\"workers_selected\":%d,\n",w->movementrebuilds,sim->filss->workers,w->workersused);
 fprintf(metrics,"\"max_penetration\":%.12g,\"contact_relaxation\":%.12g,\n\"pairs_last\":%d,\"contacts_last\":%llu,\"boxes\":%d,\"occupied_boxes\":%d\n}\n",w->maxpenetration,w->contactbound,w->npair,w->contacts,sim->boxs->nbox,sim->boxs->nsegmentbox);
 fclose(metrics);simfree(sim);return 0;
}
