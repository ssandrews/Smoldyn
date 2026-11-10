/* Optional continuous polymerization with a finite chemical work budget. LGPL.
   Uses the existing axial springs, implicit network, capsules and panel clamp.
   Geometry is never advanced by inserting a rod through an obstacle. */
#include <math.h>
#include <float.h>
#include <string.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"

int filAddSegment(filamentptr fil,const double *pos,double length,const double *angle,double thickness,char end);
void filNodes2Angles(filamentptr fil,int lo,int hi);

double filSegmentRestLength(const filamentptr fil,int seg) {
 return fil->filtype->stdlen+(fil->filtype->elongmodel && seg==fil->nseg-1?fil->growbank:0);
}

double filSegmentStiffness(const filamentptr fil,int seg) {
 filamenttypeptr t=fil->filtype;
 /* Preserve the original force convention exactly in legacy mode. In growth
    mode EA=klen*stdlen*thickness is fixed, so two half-elements in series have
    the same compliance and axial energy as the unsplit growing element. */
 if(!t->elongmodel) return t->klen;
 if(filSegmentIsConstrained(fil,seg)) return 0;
 return (t->klen>0?t->klen:t->elongstiffness)*t->stdlen*fil->segments[seg]->thk/filSegmentRestLength(fil,seg);
}

int filSegmentIsConstrained(const filamentptr fil,int seg) {
 return fil->filtype->klen<0 && !(fil->filtype->elongmodel && !fil->capped && seg==fil->nseg-1);
}

double filNominalLength(const filamentptr fil) {
 return fil->nseg*fil->filtype->stdlen+(fil->filtype->elongmodel?fil->growbank:0);
}

int filGrowthValidate(const filamenttypeptr t) {
 int f;
 if(!t->elongmodel) return 0;
 if(t->elongmodel!=1 || t->dynamics!=FDimplicitnetwork || !(t->stdlen>0) ||
    !(t->klen>0 || (t->klen<0 && t->elongstiffness>0 && isfinite(t->elongstiffness))) ||
    !(t->kT>0) || !(t->elongstep>0) || !(t->elongenergy>0) ||
    !isfinite(t->stdlen+t->klen+t->kT+t->elongstep+t->elongenergy) ||
    !isfinite(t->elongenergy/t->elongstep) || !isfinite(t->elongenergy/t->kT) ||
    !(t->elongenergy/t->kT>0) || !isfinite((t->klen>0?t->klen:t->elongstiffness)*t->stdlen) ||
    t->plusend!='b' || t->treadrate!=0 || t->stdypr[0]!=0 || t->stdypr[1]!=0 || t->stdypr[2]!=0) {
  simLog(t->filss->sim,9,"ERROR: mechanochemical growth requires implicit_network, positive standard_length/kT/elongation_step/elongation_energy, positive force_length or negative force_length with positive elongation_stiffness, plus_end back, zero standard_angle and no treadmilling\n");return 1;
 }
 for(f=0;f<t->nfil;f++) {
  filamentptr fil=t->fillist[f];
  if(fil->backend || !isfinite(fil->growbank) || fil->growbank<0 || fil->growbank>=t->stdlen) {
   simLog(t->filss->sim,9,"ERROR: mechanochemical elongation needs an unattached back end and incorporated growth bank in [0,standard_length); do not switch an active legacy bank\n");return 1;
  }
 }
 return 0;
}

/* Bisect a terminal element at rest length 2*stdlen without moving its ends,
   changing its occupied capsule union, or detaching branches at its old tip.
   A new midpoint has no angle or twist. The old tip retains its mobility. */
static int filGrowthRefine(simptr sim,filamentptr fil) {
 int last=fil->nseg-1,d,br;double endpoint[3],midpoint[3],angle[3]={0,0,0};
 double len=fil->segments[last]->len,thk=fil->segments[last]->thk,mob=fil->nodemobility[last+1];
 for(d=0;d<3;d++) {endpoint[d]=fil->nodes[last+1][d];midpoint[d]=0.5*(fil->nodes[last][d]+endpoint[d]);}
 if(filAddSegment(fil,NULL,0.5*len,angle,thk,'b')) return 1;
 for(d=0;d<3;d++) {fil->nodes[last+1][d]=midpoint[d];fil->nodes[last+2][d]=endpoint[d];}
 fil->nodemobility[last+1]=1;fil->nodemobility[last+2]=mob;
 for(br=0;br<fil->nbranch;br++) if(fil->branchspots[br]==last) fil->branchspots[br]++;
 fil->growbank=0;
 filNodes2Angles(fil,-1,-1);
 /* The old indexed capsule now covers the first half. Register the other half
    immediately for later chemistry queries; mechanics rebuilds normal boxes. */
 if(filStericRegisterSegment(sim,fil->segments[last+1])) return 1;
 fil->filtype->elongremesh++;
 return 0;
}

