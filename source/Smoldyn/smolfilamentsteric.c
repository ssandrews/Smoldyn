/* Finite-volume filament mechanics, LGPL as the surrounding Smoldyn sources. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smolfilamentsteric.h"
#include "Geometry.h"

/* Existing filament helpers used by the coupled mechanical path. */
void filComputeForces(filamentptr fil,int lo,int hi);
void filStepDynamics(filamentptr fil,double dtmu,int in,int out);
void filNodes2Angles(filamentptr fil,int lo,int hi);

int filStericEnabled(const simptr sim) {
 int ft;
 if(!sim || !sim->filss) return 0;
 for(ft=0;ft<sim->filss->ntype;ft++) if(sim->filss->filtypes[ft]->stericradius>0) return 1;
 return 0;
}

int filStericValidate(const simptr sim) {
 int ft,d,ps;filamenttypeptr t;
 if(!filStericEnabled(sim)) return 0;
 if(sim->dim!=2 && sim->dim!=3) return 1;
 for(d=0;d<2*sim->dim;d++) if(sim->wlist[d]->type=='p') {
  simLog(sim,9,"ERROR: filament sterics does not yet support periodic boundaries\n");return 1;
 }
 for(ft=0;ft<sim->filss->ntype;ft++) {
  t=sim->filss->filtypes[ft];
  if(t->dynamics!=FDeuler && t->dynamics!=FDnone) {
   simLog(sim,9,"ERROR: filament sterics requires dynamics euler or none for every filament type\n");return 1;
  }
  if(t->branchazimuthfix) {
   simLog(sim,9,"ERROR: filament sterics requires branch_azimuth_fix off (use branch_force_azimuth)\n");return 1;
  }
  if(t->stericradius>0 && (!(t->sterick>0) || !(t->stericskin>0))) {
   simLog(sim,9,"ERROR: filament steric_radius requires positive steric_stiffness and steric_skin\n");return 1;
  }
  if(t->stericradius>0 && t->confinesrf && t->confineforce>0)
   for(ps=0;ps<PSMAX;ps++) if(ps!=PSrect && t->confinesrf->npanel[ps]) {
    simLog(sim,9,"ERROR: radius-aware filament confinement currently requires rectangular panels\n");return 1;
   }
 }
 return 0;
}

void filStericFree(filamentssptr ss) {
 struct filamentstericstruct *w;
 if(!ss || !(w=ss->steric)) return;
 free(w->segments);free(w->pairs);free(w->nodes);free(w->pending);free(w);ss->steric=NULL;
}

int filStericExcluded(segmentptr a,segmentptr b) {
 return a==b || (a->fil==b->fil && abs(a->index-b->index)<=1);
}

/* Direct bonded pairs share volume only near their junction. Trim a bounded
   daughter contour region only when the mother's segment is local in contour
   distance; remote mother segments still contact the daughter's base. */
static double filJunctionTrim(segmentptr daughter,segmentptr mother) {
 filamentptr mf=mother->fil,df=daughter->fil;
 int br,spot=-1,i;double reach,md=0,dd=0;
 if(df->frontend!=mf) return 0;
 for(br=0;br<mf->nbranch;br++) if(mf->branches[br]==df) {spot=mf->branchspots[br];break;}
 if(spot<0) return 0;
 reach=2*(df->filtype->stericradius+mf->filtype->stericradius);
 if(mother->index>spot) for(i=spot+1;i<mother->index && md<=reach;i++) md+=mf->segments[i]->len;
 else for(i=mother->index+1;i<=spot && md<=reach;i++) md+=mf->segments[i]->len;
 if(md>reach) return 0;
 for(i=0;i<daughter->index && dd<reach;i++) dd+=df->segments[i]->len;
 if(dd>=reach) return 0;
 return daughter->len>0?fmin(1,(reach-dd)/daughter->len):1;
}

double filStericGeometry(segmentptr a,segmentptr b,double *s,double *t,double *normal) {
 double trimA=filJunctionTrim(a,b),trimB=filJunctionTrim(b,a),a0[3],b0[3],dist;
 int d,dim=a->fil->filtype->filss->sim->dim;
 if(trimA>=1 || trimB>=1) return DBL_MAX;
 for(d=0;d<3;d++) {
  a0[d]=a->xyzfront[d]+trimA*(a->xyzback[d]-a->xyzfront[d]);
  b0[d]=b->xyzfront[d]+trimB*(b->xyzback[d]-b->xyzfront[d]);
 }
 dist=Geo_ClosestSeg2Seg(a0,a->xyzback,b0,b->xyzback,dim,s,t,normal);
 *s=trimA+(1-trimA)*(*s);*t=trimB+(1-trimB)*(*t);
 return dist;
}

