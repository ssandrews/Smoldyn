/* Filament capsule contacts. LGPL, as the surrounding Smoldyn sources. */
#ifndef SMOL_FILAMENT_STERIC_H
#define SMOL_FILAMENT_STERIC_H

typedef struct filstericsegment {
  filamentptr fil;
  segmentptr segment;
  int index;
  double reference[6];
  double bounds[6]; /* centerline AABB, computed only during neighbor rebuilds */
  int boxlo[3];
  double radius, stiffness;
  double contactrow[2];
} FilStericSegment;

typedef struct filstericpair { int a,b; } FilStericPair;
/* One writer per pair; accumulation retains the original pair order. */
typedef struct filstericcontact {double s,t,normal[3],delta,k;} FilStericContact;

typedef struct filstericnode {
  filamentptr fil;
  int node,parent,fixed;
  double drag,force[3],position[3];
} FilStericNode;

/* Borrowed topology snapshots; positions, forces and mobilities remain live. */
typedef struct filstericfilament {
  filamentptr fil,frontend;
  int nseg,nbranch,branchstart,nodeoffset,worknext;
} FilStericFilament;
typedef struct filstericbranch {filamentptr daughter;int spot;} FilStericBranch;

struct filamentstericstruct {
  FilStericSegment *segments;
  FilStericPair *pairs;
  FilStericNode *nodes;
  segmentptr *pending;
  FilStericFilament *filaments;
  FilStericBranch *branches;
  int nfilament,maxfilament,nbranch,maxbranch,topologyvalid,workersused;
  struct filamentimplicitstruct *implicit; /* owned nonlinear solver workspace */
  int npending,maxpending,inchemistry,queryerror;
  int nsegment,maxsegment,npair,maxpair,nnode,maxnode;
  double skin,maxpenetration,energy,contactbound;
  unsigned long long rebuilds,evaluations,contacts,blockedgrowth,blockedbranches;
  unsigned long long boxgeneration;
  unsigned long long movementrebuilds;
  FilStericContact *contact;
  unsigned char *chemready;
  int maxcontact,maxchemready;
  int workfirst[256],scheduleworkers;
  int *memberfirst,*membernext,maxmember;
};

/* Segment pointers live in the original Smoldyn boxes; only contact pairs are cached. */
int filBoxesBuild(simptr sim,struct filamentstericstruct *work);
int filImplicitEnabled(const simptr sim);
int filImplicitSave(simptr sim,int restore);
int filImplicitStep(simptr sim,int workers);
void filImplicitFree(struct filamentstericstruct *work);
void filImplicitReport(simptr sim);
int filStericPrepareMovement(simptr sim);
int filStericWorkers(simptr sim);
int filWorkWorkers(simptr sim,int items,int grain);
void filWorkSchedule(simptr sim,int workers);
unsigned char *filChemWork(simptr sim,int count);
/* Each root owns its sum. Member lists are built once per topology change;
   their order matches the old serial scatter exactly, even for nested branches. */
static inline void filStericNodeForce(simptr sim,int i,int implicit) {
 struct filamentstericstruct *w=sim->filss->steric;
 FilStericNode *root=&w->nodes[i];int j,d;
 for(d=0;d<sim->dim;d++) root->force[d]=0;
 if(root->parent!=i) return;
 for(j=w->memberfirst[i];j>=0;j=w->membernext[j]) {
  FilStericNode *node=&w->nodes[j];filamentptr fil=node->fil;
  if(fil->filtype->dynamics!=FDnone && (implicit || fil->filtype->mobility*fil->nodemobility[node->node]>0))
   for(d=0;d<sim->dim;d++) root->force[d]+=fil->filwork->forces[node->node][d];
 }
}
void filStericOrdinaryForces(simptr sim,int workers,int freshnoise);
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
