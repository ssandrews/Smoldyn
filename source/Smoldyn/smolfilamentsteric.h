/* Filament capsule contacts. LGPL, as the surrounding Smoldyn sources. */
#ifndef SMOL_FILAMENT_STERIC_H
#define SMOL_FILAMENT_STERIC_H

typedef struct filstericsegment {
  filamentptr fil;
  segmentptr segment;
  int index;
  double reference[6];
  int boxlo[3];
  double radius, stiffness;
  double contactrow[2];
} FilStericSegment;

typedef struct filstericpair { int a,b; } FilStericPair;

typedef struct filstericnode {
  filamentptr fil;
  int node,parent,fixed;
  double drag,force[3],position[3];
} FilStericNode;

struct filamentstericstruct {
  FilStericSegment *segments;
  FilStericPair *pairs;
  BoxGrid grid;  /* sparse instance of the same interface as boxsuperstruct.grid */
  FilStericNode *nodes;
  segmentptr *pending;
  int npending,maxpending,inchemistry,queryerror;
  int nsegment,maxsegment,npair,maxpair,nnode,maxnode;
  double skin,maxpenetration,energy,contactbound;
  unsigned long long rebuilds,evaluations,contacts,blockedgrowth,blockedbranches;
};

/* Sparse boxes store segment IDs, using expanded bounds and unique pair ownership. */
int filBoxesBuild(simptr sim,struct filamentstericstruct *work);
int filStericEnabled(const simptr sim);
int filStericValidate(const simptr sim);
void filStericFree(filamentssptr filss);
int filStericPrepare(simptr sim);
int filStericForces(simptr sim);
int filStericDynamics(simptr sim);
int filStericSegmentBlocked(simptr sim,segmentptr segment);
int filStericRegisterSegment(simptr sim,segmentptr segment);
segmentptr filStericQuery(simptr sim,const double *a,const double *b,double radius,segmentptr trial,double *distance,segmentptr *nearest);
int filStericChemistry(simptr sim,int begin);
void filStericReport(simptr sim);
double filStericGeometry(segmentptr a,segmentptr b,double *s,double *t,double *normal);
int filStericExcluded(segmentptr a,segmentptr b);

#endif