int filStericPrepare(simptr sim) {
 struct filamentstericstruct *w;int ft,f,i,n=0,j=0,d,rebuild=0;
 double skin=DBL_MAX,move;
 if(!filStericEnabled(sim)) return 0;
 if(!sim->boxs || !sim->boxs->blist) {if(boxesupdate(sim)) return 1;}
 if(!sim->filss->steric) {
  sim->filss->steric=(struct filamentstericstruct*)calloc(1,sizeof(*w));
  if(!sim->filss->steric) return 1;
 }
 w=sim->filss->steric;
 if(w->boxgeneration!=sim->boxs->segmentgeneration) rebuild=1;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr t=sim->filss->filtypes[ft];
  if(t->stericradius<=0) continue;
  skin=fmin(skin,t->stericskin);
  for(f=0;f<t->nfil;f++) n+=t->fillist[f]->nseg;
 }
 if(n>w->maxsegment) {
  FilStericSegment *p;int cap=n+n/2+16;
  p=(FilStericSegment*)realloc(w->segments,(size_t)cap*sizeof(*p));if(!p) return 1;
  w->segments=p;w->maxsegment=cap;rebuild=1;
 }
 if(n!=w->nsegment || skin!=w->skin || !w->rebuilds) rebuild=1;
 for(ft=0;ft<sim->filss->ntype;ft++) {
  filamenttypeptr type=sim->filss->filtypes[ft];
  if(type->stericradius<=0) continue;
  for(f=0;f<type->nfil;f++) {
   filamentptr fil=type->fillist[f];
   for(i=0;i<fil->nseg;i++,j++) {
    FilStericSegment *r=&w->segments[j];segmentptr seg=fil->segments[i];
    if(!rebuild && (r->fil!=fil || r->segment!=seg || r->index!=i || r->radius!=type->stericradius || r->stiffness!=type->sterick)) rebuild=1;
    if(!rebuild) {
     double front=0,back=0;
     for(d=0;d<sim->dim;d++) {move=seg->xyzfront[d]-r->reference[d];front+=move*move;move=seg->xyzback[d]-r->reference[d+3];back+=move*move;}
     if(fmax(front,back)>=skin*skin/4) rebuild=1;
    }
    r->fil=fil;r->segment=seg;r->index=i;r->radius=type->stericradius;r->stiffness=type->sterick;
   }
  }
 }
 w->nsegment=n;w->skin=skin;
 if(rebuild) {
  if(filBoxesBuild(sim,w)) return 1;
  for(j=0;j<n;j++) for(d=0;d<3;d++) {
   w->segments[j].reference[d]=w->segments[j].segment->xyzfront[d];
   w->segments[j].reference[d+3]=w->segments[j].segment->xyzback[d];
  }
  w->rebuilds++;
 }
 return 0;
}

