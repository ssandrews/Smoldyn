/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Geometry.h"
#include "math2.h"
#include "random2.h"
#include "Rn.h"
#include "Sphere.h"
#include "string2.h"
#include "RnSparse.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"


#define FILMAXTRIES 100


/******************************************************************************/
/********************************** Filaments *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// Enumerated types
char *filFD2string(enum FilamentDynamics fd,char *string);
enum FilamentDynamics filstring2FD(const char *string);

// Low level utilities
int filReadFilName(const simptr sim,const char *str,filamenttypeptr *filtypeptr,filamentptr *filptr,char *filname);
int filGetFilIndex(const simptr sim,const char *name,int *ftptr);

// memory management
segmentptr filSegmentAlloc();
void filSegmentFree(segmentptr segment);
filamentworkptr filWorkAlloc(filamentptr fil,int maxseg);
void filWorkFree(filamentworkptr filwork,int maxseg);
filamentptr filAlloc(filamentptr fil,int maxseg,int maxbranch,int maxsequence);
void filFree(filamentptr fil);
filamenttypeptr filamentTypeAlloc(filamenttypeptr filtype,int maxfil,int maxface);
void filamentTypeFree(filamenttypeptr filtype);
filamentssptr filssAlloc(filamentssptr filss,int maxtype);
void filssFree(filamentssptr filss);

// data structure output
void filOutput(const filamentptr fil);
void filtypeOutput(const filamenttypeptr filtype);
void filssOutput(const simptr sim);
void filWrite(const simptr sim,FILE *fptr);
int filCheckParams(const simptr sim,int *warnptr);

// filaments
int filSetNodeMobility(filamentptr fil,int node,double value);

// filament type
int filtypeSetParam(filamenttypeptr filtype,const char *param,int index,double value);
int filtypeSetColor(filamenttypeptr filtype,const double *rgba);
int filtypeSetEdgePts(filamenttypeptr filtype,double value);
int filtypeSetStipple(filamenttypeptr filtype,int factor,int pattern);
int filtypeSetDrawmode(filamenttypeptr filtype,enum DrawMode dm);
int filtypeSetDrawForceArrows(filamenttypeptr filtype,double scale,const double *rgba);
int filtypeSetShiny(filamenttypeptr filtype,double shiny);
int filtypeSetDynamics(filamenttypeptr filtype,enum FilamentDynamics fd);
int filtypeAddFace(filamenttypeptr filtype,const char* facename);
filamenttypeptr filAddFilamentType(simptr sim,const char *ftname);

// filament superstructure
void filSetCondition(filamentssptr filss,enum StructCond cond,int upgrade);
int filEnableFilaments(simptr sim);
int filUpdateParams(simptr sim);
int filUpdateLists(simptr sim);
int filUpdate(simptr sim);

// user input
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr filtype,const char *word,char *line2);
filamentptr filReadString(simptr sim,ParseFilePtr pfp,filamentptr fil,filamenttypeptr filtype,const char *word,char *line2);
int filLoadType(simptr sim,ParseFilePtr *pfpptr,char *line2);
int filLoadFil(simptr sim,ParseFilePtr *pfpptr,char *line2);

// Sequences
int filSetSequence(filamentptr fil,char *str);

// Computations on segments
double filRandomLength(const filamenttypeptr filtype,double thickness,double sigmamult);
double *filRandomAngle(const filamenttypeptr filtype,int n,double thickness,double sigmamult,double *angle);

// Computations on filaments
double *filGetPositionOnFil(const filamentptr fil,double frac,double *pos);
double filStretchEnergy(const filamentptr fil,int seg1,int seg2);
double filBendEnergy(const filamentptr fil,int seg1,int seg2);
void filBendTorque(const filamentptr fil,int seg,double *torque);

// Filament updating
void filNodes2Angles(filamentptr fil,int nodemin,int nodemax);
void filCopyWorkingNodes(filamentptr fil,int from,int to);
void filFlattenNodes(filamentptr fil,int from,int to);
void filUnflattenNodes(filamentptr fil,int from,int to);
void filFlattenForces(filamentptr fil,int to,int nodemin,int nodemax);

// Filament manipulation
void filArrayShift(filamentptr fil,int shift);
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar);
int filAddRandomSegments(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr,const char *phistr, const char* thtstr,const char* psistr,double thickness);
int filAddOneRandomSegment(simptr sim,filamentptr fil,const double *x,double thickness,char endchar,int constraints);
int filTreadmill(simptr sim,filamentptr fil,char endchar);
int filRemoveSegment(filamentptr fil,char endchar);
void filTranslate(filamentptr fil,const double *vect,char func);
void filTranslateNode(filamentptr fil,int node,const double *vect,char func);
int filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func);
int filChangeThickness(filamentptr fil,int seg,double thick,char func);
void filRotateVertex(filamentptr fil,int seg,const double *angle,char endchar,char func);
int filCopyFilament(filamentptr filto,const filamentptr filfrom,const filamenttypeptr filtype);
filamentptr filAddFilament(filamenttypeptr filtype,const char *filname);

// Filament interactions
int filSegmentXSurface(const simptr sim,const segmentptr segment,panelptr *pnlptr);
int filSegmentXFilament(const simptr sim,const segmentptr segment,filamentptr *filptr);

// Force computation
void filAddStretchForces(filamentptr fil,int nodemin,int nodemax);
void filAddStretchForceMat(filamentptr fil);
void filAddThermalForces(filamentptr fil,int nodemin,int nodemax);
void filAddBendForces(filamentptr fil,int nodemin,int nodemax);
void filAddBendForceMat(filamentptr fil);
void filComputeForces(filamentptr fil,int nodemin,int nodemax);
void filComputeDerivForceMat(filamentptr fil,double dtmu);

// Dynamics functions
void filStepDynamics(filamentptr fil,double dtmu,int in,int out);
void filEulerDynamics(simptr sim,filamenttypeptr filtype);
void filRK2Dynamics(simptr sim,filamenttypeptr filtype);
void filRK4Dynamics(simptr sim,filamenttypeptr filtype);
void filEulerMatDynamics(simptr sim,filamenttypeptr filtype);
void filImplicitOldDynamics(simptr sim,filamenttypeptr filtype);
void filImplicitDynamics(simptr sim,filamenttypeptr filtype);
int filDynamics(simptr sim);


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* filFD2string */
char *filFD2string(enum FilamentDynamics fd,char *string) {
	if(fd==FDeuler) strcpy(string,"Euler");
	else if(fd==FDRK2) strcpy(string,"RK2");
	else if(fd==FDRK4) strcpy(string,"RK4");
	else if(fd==FDeulermat) strcpy(string,"EulerMat");
	else if(fd==FDimplicitold) strcpy(string,"ImplicitOld");
	else if(fd==FDimplicit) strcpy(string,"Implicit");
	else strcpy(string,"none");
	return string; }


/* filstring2FD */
enum FilamentDynamics filstring2FD(const char *string) {
	enum FilamentDynamics ans;

