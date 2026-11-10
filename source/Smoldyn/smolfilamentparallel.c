/* Optional scheduling around upstream end-state chemistry. RNG and segment
   births retain their upstream serial order; private bank/state writes scale. */
#include <math.h>
#include <limits.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "random2.h"

int filParallelEndTransitions(simptr sim,filamenttypeptr type) {
 int workers=filWorkWorkers(sim,type->nfil,16384),f,e,s,t,any=0;
 double total[2][FESMAX],prob[2][FESMAX];int count[2][FESMAX],single[2][FESMAX];
 unsigned char *next;
 if(workers<=1 || type->nfil>INT_MAX/2) return 0;
 for(e=0;e<2;e++) for(s=0;s<FESMAX;s++) {
  total[e][s]=0;count[e][s]=0;single[e][s]=s;
  for(t=0;t<FESMAX;t++) if(type->endtrans[e][s][t]>0) {
   total[e][s]+=type->endtrans[e][s][t];count[e][s]++;single[e][s]=t;
  }
  prob[e][s]=1-exp(-total[e][s]*sim->dt);any|=count[e][s]>0;
 }
 if(!any) return 1;
 next=filChemWork(sim,2*type->nfil);if(!next) return 0;
 for(f=0;f<type->nfil;f++) for(e=0;e<2;e++) {
  filamentptr fil=type->fillist[f];int state=fil->endstate[e],dest=state;
  if(fil->nseg && count[e][state] && coinrandD(prob[e][state])) {
   dest=single[e][state];
   if(count[e][state]>1) {
    double r=unirandCOD(0,total[e][state]);
    for(t=0;t<FESMAX;t++) if(type->endtrans[e][state][t]>0) {dest=t;r-=type->endtrans[e][state][t];if(r<0) break;}
   }
  }
  next[2*f+e]=(unsigned char)dest;
 }
#ifdef _OPENMP
#pragma omp parallel for num_threads(workers) schedule(static)
#endif
 for(f=0;f<type->nfil;f++) for(int end=0;end<2;end++) type->fillist[f]->endstate[end]=(enum FilEndState)next[2*f+end];
 return 1;
}

int filParallelEndDynamics(simptr sim,filamenttypeptr type) {
 int workers=filWorkWorkers(sim,type->nfil,2048),f,e,s,active[2]={0,0},end;
 unsigned char *ready;
 if(workers<=1 || type->elongmodel || type->stdlen<=0) return 0;
 /* Two growing ends can compete for one length ceiling; retain the generic
    upstream loop for that case, shrinking and force-dependent barbed kinetics. */
 for(e=0;e<2;e++) for(s=0;s<FESMAX;s++) {
  if(type->endrate[e][s]<0 || (s==FESbarbed && (type->barbedparameters[e][0]>0 || type->barbedparameters[e][1]>0))) return 0;
  active[e]|=type->endrate[e][s]>0;
 }
 if(active[0] && active[1]) return 0;
 if(!active[0] && !active[1]) return 1;
 end=active[1]?1:0;if(workers>4) workers=4;
 ready=filChemWork(sim,type->nfil);if(!ready) return 0;
#ifdef _OPENMP
#pragma omp parallel for num_threads(workers) schedule(static)
#endif
 for(f=0;f<type->nfil;f++) {
  filamentptr fil=type->fillist[f];double length=type->endrate[end][fil->endstate[end]]*sim->dt;
  ready[f]=0;
  if(!fil->nseg || length<=0 || (type->elongmaxlen>0 && filContourLength(fil)>=type->elongmaxlen)) continue;
  if(fil->lenbank[end]+length<type->stdlen) fil->lenbank[end]+=length;else ready[f]=1;
 }
 for(f=0;f<type->nfil;f++) if(ready[f]) {
  filamentptr fil=type->fillist[f];
  filEndLengthChange(sim,fil,end,type->endrate[end][fil->endstate[end]]*sim->dt);
 }
 return 1;
}
