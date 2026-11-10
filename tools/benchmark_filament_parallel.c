/* Isolated CPU phase measurements, not a biological simulation.
   Usage: benchmark_filament_parallel cap|elongation|contacts|implicit workers count steps
   Run each choice in a separate directory; state.bin includes subsequent RNG
   probes so the archived and current libraries can be checked exactly. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "random2.h"
#ifdef _OPENMP
#include <omp.h>
#endif

filamenttypeptr filAddFilamentType(simptr,const char *);
int filAddSegment(filamentptr,const double *,double,const double *,double,char);
void filCappingDynamics(simptr,filamenttypeptr);
void filElongationDynamics(simptr,filamenttypeptr);
void filComputeForces(filamentptr,int,int);
#define REQUIRE(c) do {if(!(c)) {fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);}} while(0)
static double seconds(void) {
#ifdef _OPENMP
 return omp_get_wtime();
#else
 struct timespec t;timespec_get(&t,TIME_UTC);return t.tv_sec+1e-9*t.tv_nsec;
#endif
}
int main(int argc,char **argv) {
 simptr sim;filamenttypeptr t;int workers,count,steps,f,i,d,contact;
 double angle[3]={0},x[3],start,elapsed;FILE *state,*metrics;int implicit;
 REQUIRE(argc==5);workers=atoi(argv[2]);count=atoi(argv[3]);steps=atoi(argv[4]);
 REQUIRE(count>0 && steps>0);contact=!strcmp(argv[1],"contacts");implicit=!strcmp(argv[1],"implicit");
 REQUIRE(contact || implicit || !strcmp(argv[1],"cap") || !strcmp(argv[1],"elongation"));
 sim=simalloc("");REQUIRE(sim);REQUIRE(!simsetdim(sim,3));Simsetrandseed(sim,7);
 REQUIRE(!simreadstring(sim,NULL,"boundaries","x -2 2 r"));
 REQUIRE(!simreadstring(sim,NULL,"boundaries","y -2 2 r"));
 REQUIRE(!simreadstring(sim,NULL,"boundaries","z -2 2 r"));
 REQUIRE(!filSetWorkers(sim,workers));sim->dt=1e-4;
 t=filAddFilamentType(sim,"actin");REQUIRE(t);
 t->dynamics=FDeuler;t->kT=0.001;t->brownian=0;t->stdlen=0.01;t->klen=10000;
 t->kypr[0]=t->kypr[1]=t->kypr[2]=1;
 t->stericradius=contact?0.0035:0;t->sterick=4000;t->stericskin=0.002;
 t->endtrans[1][FESopen][FEScapped]=0.36;t->endtrans[1][FEScapped][FESopen]=0.1;t->endrate[1][FESopen]=0.06;
 if(implicit) {t->dynamics=FDimplicitnetwork;t->kT=0;t->kypr[0]=t->kypr[1]=t->kypr[2]=0;t->endtrans[1][FESopen][FEScapped]=t->endtrans[1][FEScapped][FESopen]=t->endrate[1][FESopen]=0;}
 for(f=0;f<count;f++) {
  filamentptr fil=filAddFilament(t,NULL);REQUIRE(fil);
  x[0]=(f%20)*0.008;x[1]=((f/20)%20)*0.004;x[2]=(f/400)*0.004;
  REQUIRE(!filAddSegment(fil,x,0.01,angle,1,'b'));
  if(implicit) {
   fil->nodes[1][0]+=0.0002;
   for(i=1;i<512;i++) REQUIRE(!filAddSegment(fil,NULL,0.01,angle,1,'b'));
  }
  fil->endstate[1]=contact?0:f%3==0;
  fil->lenbank[1]=(f%10)*0.0009;
 }
 if(contact) {REQUIRE(!filStericPrepare(sim));for(f=0;f<count;f++) filComputeForces(t->fillist[f],-1,-1);REQUIRE(!filStericForces(sim));}
 start=seconds();
 for(i=0;i<steps;i++) {
  if(implicit) {REQUIRE(!filDynamics(sim));sim->time+=sim->dt;}
  else if(contact) REQUIRE(!filStericForces(sim));
  else if(!strcmp(argv[1],"cap")) filCappingDynamics(sim,t);
  else filElongationDynamics(sim,t);
 }
 elapsed=seconds()-start;
 state=fopen("state.bin","wb");REQUIRE(state);
 for(f=0;f<count;f++) {
  filamentptr fil=t->fillist[f];
  REQUIRE(fwrite(&fil->nseg,sizeof(int),1,state)==1);REQUIRE(fwrite(&fil->endstate[1],sizeof(int),1,state)==1);
  REQUIRE(fwrite(&fil->lenbank[1],sizeof(double),1,state)==1);
  for(i=0;i<=fil->nseg;i++) for(d=0;d<3;d++) {
   REQUIRE(fwrite(&fil->nodes[i][d],sizeof(double),1,state)==1);
   if(contact) REQUIRE(fwrite(&fil->filwork->forces[i][d],sizeof(double),1,state)==1);
  }
 }
 for(i=0;i<32;i++) {double value=gaussrandD();REQUIRE(fwrite(&value,sizeof(double),1,state)==1);}
 REQUIRE(!fclose(state));metrics=fopen("metrics.json","w");REQUIRE(metrics);
 fprintf(metrics,"{\"case\":\"%s\",\"workers\":%d,\"count\":%d,\"steps\":%d,\"seconds\":%.9f}\n",argv[1],workers,count,steps,elapsed);
 REQUIRE(!fclose(metrics));printf("%s workers=%d count=%d steps=%d seconds=%.6f\n",argv[1],workers,count,steps,elapsed);
 simfree(sim);return 0;
}