	if(strbegin(string,"none",0)) ans=FDnone;
	else if(strbegin(string,"euler",0)) ans=FDeuler;
	else if(strbegin(string,"RK2",0)) ans=FDRK2;
	else if(strbegin(string,"RK4",0)) ans=FDRK4;
	else if(strbegin(string,"eulermat",0)) ans=FDeulermat;
	else if(strbegin(string,"implicitold",0)) ans=FDimplicitold;
	else if(strbegin(string,"implicit",0)) ans=FDimplicit;
	else ans=FDnone;
	return ans; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* filReadFilName */
int filReadFilName(const simptr sim,const char *str,filamenttypeptr *filtypeptr,filamentptr *filptr,char *filname) {
	int itct,ft,f;
	char nm[STRCHAR],*fnm,*colon;
	filamenttypeptr filtype;

	if(!str) return -1;																			// str missing
	if(!sim->filss || !sim->filss->ntype) return -2;				// no filaments

	itct=sscanf(str,"%s",nm);
	if(itct!=1) return -3;																	// can't read str
	colon=strchr(nm,':');

	if(colon) {																							// format is filtype:fil
		fnm=colon+1;
		*colon='\0';
		if(filname) strcpy(filname,fnm);
		ft=stringfind(sim->filss->ftnames,sim->filss->ntype,nm);
		if(ft<0) return -4;																		// filtype not recognized
		filtype=sim->filss->filtypes[ft];
		*filtypeptr=filtype;
		f=stringfind(filtype->filnames,filtype->nfil,fnm);
		if(f<0) return -5;																		// filament not recognized
		*filptr=filtype->fillist[f]; }

	else if(*filtypeptr) {																	// known filtype, looking for fil
		filtype=*filtypeptr;
		fnm=nm;
		if(filname) strcpy(filname,fnm);
		f=stringfind(filtype->filnames,filtype->nfil,fnm);
		if(f<0) return -5;																		// filament not recognized
		*filptr=filtype->fillist[f]; }

	else {																									// unknown filtype
		fnm=nm;
		if(filname) strcpy(filname,fnm);
		for(ft=0;ft<sim->filss->ntype;ft++) {
			filtype=sim->filss->filtypes[ft];
			f=stringfind(filtype->filnames,filtype->nfil,fnm);
			if(f>=0) break; }
		if(f>=0) {
			*filtypeptr=filtype;
			*filptr=filtype->fillist[f]; }
		else
			return -5; }																						// filament not recognized

	return 0; }


/* filGetFilIndex */
int filGetFilIndex(const simptr sim,const char *name,int *ftptr) {
	int f,f1,ft,ft1;
	filamenttypeptr filtype;

	f1=-1;
	ft1=-1;
	for(ft=0;ft<sim->filss->ntype;ft++) {
		filtype=sim->filss->filtypes[ft];
		f=stringfind(filtype->filnames,filtype->nfil,name);
		if(f>=0 && f1>=0) return -2;
		else if(f>=0) {
			f1=f;
			ft1=ft; }}
	if(f1>=0) *ftptr=ft1;
	return f1; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/


/* filSegmentAlloc */
segmentptr filSegmentAlloc() {
	segmentptr segment;

	CHECKMEM(segment=(segmentptr) malloc(sizeof(struct segmentstruct)));
	segment->fil=NULL;
	segment->index=0;
	segment->xyzfront=NULL;
	segment->xyzback=NULL;
	segment->len=0;
	segment->thk=1;
	segment->ypr[0]=segment->ypr[1]=segment->ypr[2]=0;
	Sph_One2Qtn(segment->qrel);
	Sph_One2Qtn(segment->qabs);
	return segment;

 failure:
	return NULL; }


/* filSegmentFree */
void filSegmentFree(segmentptr segment) {
	if(!segment) return;
	free(segment);
	return; }


/* filWorkAlloc */
filamentworkptr filWorkAlloc(filamentptr fil,int maxseg) {
	filamentworkptr filwork;
	int dim,flatsize,seg,i;
	simptr sim;

	if(fil->filwork) filWorkFree(fil->filwork,fil->maxseg);

	sim=fil->filtype->filss->sim;
	dim=sim->dim;
	flatsize=(dim==2) ? 2*(maxseg+1) : 4*(maxseg+1)-1;
	filwork=fil->filwork;

	CHECKMEM(filwork=(filamentworkptr) malloc(sizeof(struct filamentworkstruct)));
	filwork->fil=fil;
	filwork->wnodes=NULL;
	filwork->wroll=NULL;
	filwork->flatnodes=NULL;
	filwork->flatforces=NULL;
	filwork->wseg0up=NULL;
	filwork->forces=NULL;
	filwork->torques=NULL;
	filwork->forcemat=NULL;
	filwork->thermtime=sim->time-sim->dt;
	filwork->thermforce=NULL;

	CHECKMEM(filwork->wnodes=(double***) calloc(3,sizeof(double**)));
	filwork->wnodes[0]=fil->nodes;
	filwork->wnodes[1]=filwork->wnodes[2]=NULL;
	CHECKMEM(filwork->wnodes[1]=(double**) calloc(maxseg+1,sizeof(double*)));
	CHECKMEM(filwork->wnodes[2]=(double**) calloc(maxseg+1,sizeof(double*)));
	CHECKMEM(filwork->thermforce=(double**) calloc(maxseg+1,sizeof(double*)));
	for(seg=0;seg<=maxseg;seg++) {
		CHECKMEM(filwork->wnodes[1][seg]=(double*) calloc(3,sizeof(double)));
		filwork->wnodes[1][seg][0]=filwork->wnodes[1][seg][1]=filwork->wnodes[1][seg][2]=0;
		CHECKMEM(filwork->wnodes[2][seg]=(double*) calloc(3,sizeof(double)));
		filwork->wnodes[2][seg][0]=filwork->wnodes[2][seg][1]=filwork->wnodes[2][seg][2]=0;
		CHECKMEM(filwork->thermforce[seg]=(double*) calloc(3,sizeof(double)));
		filwork->thermforce[seg][0]=filwork->thermforce[seg][1]=filwork->thermforce[seg][2]=0; }

	CHECKMEM(filwork->wroll=(double**) calloc(3,sizeof(double*)));
	filwork->wroll[0]=fil->roll;
	filwork->wroll[1]=filwork->wroll[2]=NULL;
	CHECKMEM(filwork->wroll[1]=(double*) calloc(maxseg,sizeof(double)));
	CHECKMEM(filwork->wroll[2]=(double*) calloc(maxseg,sizeof(double)));
	for(seg=0;seg<maxseg;seg++)
		filwork->wroll[1][seg]=filwork->wroll[2][seg]=0;

	CHECKMEM(filwork->flatnodes=(double**) calloc(3,sizeof(double*)));
	filwork->flatnodes[0]=filwork->flatnodes[1]=filwork->flatnodes[2]=NULL;
	CHECKMEM(filwork->flatnodes[0]=(double*) calloc(flatsize,sizeof(double)));
	CHECKMEM(filwork->flatnodes[1]=(double*) calloc(flatsize,sizeof(double)));
	CHECKMEM(filwork->flatnodes[2]=(double*) calloc(flatsize,sizeof(double)));
	for(i=0;i<flatsize;i++)
		filwork->flatnodes[0][i]=filwork->flatnodes[1][i]=filwork->flatnodes[2][i]=0;

	CHECKMEM(filwork->flatforces=(double**) calloc(2,sizeof(double*)));
	filwork->flatforces[0]=filwork->flatforces[1]=NULL;
	CHECKMEM(filwork->flatforces[0]=(double*) calloc(flatsize,sizeof(double)));
	CHECKMEM(filwork->flatforces[1]=(double*) calloc(flatsize,sizeof(double)));
	for(i=0;i<flatsize;i++)
		filwork->flatforces[0][i]=filwork->flatforces[1][i]=0;

	CHECKMEM(filwork->wseg0up=(double**) calloc(3,sizeof(double*)));
	filwork->wseg0up[0]=fil->seg0up;
	filwork->wseg0up[1]=filwork->wseg0up[2]=NULL;
	CHECKMEM(filwork->wseg0up[1]=(double*) calloc(3,sizeof(double)));
	filwork->wseg0up[1][0]=filwork->wseg0up[1][1]=0;
	filwork->wseg0up[1][2]=1;
	CHECKMEM(filwork->wseg0up[2]=(double*) calloc(3,sizeof(double)));
	filwork->wseg0up[2][0]=filwork->wseg0up[2][1]=0;
	filwork->wseg0up[2][2]=1;

	CHECKMEM(filwork->forces=(double**) calloc(maxseg+1,sizeof(double*)));
	for(seg=0;seg<=maxseg;seg++) {
		CHECKMEM(filwork->forces[seg]=(double*) calloc(3,sizeof(double)));
		filwork->forces[seg][0]=filwork->forces[seg][1]=filwork->forces[seg][2]=0; }

	CHECKMEM(filwork->torques=(double*) calloc(maxseg,sizeof(double)));
	for(seg=0;seg<maxseg;seg++)
		filwork->torques[seg]=0;

	CHECKMEM(filwork->forcemat=sparseAllocBandM(filwork->forcemat,flatsize,dim==2?5:11,0));

	fil->filwork=filwork;

	return filwork;

 failure:
	return NULL; }


/* filWorkFree */
void filWorkFree(filamentworkptr filwork,int maxseg) {
	int seg;

	if(!filwork) return;

	if(filwork->wnodes && filwork->wnodes[1] && filwork->wnodes[2]) {
		for(seg=0;seg<=maxseg;seg++) {			// filwork doesn't own wnodes[0]
			free(filwork->wnodes[1][seg]);
			free(filwork->wnodes[2][seg]); }
		free(filwork->wnodes[1]);
		free(filwork->wnodes[2]);
		free(filwork->wnodes); }

	if(filwork->wroll) {
		free(filwork->wroll[1]);
		free(filwork->wroll[2]);
		free(filwork->wroll); }

	if(filwork->flatnodes) {
		free(filwork->flatnodes[0]);
		free(filwork->flatnodes[1]);
		free(filwork->flatnodes[2]);
		free(filwork->flatnodes); }

	if(filwork->flatforces) {
		free(filwork->flatforces[0]);
		free(filwork->flatforces[1]);
		free(filwork->flatforces); }

	if(filwork->wseg0up) {
		free(filwork->wseg0up[1]);
		free(filwork->wseg0up[2]);
		free(filwork->wseg0up); }

	if(filwork->forces) {
		for(seg=0;seg<=maxseg;seg++)
			free(filwork->forces[seg]);
		free(filwork->forces); }

	free(filwork->torques);
	free(filwork->thermforce);
	sparseFreeM(filwork->forcemat);
	free(filwork);
	return; }


/* filAlloc */
filamentptr filAlloc(filamentptr fil,int maxseg,int maxbranch,int maxsequence) {
	int seg,br,seqi;
	segmentptr *newsegments;
	int *newbranchspots;
	filamentptr *newbranches;
	char *newsequence;
	double **newnodes,**newnodesx,*newroll,*newnodemobility;

	if(!fil) {
		CHECKMEM(fil=(filamentptr) malloc(sizeof(struct filamentstruct)));
		fil->filtype=NULL;
		fil->filname=NULL;
		fil->maxseg=0;
		fil->nseg=0;
		fil->segments=NULL;
		fil->nodes=NULL;
		fil->nodesx=NULL;
		fil->roll=NULL;
		fil->nodemobility=NULL;
		fil->seg0up[0]=fil->seg0up[1]=0;
		fil->seg0up[2]=1;
		fil->frontend=NULL;
		fil->backend=NULL;
		fil->maxbranch=0;
		fil->nbranch=0;
		fil->branchspots=NULL;
		fil->branches=NULL;
		fil->maxsequence=0;
		fil->nsequence=0;
		fil->sequence=NULL; }

	if(maxseg>fil->maxseg) {
		CHECKMEM(newsegments=(segmentptr*) calloc(maxseg,sizeof(struct segmentstruct)));
		CHECKMEM(newnodes=(double**) calloc(maxseg+1,sizeof(double*)));
		CHECKMEM(newnodesx=(double**) calloc(maxseg+1,sizeof(double*)));
		CHECKMEM(newroll=(double*) calloc(maxseg,sizeof(double)));
		CHECKMEM(newnodemobility=(double*) calloc(maxseg+1,sizeof(double)));

		for(seg=0;seg<=maxseg;seg++) newnodes[seg]=NULL;		// nodes
		seg=0;
		if(fil->maxseg>0) {
			for(seg=0;seg<=fil->maxseg;seg++) {
				newnodes[seg]=fil->nodes[seg];
				newnodesx[seg]=fil->nodesx[seg];
				newnodemobility[seg]=fil->nodemobility[seg]; }}
		for(;seg<=maxseg;seg++) {
			CHECKMEM(newnodes[seg]=calloc(3,sizeof(double)));
			CHECKMEM(newnodesx[seg]=calloc(3,sizeof(double)));
			newnodes[seg][0]=newnodes[seg][1]=newnodes[seg][2]=0;
			newnodemobility[seg]=1; }

		for(seg=0;seg<maxseg;seg++) {													// segments and torques
			newsegments[seg]=NULL;
			newroll[seg]=0; }
		for(seg=0;seg<fil->maxseg;seg++) {
			newsegments[seg]=fil->segments[seg];
			newroll[seg]=fil->roll[seg]; }
		for(;seg<maxseg;seg++) {
			CHECKMEM(newsegments[seg]=filSegmentAlloc());
			newsegments[seg]->fil=fil;
			newsegments[seg]->index=seg;
			newsegments[seg]->xyzfront=newnodes[seg];
			newsegments[seg]->xyzback=newnodes[seg+1];
			newroll[seg]=0; }

		free(fil->segments);
		free(fil->nodes);
		free(fil->nodesx);
		free(fil->roll);
		free(fil->nodemobility);
		fil->segments=newsegments;
		fil->nodes=newnodes;
		fil->nodesx=newnodesx;
		fil->roll=newroll;
		fil->nodemobility=newnodemobility;
		if(fil->filtype && fil->filtype->dynamics!=FDnone)
			filWorkAlloc(fil,maxseg);
		fil->maxseg=maxseg; }

	if(maxbranch>fil->maxbranch) {
		CHECKMEM(newbranchspots=(int*) calloc(maxbranch,sizeof(int)));
		CHECKMEM(newbranches=(filamentptr*) calloc(maxbranch,sizeof(filamentptr)));
		for(br=0;br<fil->maxbranch;br++) {
			newbranchspots[br]=fil->branchspots[br];
			newbranches[br]=fil->branches[br]; }
		for(;br<maxbranch;br++) {
			newbranchspots[br]=0;
			newbranches[br]=NULL; }
		free(fil->branches);
		fil->branches=newbranches;
		fil->maxbranch=maxbranch;	}

	if(maxsequence>fil->maxsequence) {
		CHECKMEM(newsequence=(char*) calloc(maxsequence,sizeof(char)));
		for(seqi=0;seqi<fil->maxsequence;seqi++)
			newsequence[seqi]=fil->sequence[seqi];
		for(;seqi<maxsequence;seqi++)
			newsequence[seqi]='\0';
		free(fil->sequence);
		fil->sequence=newsequence;
		fil->maxsequence=maxsequence; }

	return fil;

failure:		// no memory is freed, which causes leaks, but otherwise risk segfault
	return NULL; }


/* filFree */
void filFree(filamentptr fil) {
	int seg;

	if(!fil) return;
	if(fil->segments) {
		for(seg=0;seg<fil->maxseg;seg++)
			filSegmentFree(fil->segments[seg]);
		free(fil->segments); }
	if(fil->nodes) {
		for(seg=0;seg<=fil->maxseg;seg++)
			free(fil->nodes[seg]);
		free(fil->nodes); }
	if(fil->nodesx) {
		for(seg=0;seg<=fil->maxseg;seg++)
			free(fil->nodesx[seg]);
		free(fil->nodesx); }

	free(fil->roll);
	free(fil->nodemobility);
	free(fil->branchspots);
	free(fil->branches);
	free(fil->sequence);
	free(fil);
	return; }


/* filamentTypeAlloc */
filamenttypeptr filamentTypeAlloc(filamenttypeptr filtype,int maxfil,int maxface) {
	int f,fc;
	filamentptr *newfillist;
	char **newfacename,**newfilnames;

	if(!filtype) {
		CHECKMEM(filtype=(filamenttypeptr) malloc(sizeof(struct filamenttypestruct)));
		filtype->filss=NULL;
		filtype->ftname=NULL;
		filtype->dynamics=FDnone;
		filtype->bundlevalue=1;

		filtype->color[0]=filtype->color[1]=filtype->color[2]=0;
		filtype->color[3]=1;
		filtype->edgepts=1;
		filtype->edgestipple[0]=1;
		filtype->edgestipple[1]=0xFFFF;
		filtype->drawmode=DMedge;
		filtype->shiny=0;
		filtype->drawforcescale=0;
		filtype->drawforcecolor[0]=filtype->drawforcecolor[0]=filtype->drawforcecolor[0]=0;
		filtype->drawforcecolor[3]=1;

		filtype->stdlen=1;
		filtype->stdypr[0]=filtype->stdypr[1]=filtype->stdypr[2]=0;
		filtype->klen=1;
		filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=1;
		filtype->kT=1;
		filtype->treadrate=0;
		filtype->mobility=1;
		filtype->filradius=1;

		filtype->maxface=0;
		filtype->nface=0;
		filtype->facename=NULL;
		filtype->facetwist=0;

		filtype->maxfil=0;
		filtype->nfil=0;
		filtype->autonamenum=0;
		filtype->fillist=NULL;
		filtype->filnames=NULL; }

	if(maxfil>filtype->maxfil) {
		CHECKMEM(newfillist=(filamentptr*) calloc(maxfil,sizeof(filamentptr)));
		CHECKMEM(newfilnames=(char**) calloc(maxfil,sizeof(char*)));
		for(f=0;f<maxfil;f++) {
			newfillist[f]=NULL;
			newfilnames[f]=NULL; }
		for(f=0;f<filtype->maxfil;f++) {
			newfillist[f]=filtype->fillist[f];
			newfilnames[f]=filtype->filnames[f]; }
		for(;f<maxfil;f++) {
			CHECKMEM(newfillist[f]=filAlloc(NULL,0,0,0));
			CHECKMEM(newfilnames[f]=EmptyString());
			newfillist[f]->filtype=filtype;
			newfillist[f]->filname=newfilnames[f]; }
		free(filtype->fillist);
		free(filtype->filnames);
		filtype->fillist=newfillist;
		filtype->filnames=newfilnames;
		filtype->maxfil=maxfil; }

	if(maxface>filtype->maxface) {
		CHECKMEM(newfacename=(char**) calloc(maxface,sizeof(char *)));
		for(fc=0;fc<maxface;fc++)
			newfacename[fc]=NULL;
		for(fc=0;fc<filtype->maxface;fc++)
			newfacename[fc]=filtype->facename[fc];
		for(;fc<maxface;fc++)
			CHECKMEM(newfacename[fc]=EmptyString());
		filtype->facename=newfacename;
		filtype->maxface=maxface; }

	return filtype;

 failure:
	return NULL; }


/* filamentTypeFree */
void filamentTypeFree(filamenttypeptr filtype) {
	if(!filtype) return;
	int f,fc;

	if(filtype->fillist && filtype->filnames) {
		for(f=0;f<filtype->maxfil;f++) {
			filFree(filtype->fillist[f]);
			free(filtype->filnames[f]); }
		free(filtype->fillist);
		free(filtype->filnames); }

	if(filtype->facename) {
		for(fc=0;fc<filtype->maxface;fc++)
			free(filtype->facename[fc]);
		free(filtype->facename); }

	free(filtype);
	return; }


/* filssAlloc */
filamentssptr filssAlloc(filamentssptr filss,int maxtype) {
	char **newftnames;
	filamenttypeptr *newfiltypes;
	int ft;

	if(!filss) {
		CHECKMEM(filss=(filamentssptr) malloc(sizeof(struct filamentsuperstruct)));
		filss->condition=SCinit;
		filss->sim=NULL;
		filss->maxtype=0;
		filss->ntype=0;
		filss->ftnames=NULL;
		filss->filtypes=NULL; }

	if(maxtype>=filss->maxtype) {
		CHECKMEM(newfiltypes=(filamenttypeptr*) calloc(maxtype,sizeof(filamenttypeptr)));
		CHECKMEM(newftnames=(char**) calloc(maxtype,sizeof(char*)));
		for(ft=0;ft<maxtype;ft++) {
			newfiltypes[ft]=NULL;
			newftnames[ft]=NULL; }
		for(ft=0;ft<filss->maxtype;ft++) {
			newfiltypes[ft]=filss->filtypes[ft];
			newftnames[ft]=filss->ftnames[ft]; }
		for(;ft<maxtype;ft++) {
			CHECKMEM(newfiltypes[ft]=filamentTypeAlloc(NULL,0,0));
			CHECKMEM(newftnames[ft]=EmptyString());
			newfiltypes[ft]->filss=filss;
			newfiltypes[ft]->ftname=newftnames[ft]; }

		free(filss->ftnames);
		filss->ftnames=newftnames;
		free(filss->filtypes);
		filss->filtypes=newfiltypes;
		filss->maxtype=maxtype; }

	return filss;

failure:
	return NULL; }


/* filssFree */
void filssFree(filamentssptr filss) {
	int ft;

	if(!filss) return;

	if(filss->filtypes) {
		for(ft=0;ft<filss->maxtype;ft++)
			filamentTypeFree(filss->filtypes[ft]);
		free(filss->filtypes); }

	if(filss->ftnames) {
		for(ft=0;ft<filss->maxtype;ft++)
			free(filss->ftnames[ft]);
		free(filss->ftnames); }

	free(filss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* filOutput */
void filOutput(const filamentptr fil) {
	int seg,br,seqi,dim;
	simptr sim;
	segmentptr segment;

	if(!fil) {
		simLog(NULL,2," NULL filament\n");
		return; }

	if(fil->filtype && fil->filtype->filss) {
		sim=fil->filtype->filss->sim;
		dim=sim->dim; }
	else {
		sim=NULL;
		dim=3; }

	simLog(sim,2,"  Filament: %s\n",fil->filname);
	simLog(sim,1,"   type: %s\n",fil->filtype?fil->filtype->ftname:"None (assuming dim=3)");
	simLog(sim,1,"   allocated segments: %i\n",fil->maxseg);
	simLog(sim,2,"   number of segments: %i\n",fil->nseg);
	if(dim==3)
		simLog(sim,2,"   segment 0 up vector: (%1.3g %1.3g %1.3g)\n",fil->seg0up[0],fil->seg0up[1],fil->seg0up[2]);

	simLog(sim,2,"   segments:\n");
	if(dim==2) {
		for(seg=0;seg<fil->nseg;seg++) {
			segment=fil->segments[seg];
			simLog(sim,seg>5?1:2,"    %i length=%1.3g, thick=%1.3g, front pos.=(%1.3g %1.3g), rel. angle=%1.3g",segment->index,segment->len,segment->thk,segment->xyzfront[0],segment->xyzfront[1],segment->ypr[0]);
			simLog(sim,fil->nodemobility[seg]==1?1:2," front mobility=%g",fil->nodemobility[seg]);
			simLog(sim,1,", back pos.=(%1.3g %1.3g), qrel=(%1.3g %1.3g %1.3g %1.3g), qabs=(%1.3g %1.3g %1.3g %1.3g)",segment->xyzback[0],segment->xyzback[1],segment->qrel[0],segment->qrel[1],segment->qrel[2],segment->qrel[3],segment->qabs[0],segment->qabs[1],segment->qabs[2],segment->qabs[3]);
			simLog(sim,seg>5?1:2,"\n"); }
		segment=fil->segments[fil->nseg-1];
		simLog(sim,2,"     back pos.=(%1.3g %1.3g)",segment->xyzback[0],segment->xyzback[1]);
		simLog(sim,fil->nodemobility[seg]==1?1:2," back mobility=%g",fil->nodemobility[seg]);
		simLog(sim,2,"\n"); }
	else {
		for(seg=0;seg<fil->nseg;seg++) {
			segment=fil->segments[seg];
			simLog(sim,seg>5?1:2,"    %i length=%1.3g, thick=%1.3g, front pos.=(%1.3g %1.3g %1.3g), rel. angle=(%1.3g %1.3g %1.3g)",segment->index,segment->len,segment->thk,segment->xyzfront[0],segment->xyzfront[1],segment->xyzfront[2],segment->ypr[0],segment->ypr[1],segment->ypr[2]);
			simLog(sim,fil->nodemobility[seg]==1?1:2," front mobility=%g",fil->nodemobility[seg]);
			simLog(sim,1,", back pos.=(%1.3g %1.3g %1.3g), qrel=(%1.3g %1.3g %1.3g %1.3g), qabs=(%1.3g %1.3g %1.3g %1.3g)",segment->xyzback[0],segment->xyzback[1],segment->xyzback[2],segment->qrel[0],segment->qrel[1],segment->qrel[2],segment->qrel[3],segment->qabs[0],segment->qabs[1],segment->qabs[2],segment->qabs[3]);
			simLog(sim,seg>5?1:2,"\n"); }
		segment=fil->segments[fil->nseg-1];
		simLog(sim,2,"     back pos.=(%1.3g %1.3g %1.3g)\n",segment->xyzback[0],segment->xyzback[1],segment->xyzback[2]);
		simLog(sim,fil->nodemobility[seg]==1?1:2," back mobility=%g",fil->nodemobility[seg]); }

	if(fil->frontend)
		simLog(sim,2,"   front branched from: %s\n",fil->frontend->filname);
	if(fil->backend)
		simLog(sim,2,"   back branched from: %s\n",fil->backend->filname);

	simLog(sim,1,"   allocated branches: %i\n",fil->maxbranch);
	simLog(sim,fil->nbranch>0?2:1,"   number of branches: %i\n",fil->nbranch);
	for(br=0;br<fil->nbranch;br++)
		simLog(sim,2,"    %s at %i\n",fil->branches[br]->filname,fil->branchspots[br]);

	simLog(sim,1,"   sequence codes: %i of %i allocated,",fil->nsequence,fil->maxsequence);
	if(fil->nsequence) {
		simLog(sim,2,"   sequence code: ");
		for(seqi=0;seqi<fil->nsequence;seqi++)
			simLog(sim,2,"%c",fil->sequence[seqi]);
		simLog(sim,2,"\n"); }

	if(fil->filtype->klen>0)
		simLog(sim,2,"   stretching energy: %g|E\n",filStretchEnergy(fil,-1,-1));
	if(fil->filtype->kypr[0]>0 || fil->filtype->kypr[1]>0 || fil->filtype->kypr[2]>0)
		simLog(sim,2,"   bending energy: %g|E\n",filBendEnergy(fil,-1,-1));

	return; }


/* filtypeOutput */
void filtypeOutput(const filamenttypeptr filtype) {
	char string[STRCHAR];
	int fc,f,dim;
	simptr sim;

	if(!filtype) {
		simLog(NULL,2," NULL filament type\n");
		return; }

	if(filtype->filss) {
		sim=filtype->filss->sim;
		dim=sim->dim; }
	else {
		sim=NULL;
		dim=3; }

	simLog(sim,2," Filament type: %s\n",filtype->ftname);
	simLog(sim,1,"  superstructure: %s\n",filtype->filss?"assigned":"missing (assuming dim=3)");
	simLog(sim,2,"  dynamics: %s\n",filFD2string(filtype->dynamics,string));
	simLog(sim,filtype->bundlevalue!=1?2:1,"  bundle value: %g\n",filtype->bundlevalue);

	simLog(sim,2,"  color: %g %g %g %g\n",filtype->color[0],filtype->color[1],filtype->color[2],filtype->color[3]);
	if(filtype->drawforcescale!=0)
		simLog(sim,2,"  force arrows scale: %g, color: %g %g %g %g\n",filtype->drawforcescale,filtype->drawforcecolor[0],filtype->drawforcecolor[1],filtype->drawforcecolor[2],filtype->drawforcecolor[3]);
	simLog(sim,2,"  edge points: %g, polygon mode: %s\n",filtype->edgepts,surfdm2string(filtype->drawmode,string));
	if(filtype->edgestipple[1]!=0xFFFF) simLog(sim,2,"  edge stippling: %ui %X\n",filtype->edgestipple[0],filtype->edgestipple[1]);
	if(filtype->shiny!=0) simLog(sim,2,"  shininess: %g\n",filtype->shiny);

	simLog(sim,2,"  %s length: %g|L\n",filtype->klen>=0?"standard":"fixed",filtype->stdlen);
	if(filtype->klen>0) simLog(sim,2,"  length force constant: %g|E/L\n",filtype->klen);

	if(dim==2) {
		simLog(sim,2,"  %s angle: %g\n",filtype->kypr[0]>=0?"standard":"fixed",filtype->stdypr[0]);
		simLog(sim,2,"  bending force constant: %g|E\n",filtype->kypr[0]); }
	else {
		simLog(sim,2,"  standard angles: %g, %g, %g\n",filtype->stdypr[0],filtype->stdypr[1],filtype->stdypr[2]);
		simLog(sim,2,"  bending force constants: %g|E, %g|E, %g|E\n",filtype->kypr[0],filtype->kypr[1],filtype->kypr[2]); }

	simLog(sim,2,"  kT: %g|E\n",filtype->kT);
	simLog(sim,filtype->treadrate!=0?2:1,"  treadmilling rate: %g\n",filtype->treadrate);
	simLog(sim,2,"  mobility: %g\n",filtype->mobility);
	simLog(sim,2,"  filament radius: %g\n",filtype->filradius);

	if(filtype->nface>0) {
		simLog(sim,2,"  %i faces with twist of %g:",filtype->nface,filtype->facetwist);
		for(fc=0;fc<filtype->nface;fc++)
			simLog(sim,2," %s,",filtype->facename[fc]);
		simLog(sim,2,"\n"); }

	simLog(sim,2,"  %i filaments:\n",filtype->nfil);
	for(f=0;f<filtype->nfil;f++)
		filOutput(filtype->fillist[f]);

	return; }


/* filssOutput */
void filssOutput(const simptr sim) {
	char string[STRCHAR];
	filamentssptr filss;
	int ft;

	filss=sim->filss;
	if(!filss) return;

	simLog(sim,2,"FILAMENT PARAMETERS\n");
	simLog(sim,filss->condition<SCok?2:1," condition: %s\n",simsc2string(filss->condition,string));
	simLog(sim,1," filament types allocated: %i,",filss->maxtype);
	simLog(sim,2," filament types defined: %i\n",filss->ntype);
	for(ft=0;ft<filss->ntype;ft++)
		filtypeOutput(filss->filtypes[ft]);

	simLog(sim,2,"\n");
	return; }


/* filWrite */
void filWrite(const simptr sim,FILE *fptr) {
	filamentssptr filss;

	filss=sim->filss;
	if(!filss) return;
	fprintf(fptr,"# filament parameters\n");
	fprintf(fptr,"# ERROR: code not written yet\n\n");
	//??TODO: write filWrite
	return; }


/* filCheckParams */
int filCheckParams(const simptr sim,int *warnptr) {
	int error,warn,dim,f,seg,ft;
	filamentssptr filss;
	filamentptr fil;
	filamenttypeptr filtype;
	segmentptr segment;
	char string[STRCHAR];

	error=warn=0;
	dim=sim->dim;
	filss=sim->filss;
	if(!filss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(filss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: filament structure %s\n",simsc2string(filss->condition,string)); }

	if(filss->ntype==0) {warn++;simLog(sim,5,"WARNING: Filament superstructure is defined, but no filament types are defined\n");}
	for(ft=0;ft<filss->ntype;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil;f++) {
			fil=filtype->fillist[f];
			for(seg=0;seg<fil->nseg;seg++) {
				segment=fil->segments[seg];
				if(segment->len<=0) {error++;simLog(sim,9,"ERROR: filament %s segment %i has length %1/3g; it should be >=0",fil->filname,seg,segment->len);}
				if(segment->thk<=0) {error++;simLog(sim,9,"ERROR: filament %s segment %i has thickness %1/3g; it should be >=0",fil->filname,seg,segment->len);}
				if(segment->ypr[0]<-PI || segment->ypr[0]>PI) {warn++;simLog(sim,5,"WARNING: filament %s segment %i yaw bending angle is outside of (-PI,PI)\n",fil->filname,seg);}
				if(dim==3 && (segment->ypr[1]<-PI/2 || segment->ypr[1]>PI/2)) {warn++;simLog(sim,5,"WARNING: filament %s segment %i pitch bending angle is outside of (-PI/2,PI/2)\n",fil->filname,seg);}
				if(dim==3 && (segment->ypr[2]<-PI || segment->ypr[2]>PI)) {warn++;simLog(sim,5,"WARNING: filament %s segment %i roll bending angle is outside of (-PI,PI)\n",fil->filname,seg);}
				if(dim==2 && segment->ypr[1]!=0) {error++;simLog(sim,9,"ERROR: filament %s segment %i pitch bending angle is non-zero in a 2D system\n",fil->filname,seg);}
				if(dim==2 && segment->ypr[2]!=0) {error++;simLog(sim,9,"ERROR: filament %s segment %i roll bending angle is non-zero in a 2D system\n",fil->filname,seg);} }}}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/********************************* filaments *********************************/
/******************************************************************************/


/* filSetNodeMobility */
int filSetNodeMobility(filamentptr fil,int node,double value) {
	int i;

	if(node<0) i=fil->nseg+1+node;
	else i=node;
	if(i>fil->nseg) return 1;
	fil->nodemobility[i]=value;
	return 0; }



/******************************************************************************/
/********************************* filament type ******************************/
/******************************************************************************/


/* filtypeSetParam */
int filtypeSetParam(filamenttypeptr filtype,const char *param,int index,double value) {
	int er;

	er=0;
	if(!strcmp(param,"stdlen")) {
		if(value<0) er=2;
		else filtype->stdlen=value; }

	else if(!strcmp(param,"stdypr")) {
		if(value<-PI || value>PI) er=2;
		else if(index<0)
			filtype->stdypr[0]=filtype->stdypr[1]=filtype->stdypr[2]=value;
		else filtype->stdypr[index]=value; }

	else if(!strcmp(param,"klen")) {
		filtype->klen=value; }

	else if(!strcmp(param,"kypr")) {
		if(index<0) filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=value;
		else filtype->kypr[index]=value; }

	else if(!strcmp(param,"kT")) {
		if(value<0) er=2;
		else filtype->kT=value; }

	else if(!strcmp(param,"treadrate")) {
		filtype->treadrate=value; }

	else if(!strcmp(param,"mobility")) {
		if(value<=0) er=2;
		else filtype->mobility=value; }

	else if(!strcmp(param,"bundle")) {
		if(value<=0) er=2;
		else filtype->bundlevalue=value; }

	else if(!strcmp(param,"radius")) {
		if(value<=0) er=2;
		else filtype->filradius=value; }

	else if(!strcmp(param,"facetwist")) {
		filtype->facetwist=value; }

	return er; }


/* filtypeSetColor */
int filtypeSetColor(filamenttypeptr filtype,const double *rgba) {
	int col;

	for(col=0;col<4;col++)
		if(rgba[col]<0 || rgba[col]>1) return 2;

	for(col=0;col<4;col++) filtype->color[col]=rgba[col];
	return 0; }


/* filtypeSetEdgePts */
int filtypeSetEdgePts(filamenttypeptr filtype,double value) {
	if(value<0) return 2;
	filtype->edgepts=value;
	return 0; }


/* filtypeSetStipple */
int filtypeSetStipple(filamenttypeptr filtype,int factor,int pattern) {
	if(factor>=0) {
		if(factor==0) return 2;
		filtype->edgestipple[0]=(unsigned int) factor; }
	if(pattern>=0) {
		if(pattern>0xFFFF) return 2;
		filtype->edgestipple[1]=(unsigned int) pattern; }
	return 0; }


/* filtypeSetDrawmode */
int filtypeSetDrawmode(filamenttypeptr filtype,enum DrawMode dm) {
	if(dm==DMnone) return 2;
	filtype->drawmode=dm;
	return 0; }


/* filtypeSetDrawForceArrows */
int filtypeSetDrawForceArrows(filamenttypeptr filtype,double scale,const double *rgba) {
	int col;

	if(scale>=0)
		filtype->drawforcescale=scale;
	if(rgba)
		for(col=0;col<4;col++) filtype->drawforcecolor[col]=rgba[col];

	return 0; }


/* filtypeSetShiny */
int filtypeSetShiny(filamenttypeptr filtype,double shiny) {
	if(!filtype) return 1;
	if(shiny<0 || shiny>128) return 2;
	filtype->shiny=shiny;
	return 0; }


/* filtypeSetDynamics */
int filtypeSetDynamics(filamenttypeptr filtype,enum FilamentDynamics fd) {
	filtype->dynamics=fd;
	return 0; }


/* filtypeAddFace */
int filtypeAddFace(filamenttypeptr filtype,const char* facename) {
	if(filtype->nface==filtype->maxface) {
		filtype=filamentTypeAlloc(filtype,0,filtype->maxface*2+1);
		if(!filtype) return -1; }
	strcpy(filtype->facename[filtype->nface++],facename);
	return 0; }


/* filAddFilamentType */
filamenttypeptr filAddFilamentType(simptr sim,const char *ftname) {
	int ft,er;
	filamentssptr filss;
	filamenttypeptr filtype;

	if(!sim->filss) {
		er=filEnableFilaments(sim);
		if(er) return NULL; }
	filss=sim->filss;

	ft=stringfind(filss->ftnames,filss->ntype,ftname);
	if(ft<0) {
		if(filss->ntype==filss->maxtype) {
			filss=filssAlloc(filss,filss->maxtype*2+1);
			if(!filss) return NULL; }
		ft=filss->ntype++;
		strncpy(filss->ftnames[ft],ftname,STRCHAR-1);
		filss->ftnames[ft][STRCHAR-1]='\0';
		filtype=filss->filtypes[ft];
		filSetCondition(filss,SClists,0); }
	else
		filtype=filss->filtypes[ft];

	return filtype; }



/******************************************************************************/
/**************************** filament superstructure *************************/
/******************************************************************************/


/* filSetCondition */
void filSetCondition(filamentssptr filss,enum StructCond cond,int upgrade) {
	if(!filss) return;
	if(upgrade==0 && filss->condition>cond) filss->condition=cond;
	else if(upgrade==1 && filss->condition<cond) filss->condition=cond;
	else if(upgrade==2) filss->condition=cond;
	if(filss->sim && filss->condition<filss->sim->condition) {
		cond=filss->condition;
		simsetcondition(filss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* filEnableFilaments */
int filEnableFilaments(simptr sim) {
	filamentssptr filss;

	if(sim->filss) return 0;

	filss=filssAlloc(sim->filss,0);
	if(!filss) return 1;
	sim->filss=filss;
	filss->sim=sim;
	filSetCondition(sim->filss,SClists,0);
	return 0; }


/* filUpdateParams */
int filUpdateParams(simptr sim) {
	(void) sim;
	return 0; }


/* filUpdateLists */
int filUpdateLists(simptr sim) {
	(void) sim;
	return 0; }


/* filUpdate */
int filUpdate(simptr sim) {
	int er;
	filamentssptr filss;

	filss=sim->filss;
	if(filss) {
		if(filss->condition<=SClists) {
			er=filUpdateLists(sim);
			if(er) return er;
			filSetCondition(filss,SCparams,1); }
		if(filss->condition==SCparams) {
			er=filUpdateParams(sim);
			if(er) return er;
			filSetCondition(filss,SCok,1); }}
	return 0; }


/******************************************************************************/
/********************************** user input ********************************/
/******************************************************************************/


/* filtypeReadString */
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr filtype,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar,dim;

	int itct,er,i1,i2;
	char nm[STRCHARLONG],nm1[STRCHARLONG];
	double fltv1[9],f1;
	enum DrawMode dm;
	enum FilamentDynamics fd;

//	printf("%s %s\n",word,line2);//?? debug
	dim=sim->dim;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(!strcmp(word,"name")) {								// name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading filament type name");
		filtype=filAddFilamentType(sim,nm);
		CHECKS(filtype,"failed to add filament type");
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"dynamics")) {				// dynamics
		CHECKS(filtype,"need to enter filament type name before dynamics");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"dynamics options: none, euler, RK2, RK4");
		fd=filstring2FD(nm1);
//		CHECKS(fd!=FDnone,"dynamics options: none, euler, RK2, RK4");		//?? fix this
		er=filtypeSetDynamics(filtype,fd);
		CHECKS(!er,"BUG: error setting filament dynamics");
		CHECKS(!strnword(line2,2),"unexpected text following dynamics"); }

	else if(!strcmp(word,"color") || !strcmp(word,"colour")) {		// color
		CHECKS(filtype,"need to enter filament type name before color");
		er=graphicsreadcolor(&line2,fltv1);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=filtypeSetColor(filtype,fltv1);
		CHECKS(!er,"BUG: error in filtypeSetColor");
		CHECKS(!line2,"unexpected text following color"); }

	else if(!strcmp(word,"thickness")) {				// thickness
		CHECKS(filtype,"need to enter filament type name before thickness");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"thickness value is missing");
		CHECKS(f1>=0,"thickness value needs to be at least 0");
		er=filtypeSetEdgePts(filtype,f1);
		CHECKS(!er,"BUG: error in filtypeSetEdgePts");
		CHECKS(!strnword(line2,2),"unexpected text following thickness"); }

	else if(!strcmp(word,"stipple")) {					// stipple
		CHECKS(filtype,"need to enter filament type name before stipple");
		itct=strmathsscanf(line2,"%mi %mi",varnames,varvalues,nvar,&i1,&i2);
		CHECKS(itct==2,"stipple format: factor pattern");
		CHECKS(i1>=1,"stipple factor needs to be >=1");
		CHECKS(i2>=0 && i2 <=0xFFFF,"stipple pattern needs to be between 0x00 and 0xFFFF");
		er=filtypeSetStipple(filtype,i1,i2);
		CHECKS(!er,"BUG: error in filtypeSetStipple");
		CHECKS(!strnword(line2,3),"unexpected text following stipple"); }

	else if(!strcmp(word,"polygon")) {					// polygon
		CHECKS(filtype,"need to enter filament type name before polygon");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"polygon format: drawmode");
		dm=surfstring2dm(nm1);
		CHECKS(dm!=DMnone,"in polygon, drawing mode is not recognized");
		er=filtypeSetDrawmode(filtype,dm);
		CHECKS(!er,"BUG: error in filtypeSetDrawmode");
		CHECKS(!strnword(line2,2),"unexpected text following polygon"); }

	else if(!strcmp(word,"shininess")) {				// shininess
		CHECKS(filtype,"need to enter filament type name before shininess");
		itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"shininess format: value");
		CHECKS(f1>=0 && f1<=128,"shininess value needs to be between 0 and 128");
		er=filtypeSetShiny(filtype,f1);
		CHECKS(!er,"BUG: error in filtypeSetShiny");
		CHECKS(!strnword(line2,2),"unexpected text following shininess"); }

	else if(!strcmp(word,"force_arrows")) {			// force_arrows
		CHECKS(filtype,"need to enter filament type name before force_arrows");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"force_arrows format: scale color");
		er=filtypeSetDrawForceArrows(filtype,f1,NULL);
		CHECKS(!er,"BUG: error in filtypeSetDrawForceArrows");
		line2=strnword(line2,2);
		if(line2) {
			er=graphicsreadcolor(&line2,fltv1);
			CHECKS(er!=3,"color values need to be between 0 and 1");
			CHECKS(er!=4,"color name not recognized");
			CHECKS(er!=6,"alpha values need to be between 0 and 1");
			CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
			er=filtypeSetDrawForceArrows(filtype,-1,fltv1);
			CHECKS(!er,"BUG: error in filtypeSetDrawForceArrows"); }
		CHECKS(!line2,"unexpected text following force_arrows"); }

	else if(!strcmp(word,"kT")) {								// kT
		CHECKS(filtype,"need to enter filament type name before kT");
		itct=strmathsscanf(line2,"%mlg|E",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"kT format: value");
		CHECKS(f1>=0,"kT value needs to be >=0");
		filtypeSetParam(filtype,"kT",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following kT"); }

	else if(!strcmp(word,"treadmill_rate")) {		// treadmill_rate
		CHECKS(filtype,"need to enter filament type name before treadmill_rate");
		itct=strmathsscanf(line2,"%mlg|/T",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"treadmill_rate format: value");
		filtypeSetParam(filtype,"treadrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following treadmill_rate"); }

	else if(!strcmp(word,"mobility")) {			// mobility
		CHECKS(filtype,"need to enter filament type name before mobility");
		itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);				//?? Units are incorrect
		CHECKS(itct==1,"mobility format: value");
		CHECKS(f1>0,"mobility value needs to be greater than 0");
		filtypeSetParam(filtype,"mobility",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following mobility"); }

	else if(!strcmp(word,"standard_length")) {	// standard_length
		CHECKS(filtype,"need to enter filament type name before standard_length");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"standard_length format: value");
		CHECKS(f1>=0,"standard_length value needs to be >=0");
		filtypeSetParam(filtype,"stdlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following standard_length"); }

	else if(!strcmp(word,"standard_angle")) {	// standard_angle
		CHECKS(filtype,"need to enter filament type name before standard_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&fltv1[0]);
			CHECKS(itct==1,"standard_angle format: angle");
			fltv1[1]=fltv1[2]=0; }
		else {
			itct=strmathsscanf(line2,"%mlg| %mlg| %mlg|",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKS(itct==3,"standard_angle format: yaw pitch roll"); }
		filtypeSetParam(filtype,"stdypr",0,fltv1[0]);
		filtypeSetParam(filtype,"stdypr",1,fltv1[1]);
		filtypeSetParam(filtype,"stdypr",2,fltv1[2]);
		CHECKS(!strnword(line2,dim==2?2:4),"unexpected text following standard_angle"); }

	else if(!strcmp(word,"force_length")) {		// force_length
		CHECKS(filtype,"need to enter filament type name before force_length");
		itct=strmathsscanf(line2,"%mlg|E/L",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"force_length format: value");
		filtypeSetParam(filtype,"klen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following force_length"); }

	else if(!strcmp(word,"force_angle")) {		// force_angle
		CHECKS(filtype,"need to enter filament type name before force_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|E",varnames,varvalues,nvar,&fltv1[0]);
			CHECKS(itct==1,"force_angle format: value");
			fltv1[1]=fltv1[2]=-1; }
		else {
			itct=strmathsscanf(line2,"%mlg|E %mlg|E %mlg|E",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKS(itct==3,"force_angle format: yaw pitch roll"); }
		filtypeSetParam(filtype,"kypr",0,fltv1[0]);
		filtypeSetParam(filtype,"kypr",1,fltv1[1]);
		filtypeSetParam(filtype,"kypr",2,fltv1[2]);
		CHECKS(!strnword(line2,4),"unexpected text following force_angle"); }

//?? Need to add faces and facetwist

	else {																				// unknown word
		CHECKS(0,"syntax error within filament_type block: statement not recognized"); }

	return filtype;

failure:
	simParseError(sim,pfp);
	return NULL; }


/* filReadString */
filamentptr filReadString(simptr sim,ParseFilePtr pfp,filamentptr fil,filamenttypeptr filtype,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar,dim;

	filamentptr fil2;
	filamenttypeptr filtype2;
	int itct,er,i1,seg;
	char nm[STRCHAR],nm1[STRCHAR],endchar,str1[STRCHAR],str2[STRCHAR],str3[STRCHAR],str4[STRCHAR],str5[STRCHAR],str6[STRCHAR],symbol;
	double fltv1[9],length,angle[3],thick,f1,pos[3];

	dim=sim->dim;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(!strcmp(word,"name")) {
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"correct format: filament_type:filament_name");
		filtype2=filtype;
		fil2=NULL;
		er=filReadFilName(sim,nm,&filtype2,&fil2,nm1);
		CHECKS(er!=-1 && er!=-3,"correct format: filament_type:filament_name");
		CHECKS(er!=-2,"filaments have not been defined yet");
		CHECKS(er!=-4,"filament type not recognized");
		CHECKS(filtype2,"missing filament type. Format: filament_type:filament_name");
		filtype=filtype2;
		if(er==-5) {
			fil=filAddFilament(filtype,nm1);
			CHECKS(fil,"failed to add filament"); }
		else
			fil=fil2;
		CHECKS(!strnword(line2,2),"unexpected text following filament name"); }

	else if(!strcmp(word,"first_segment")) {		// first_segment
		CHECKS(fil,"need to enter filament name before first_segment");
		CHECKS(fil->nseg==0,"filament already has segments in it");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|L %mlg|L %mlg|L %mlg|",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&length,&angle[0]);
			fltv1[2]=angle[1]=angle[2]=0;
			CHECKS(itct==4,"first_segment format: x y length angle [thickness]"); }
		else {
			itct=strmathsscanf(line2,"%mlg|L %mlg|L %mlg|L %mlg|L %mlg| %mlg| %mlg|",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2],&length,&angle[0],&angle[1],&angle[2]);
			CHECKS(itct==7,"first_segment format: x y z length yaw pitch roll [thickness]"); }
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,itct+1);
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			if(dim==2) {CHECKS(itct==1,"first_segment format: x y length angle [thickness]");}
			else {CHECKS(itct==1,"first_segment format: x y z length yaw pitch roll [thickness]");}
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddSegment(fil,fltv1,length,angle,thick,'b');
		CHECKS(er==0,"BUG: error in filAddsegment");
		CHECKS(!line2,"unexpected text following first_segment"); }

	else if(!strcmp(word,"add_segment")) {			// add_segment
		CHECKS(fil,"need to enter filament name before add_segment");
		CHECKS(fil->nseg>0,"use first_segment to enter the first segment");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|L %mlg|",varnames,varvalues,nvar,&length,&angle[0]);
			angle[1]=angle[2]=0;
			CHECKS(itct==2,"add_segment format: length angle0 [thickness [end]]"); }
		else {
			itct=strmathsscanf(line2,"%mlg|L %mlg| %mlg| %mlg|",varnames,varvalues,nvar,&length,&angle[0],&angle[1],&angle[2]);
			CHECKS(itct==4,"add_segment format: length angle0 angle1 angle2 [thickness [end]]"); }
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,itct+1);
		thick=1;
		endchar='b';
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			if(dim==2) {CHECKS(itct==1,"add_segment format: length angle0 [thickness [end]]");}
			else {CHECKS(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [end]]");}
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2);
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				if(dim==2) {CHECKS(itct==1,"add_segment format: length angle0 [thickness [end]]");}
				else {CHECKS(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [end]]");}
				if(nm1[0]=='B' || nm1[0]=='b') endchar='b';
				else if(nm1[0]=='F' || nm1[0]=='f') endchar='f';
				else CHECKS(0,"end needs to be 'back' or 'front'");
				line2=strnword(line2,2); }}
		er=filAddSegment(fil,NULL,length,angle,thick,endchar);
		CHECKS(er==0,"BUG: error in filAddsegment");
		CHECKS(!line2,"unexpected text following add_segment"); }

	else if(!strcmp(word,"remove_segment")) {			// remove_segment
		CHECKS(fil,"need to enter filament name before remove_segment");
		CHECKS(fil->nseg>0,"filament has no segments to remove");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"remove_segment format: end");
		endchar='b';
		if(nm1[0]=='B' || nm1[0]=='b') endchar='b';
		else if(nm1[0]=='F' || nm1[0]=='f') endchar='f';
		else CHECKS(0,"end needs to be 'back' or 'front'");
		er=filRemoveSegment(fil,endchar);
		CHECKS(!er,"BUG: error in filRemovesegment");
		CHECKS(!strnword(line2,2),"unexpected text following remove_segment"); }

	else if(!strcmp(word,"random_segments")) {		// random_segments
		CHECKS(fil,"need to enter filament name before random_segments");
		CHECKS(fil->filtype,"need to enter filament type before random_segments");
		CHECKS(fil->filtype->klen==-1 || fil->filtype->klen>0,"cannot compute random segments because the filament type has length force constant equal to 0");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]");
		CHECKS(i1>0,"number needs to be >0");
		line2=strnword(line2,2);
		sprintf(str1,"%i",0);
		sprintf(str2,"%i",0);
		sprintf(str3,"%i",0);
		sprintf(str4,"%i",0);
		sprintf(str5,"%i",0);
		sprintf(str6,"%i",0);
		if(fil->nseg==0) {
			CHECKS(line2,"missing position and angle information");
			if(dim==2) {
				itct=sscanf(line2,"%s %s %s",str1,str2,str4);
				CHECKS(itct==3,"random_segments format: number [x y phi] [thickness]");
				line2=strnword(line2,4); }
			else {
				itct=sscanf(line2,"%s %s %s %s %s %s",str1,str2,str3,str4,str5,str6);
				CHECKS(itct==6,"random_segments format: number [x y z phi theta psi] [thickness]");
				line2=strnword(line2,7); }}
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			if(dim==2) {CHECKS(itct==1,"random_segments format: number [x y phi] [thickness]");}
			else {CHECKS(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]");}
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddRandomSegments(fil,i1,str1,str2,str3,str4,str5,str6,thick);
		CHECKS(er!=2,"random_segments positions need to be 'u' or value");
		CHECKS(er!=3,"random_segments angles need to be 'u' or value");
		CHECKS(er==0,"BUG: error in filAddRandomsegments");
		CHECKS(!line2,"unexpected text following random_segments"); }

	else if(!strcmp(word,"translate")) {					// translate
		CHECKS(fil,"need to enter filament name before translate");
		if(dim==2) {
			itct=strmathsscanf(line2,"%c %mlg|L %mlg|L",varnames,varvalues,nvar,&symbol,&fltv1[0],&fltv1[1]);
			CHECKS(itct==3,"translate format: symbol x y");
			fltv1[2]=0;
			line2=strnword(line2,4); }
		else {
			itct=strmathsscanf(line2,"%c %mlg|L %mlg|L %mlg|L",varnames,varvalues,nvar,&symbol,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKS(itct==4,"translate format: symbol x y z");
			line2=strnword(line2,5); }
		CHECKS(symbol=='=' || symbol=='+' || symbol=='-',"symbol needs to be '=', '+', or '-'");
		filTranslate(fil,fltv1,symbol);
		CHECKS(!line2,"unexpected text following translate"); }

	else if(!strcmp(word,"modify_segment")) {				// modify_segment
		CHECKS(fil,"need to enter filament name before modify_segment");
		itct=strmathsscanf(line2,"%mi| %s %c",varnames,varvalues,nvar,&seg,nm,&symbol);
		CHECKS(itct==3,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
		CHECKS(seg>=0 && seg<fil->nseg,"segment number is not within filament");
		line2=strnword(line2,4);
		CHECKS(line2,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
		CHECKS(symbol=='=' || symbol=='+' || symbol=='-',"symbol needs to be '=', '+', or '-'");
		if(!strcmp(nm,"length")) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&length);
			CHECKS(itct==1,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
			line2=strnword(line2,2);
			endchar='b';
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
				line2=strnword(line2,2);
				if(nm1[0]=='B' || nm1[0]=='b') endchar='b';
				else if(nm1[0]=='F' || nm1[0]=='f') endchar='f';
				else CHECKS(0,"end needs to be 'back' or 'front'"); }
			er=filLengthenSegment(fil,seg,length,endchar,symbol);
			CHECKS(!er,"Error while trying to change segment length - new length must be positive"); }
		else if(!strcmp(nm,"angle")) {
			angle[0]=angle[1]=angle[2]=0;
			if(dim==2) {
				itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&angle[0]);
				CHECKS(itct==1,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
				line2=strnword(line2,2); }
			else {
				itct=strmathsscanf(line2,"%mlg| %mlg| %mlg|",varnames,varvalues,nvar,&angle[0],&angle[1],&angle[2]);
				CHECKS(itct==3,"insufficient angle values");
				line2=strnword(line2,4); }
			endchar='b';
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
				line2=strnword(line2,2);
				if(nm1[0]=='B' || nm1[0]=='b') endchar='b';
				else if(nm1[0]=='F' || nm1[0]=='f') endchar='f';
				else CHECKS(0,"end needs to be 'back' or 'front'"); }
			filRotateVertex(fil,seg,angle,endchar,symbol); }
		else if(!strcmp(nm,"front_position")) {
			pos[0]=pos[1]=pos[2]=0;
			if(dim==2) {
				itct=strmathsscanf(line2,"%mlg|L $mlg|L",varnames,varvalues,nvar,&pos[0],&pos[1]);
				CHECKS(itct==2,"insufficient position values");
				line2=strnword(line2,3); }
			else {
				itct=strmathsscanf(line2,"%mlg|L %mlg|L %mlg|L",varnames,varvalues,nvar,&pos[0],&pos[1],&pos[2]);
				CHECKS(itct==3,"insufficient position values");
				line2=strnword(line2,4); }
			filTranslateNode(fil,seg,pos,symbol); }
		else if(!strcmp(nm,"thickness")) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			CHECKS(itct==1,"modify_segment format: segment length/angle/thickness +/-/= value [end]");
			line2=strnword(line2,2);
			er=filChangeThickness(fil,seg,thick,symbol);
			CHECKS(!er,"Error while trying to change segment thickness - new value cannot be negative"); }
		else
			CHECKS(0,"modification option needs to be 'length', 'angle', or 'thickness'");
		CHECKS(!line2,"unexpected text following modify_segment"); }

	else if(!strcmp(word,"copy_to")) {							// copy_to
		CHECKS(fil,"need to enter filament name before copy_to");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading filament name");
		filtype2=filtype;
		fil2=NULL;
		er=filReadFilName(sim,nm,&filtype2,&fil2,nm1);
		CHECKS(er!=-1 && er!=-3,"error reading filament name");
		CHECKS(er!=-4,"unknown filament type");
		CHECKS(fil2!=fil,"a filament cannot be copied to itself");
		if(!fil2) {
			fil2=filAddFilament(filtype2,nm1);
			CHECKS(fil2,"failed to add filament"); }
		er=filCopyFilament(fil2,fil,filtype2);
		CHECKS(!strnword(line2,2),"unexpected text following copy_to"); }

	else if(!strcmp(word,"sequence")) {						// sequence
		CHECKS(fil,"need to enter filament name before sequence");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"error reading sequence string");
		er=filSetSequence(fil,str1);
		CHECKS(!er,"error while trying to set sequence string");
		CHECKS(!strnword(line2,2),"unexpected text following sequence"); }

	else if(!strcmp(word,"node_mobility")) {			// node_mobility
		CHECKS(fil,"need to enter filament name before node_mobility");
		CHECKS(fil->nseg>0,"filament has no segments");
		itct=strmathsscanf(line2,"%mi %mlg|",varnames,varvalues,nvar,&i1,&f1);
		CHECKS(itct==2,"node_mobility format: node value");
		CHECKS(i1<=fil->nseg && i1>-fil->nseg,"node number is outside of filment size");
		CHECKS(f1>=0,"mobility value needs to be >=0");
		er=filSetNodeMobility(fil,i1,f1);
		CHECKS(!er,"error while trying to set node mobility");
		CHECKS(!strnword(line2,3),"unexpected text following node_mobility"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within filament block: statement not recognized"); }

	return fil;

failure:
	simParseError(sim,pfp);
	return NULL; }


/* filLoadType */
int filLoadType(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHARLONG];
	int done,pfpcode,firstline2;
	filamenttypeptr filtype;

	pfp=*pfpptr;
	done=0;
	filtype=NULL;
	firstline2=line2?1:0;
	filEnableFilaments(sim);

	while(!done) {
		if(pfp->lctr==0)
			simLog(sim,2," Reading file: '%s'\n",pfp->fname);
		if(firstline2) {
			strcpy(word,"name");
			pfpcode=1;
			firstline2=0; }
		else
			pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);

        *pfpptr=pfp;
        CHECKS(pfpcode!=3,"%s",errstring);

        if(pfpcode==0);																// already taken care of
        else if(pfpcode==2) {													// end reading
            done=1; }
        else if(!strcmp(word,"end_filament_type")) {		// end_filament_type
            CHECKS(!line2,"unexpected text following end_filament_type");
            return 0; }
        else if(!line2) {															// just word
            CHECKS(0,"unknown word or missing parameter"); }
        else {
            filtype=filtypeReadString(sim,pfp,filtype,word,line2);
            CHECK(filtype); }}													// failed but error has already been sent

	CHECKS(0,"end of file encountered before end_filament_type statement");	// end of file

	failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* filLoadFil */