int filStericForces(simptr sim) {
 struct filamentstericstruct *w;int p,d;
 if(filStericPrepare(sim)) return 1;
 w=sim->filss->steric;if(!w) return 0;
 w->contacts=0;w->maxpenetration=w->energy=w->contactbound=0;
 for(p=0;p<w->nsegment;p++) w->segments[p].contactrow[0]=w->segments[p].contactrow[1]=0;
 for(p=0;p<w->npair;p++) {
  FilStericSegment *a=&w->segments[w->pairs[p].a],*b=&w->segments[w->pairs[p].b];
  double s,t,normal[3],dist,delta,k,fa,fb;
  dist=filStericGeometry(a->segment,b->segment,&s,&t,normal);
  delta=a->radius+b->radius-dist;if(delta<=0) continue;
  /* Harmonic mean: equal-type contacts retain the user stiffness. */
  k=2/(1/a->stiffness+1/b->stiffness);
  w->contacts++;w->maxpenetration=fmax(w->maxpenetration,delta);w->energy+=0.5*k*delta*delta;
  for(d=0;d<sim->dim;d++) {
   fa=k*delta*normal[d];fb=-fa;
   if(a->fil->filtype->dynamics!=FDnone) {
    a->fil->filwork->forces[a->index][d]+=(1-s)*fa;a->fil->filwork->forces[a->index+1][d]+=s*fa;
   }
   if(b->fil->filtype->dynamics!=FDnone) {
    b->fil->filwork->forces[b->index][d]+=(1-t)*fb;b->fil->filwork->forces[b->index+1][d]+=t*fb;
   }
  }
  /* Row-sum bound on normal contact stiffness, including accumulated contacts.
     Internal elastic modes still require timestep convergence checks. */
  a->contactrow[0]+=2*k*(1-s);a->contactrow[1]+=2*k*s;
  b->contactrow[0]+=2*k*(1-t);b->contactrow[1]+=2*k*t;
 }
 for(p=0;p<w->nsegment;p++) {
  FilStericSegment *a=&w->segments[p];double row=a->contactrow[0];
  if(a->fil->filtype->dynamics==FDnone) continue;
  if(p && w->segments[p-1].fil==a->fil && w->segments[p-1].index==a->index-1) row+=w->segments[p-1].contactrow[1];
  w->contactbound=fmax(w->contactbound,sim->dt*a->fil->filtype->mobility*a->fil->nodemobility[a->index]*row);
  if(a->index==a->fil->nseg-1) w->contactbound=fmax(w->contactbound,sim->dt*a->fil->filtype->mobility*a->fil->nodemobility[a->index+1]*a->contactrow[1]);
 }
 w->evaluations++;
 return 0;
}

/* At chemistry entry the cached inflated boxes cover every existing capsule:
   endpoints are at most skin/2 from their reference positions. Chemistry only
   inserts/removes segments; accepted insertions go in a small pending list until
   the mechanical phase rebuilds the index. No full-network scan per growth retry. */
int filStericChemistry(simptr sim,int begin) {
 struct filamentstericstruct *w;
 if(begin) {
  if(filStericPrepare(sim)) return 1;
  w=sim->filss->steric;w->npending=w->queryerror=0;w->inchemistry=1;
 } else {w=sim->filss->steric;w->inchemistry=0;}
 return w->queryerror;
}

static int filStericTrialContact(simptr sim,segmentptr trial,segmentptr other) {
 int d;double extent,s,t,n[3];filamentptr f=other->fil;
 /* Removed segments remain allocated and may appear in the old cache/pending
    list. Only current members of the live segment array can obstruct growth. */
 if(other->index<0 || other->index>=f->nseg || f->segments[other->index]!=other || filStericExcluded(trial,other)) return 0;
 extent=trial->fil->filtype->stericradius+f->filtype->stericradius;
 for(d=0;d<sim->dim;d++)
  if(fmin(trial->xyzfront[d],trial->xyzback[d])>fmax(other->xyzfront[d],other->xyzback[d])+extent || fmin(other->xyzfront[d],other->xyzback[d])>fmax(trial->xyzfront[d],trial->xyzback[d])+extent) return 0;
 return filStericGeometry(trial,other,&s,&t,n)+1e-12<extent;
}

/* Full trial capsule, including its shaft: an endpoint cannot jump through an
   obstacle. Query sorted occupied cells, then newly accepted uncached segments. */
int filStericSegmentBlocked(simptr sim,segmentptr trial) {
 struct filamentstericstruct *w;int lo[3],hi[3],cell[3],d,i;
 BoxGridCell found;
 double radius=trial->fil->filtype->stericradius;
 if(radius<=0) return 0;
 w=sim->filss->steric;
 if(!w || !w->inchemistry) {if(filStericPrepare(sim)) return 1;w=sim->filss->steric;}
 if(sim->boxs->grid.width[0]>0) {
  if(boxGridBounds(&sim->boxs->grid,trial->xyzfront,trial->xyzback,radius,lo,hi)) {w->queryerror=1;return 1;}
  for(cell[0]=lo[0];cell[0]<=hi[0];cell[0]++) for(cell[1]=lo[1];cell[1]<=hi[1];cell[1]++) for(cell[2]=lo[2];cell[2]<=hi[2];cell[2]++) {
   if(!boxGridFindCell(&sim->boxs->grid,cell,&found)) continue;
   for(i=0;i<found.box->nsegment;i++) {
    segmentptr other=found.box->segment[i];FilStericSegment *s;
    if(other->fil->filtype->stericradius<=0) continue;
    if(other->stericindex<0 || other->stericindex>=w->nsegment) continue;
    s=&w->segments[other->stericindex];
    for(d=0;d<3;d++) if(cell[d]!=(lo[d]>s->boxlo[d]?lo[d]:s->boxlo[d])) break;
    if(d==3 && filStericTrialContact(sim,trial,s->segment)) return 1;
   }
  }
 }
 for(i=0;i<w->npending;i++) if(filStericTrialContact(sim,trial,w->pending[i])) return 1;
 return filStericRegisterSegment(sim,trial);
}