int filMechanochemicalElongate(simptr sim,filamentptr fil,double length) {
 filamenttypeptr t=fil->filtype;int last=fil->nseg-1,added=0;
 double rest,len,EA,g,drive,factor,increment,limit,oldenergy,newenergy,stall;
 if(last<0 || fil->capped || length<=0) return 0;
 if(!isfinite(length) || fil->backend || t->plusend!='b' ||
    !(t->klen>0 || (t->klen<0 && t->elongstiffness>0)) || t->kT<=0 ||
    t->elongstep<=0 || t->elongenergy<=0) return -1;
 t->elongattempted+=length;
 rest=filSegmentRestLength(fil,last);len=fil->segments[last]->len;
 EA=(t->klen>0?t->klen:t->elongstiffness)*t->stdlen*fil->segments[last]->thk;
 if(!(len>0) || !(rest>0) || !(EA>0) || !isfinite(rest+len+EA)) return -1;
 /* E=EA*(l-r)^2/(2r); g=dE/dr=EA/2*(1-(l/r)^2).
    Use its positive part as the work opposing polymerization. The exponential
    forward/backward-rate difference is normalized to unloaded elongation_rate.
    Reverse shrinkage is not modeled: negative net growth is clipped to zero. */
 g=fmax(0,0.5*EA*(1-(len/rest)*(len/rest)));
 drive=t->elongenergy/t->kT;stall=t->elongenergy/t->elongstep;
 factor=g>=stall?0:exp(-g*t->elongstep/t->kT)*(-expm1(-(t->elongenergy-g*t->elongstep)/t->kT))/(-expm1(-drive));
 increment=length*factor;
 if(t->elongmaxlen>0) increment=fmin(increment,fmax(0,t->elongmaxlen-filNominalLength(fil)));
 /* Frozen-geometry energy cap. g(r) is monotone increasing, so bounding its
    endpoint by stall ensures Delta E <= (Delta mu / delta)*Delta r even for
    a large chemical interval. Smaller timesteps are still needed for kinetics. */
 if(stall<0.5*EA) {
  limit=len/sqrt(1-2*stall/EA);
  increment=fmin(increment,fmax(0,limit-rest));
 }
 if(increment<=0) {t->elongstalled++;return 0;}
 /* One refinement per call; retain no unincorporated debt after rate limiting.
    Normal timesteps add far less than a segment. Reject oversized requested
    increments instead of quietly discarding most of an arbitrarily large dt. */
 if(length>0.25*t->stdlen) {
  simLog(sim,9,"ERROR: mechanochemical elongation_rate*time_step must be <= standard_length/4; reduce time_step\n");return -1;
 }
 increment=fmin(increment,2*t->stdlen-rest);
 oldenergy=0.5*EA*(len-rest)*(len-rest)/rest;
 fil->growbank+=increment;
 newenergy=0.5*EA*(len-rest-increment)*(len-rest-increment)/(rest+increment);
 t->elongaccepted+=increment;t->elongwork+=newenergy-oldenergy;
 if(fil->growbank>=t->stdlen*(1-8*DBL_EPSILON)) {
  if(filGrowthRefine(sim,fil)) return -1;
  added=1;
 }
 return added;
}

void filGrowthReport(simptr sim) {
 int ft;
 if(!sim->filss) return;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];if(!t->elongmodel) continue;
  simLog(sim,2,"Mechanochemical growth %s: requested %g, incorporated %g length; %llu stalled calls, %llu refinements\n",
   t->ftname,t->elongattempted,t->elongaccepted,t->elongstalled,t->elongremesh);
  simLog(sim,2,"  elastic work %g, available chemical work %g energy units (elongation only; branch births excluded)\n",
   t->elongwork,t->elongaccepted*t->elongenergy/t->elongstep);
 }
}