int filLoadFil(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHARLONG];
	int done,pfpcode,firstline2;
	filamentptr fil;
	filamenttypeptr filtype;

	pfp=*pfpptr;
	done=0;
	firstline2=line2?1:0;
	filEnableFilaments(sim);
	fil=NULL;
	filtype=NULL;

	while(!done) {
		if(pfp->lctr==0)
			simLog(sim,2," Reading file: '%s'\n",pfp->fname);
		if(firstline2) {
			strcpy(word,"name");
			pfpcode=1;
			firstline2=0; }
		else
			pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);

		*pfpptr=pfp;
		CHECKS(pfpcode!=3,"%s",errstring);

		if(pfpcode==0);																// already taken care of
		else if(pfpcode==2) {													// end reading
			done=1; }
		else if(!strcmp(word,"end_filament")) {				// end_filament
			CHECKS(!line2,"unexpected text following end_filament");
			return 0; }
		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }
		else {
			fil=filReadString(sim,pfp,fil,filtype,word,line2);
			CHECK(fil); 															// failed but error has already been sent
			filtype=fil->filtype; }}

	CHECKS(0,"end of file encountered before end_filament statement");	// end of file

	failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/*******************************************************************************/
/*********************************** Sequences  *****************************/
/*******************************************************************************/