/* Also register accepted unchecked growth so other tips can see it immediately. */
int filStericRegisterSegment(simptr sim,segmentptr trial) {
 struct filamentstericstruct *w=sim->filss->steric;int i;
 if(w && w->inchemistry && trial->fil->filtype->stericradius>0) {
  for(i=0;i<w->npending && w->pending[i]!=trial;i++);
  if(i==w->npending) {
   if(w->npending==w->maxpending) {
    int cap=w->maxpending?2*w->maxpending:32;segmentptr *p=(segmentptr*)realloc(w->pending,(size_t)cap*sizeof(*p));
    if(!p) {w->queryerror=1;simLog(sim,9,"ERROR: cannot allocate filament growth contact workspace\n");return 1;}
    w->pending=p;w->maxpending=cap;
   }
   w->pending[w->npending++]=trial;
  }
 }
 return 0;
}

/* Exact capsule clearance, with an AABB lower bound to avoid unnecessary
   closest-point calculations. Queries never insert into the chemistry cache. */
static void filQueryCandidate(simptr sim,const double *a,const double *b,double radius,
 segmentptr trial,segmentptr other,double *best,segmentptr *closest) {
 int d;double bound2=0,clearance,s,t,n[3],extent;
 filamentptr f=other->fil;
 if(other->index<0 || other->index>=f->nseg || f->segments[other->index]!=other || f->filtype->stericradius<=0) return;
 if(trial && filStericExcluded(trial,other)) return;
 extent=radius+f->filtype->stericradius;
 for(d=0;d<sim->dim;d++) {
  double al=fmin(a[d],b[d]),ah=fmax(a[d],b[d]);
  double bl=fmin(other->xyzfront[d],other->xyzback[d]),bh=fmax(other->xyzfront[d],other->xyzback[d]);
  double gap=fmax(0,fmax(al-bh,bl-ah));bound2+=gap*gap;
 }
 if(*closest && sqrt(bound2)-extent>=*best) return;
 clearance=(trial?filStericGeometry(trial,other,&s,&t,n):
  Geo_ClosestSeg2Seg(a,b,other->xyzfront,other->xyzback,sim->dim,&s,&t,n))-extent;
 if(clearance<*best) {*best=clearance;*closest=other;}
}