/* filSetSequence */
int filSetSequence(filamentptr fil,char *str) {
	int len;

	len=strlen(str);
	if(len > fil->maxsequence) {
		fil=filAlloc(fil,0,0,len);
		if(!fil) return 1; }
	strcpy(fil->sequence,str);
	fil->nsequence=len;
	return 0; }


/******************************************************************************/
/*********************** Computations on segments **********************/
/******************************************************************************/

/* filRandomLength */
double filRandomLength(const filamenttypeptr filtype,double thickness,double sigmamult) {
	double lsigma;
	double length;

	if(filtype->klen<0) return filtype->stdlen;
	lsigma=sigmamult*sqrt(filtype->kT/(thickness*filtype->klen));
	length=0;
	while(length<=0)
		length=filtype->stdlen+lsigma*gaussrandD();
	return length; }


/* filRandomAngle */
double *filRandomAngle(const filamenttypeptr filtype,int n,double thickness,double sigmamult,double *angle) {
	int d,dim;
	double stddev;

	dim=filtype->filss->sim->dim;
	if(n>0 && dim==3) {										// 3D not first segment
		for(d=0;d<3;d++){
			if(filtype->kypr[d]<0)
				angle[d]=filtype->stdypr[d];
			else if(filtype->kypr[d]==0) {
				if(d==2) angle[d]=unirandOCD(-PI/2,PI/2);
				else angle[d]=unirandOCD(-PI,PI); }
			else {
				stddev=sigmamult*sqrt(filtype->kT/(thickness*filtype->kypr[d]));
				if(d==1) angle[d]=gaussrandtruncOCD(filtype->stdypr[d],stddev,-PI/2,PI/2);
				else angle[d]=gaussrandtruncOCD(filtype->stdypr[d],stddev,-PI,PI); }}}
	else if(n>0) {												// 2D not first segment
		if(filtype->kypr[0]<0)
			angle[0]=filtype->stdypr[0];
		else if(filtype->kypr[0]==0)
			angle[0]=unirandOCD(-PI,PI);
		else {
			stddev=sigmamult*sqrt(filtype->kT/(thickness*filtype->kypr[0]));
			angle[0]=gaussrandtruncOCD(filtype->stdypr[0],stddev,-PI,PI); }
		angle[1]=angle[2]=0; }
	else if(dim==3) {											// 3D first segment
		angle[0]=thetarandCCD();
		angle[1]=unirandCOD(0,2*PI);
		angle[2]=unirandCOD(0,2*PI);
		Sph_Eax2Ypr(angle,angle); }
	else {																// 2D first segment
		angle[0]=unirandOCD(-PI,PI);
		angle[1]=angle[2]=0; }
	return angle; }


/******************************************************************************/
/*************************** computations on filaments ************************/
/******************************************************************************/


// filGetPositionOnFil
double *filGetPositionOnFil(const filamentptr fil,double frac,double *pos) {
	int seg;
	double path,fpath,newpath,x;
	segmentptr segment;

	path=0;
	for(seg=0;seg<fil->nseg;seg++)					// compute total path length
		path+=fil->segments[seg]->len;
	fpath=frac*path;												// desired distance along path
	newpath=0;
	for(seg=0;seg<fil->nseg && newpath<fpath;seg++)			// first endpoint after desired distance
		newpath+=fil->segments[seg]->len;

	segment=fil->segments[seg>0?seg-1:0];
	x=(newpath-fpath)/segment->len;						// interpolate within segment
	pos[0]=x*segment->xyzfront[0]+(1-x)*segment->xyzback[0];
	pos[1]=x*segment->xyzfront[1]+(1-x)*segment->xyzback[1];
	pos[2]=x*segment->xyzfront[2]+(1-x)*segment->xyzback[2];
	return pos; }


// filStretchEnergy
double filStretchEnergy(const filamentptr fil,int seg1,int seg2) {
	double thk,klen,len,stdlen,energy;
	filamenttypeptr filtype;
	int seg;

	filtype=fil->filtype;
	stdlen=filtype->stdlen;
	klen=filtype->klen;
	if(klen<=0) return 0;

	energy=0;
	if(seg1==-1) seg1=0;
	if(seg2==-1) seg2=fil->nseg;
	for(seg=seg1;seg<seg2;seg++) {
		thk=fil->segments[seg]->thk;
		len=fil->segments[seg]->len;
		energy+=0.5*thk*klen*(len-stdlen)*(len-stdlen); }
	return energy; }


// filBendEnergy
double filBendEnergy(const filamentptr fil,int seg1,int seg2) {
	double *kypr,*stdypr,*ypr,thk,energy;
	filamenttypeptr filtype;
	segmentptr segment,segmentm1;
	int seg;

	filtype=fil->filtype;
	kypr=filtype->kypr;
	stdypr=filtype->stdypr;

	energy=0;
	if(seg1==-1) seg1=0;
	if(seg2==-1) seg2=fil->nseg-1;
	for(seg=seg1+1;seg<=seg2;seg++) {
		segment = fil->segments[seg];
		segmentm1 = fil->segments[seg-1];
		ypr = segment->ypr;
		thk=0.5*(segmentm1->thk+segment->thk);
		if(kypr[0]>0) energy+=0.5*thk*kypr[0]*(ypr[0]-stdypr[0])*(ypr[0]-stdypr[0]);
		if(kypr[1]>0) energy+=0.5*thk*kypr[1]*(ypr[1]-stdypr[1])*(ypr[1]-stdypr[1]);
		if(kypr[2]>0) energy+=0.5*thk*kypr[2]*(ypr[2]-stdypr[2])*(ypr[2]-stdypr[2]); }
	return energy; }


// filBendTorque
void filBendTorque(const filamentptr fil,int node,double *torque) {
	filamenttypeptr filtype;
	double *kypr,*stdypr,*ypr,cphi,sphi,ctht,stht,deltaypr[3],reltorque[3];
	int dim;

	if(node<=0 || node>=fil->nseg) {
		torque[0]=torque[1]=torque[2]=0;
		return; }

	filtype=fil->filtype;
	kypr=filtype->kypr;
	stdypr=filtype->stdypr;
	ypr=fil->segments[node]->ypr;
	dim=filtype->filss->sim->dim;

	if(dim==2) {
		deltaypr[0]=kypr[0]*(ypr[0]-stdypr[0]);
		torque[0]=torque[1]=0;
		torque[2]=-deltaypr[0]; }
	else {
		cphi=cos(ypr[0]);
		sphi=sin(ypr[0]);
		ctht=cos(ypr[1]);
		stht=sin(ypr[1]);

		deltaypr[0]=kypr[0]*(ypr[0]-stdypr[0]);
		deltaypr[1]=kypr[1]*(ypr[1]-stdypr[1]);
		deltaypr[2]=kypr[2]*(ypr[2]-stdypr[2]);

		reltorque[0]=-deltaypr[2]*ctht*cphi+deltaypr[1]*sphi;					// compute torque in segment reference frame
		reltorque[1]=-deltaypr[1]*cphi-deltaypr[2]*sphi;
		reltorque[2]=-deltaypr[0]+deltaypr[2]*cphi*stht;

		Sph_QtniRotate(fil->segments[node-1]->qabs,reltorque,torque); }	// convert torque to system reference frame
	return; }


/************************************************************/
/************ Filament updating ************************/
/************************************************************/


/* filNodes2Angles */
void filNodes2Angles(filamentptr fil,int nodemin,int nodemax) {
	int seg,dim;
	segmentptr segment,segmentm1;
	double x[3],aphim1,aphi,len;

	if(nodemin<0) nodemin=0;				//?? these are not used
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	dim=fil->filtype->filss->sim->dim;
	if(dim==2) {
		if(nodemin==0) aphim1=0; 									// absolute phi for segment i-1
		else aphim1=Sph_Qtn2Yaw(fil->segments[nodemin-1]->qabs);
		for(seg=nodemin;seg<nodemax;seg++) {
			segment=fil->segments[seg];
			x[0]=segment->xyzback[0]-segment->xyzfront[0];
			x[1]=segment->xyzback[1]-segment->xyzfront[1];
			len=sqrt(x[0]*x[0]+x[1]*x[1]);
			segment->len=len;												// segment->len
			aphi=atan2(x[1],x[0]);									// absolute yaw
			segment->ypr[0]=aphi-aphim1;						// segment->ypr
			if(segment->ypr[0]<-PI) segment->ypr[0]+=2*PI;
			else if(segment->ypr[0]>PI) segment->ypr[0]-=2*PI;
			Sph_Yaw2Qtn(segment->ypr[0],segment->qrel);
			Sph_Yaw2Qtn(aphi,segment->qabs);
			aphim1=aphi; }
		if(seg<fil->nseg) {												// set angle at nodemax if not at end
			segment=fil->segments[seg];
			aphi=Sph_Qtn2Yaw(segment->qabs);
			segment->ypr[0]=aphi-aphim1;
			if(segment->ypr[0]<-PI) segment->ypr[0]+=2*PI;
			else if(segment->ypr[0]>PI) segment->ypr[0]-=2*PI;
			Sph_Yaw2Qtn(segment->ypr[0],segment->qrel); }}

	else {																				// 3D
		if(nodemin==0) {
			segment=fil->segments[0];
			x[0]=segment->xyzback[0]-segment->xyzfront[0];			// displacement of segment
			x[1]=segment->xyzback[1]-segment->xyzfront[1];
			x[2]=segment->xyzback[2]-segment->xyzfront[2];
			len=sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
			segment->len=len;
			Sph_XZ2Qtni(x,fil->seg0up,segment->qrel);
			Sph_Qtn2Qtn(segment->qrel,segment->qabs);
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);
			seg=1; }
		else {
			seg=nodemin; }
		for(;seg<nodemax;seg++) {
			segment=fil->segments[seg];
			x[0]=segment->xyzback[0]-segment->xyzfront[0];			// displacement of segment
			x[1]=segment->xyzback[1]-segment->xyzfront[1];
			x[2]=segment->xyzback[2]-segment->xyzfront[2];
			len=sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
			segment->len=len;
			Sph_QtnRotate(fil->segments[seg-1]->qabs,x,x);			// rotate x into prior segment reference frame
			segment->ypr[0]=atan2(x[1],x[0]);										// local yaw. Always in [-pi,pi].
			segment->ypr[1]=-asin(x[2]/len);										// local pitch. Always in [-pi/2,pi/2]
			Sph_Ypr2Qtn(segment->ypr,segment->qrel);
			Sph_QtnxQtn(fil->segments[seg-1]->qabs,segment->qrel,segment->qabs); }			// b_i = b_{i-1} . a_i
		if(seg<fil->nseg) {
			segmentm1=segment;
			segment=fil->segments[seg];
			Sph_QtnixQtn(segmentm1->qabs,segment->qabs,segment->qrel);	// combine prior and this qabs to get qrel
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);						// use qrel to get ypr
			}}
	return; }


/* filCopyRollToYpr2 */
void filCopyRollToYpr2(filamentptr fil) {
	int seg;

	for(seg=0;seg<fil->nseg;seg++)
		fil->roll[seg]=fil->segments[seg]->ypr[2];
	return;	}


/* filCopyYpr2ToRoll */
void filCopyYpr2ToRoll(filamentptr fil) {
	int seg;

	for(seg=0;seg<fil->nseg;seg++)
		fil->segments[seg]->ypr[2]=fil->roll[seg];
	return;	}


/* filCopyWorkingNodes */
void filCopyWorkingNodes(filamentptr fil,int from,int to) {
	int node,seg,dim;
	filamentworkptr filwork;
	double **nodesfrom,**nodesto,*seg0upfrom,*seg0upto,*rollfrom,*rollto;

	filwork=fil->filwork;
	dim=fil->filtype->filss->sim->dim;

	nodesfrom=filwork->wnodes[from];
	nodesto=filwork->wnodes[to];

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			nodesto[node][0]=nodesfrom[node][0];
			nodesto[node][1]=nodesfrom[node][1]; }}
	else {
		if(from==0) filCopyYpr2ToRoll(fil);

		seg0upfrom=filwork->wseg0up[from];
		seg0upto=filwork->wseg0up[to];
		rollfrom=filwork->wroll[from];
		rollto=filwork->wroll[to];

		seg0upto[0]=seg0upfrom[0];
		seg0upto[1]=seg0upfrom[1];
		seg0upto[2]=seg0upfrom[2];
		for(seg=0;seg<fil->nseg;seg++)
			rollto[seg]=rollfrom[seg];
		for(node=0;node<=fil->nseg;node++) {
			nodesto[node][0]=nodesfrom[node][0];
			nodesto[node][1]=nodesfrom[node][1];
			nodesto[node][2]=nodesfrom[node][2]; }}
	return; }


/* filFlattenNodes */
void filFlattenNodes(filamentptr fil,int from,int to) {
	int node,dim;
	double **nodes,*flatnodes,*roll;
	filamentworkptr filwork;

	filwork=fil->filwork;
	nodes=filwork->wnodes[from];
	flatnodes=filwork->flatnodes[to];
	dim=fil->filtype->filss->sim->dim;

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			flatnodes[2*node+0]=nodes[node][0];
			flatnodes[2*node+1]=nodes[node][1]; }}
	else {
		if(from==0) filCopyYpr2ToRoll(fil);

		roll=filwork->wroll[from];
		for(node=0;node<fil->nseg;node++) {
			flatnodes[4*node+0]=nodes[node][0];
			flatnodes[4*node+1]=nodes[node][1];
			flatnodes[4*node+2]=nodes[node][2];
			flatnodes[4*node+3]=roll[node]; }
		flatnodes[4*node+0]=nodes[node][0];
		flatnodes[4*node+1]=nodes[node][1];
		flatnodes[4*node+2]=nodes[node][2]; }
	return; }


/* filUnflattenNodes */
void filUnflattenNodes(filamentptr fil,int from,int to) {
	int node,dim;
	double **nodes,*flatnodes,*roll,axis[3],*seg0upin,*seg0upout;
	filamentworkptr filwork;

	filwork=fil->filwork;
	nodes=filwork->wnodes[to];
	flatnodes=filwork->flatnodes[from];
	dim=fil->filtype->filss->sim->dim;

	if(dim==2)
		for(node=0;node<=fil->nseg;node++) {
			nodes[node][0]=flatnodes[2*node];
			nodes[node][1]=flatnodes[2*node+1]; }
	else {
		roll=filwork->wroll[to];
		for(node=0;node<fil->nseg;node++) {
			nodes[node][0]=flatnodes[4*node+0];
			nodes[node][1]=flatnodes[4*node+1];
			nodes[node][2]=flatnodes[4*node+2];
			roll[node]=flatnodes[4*node+3]; }
		nodes[node][0]=flatnodes[4*node+0];
		nodes[node][1]=flatnodes[4*node+1];
		nodes[node][2]=flatnodes[4*node+2];

		if(to==0) filCopyRollToYpr2(fil);

		seg0upin=filwork->wseg0up[from];
		seg0upout=filwork->wseg0up[to];
		axis[0]=nodes[1][0]-nodes[0][0];					// process segment 0 torque
		axis[1]=nodes[1][1]-nodes[0][1];
		axis[2]=nodes[1][2]-nodes[0][2];
		Sph_RotateVectorAxisAngle(seg0upin,axis,roll[0],seg0upout); }

	return; }