segmentptr filStericQuery(simptr sim,const double *a,const double *b,double radius,
 segmentptr trial,double *distance,segmentptr *nearest) {
 struct filamentstericstruct *w;int lo[3],hi[3],cell[3],d,i,global=distance || nearest;
 double best=DBL_MAX,cells=1;segmentptr closest=NULL;BoxGridCell found;
 if(distance) *distance=DBL_MAX;
 if(nearest) *nearest=NULL;
 if(!sim || !a || !b || !isfinite(radius) || radius<0) goto invalid;
 for(d=0;d<sim->dim;d++) if(!isfinite(a[d]) || !isfinite(b[d])) goto invalid;
 if(!filStericEnabled(sim)) return NULL;
 w=sim->filss->steric;
 /* A chemistry snapshot includes a separate list of accepted new segments. */
 if(!w || !w->inchemistry) {if(filStericPrepare(sim)) goto invalid;w=sim->filss->steric;}
 if(sim->boxs->grid.width[0]>0) {
  if(boxGridBounds(&sim->boxs->grid,a,b,radius,lo,hi)) goto invalid;
  for(d=0;d<3;d++) cells*=1.0+(double)hi[d]-lo[d];
  /* Large diagonal lines must not enumerate an enormous mostly empty AABB.
     Global nearest queries also use a linear bounds pass with exact pruning. */
  if(!global && cells<=4.0*fmax(1,w->nsegment)) {
   for(cell[0]=lo[0];cell[0]<=hi[0];cell[0]++) for(cell[1]=lo[1];cell[1]<=hi[1];cell[1]++) for(cell[2]=lo[2];cell[2]<=hi[2];cell[2]++) {
    if(!boxGridFindCell(&sim->boxs->grid,cell,&found)) continue;
    for(i=0;i<found.box->nsegment;i++) {
     segmentptr other=found.box->segment[i];FilStericSegment *s;
     if(other->fil->filtype->stericradius<=0) continue;
     if(other->stericindex<0 || other->stericindex>=w->nsegment) continue;
     s=&w->segments[other->stericindex];
     for(d=0;d<3;d++) if(cell[d]!=(lo[d]>s->boxlo[d]?lo[d]:s->boxlo[d])) break;
     if(d==3) filQueryCandidate(sim,a,b,radius,trial,s->segment,&best,&closest);
     if(closest && best<=0) return closest;
    }
   }
  } else global=1;
 }
 if(global) for(i=0;i<w->nsegment;i++) filQueryCandidate(sim,a,b,radius,trial,w->segments[i].segment,&best,&closest);
 for(i=0;i<w->npending;i++) {
  filQueryCandidate(sim,a,b,radius,trial,w->pending[i],&best,&closest);
  if(!global && closest && best<=0) return closest;
 }
 if(distance) *distance=best;
 if(nearest) *nearest=closest;
 return closest && best<=0?closest:NULL;
invalid:
 if(distance) *distance=NAN;
 return NULL;
}

static int filNodeRoot(FilStericNode *nodes,int i) {
 int root=i,next;
 while(nodes[root].parent!=root) root=nodes[root].parent;
 while(nodes[i].parent!=i) {next=nodes[i].parent;nodes[i].parent=root;i=next;}
 return root;
}
static int filNodeOffset(struct filamentstericstruct *w,filamentptr f) {
 int i=f->stericnodeoffset;
 return i>=0 && i<w->nnode && w->nodes[i].fil==f?i:-1;
}

/* Shared branch-point displacement is the sum of member forces divided by the
   sum of their drags. Summed independent thermal forces give the corresponding
   constrained diffusion (for equal temperature). A fixed member anchors the
   group. Daughter node 0 is moved with its mother; other nodes are never rigidly
   translated as an after-the-fact correction. */