/* filFlattenForces */
void filFlattenForces(filamentptr fil,int to,int nodemin,int nodemax) {
	int node,dim;
	double **forces,*flatforces,*torques;
	filamentworkptr filwork;

	filwork=fil->filwork;
	forces=filwork->forces;
	flatforces=filwork->flatforces[to];
	dim=fil->filtype->filss->sim->dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	if(dim==2) {
		for(node=nodemin;node<=nodemax;node++) {
			flatforces[2*node+0]=forces[node][0];
			flatforces[2*node+1]=forces[node][1]; }}
	else {
		torques=filwork->torques;
		for(node=nodemin;node<nodemax;node++) {
			flatforces[4*node+0]=forces[node][0];
			flatforces[4*node+1]=forces[node][1];
			flatforces[4*node+2]=forces[node][2];
			flatforces[4*node+3]=torques[node]; }
		flatforces[4*node+0]=forces[node][0];
		flatforces[4*node+1]=forces[node][1];
		flatforces[4*node+2]=forces[node][2];
		if(nodemax<fil->nseg)
			flatforces[4*node+3]=torques[node]; }
	return; }



/******************************************************************************/
/*************************** Filament manipulation *************************/
/******************************************************************************/


/* filArrayShift */
void filArrayShift(filamentptr fil,int shift) {
	int i;
	segmentptr newsegment;
	double *newnode;

	if(shift>0) {
		if(fil->nseg+shift>fil->maxseg)
			simLog(fil->filtype->filss->sim,10,"BUG in filArrayShift. Memory overwrite.\n");
		for(i=fil->nseg+shift-1;i>=shift;i--) {			// i is new index
			newsegment = fil->segments[i];
			fil->segments[i]=fil->segments[i-shift];
			fil->segments[i-shift] = newsegment; }
		for(i=fil->nseg+shift;i>=shift;i--) {
			newnode = fil->nodes[i];
			fil->nodes[i]=fil->nodes[i-shift];
			fil->nodes[i-shift]=newnode; }
		for(i=0;i<shift;i++) {
			fil->segments[i]->xyzfront=fil->nodes[i];
			fil->segments[i]->xyzback=fil->nodes[i+1]; }}

	else if(shift<0) {
		shift=-shift;								// now shift is positive
		for(i=0;i<fil->nseg-shift;i++) {					// i is new index
			newsegment = fil->segments[i];
			fil->segments[i]=fil->segments[i+shift];
			fil->segments[i+shift] = newsegment; }
		for(i=0;i<=fil->nseg-shift;i++) {
			newnode = fil->nodes[i];
			fil->nodes[i]=fil->nodes[i+shift];
			fil->nodes[i+shift]=newnode; }
		for(i=fil->nseg-shift;i<fil->nseg;i++) {
			fil->segments[i]->xyzfront=fil->nodes[i];
			fil->segments[i]->xyzback=fil->nodes[i+1]; }}

	for(i=0;i<fil->nseg;i++)
		fil->segments[i]->index=i;

	return; }


/* filAddSegment */
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar) {
	int seg;
	segmentptr segment,segmentm1,segmentp1;
	double zero[3]={0,0,0};

	if(fil->nseg==fil->maxseg) {
		fil=filAlloc(fil,fil->maxseg*2+1,0,0);
		if(!fil) return 1; }		// out of memory

	if(endchar=='b') {
		seg=fil->nseg;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		Sph_Ypr2Ypr(angle,segment->ypr);												// ypr = angle
		Sph_Ypr2Qtn(angle,segment->qrel);												// a = Qtn(angle)
		if(seg==0) {
			segment->xyzfront[0]=x[0];														// x_i = input value
			segment->xyzfront[1]=x[1];
			segment->xyzfront[2]=x[2];
			Sph_Qtn2Qtn(segment->qrel,segment->qabs); 						// b = a
			Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }		// rotate segment z to lab frame
		else {
			segmentm1=fil->segments[seg-1];
			Sph_QtnxQtn(segmentm1->qabs,segment->qrel,segment->qabs);	}	// b_i = b_{i-1} . a_i
		Sph_QtniRotateUnitx(segment->qabs,segment->xyzback,segment->xyzfront,segment->len);			// x_{i+1} = x_i + l_i * b.xhat.bT
		fil->nseg++; }

	else {
		filArrayShift(fil,1);
		seg=0;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		if(fil->nseg==0) {
			Sph_Ypr2Qtni(angle,segment->qabs);									// b_0
			segment->xyzback[0]=x[0];														// back of segment = input value
			segment->xyzback[1]=x[1];
			segment->xyzback[2]=x[2]; }
		else {
			segmentp1=fil->segments[1];
			Sph_Ypr2Ypr(angle,segmentp1->ypr);
			Sph_Ypr2Qtn(angle,segmentp1->qrel);
			Sph_QtnxQtni(segmentp1->qabs,segmentp1->qrel,segment->qabs); }		// b_i = b_{i+1} . a_{i+1}*
		Sph_Qtn2Qtn(segment->qabs,segment->qrel);									// a_i = b_i
		Sph_Qtn2Ypr(segment->qrel,segment->ypr);									// a_0 = Ypr(B_0)
		Sph_QtniRotateUnitx(segment->qabs,segment->xyzfront,segment->xyzback,-segment->len);
		Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1);
		fil->nseg++; }
	return 0; }


/* filAddRandomSegments */
int filAddRandomSegments(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr,const char *phistr, const char* thtstr,const char* psistr,double thickness) {
	int i,dim,er;
	double f1,pos[3],angle[3],len;
	char **varnames;
	double *varvalues;
	int nvar;
	filamenttypeptr filtype;
	simptr sim;

	filtype=fil->filtype;
	sim=filtype->filss->sim;
	dim=sim->dim;
	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(fil->nseg==0) {								// new filament
		systemrandpos(sim,pos);
		if(!strcmp(xstr,"u"));
		else if(strmathsscanf(xstr,"%mlg|L",varnames,varvalues,nvar,&f1)==1) pos[0]=f1;
		else return 2;
		if(!strcmp(ystr,"u"));
		else if(strmathsscanf(ystr,"%mlg|L",varnames,varvalues,nvar,&f1)==1) pos[1]=f1;
		else return 2;
		if(dim==2) pos[2]=0;
		else if(!strcmp(zstr,"u"));
		else if(strmathsscanf(zstr,"%mlg|L",varnames,varvalues,nvar,&f1)==1) pos[2]=f1;
		else return 2;

		if(dim==2) {
			angle[0]=unirandOCD(-PI,PI);
			angle[1]=angle[2]=0; }
		else {
			angle[0]=thetarandCCD();
			angle[1]=unirandCOD(0,2*PI);
			angle[2]=unirandCOD(0,2*PI);
			Sph_Eax2Ypr(angle,angle); }
		if(!strcmp(phistr,"u"));
		else if(strmathsscanf(phistr,"%mlg|",varnames,varvalues,nvar,&f1)==1) angle[0]=f1;
		else return 3;
		if(dim==3) {
			if(!strcmp(thtstr,"u"));
			else if(strmathsscanf(thtstr,"%mlg|",varnames,varvalues,nvar,&f1)==1) angle[1]=f1;
			else return 3;
			if(!strcmp(psistr,"u"));
			else if(strmathsscanf(psistr,"%mlg|",varnames,varvalues,nvar,&f1)==1) angle[2]=f1;
			else return 3; }}
	else {
		pos[0]=pos[1]=pos[2]=0;
		angle[0]=angle[1]=angle[2]=0; }

	for(i=0;i<number;i++) {
		len=filRandomLength(filtype,thickness,1);
		if(fil->nseg>0) {
			filRandomAngle(filtype,fil->nseg,thickness,1,angle);
			if(dim==2) angle[1]=angle[2]=0; }
		er=filAddSegment(fil,pos,len,angle,thickness,'b');
		if(er) return er; }

	return 0; }


/* filAddOneRandomSegment */
int filAddOneRandomSegment(simptr sim,filamentptr fil,const double *x,double thickness,char endchar,int constraints) {
	int er,iter,cross,tryagain;
	filamenttypeptr filtype;
	double len,angle[3];
	panelptr pnl;
	segmentptr segment;

	filtype=fil->filtype;

	tryagain=1;
	len=filRandomLength(filtype,thickness,1);
	filRandomAngle(filtype,fil->nseg,thickness,1,angle);
	er=filAddSegment(fil,x,len,angle,thickness,endchar);
	if(er) return er;
	segment=fil->segments[((endchar=='f')?0:fil->nseg-1)];
	if(constraints==0) tryagain=0;
	for(iter=0;iter<FILMAXTRIES && tryagain;iter++) {
		tryagain=0;
		if(constraints & 1) {	// check for surface crossing
			cross=filSegmentXSurface(sim,segment,&pnl);
			if(cross) {
				len=filRandomLength(filtype,thickness,1);
				filRandomAngle(filtype,fil->nseg,thickness,1,angle);
				filLengthenSegment(fil,segment->index,len,endchar,'=');
				filRotateVertex(fil,segment->index,angle,endchar,'=');
				tryagain=1; }}}

	if(tryagain) {
		filRemoveSegment(fil,endchar);
		return 2; }

	return 0; }


/* filTreadmill */
int filTreadmill(simptr sim,filamentptr fil,char endchar) {
	int er;

	er=0;
	if(fil->nseg<1) return 2;
	er=filAddOneRandomSegment(sim,fil,NULL,fil->segments[(endchar=='b')?fil->nseg-1:0]->thk,endchar,1);
	if(!er)
		filRemoveSegment(fil,(endchar=='b')?'f':'b');
	return er; }



/* filRemoveSegment */
int filRemoveSegment(filamentptr fil,char endchar) {
	int seg;
	segmentptr segment;
	double zero[3]={0,0,0};

	if(fil->nseg==0) return -1;

	if(endchar=='b')
		fil->nseg--;
	else {
		seg=1;					// new front segment
		segment=fil->segments[seg];
		Sph_Qtn2Qtn(segment->qabs,segment->qrel);
		Sph_Qtn2Ypr(segment->qrel,segment->ypr);
		Sph_QtniRotateUnitz(segment->qrel,fil->seg0up,zero,1);
		filArrayShift(fil,-1);
		fil->nseg--; }

	return 0; }


/* filTranslate */
void filTranslate(filamentptr fil,const double *vect,char func) {
	int seg;
	double shift[3],*node;
	segmentptr segment;

	if(fil->nseg==0) return;

	if(func=='=') {
		segment=fil->segments[0];
		shift[0]=vect[0]-segment->xyzfront[0];
		shift[1]=vect[1]-segment->xyzfront[1];
		shift[2]=vect[2]-segment->xyzfront[2]; }
	else if(func=='-') {
		shift[0]=-vect[0];
		shift[1]=-vect[1];
		shift[2]=-vect[2]; }
	else {
		shift[0]=vect[0];
		shift[1]=vect[1];
		shift[2]=vect[2]; }

	for(seg=0;seg<=fil->nseg;seg++) {
		node=fil->nodes[seg];
		node[0]+=shift[0];
		node[1]+=shift[1];
		node[2]+=shift[2]; }

	return; }


/* filTranslateNode */
void filTranslateNode(filamentptr fil,int node,const double *vect,char func) {
	double shift[3];

	if(func=='=') {
		shift[0]=vect[0]-fil->nodes[node][0];
		shift[1]=vect[1]-fil->nodes[node][1];
		shift[2]=vect[2]-fil->nodes[node][2]; }
	else if(func=='-') {
		shift[0]=-vect[0];
		shift[1]=-vect[1];
		shift[2]=-vect[2]; }
	else {
		shift[0]=vect[0];
		shift[1]=vect[1];
		shift[2]=vect[2]; }

	fil->nodes[node][0]+=shift[0];
	fil->nodes[node][1]+=shift[1];
	fil->nodes[node][2]+=shift[2];

	filNodes2Angles(fil,node-1,node+1);

	return; }


/* filLengthenSegment */
int filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func) {
	int i;
	double lenold,lendelta,xdelta[3],**nodes,zero[3]={0,0,0};
	segmentptr segment;

	segment=fil->segments[seg];
	lenold=segment->len;
	if(func=='=') lendelta=length-lenold;
	else if(func=='+') lendelta=length;
	else lendelta=-length;

	if(lenold+lendelta<=0) return 1;

	Sph_QtniRotateUnitx(segment->qabs,xdelta,zero,lendelta);			// rotate to lab frame. Delta Delta x = Delta l * b_i xhat b_iT
	nodes=fil->nodes;
	if(endchar=='b') {
		for(i=seg+1;i<=fil->nseg;i++) {
			nodes[i][0]+=xdelta[0];
			nodes[i][1]+=xdelta[1];
			nodes[i][2]+=xdelta[2]; }}
	else {
		for(i=seg;i>=0;i--) {
			nodes[i][0]-=xdelta[0];
			nodes[i][1]-=xdelta[1];
			nodes[i][2]-=xdelta[2]; }}

	return 0; }


int filChangeThickness(filamentptr fil,int seg,double thick,char func) {
	segmentptr segment;
	double thkold,thkdelta;

	segment=fil->segments[seg];
	thkold=segment->thk;
	if(func=='=') thkdelta=thick-thkold;
	else if(func=='+') thkdelta=thick;
	else thkdelta=-thick;

	if(thkold+thkdelta<0) return 1;
	segment->thk+=thkdelta;

	return 0; }


/* filRotateVertex */
void filRotateVertex(filamentptr fil,int seg,const double *angle,char endchar,char func) {
	int i;
	double qtndelta[4],zero[3]={0,0,0};
	segmentptr segment,segmentm1,segmentp1;

	segment=fil->segments[seg];
	Sph_Ypr2Qtn(angle,qtndelta);
	if(func=='=') {
		Sph_Qtn2Qtn(qtndelta,segment->qrel); }
	else if(func=='+') {
		Sph_QtnxQtn(segment->qrel,qtndelta,segment->qrel); }
	else {
		Sph_QtnxQtni(segment->qrel,qtndelta,segment->qrel); }

	if(endchar=='b') {
		for(i=seg;i<fil->nseg;i++) {
			segment=fil->segments[i];
			if(i==0) {
				Sph_Qtn2Qtn(segment->qrel,segment->qabs); 						// b_0 = a_0
				Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }
			else {
				segmentm1=fil->segments[i-1];
				Sph_QtnxQtn(segmentm1->qabs,segment->qrel,segment->qabs); }			// b_i = b_{i-1} a_i
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);
			Sph_QtniRotateUnitx(segment->qabs,segment->xyzback,segment->xyzfront,segment->len);
			}}
	else {
		for(i=seg;i>=0;i--) {
			segment=fil->segments[i];
			if(i==fil->nseg-1);
			else {
				segmentp1=fil->segments[i+1];
				Sph_QtnxQtni(segmentp1->qabs,segmentp1->qrel,segment->qabs); }		// b_i = b_i a_{i+1)T
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);
			Sph_QtniRotateUnitx(segment->qabs,segment->xyzfront,segment->xyzback,-segment->len); }
		Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }

	return; }


/* filCopyFilament */
int filCopyFilament(filamentptr filto,const filamentptr filfrom,const filamenttypeptr filtype) {
	int i;
	segmentptr segmentfrom;

	if(!filto || !filfrom) return 2;

	filto->filtype=filtype?filtype:filfrom->filtype;

	filto->nseg=0;
	filto->nbranch=0;
	filto->nsequence=0;
	filto=filAlloc(filto,filfrom->nseg,filfrom->nbranch,filfrom->nsequence);
	if(!filto) return 1;

	for(i=0;i<filfrom->nseg;i++) {
		segmentfrom=filfrom->segments[i];
		filAddSegment(filto,segmentfrom->xyzfront,segmentfrom->len,segmentfrom->ypr,segmentfrom->thk,'b'); }

	filto->seg0up[0]=filfrom->seg0up[0];
	filto->seg0up[1]=filfrom->seg0up[1];
	filto->seg0up[2]=filfrom->seg0up[2];

	filto->frontend=filfrom->frontend;
	filto->backend=filfrom->backend;

	for(i=0;i<filfrom->nbranch;i++) {
		filto->branchspots[i]=filfrom->branchspots[i];
		filto->branches[i]=filfrom->branches[i]; }
	filto->nbranch=filfrom->nbranch;

	for(i=0;i<filfrom->nsequence;i++)
		filto->sequence[i]=filfrom->sequence[i];
	filto->nsequence=filfrom->nsequence;

	return 0; }


/* filAddFilament */
filamentptr filAddFilament(filamenttypeptr filtype,const char *filname) {
	int f;
	filamentptr fil;
	char autoname[STRCHAR];

	if(filname) {
		f=stringfind(filtype->filnames,filtype->nfil,filname);
		if(f>=0) return filtype->fillist[f]; }
	else {
		sprintf(autoname,"%s.%i",filtype->ftname,filtype->autonamenum++);
		filname=autoname; }

	if(filtype->nfil==filtype->maxfil) {
		filtype=filamentTypeAlloc(filtype,filtype->maxfil*2+1,0);
		if(!filtype) return NULL; }
	f=filtype->nfil++;
	strncpy(filtype->filnames[f],filname,STRCHAR-1);
	filtype->filnames[f][STRCHAR-1]='\0';
	fil=filtype->fillist[f];
	fil->filtype=filtype;
	fil->nseg=0;
	fil->frontend=fil->backend=NULL;
	fil->nbranch=0;
	fil->nsequence=0;
	filSetCondition(filtype->filss,SClists,0);

	return fil; }


/******************************************************************************/
/**************************** filament interactions ***********************/
/******************************************************************************/


/* filSegmentXSurface */
int filSegmentXSurface(const simptr sim,const segmentptr segment,panelptr *pnlptr) {
	int s,p,cross;
	surfaceptr srf;
	panelptr pnl;
	double *pt1,*pt2,crosspt[3];
	enum PanelShape ps;

	if(!sim->srfss) return 0;
	pt1=segment->xyzfront;
	pt2=segment->xyzback;

	cross=0;
	for(s=0;s<sim->srfss->nsrf && !cross;s++) {
		srf=sim->srfss->srflist[s];
		for(ps=(enum PanelShape)0;ps<PSMAX && !cross;ps=(enum PanelShape)(ps+1))
			for(p=0;p<srf->npanel[ps] && !cross;p++) {
				pnl=srf->panels[ps][p];
				cross=lineXpanel(pt1,pt2,pnl,sim->dim,crosspt,NULL,NULL,NULL,NULL,NULL,0); }}
	if(cross && pnlptr) *pnlptr=pnl;
	return cross; }


/* filSegmentXFilament */
int filSegmentXFilament(const simptr sim,const segmentptr segment,filamentptr *filptr) {
	int f,i,ft,cross;
	double thk,*ptf,*ptb,dist;
	filamentssptr filss;
	filamenttypeptr filtype;
	filamentptr fil,fil2;
	segmentptr segmentm1,segmentp1,segment2;

	fil=segment->fil;
	ptf=segment->xyzfront;
	ptb=segment->xyzback;
	thk=segment->thk;
	segmentm1=(segment->index==0) ? NULL:fil->segments[segment->index-1];
	segmentp1=(segment->index==fil->nseg-1) ? NULL:fil->segments[segment->index+1];

	cross=0;
	filss=sim->filss;
	for(ft=0;ft<filss->ntype && !cross;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil && !cross;f++) {
			fil2=filtype->fillist[f];
			for(i=0;i<fil2->nseg && !cross;i++) {
				segment2=fil2->segments[i];
				if(!(segment2==segment || segment2==segmentm1 || segment2==segmentp1)) {
					dist=Geo_NearestSeg2SegDist(ptf,ptb,segment2->xyzfront,segment2->xyzback);
					if(dist<thk+segment2->thk) cross=1; }}}}
	if(cross && filptr)
		*filptr=fil2;
	return cross; }


/******************************************************************************/
/**************************** Force computation *************************/
/******************************************************************************/


/* filAddStretchForces */
void filAddStretchForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,klen,stdlen,sforce,len,xvect[3],fdx,**nodes;
	int dim,node;

	forces=fil->filwork->forces;
	nodes=fil->nodes;
	klen=fil->filtype->klen;
	stdlen=fil->filtype->stdlen;
	dim=fil->filtype->filss->sim->dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	if(dim==2) {
		for(node=nodemin;node<nodemax;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]);
			sforce=-klen*(len-stdlen)/len;			// this is force/length
			fdx=sforce*xvect[0];
			forces[node][0]-=fdx;
			forces[node+1][0]+=fdx;
			fdx=sforce*xvect[1];
			forces[node][1]-=fdx;
			forces[node+1][1]+=fdx; }}
	else {
		for(node=nodemin;node<nodemax;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			xvect[2]=nodes[node+1][2]-nodes[node][2];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2]);
			sforce=-klen*(len-stdlen)/len;			// this is force/length
			fdx=sforce*xvect[0];
			forces[node][0]-=fdx;
			forces[node+1][0]+=fdx;
			fdx=sforce*xvect[1];
			forces[node][1]-=fdx;
			forces[node+1][1]+=fdx;
			fdx=sforce*xvect[2];
			forces[node][2]-=fdx;
			forces[node+1][2]+=fdx; }}
	return; }


/* filAddStretchForceMat */
void filAddStretchForceMat(filamentptr fil) {
	double klen,stdlen,sforce,len,xvect[3],**nodes;
	int dim,node;
	sparsematrix forcemat;

	nodes=fil->nodes;
	forcemat=fil->filwork->forcemat;
	klen=fil->filtype->klen;
	stdlen=fil->filtype->stdlen;
	dim=fil->filtype->filss->sim->dim;

	if(dim==2) {
		for(node=0;node<fil->nseg;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]);
			sforce=-klen*(len-stdlen)/len;			// this is force/length
			sparseAddToM(forcemat,2*node+0,2*node+0,sforce);
			sparseAddToM(forcemat,2*node+1,2*node+1,sforce);
			sparseAddToM(forcemat,2*node+0,2*(node+1)+0,-sforce);
			sparseAddToM(forcemat,2*node+1,2*(node+1)+1,-sforce);
			sparseAddToM(forcemat,2*(node+1)+0,2*node+0,-sforce);
			sparseAddToM(forcemat,2*(node+1)+1,2*node+1,-sforce);
			sparseAddToM(forcemat,2*(node+1)+0,2*(node+1)+0,sforce);
			sparseAddToM(forcemat,2*(node+1)+1,2*(node+1)+1,sforce); }}
	else {
		for(node=0;node<fil->nseg;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			xvect[2]=nodes[node+1][2]-nodes[node][2];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2]);
			sforce=-klen*(len-stdlen)/len;			// this is force/length
			sparseAddToM(forcemat,4*node+0,4*node+0,sforce);
			sparseAddToM(forcemat,4*node+1,4*node+1,sforce);
			sparseAddToM(forcemat,4*node+2,4*node+2,sforce);
			sparseAddToM(forcemat,4*node+0,4*(node+1)+0,-sforce);
			sparseAddToM(forcemat,4*node+1,4*(node+1)+1,-sforce);
			sparseAddToM(forcemat,4*node+2,4*(node+1)+2,-sforce);
			sparseAddToM(forcemat,4*(node+1)+0,4*node+0,-sforce);
			sparseAddToM(forcemat,4*(node+1)+1,4*node+1,-sforce);
			sparseAddToM(forcemat,4*(node+1)+2,4*node+2,-sforce);
			sparseAddToM(forcemat,4*(node+1)+0,4*(node+1)+0,sforce);
			sparseAddToM(forcemat,4*(node+1)+1,4*(node+1)+1,sforce);
			sparseAddToM(forcemat,4*(node+1)+2,4*(node+1)+2,sforce); }}
	return; }


/* filAddThermalForces */
void filAddThermalForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,*kypr,kT,stdlen,frms;
	filamenttypeptr filtype;
	filamentworkptr filwork;
	int dim,node;
	simptr sim;

	filtype=fil->filtype;
	filwork=fil->filwork;
	sim=filtype->filss->sim;
	dim=sim->dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	if(sim->time>filwork->thermtime) {				// compute random forces on each node
		kypr=filtype->kypr;
		stdlen=filtype->stdlen;
		kT=filtype->kT;
		frms=sqrt(kypr[0]*kT)/stdlen;						//?? This equation is almost certainly incorrect
		if(dim==2)
			for(node=0;node<=fil->nseg;node++) {
				filwork->thermforce[node][0]=2*frms*gaussrandD();
				filwork->thermforce[node][1]=2*frms*gaussrandD(); }
		else
			for(node=0;node<=fil->nseg;node++) {
				filwork->thermforce[node][0]=2*frms*gaussrandD();
				filwork->thermforce[node][1]=2*frms*gaussrandD();
				filwork->thermforce[node][2]=2*frms*gaussrandD(); }
		filwork->thermtime=sim->time; }

//??	torques=fil->torques;
	forces=filwork->forces;										// report forces
	if(dim==2)
		for(node=nodemin;node<=nodemax;node++) {
			forces[node][0]+=filwork->thermforce[node][0];
			forces[node][1]+=filwork->thermforce[node][1]; }
	else
		for(node=nodemin;node<=nodemax;node++) {
			forces[node][0]+=filwork->thermforce[node][0];
			forces[node][1]+=filwork->thermforce[node][1];
			forces[node][2]+=filwork->thermforce[node][2]; }

	return; }


void filAddBendForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,*torques,bendtorque[3],forcem1[3],forcep1[3],force[3],**nodes;
	double xvect[3],xvectm1[3],len2inv,len2m1inv;
	int node,dim;

	forces=fil->filwork->forces;
	nodes=fil->nodes;
	torques=fil->filwork->torques;
	dim=fil->filtype->filss->sim->dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	if(dim==2) {
		node=nodemin;
		xvect[0]=nodes[node+1][0]-nodes[node][0];
		xvect[1]=nodes[node+1][1]-nodes[node][1];
		len2inv=1.0/(xvect[0]*xvect[0]+xvect[1]*xvect[1]);					// inverse squared length of segment i
		for(node=nodemin+1;node<nodemax;node++) {										// compute bending forces
			filBendTorque(fil,node,bendtorque);												// get bending torque in system reference frame

			xvectm1[0]=xvect[0];																			// Delta x_(i-1)
			xvectm1[1]=xvect[1];
			xvect[0]=nodes[node+1][0]-nodes[node][0];									// Delta x_i
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len2m1inv=len2inv;
			len2inv=1.0/(xvect[0]*xvect[0]+xvect[1]*xvect[1]);				// inverse squared length of segment i

			forcep1[0]=len2inv*(-bendtorque[2]*xvect[1]);
			forcep1[1]=len2inv*(bendtorque[2]*xvect[0]);
			forcem1[0]=len2m1inv*(-bendtorque[2]*xvectm1[1]);
			forcem1[1]=len2m1inv*(bendtorque[2]*xvectm1[0]);
			force[0]=-(forcep1[0]+forcem1[0]);													// force on node i
			force[1]=-(forcep1[1]+forcem1[1]);

			forces[node-1][0]+=forcem1[0];
			forces[node-1][1]+=forcem1[1];
			forces[node][0]+=force[0];
			forces[node][1]+=force[1];
			forces[node+1][0]+=forcep1[0];
			forces[node+1][1]+=forcep1[1]; }}
	else {
		node=nodemin;
		xvect[0]=nodes[node+1][0]-nodes[node][0];
		xvect[1]=nodes[node+1][1]-nodes[node][1];
		xvect[2]=nodes[node+1][2]-nodes[node][2];
		len2inv=1.0/(xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2]);	// inverse squared length of segment i
		for(node=nodemin+1;node<nodemax;node++) {										// compute bending forces
			filBendTorque(fil,node,bendtorque);												// get bending torque in system reference frame

			xvectm1[0]=xvect[0];
			xvectm1[1]=xvect[1];
			xvectm1[2]=xvect[2];
			xvect[0]=nodes[node+1][0]-nodes[node][0];									// Delta x_i
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			xvect[2]=nodes[node+1][2]-nodes[node][2];
			len2m1inv=len2inv;
			len2inv=1.0/(xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2]);			// inverse squared length of segment i

			forcep1[0]=len2inv*(bendtorque[1]*xvect[2]-bendtorque[2]*xvect[1]);				// torque x xvect / len^2
			forcep1[1]=len2inv*(bendtorque[2]*xvect[0]-bendtorque[0]*xvect[2]);
			forcep1[2]=len2inv*(bendtorque[0]*xvect[1]-bendtorque[1]*xvect[0]);
			forcem1[0]=len2m1inv*(bendtorque[1]*xvectm1[2]-bendtorque[2]*xvectm1[1]);	// torque x xvectm1 / lenm1^2
			forcem1[1]=len2m1inv*(bendtorque[2]*xvectm1[0]-bendtorque[0]*xvectm1[2]);
			forcem1[2]=len2m1inv*(bendtorque[0]*xvectm1[1]-bendtorque[1]*xvectm1[0]);
			force[0]=-(forcep1[0]+forcem1[0]);																				// force on node i
			force[1]=-(forcep1[1]+forcem1[1]);
			force[2]=-(forcep1[2]+forcem1[2]);

			forces[node-1][0]+=forcem1[0];
			forces[node-1][1]+=forcem1[1];
			forces[node-1][2]+=forcem1[2];
			forces[node][0]+=force[0];
			forces[node][1]+=force[1];
			forces[node][2]+=force[2];
			forces[node+1][0]+=forcep1[0];
			forces[node+1][1]+=forcep1[1];
			forces[node+1][2]+=forcep1[2];

			torques[node]+=(bendtorque[0]*xvect[0]+bendtorque[1]*xvect[1]+bendtorque[2]*xvect[2])*sqrt(len2inv);
			torques[node-1]-=(bendtorque[0]*xvectm1[0]+bendtorque[1]*xvectm1[1]+bendtorque[2]*xvectm1[2])*sqrt(len2m1inv); }}

	return; }


/* filAddBendForceMat */
void filAddBendForceMat(filamentptr fil) {
	int dim,node;
	double bendtorquei[3],bendtorqueip1[3],bvecti[3],bvectim1[3],bpvecti[3],xvect[3],len2,len2inv,len;
	double **nodes;
	sparsematrix forcemat;

	dim=fil->filtype->filss->sim->dim;
	nodes=fil->nodes;
	forcemat=fil->filwork->forcemat;

	bvecti[0]=bvecti[1]=bvecti[2]=0;
	bvectim1[0]=bvectim1[1]=bvectim1[2]=0;
	bpvecti[0]=bpvecti[1]=bpvecti[2]=0;
	filBendTorque(fil,0,bendtorquei);															// tau_0
	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			filBendTorque(fil,node+1,bendtorqueip1);									// tau_{i+1}
			xvect[0]=nodes[node+1][0]-nodes[node][0];									// Delta x_i
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len2=xvect[0]*xvect[0]+xvect[1]*xvect[1];									// length of segment i
			len2inv=1.0/len2;
			bvecti[2]=len2inv*(bendtorqueip1[2]-bendtorquei[2]);			// b_i
			if(node>0) {
				sparseAddToM(forcemat,2*node+0,2*(node-1)+1,-bvectim1[2]);
				sparseAddToM(forcemat,2*node+1,2*(node-1)+0,bvectim1[2]); }
			sparseAddToM(forcemat,2*node+0,2*node+1,bvecti[2]+bvectim1[2]);
			sparseAddToM(forcemat,2*node+1,2*node+0,-bvecti[2]-bvectim1[2]);
			if(node<fil->nseg) {
				sparseAddToM(forcemat,2*node+0,2*(node+1)+1,-bvecti[2]);
				sparseAddToM(forcemat,2*node+1,2*(node+1)+0,bvecti[2]); }
			bendtorquei[2]=bendtorqueip1[2];
			bvectim1[2]=bvecti[2]; }}
	else {
		for(node=0;node<=fil->nseg;node++) {
			filBendTorque(fil,node+1,bendtorqueip1);									// tau_{i+1}
			xvect[0]=nodes[node+1][0]-nodes[node][0];									// Delta x_i
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			xvect[2]=nodes[node+1][2]-nodes[node][2];
			len2=xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2];			// length of segment i
			len2inv=1.0/len2;
			len=sqrt(len2);
			bvecti[0]=len2inv*(bendtorqueip1[0]-bendtorquei[2]);
			bvecti[1]=len2inv*(bendtorqueip1[1]-bendtorquei[1]);
			bvecti[2]=len2inv*(bendtorqueip1[2]-bendtorquei[2]);			// b_i
			bpvecti[0]=bvecti[0]*len;
			bpvecti[1]=bvecti[1]*len;
			bpvecti[2]=bvecti[2]*len;
			if(node>0) {
				sparseAddToM(forcemat,4*node+0,4*(node-1)+1,-bvectim1[2]);
				sparseAddToM(forcemat,4*node+0,4*(node-1)+2,bvectim1[1]);
				sparseAddToM(forcemat,4*node+1,4*(node-1)+0,bvectim1[2]);
				sparseAddToM(forcemat,4*node+1,4*(node-1)+2,-bvectim1[0]);
				sparseAddToM(forcemat,4*node+2,4*(node-1)+0,-bvectim1[1]);
				sparseAddToM(forcemat,4*node+2,4*(node-1)+1,bvectim1[0]); }
			sparseAddToM(forcemat,4*node+0,4*node+1,bvecti[2]+bvectim1[2]);
			sparseAddToM(forcemat,4*node+0,4*node+2,-bvecti[1]-bvectim1[1]);
			sparseAddToM(forcemat,4*node+1,4*node+0,-bvecti[2]-bvectim1[2]);
			sparseAddToM(forcemat,4*node+1,4*node+2,bvecti[0]+bvectim1[0]);
			sparseAddToM(forcemat,4*node+2,4*node+0,bvecti[1]+bvectim1[1]);
			sparseAddToM(forcemat,4*node+2,4*node+1,-bvecti[0]-bvectim1[0]);
			if(node<fil->nseg) {
				sparseAddToM(forcemat,4*node+0,4*(node+1)+1,-bvecti[2]);
				sparseAddToM(forcemat,4*node+0,4*(node+1)+2,bvecti[1]);
				sparseAddToM(forcemat,4*node+1,4*(node+1)+0,bvecti[2]);
				sparseAddToM(forcemat,4*node+1,4*(node+1)+2,-bvecti[0]);
				sparseAddToM(forcemat,4*node+2,4*(node+1)+0,-bvecti[1]);
				sparseAddToM(forcemat,4*node+2,4*(node+1)+1,bvecti[0]);
				sparseAddToM(forcemat,4*node+3,4*node+0,bpvecti[0]);
				sparseAddToM(forcemat,4*node+3,4*node+1,bpvecti[1]);
				sparseAddToM(forcemat,4*node+3,4*node+2,bpvecti[2]);
				sparseAddToM(forcemat,4*node+3,4*(node+1)+0,-bpvecti[0]);
				sparseAddToM(forcemat,4*node+3,4*(node+1)+1,-bpvecti[1]);
				sparseAddToM(forcemat,4*node+3,4*(node+1)+2,-bpvecti[2]); }
			bendtorquei[0]=bendtorqueip1[0];
			bendtorquei[1]=bendtorqueip1[1];
			bendtorquei[2]=bendtorqueip1[2];
			bvectim1[0]=bvecti[0];
			bvectim1[1]=bvecti[1];
			bvectim1[2]=bvecti[2]; }}
	return; }