static int filStericMove(simptr sim) {
 struct filamentstericstruct *w=sim->filss->steric;int n=0,i,j,ft,f,br,base,other,root,d;
 for(ft=0;ft<sim->filss->ntype;ft++) for(f=0;f<sim->filss->filtypes[ft]->nfil;f++) if(sim->filss->filtypes[ft]->fillist[f]->nseg) n+=sim->filss->filtypes[ft]->fillist[f]->nseg+1;
 if(n>w->maxnode) {
  FilStericNode *p=(FilStericNode*)realloc(w->nodes,(size_t)(n+n/2+16)*sizeof(*p));if(!p) return 1;
  w->nodes=p;w->maxnode=n+n/2+16;
 }
 w->nnode=n;
 for(ft=0,i=0;ft<sim->filss->ntype;ft++) for(f=0;f<sim->filss->filtypes[ft]->nfil;f++) {
  filamentptr fil=sim->filss->filtypes[ft]->fillist[f];
  fil->stericnodeoffset=-1;if(!fil->nseg) continue;fil->stericnodeoffset=i;
  for(j=0;j<=fil->nseg;j++,i++) {memset(&w->nodes[i],0,sizeof(w->nodes[i]));w->nodes[i].fil=fil;w->nodes[i].node=j;w->nodes[i].parent=i;}
 }
 for(base=0;base<n;base+=w->nodes[base].fil->nseg+1) {
  filamentptr mother=w->nodes[base].fil;
  for(br=0;br<mother->nbranch;br++) {
   filamentptr daughter=mother->branches[br];int spot=mother->branchspots[br];
   if(!daughter || daughter->frontend!=mother || spot<0 || spot>=mother->nseg) continue;
   other=filNodeOffset(w,daughter);if(other<0 || !daughter->nseg) continue;
   if(daughter->filtype->kT!=mother->filtype->kT) {
    simLog(sim,9,"ERROR: constrained filament branch points require matching kT\n");return 1;
   }
   root=filNodeRoot(w->nodes,base+spot+1);
   w->nodes[filNodeRoot(w->nodes,other)].parent=root;
  }
 }
 for(i=0;i<n;i++) {
  FilStericNode *node=&w->nodes[i];filamentptr fil=node->fil;
  double mu=fil->filtype->dynamics==FDnone?0:fil->filtype->mobility*fil->nodemobility[node->node];
  root=filNodeRoot(w->nodes,i);
  if(mu<=0) w->nodes[root].fixed=1;
  else {w->nodes[root].drag+=1/mu;for(d=0;d<sim->dim;d++) w->nodes[root].force[d]+=fil->filwork->forces[node->node][d];}
  if(root!=i) for(d=0;d<sim->dim;d++) if(fabs(fil->nodes[node->node][d]-w->nodes[root].fil->nodes[w->nodes[root].node][d])>1e-8) {
   simLog(sim,9,"ERROR: filament branch point is detached; repair initial geometry before steric dynamics\n");return 1;
  }
 }
 for(i=0;i<n;i++) if(filNodeRoot(w->nodes,i)==i) for(d=0;d<sim->dim;d++) {
  FilStericNode *node=&w->nodes[i];
  node->position[d]=node->fil->nodes[node->node][d]+(node->fixed?0:sim->dt*node->force[d]/node->drag);
  if(!isfinite(node->position[d])) {simLog(sim,9,"ERROR: nonfinite filament position; reduce timestep or increase steric_substeps\n");return 1;}
 }
 /* Keep the existing material-frame/roll update, then set the common-state
    Cartesian positions computed above. There is no whole-filament repinning. */
 for(base=0;base<n;base+=w->nodes[base].fil->nseg+1) {
  filamentptr fil=w->nodes[base].fil;
  if(fil->nseg && fil->filtype->dynamics==FDeuler) filStepDynamics(fil,sim->dt*fil->filtype->mobility,0,0);
 }
 for(i=0;i<n;i++) {
  root=filNodeRoot(w->nodes,i);
  for(d=0;d<sim->dim;d++) w->nodes[i].fil->nodes[w->nodes[i].node][d]=w->nodes[root].position[d];
 }
 for(base=0;base<n;base+=w->nodes[base].fil->nseg+1) if(w->nodes[base].fil->nseg) filNodes2Angles(w->nodes[base].fil,-1,-1);
 return 0;
}

int filStericDynamics(simptr sim) {
 int step,steps=1,ft,f,er=0;double dt=sim->dt;
 if(filStericValidate(sim)) return 1;
 for(ft=0;ft<sim->filss->ntype;ft++) if(sim->filss->filtypes[ft]->stericsubsteps>steps) steps=sim->filss->filtypes[ft]->stericsubsteps;
 sim->dt=dt/steps;
 for(step=0;step<steps;step++) {
  for(ft=0;ft<sim->filss->ntype;ft++) {
   filamenttypeptr t=sim->filss->filtypes[ft];if(t->dynamics!=FDeuler) continue;
   for(f=0;f<t->nfil;f++) {
    filamentptr fil=t->fillist[f];if(!fil->nseg) continue;
    /* Independent Brownian increments for each mechanical substep. */
    fil->filwork->thermtime=-DBL_MAX;filComputeForces(fil,-1,-1);
   }
  }
  if((er=filAddFilamentForce(sim))) break;
  if(sim->filss->steric->contactbound>0.5) {
   simLog(sim,9,"ERROR: contact relaxation factor %g exceeds 0.5; reduce time_step or increase steric_substeps\n",sim->filss->steric->contactbound);er=1;break;
  }
  if((er=filStericMove(sim))) break;
 }
 sim->dt=dt;
 return er;
}

void filStericReport(simptr sim) {
 struct filamentstericstruct *w;
 if(!sim->filss || !(w=sim->filss->steric)) return;
 simLog(sim,2,"Filament sterics (last force evaluation): %i segments, %i neighbor pairs, %llu contacts, max penetration %g, energy %g\n",w->nsegment,w->npair,w->contacts,w->maxpenetration,w->energy);
 simLog(sim,2,"  %llu neighbor rebuilds, %llu force evaluations, %llu blocked growth segments, %llu blocked branches\n",w->rebuilds,w->evaluations,w->blockedgrowth,w->blockedbranches);
}