/* filComputeForces */
void filComputeForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,*torques;
	int node,dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	dim=fil->filtype->filss->sim->dim;
	forces=fil->filwork->forces;
	torques=fil->filwork->torques;

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++)							// clear all forces
			forces[node][0]=forces[node][1]=0; }
	else {
		for(node=0;node<=fil->nseg;node++)							// clear all forces
			forces[node][0]=forces[node][1]=forces[node][2]=0;
		for(node=0;node<fil->nseg;node++)								// clear all torques
			torques[node]=0; }

	filAddStretchForces(fil,nodemin,nodemax);
	filAddBendForces(fil,nodemin-1,nodemax+1);
	filAddThermalForces(fil,nodemin,nodemax);
	return; }


/* filComputeForceMatrix */
void filComputeForceMatrix(filamentptr fil) {
	int dim,flatsize;
	sparsematrix forcemat;

	dim=fil->filtype->filss->sim->dim;
	flatsize= (dim==2) ? 2*(fil->nseg+1) : 4*(fil->nseg+1)-1;
	forcemat=fil->filwork->forcemat;
	sparseSetSizeM(forcemat,flatsize,flatsize);
	sparseClearM(forcemat);

	filAddStretchForceMat(fil);
	filAddBendForceMat(fil);
	return; }


/* filComputeDerivForceMat */
void filComputeDerivForceMat(filamentptr fil,double dtmu) {
	int dim,node,node2,d,flatsize,node2first,node2last;
	double **nodes,**flatnodes,**flatforces;
	double dx,dxinv;
	sparsematrix forcemat;

	dim=fil->filtype->filss->sim->dim;
	flatsize= (dim==2) ? 2*(fil->nseg+1) : 4*(fil->nseg+1)-1;
	forcemat=fil->filwork->forcemat;
	sparseSetSizeM(forcemat,flatsize,flatsize);
	sparseClearM(forcemat);

	nodes=fil->nodes;
	flatforces=fil->filwork->flatforces;
	flatnodes=fil->filwork->flatnodes;

	filFlattenNodes(fil,0,0);																		// flatnodes[0] is original node positions
	filComputeForces(fil,-1,-1);
	filFlattenForces(fil,0,-1,-1);															// flatforces[0] is original forces.
	multKVD(dtmu/1000,flatforces[0],flatnodes[1],flatsize);			// flatnodes[1] is 1/1000 of forward Euler step

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			for(d=0;d<2;d++) {
				dx=flatnodes[1][2*node+d];
				if(dx==0) break;
				dxinv=1.0/dx;
				nodes[node][d]+=dx;
				filNodes2Angles(fil,(d==0)?node-2:node-1,node+1);			// update for new change and fix prior change
				filComputeForces(fil,node-2,node+2);
				filFlattenForces(fil,1,node-2,node+2);								// flatforces[1] is modified forces
				nodes[node][d]=flatnodes[0][2*node+d];
				node2first=(node<2)?0:node-2;
				node2last=(node+2>fil->nseg)?fil->nseg:node+2;
				for(node2=node2first;node2<=node2last;node2++) {			// row block is node2, column block is node
					sparseAddToM(forcemat,2*node2,2*node+d,dxinv*(flatforces[1][2*node2]-flatforces[0][2*node2]));
					sparseAddToM(forcemat,2*node2+1,2*node+d,dxinv*(flatforces[1][2*node2+1]-flatforces[0][2*node2+1])); }}}}
	else {
		for(node=0;node<=fil->nseg;node++) {
			for(d=0;d<4;d++) {
				if(node==fil->nseg && d==4) break;
				dx=flatnodes[1][4*node+d];
				if(dx==0) break;
				dxinv=1.0/dx;
				if(d<3) nodes[node][d]+=dx;
				else fil->segments[node]->ypr[2]+=dx;
				filNodes2Angles(fil,(d==0)?node-2:node-1,node+1);	// update for new change and fix prior change

				filComputeForces(fil,node-2,node+2);
				filFlattenForces(fil,1,node-2,node+2);						// flatforces[1] is modified forces

				if(d<3) nodes[node][d]=flatnodes[0][4*node+d];
				else fil->segments[node]->ypr[2]=flatnodes[0][4*node+d];
				node2first=(node<2)?0:node-2;
				node2last=(node+2>fil->nseg)?fil->nseg:node+2;
				for(node2=node2first;node2<=node2last;node2++) {		// row block is node2, column block is node
					sparseAddToM(forcemat,4*node2+0,4*node+d,dxinv*(flatforces[1][4*node2+0]-flatforces[0][4*node2+0]));
					sparseAddToM(forcemat,4*node2+1,4*node+d,dxinv*(flatforces[1][4*node2+1]-flatforces[0][4*node2+1]));
					sparseAddToM(forcemat,4*node2+2,4*node+d,dxinv*(flatforces[1][4*node2+2]-flatforces[0][4*node2+2]));
					sparseAddToM(forcemat,4*node2+3,4*node+d,dxinv*(flatforces[1][4*node2+3]-flatforces[0][4*node2+3])); }}}}

	filNodes2Angles(fil,node-2,-1);
	sparseMultiplyMV(forcemat,flatnodes[0],flatforces[1]);						// flatforces[1] is F1.x
	sumVD(1,flatforces[0],-1,flatforces[1],flatforces[1],flatsize);		// flatforces[1] is F0

	return; }


/******************************************************************************/
/**************************** dynamics functions ***********************/
/******************************************************************************/


/* filStepDynamics */
void filStepDynamics(filamentptr fil,double dtmu,int in,int out) {
	int node,seg,dim;
	double axis[3],**nodesin,**nodesout,*seg0upin,*seg0upout,*rollin,*rollout;
	double **forces,*nodemobility;
	filamentworkptr filwork;

	dim=fil->filtype->filss->sim->dim;
	filwork=fil->filwork;

	nodesin=filwork->wnodes[in];
	seg0upin=filwork->wseg0up[in];
	rollin=filwork->wroll[in];
	nodesout=filwork->wnodes[out];
	seg0upout=filwork->wseg0up[out];
	rollout=filwork->wroll[out];
	forces=filwork->forces;

	nodemobility=fil->nodemobility;

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			nodesout[node][0]=nodesin[node][0]+dtmu*nodemobility[node]*forces[node][0];
			nodesout[node][1]=nodesin[node][1]+dtmu*nodemobility[node]*forces[node][1]; }}
	else {
		if(in==0) filCopyYpr2ToRoll(fil);
		axis[0]=nodesin[1][0]-nodesin[0][0];					// process segment 0 torque
		axis[1]=nodesin[1][1]-nodesin[0][1];
		axis[2]=nodesin[1][2]-nodesin[0][2];
		Sph_RotateVectorAxisAngle(seg0upin,axis,dtmu*nodemobility[0]*filwork->torques[0],seg0upout);
		for(seg=1;seg<fil->nseg;seg++) {					// process other segment torques
			rollout[seg]=rollin[seg]+dtmu*nodemobility[1]*filwork->torques[seg]; }
		for(node=0;node<=fil->nseg;node++) {			// process node forces
			nodesout[node][0]=nodesin[node][0]+dtmu*nodemobility[node]*forces[node][0];
			nodesout[node][1]=nodesin[node][1]+dtmu*nodemobility[node]*forces[node][1];
			nodesout[node][2]=nodesin[node][2]+dtmu*nodemobility[node]*forces[node][2]; }
		if(out==0) filCopyRollToYpr2(fil); }
	return; }


/* filEulerDynamics */
void filEulerDynamics(simptr sim,filamenttypeptr filtype) {
	int f;
	double dtmu;
	filamentptr fil;

	dtmu=filtype->mobility*sim->dt;
	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filStepDynamics(fil,dtmu,0,0);
		filNodes2Angles(fil,-1,-1); }
	return; }


/* filRK2Dynamics */
void filRK2Dynamics(simptr sim,filamenttypeptr filtype) {
	int f;
	double dtmu;
	filamentptr fil;

	dtmu=filtype->mobility*sim->dt;
	for(f=0;f<filtype->nfil;f++) {						// store initial state in nodes1 and roll1 and then take a half step
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filCopyWorkingNodes(fil,0,1);
		filStepDynamics(fil,dtmu/2,0,0);
		filNodes2Angles(fil,-1,-1); }

	for(f=0;f<filtype->nfil;f++) {							// compute force from new state and then take full step from initial state
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filStepDynamics(fil,dtmu,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }


/* filRK4Dynamics */
void filRK4Dynamics(simptr sim,filamenttypeptr filtype) {
	int f;
	double dtmu;
	filamentptr fil;

	dtmu=filtype->mobility*sim->dt;
	for(f=0;f<filtype->nfil;f++) {						// copy initial state to nodes1/roll1, then take a half step and add to nodes2/roll2
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filCopyWorkingNodes(fil,0,1);
		filStepDynamics(fil,dtmu/6,0,2);
		filStepDynamics(fil,dtmu/2,0,0);
		filNodes2Angles(fil,-1,-1); }

	for(f=0;f<filtype->nfil;f++) {							// with new force, take half step from initial state, and add to nodes2/roll2
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filStepDynamics(fil,dtmu/2,1,0);
		filStepDynamics(fil,dtmu/3,2,2);
		filNodes2Angles(fil,-1,-1); }

	for(f=0;f<filtype->nfil;f++) {							// with new force, take full step from initial state, and add to nodes2/roll2
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filStepDynamics(fil,dtmu,1,0);
		filStepDynamics(fil,dtmu/3,2,2);
		filNodes2Angles(fil,-1,-1); }

	for(f=0;f<filtype->nfil;f++) {							// with new force, add to nodes2/roll2
		fil=filtype->fillist[f];
		filComputeForces(fil,-1,-1);
		filStepDynamics(fil,dtmu/6,2,0);
		filNodes2Angles(fil,-1,-1); }

	return; }


/* filEulerMatDynamics */
void filEulerMatDynamics(simptr sim,filamenttypeptr filtype) {	//?? This ignores nodemobility
	double dtmu;
	int f;
	filamentptr fil;
	filamentworkptr filwork;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filwork=fil->filwork;

		filComputeForceMatrix(fil);
		filFlattenNodes(fil,0,0);
		dtmu=filtype->mobility*sim->dt;
		sparseMultiplyScalarM(filwork->forcemat,dtmu);
		sparseAddIdentityM(filwork->forcemat,1);
		sparseMultiplyMV(filwork->forcemat,filwork->flatnodes[0],filwork->flatnodes[1]);
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }


/* filImplicitOldDynamics */
void filImplicitOldDynamics(simptr sim,filamenttypeptr filtype) {	//?? This ignores nodemobility
	double dtmu;
	int f;
	filamentptr fil;
	filamentworkptr filwork;
	sparsematrix forcemat;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filwork=fil->filwork;
		forcemat=filwork->forcemat;
		if(!forcemat->BiCGSTAB)
			sparseAllocBandM(forcemat,-1,-1,1);

		filComputeForceMatrix(fil);
		filFlattenNodes(fil,0,0);													// flatnodes[0] is current state
		dtmu=filtype->mobility*sim->dt;
		sparseMultiplyScalarM(forcemat,dtmu);
		sparseAddIdentityM(forcemat,1);										// forcemat is 1+dtmu*force
		sparseMultiplyMV(forcemat,filwork->flatnodes[0],filwork->flatnodes[1]);	// flatnodes[1] is forward Euler next state

		sparseMultiplyScalarM(forcemat,-1);
		sparseAddIdentityM(forcemat,2);										// forcemat is 1-dtmu*force
		sparseBiCGSTAB(forcemat,filwork->flatnodes[0],filwork->flatnodes[1],1e-9);	// flatnodes[1] is backward Euler next state
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }



/* filImplicitDynamics */
void filImplicitDynamics(simptr sim,filamenttypeptr filtype) {	//?? This ignores nodemobility
	double dtmu,**flatnodes,**flatforces;
	int f,flatsize,dim;
	filamentptr fil;
	filamentworkptr filwork;
	sparsematrix forcemat;

	dim=filtype->filss->sim->dim;
	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filwork=fil->filwork;
		forcemat=filwork->forcemat;
		flatnodes=filwork->flatnodes;
		flatforces=filwork->flatforces;
		flatsize=(dim==2) ? 2*(fil->nseg+1) : 4*(fil->nseg+1)-1;
		if(!forcemat->BiCGSTAB)
			sparseAllocBandM(forcemat,-1,-1,1);

		dtmu=filtype->mobility*sim->dt;
		filComputeDerivForceMat(fil,dtmu);															// nodes are in flatnodes[0], forces are in flatforces[0], F0 is in flatforces[1] and F1 is in forcemat
		sumVD(1,flatnodes[0],dtmu,flatforces[0],flatnodes[1],flatsize);	// flatnodes[1] is forward Euler next state
		sumVD(1,flatnodes[0],dtmu,flatforces[1],flatnodes[2],flatsize);	// flatnodes[2] is x(t)+dtmu*F0

		sparseMultiplyScalarM(forcemat,-dtmu);
		sparseAddIdentityM(forcemat,1);																	// forcemat is 1-dtmu*force
		sparseBiCGSTAB(forcemat,flatnodes[2],flatnodes[1],1e-9);				// flatnodes[1] is backward Euler next state
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }



/* filDynamics */
int filDynamics(simptr sim) {
	filamentssptr filss;
	filamentptr fil;
	filamenttypeptr filtype;
	int f,ft,i,treadnum;

	filss=sim->filss;
	if(!filss) return 0;

	for(ft=0;ft<filss->ntype;ft++) {
		filtype=filss->filtypes[ft];
		if(filtype->treadrate!=0) {
			for(f=0;f<filtype->nfil;f++) {
				fil=filtype->fillist[f];
				treadnum=poisrandD(fabs(filtype->treadrate)*sim->dt);
				for(i=0;i<treadnum;i++)
					filTreadmill(sim,fil,filtype->treadrate>0?'b':'f'); }}

		if(filtype->dynamics==FDeuler)
			filEulerDynamics(sim,filtype);
		else if(filtype->dynamics==FDRK2)
			filRK2Dynamics(sim,filtype);
		else if(filtype->dynamics==FDRK4)
			filRK4Dynamics(sim,filtype);
		else if(filtype->dynamics==FDeulermat)
			filEulerMatDynamics(sim,filtype);
		else if(filtype->dynamics==FDimplicit)
			filImplicitDynamics(sim,filtype);
		else if(filtype->dynamics==FDimplicitold)
			filImplicitOldDynamics(sim,filtype);
			}

	return 0; }




