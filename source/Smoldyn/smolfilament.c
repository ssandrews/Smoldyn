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
#define FILMAXGROW 1000					// max segments one filament may gain in a single elongation step
#define FILREMOVED -2000				// filEndLengthChange return value: the filament shrank away and should be removed
#define FILJUNCTMINNODE 0				// lowest target node a junction may attach to
#define FILCOSPMIN 0.05					// smallest cos(pitch) for bending torques and their Jacobian, which diverge at gimbal lock
#define FILCONSTRAINTITER 20			// maximum Newton iterations for projecting onto constraints
#define FILCONSTRAINTTOL 1e-10			// constraint tolerance, relative to standard_length for lengths, and in radians for angles


/******************************************************************************/
/********************************** Filaments *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// Enumerated types
char *filFD2string(enum FilamentDynamics fd,char *string);
enum FilamentDynamics filstring2FD(const char *string);
int filstring2end(const char *string);
char filend2char(int end);
char *filFSA2string(enum FilSurfAction fsa,char *string);
enum FilSurfAction filstring2FSA(const char *string);
int filstring2parts(const char *string,int *filpart);
char *filparts2string(const int *filpart,char *string);

// Low level utilities
int filReadFilName(const simptr sim,const char *str,filamenttypeptr *filtypeptr,filamentptr *filptr,char *filname);
int filGetFilIndex(const simptr sim,const char *name,int *ftptr);

// memory management
segmentptr filSegmentAlloc();
void filSegmentFree(segmentptr segment);
filamentworkptr filWorkAlloc(filamentptr fil,int maxseg);
void filWorkFree(filamentworkptr filwork,int maxseg);
filamentptr filAlloc(filamentptr fil,int maxseg,int maxjunct,int maxsequence);
void filFree(filamentptr fil);
filamenttypeptr filamentTypeAlloc(filamenttypeptr filtype,int maxfil,int maxface);
void filamentTypeFree(filamenttypeptr filtype);
filsurfinteractptr filSurfInteractAlloc(filamenttypeptr filtype);
void filSurfInteractFree(filsurfinteractptr fsi);
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
filsurfinteractptr filtypeAddSurfInteract(filamenttypeptr filtype,surfaceptr srf,enum PanelFace face,const int *filpart,enum FilSurfAction action,double kforce);
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
void filTransportFrames(filamentptr fil,double **nodesold,const double *qold,const double *segnew,const double *dphi,int dphistride,double *qnew,double *seg0upnew,double *rollnew);
void filWorkFramesFromSegments(filamentptr fil);
void filFlattenNodes(filamentptr fil,int from,int to);
void filUnflattenNodes(filamentptr fil,int from,int to);
void filFlattenForces(filamentptr fil,int to,int nodemin,int nodemax);

// Filament manipulation
void filArrayShift(filamentptr fil,int shift);
void filShiftJunctionNodes(filamentptr fil,int shift,int nseg);
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
void filReset(filamentptr fil);
filamentptr filAddFilament(filamenttypeptr filtype,const char *filname);
int filRemoveFilament(filamentptr fil);

// Branching and crosslinking
void filBranchJunctionNodes(const filamentptr daughter,char end,int *nearptr,int *farptr);
filjunctionptr filRigidParent(const filamentptr fil);
filamentptr filTreeRoot(filamentptr fil);
int filInSubtree(filamentptr fil,const filamentptr top);
void filClassifyJunctions(filamentssptr filss);
filjunctionptr filAddJunction(filamentptr fil,char end,filamentptr target,int node,int bend);
void filRemoveJunction(filjunctionptr junct);
void filRemoveFilJunctions(filamentptr fil);
int filAttachEnd(filamentptr fil,char end,filamentptr target,int node);
double filBranchAzimuth(const filjunctionptr junct);
int filBranchDazim(const filjunctionptr junct,double *dphiptr);
filamentptr filAddBranch(simptr sim,filamentptr mother,int node,char end,double length0,const double *angle,double thickness,const char *daughtername);
int filBranchPointInRegion(simptr sim,const filamenttypeptr filtype,double *pos);
int filBranchNodeSpaced(const filamentptr mother,int node,double spacing,const filjunctionptr skip);
void filBranchDynamics(simptr sim,filamenttypeptr filtype);
void filPinJunction(filjunctionptr junct);
void filPinTree(filamentptr fil);
void filPinJunctions(filamentssptr filss);

// Filament elongation and capping
double filContourLength(const filamentptr fil);
int filEndLengthChange(simptr sim,filamentptr fil,int end,double dlength);
void filEndTransitions(simptr sim,filamenttypeptr filtype);
void filEndDynamics(simptr sim,filamenttypeptr filtype);

// Filament interactions
int filSurfBlocksSegment(const filamentptr fil,const segmentptr segment,char endchar);
int filSurfForceOn(const filamenttypeptr filtype);
int filSegmentXFilament(const simptr sim,const segmentptr segment,filamentptr *filptr);

// Force computation
void filAddStretchForces(filamentptr fil,int nodemin,int nodemax);
void filAddStretchForceMat(filamentptr fil);
void filDrawThermalForces(filamentptr fil);
void filAddThermalForces(filamentptr fil,int nodemin,int nodemax);
void filAddMatrixForceStep(filamentptr fil,double dtmu,double *flatvect);
void filAddBendForces(filamentptr fil,int nodemin,int nodemax);
void filAddBendForceMat(filamentptr fil);
void filJunctionParams(const filjunctionptr junct,int dim,double *kappaptr,double *theta0ptr,double *kazptr,double *kjptr);
int filJunctionGeometry(const filjunctionptr junct,int dim,double *tm,double *td,double *lenmptr,double *lendptr,double *costhptr,double *sinthptr,int *maptr,int *mbptr);
void filSubtreeSpringForce(filamentptr fil,int dim,double *sum);
void filAddJunctionForces(filamentptr fil,int nodemin,int nodemax);
double filNodeThickness(const filamentptr fil,int node);
double filNodeMobility(const filamentptr fil,int node);
void filAddSurfForces(filamentptr fil,int nodemin,int nodemax);
void filComputeForces(filamentptr fil,int nodemin,int nodemax);
void filComputeDerivForceMat(filamentptr fil,double dtmu);

// Dynamics functions
void filStepDynamics(filamentptr fil,double dtmu,int in,int out);
void filEulerDynamics(simptr sim,filamenttypeptr filtype);
void filRK2Dynamics(simptr sim,filamenttypeptr filtype);
void filRK4Dynamics(simptr sim,filamenttypeptr filtype);
int filMobilityWeights(filamentptr fil,double *wt);
void filScaleMatrixRows(sparsematrix sprs,const double *wt,int nrows);
void filEulerMatDynamics(simptr sim,filamenttypeptr filtype);
void filImplicitOldDynamics(simptr sim,filamenttypeptr filtype);
void filImplicitDynamics(simptr sim,filamenttypeptr filtype);
int filConstrained(filamenttypeptr filtype);
int filConstraintGrad(filamentptr fil,int c,int *idx,double *g,double *res);
void filConstraintStart(filamentptr fil);
int filProjectConstraints(filamentptr fil);
int filUnstable(simptr sim,filamenttypeptr filtype);
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
	else if(strbegin(string,"implicit",0)) ans=FDimplicit;	// before implicitold, which "implicit" abbreviates
	else if(strbegin(string,"implicitold",0)) ans=FDimplicitold;
	else ans=FDnone;
	return ans; }


/* filFES2string */
char *filFES2string(enum FilEndState fes,char *string) {
	if(fes==FESopen) strcpy(string,"open");
	else if(fes==FEScapped) strcpy(string,"capped");
	else if(fes==FESshrink) strcpy(string,"shrink");
	else strcpy(string,"none");
	return string; }


/* filstring2FES */
enum FilEndState filstring2FES(const char *string) {
	enum FilEndState ans;

	if(!strcmp(string,"open")) ans=FESopen;
	else if(!strcmp(string,"capped")) ans=FEScapped;
	else if(!strcmp(string,"shrink")) ans=FESshrink;
	else ans=FESnone;
	return ans; }


/* filstring2end */
// Filament end index for the word "minus" (0) or "plus" (1); -1 for anything else.
int filstring2end(const char *string) {
	if(!strcmp(string,"minus")) return 0;
	if(!strcmp(string,"plus")) return 1;
	return -1; }


/* filend2char */
// End character, 'm' or 'p', used by the segment functions for end index 0 or 1.
char filend2char(int end) {
	return end==1?'p':'m'; }


/* filFSA2string */
char *filFSA2string(enum FilSurfAction fsa,char *string) {
	if(fsa==FSAforce) strcpy(string,"force");
	else if(fsa==FSAblock) strcpy(string,"block");
	else if(fsa==FSAadsorb) strcpy(string,"adsorb");
	else if(fsa==FSAdesorb) strcpy(string,"desorb");
	else strcpy(string,"none");
	return string; }


/* filstring2FSA */
enum FilSurfAction filstring2FSA(const char *string) {
	enum FilSurfAction ans;

	if(!strcmp(string,"force")) ans=FSAforce;
	else if(!strcmp(string,"block")) ans=FSAblock;
	else if(!strcmp(string,"adsorb")) ans=FSAadsorb;
	else if(!strcmp(string,"desorb")) ans=FSAdesorb;
	else ans=FSAnone;
	return ans; }


/* filstring2parts */
// Reads a comma-separated list of filament parts, from "plus", "minus", "side", and
// "all", into filpart, which is indexed by FPplus, FPminus, and FPside and is set to 1
// for each listed part and 0 for the others. Returns 0 for success or 1 for an
// unrecognized or empty list.
int filstring2parts(const char *string,int *filpart) {
	char str[STRCHAR],*word;
	int part,any;

	for(part=0;part<FILPARTMAX;part++) filpart[part]=0;
	strncpy(str,string,STRCHAR-1);
	str[STRCHAR-1]='\0';
	any=0;
	for(word=strtok(str,",");word;word=strtok(NULL,",")) {
		if(!strcmp(word,"plus")) filpart[FPplus]=1;
		else if(!strcmp(word,"minus")) filpart[FPminus]=1;
		else if(!strcmp(word,"side")) filpart[FPside]=1;
		else if(!strcmp(word,"all")) filpart[FPplus]=filpart[FPminus]=filpart[FPside]=1;
		else return 1;
		any=1; }
	return any?0:1; }


/* filparts2string */
// Writes the filament parts in filpart as a comma-separated list, or "all" or "none".
char *filparts2string(const int *filpart,char *string) {
	string[0]='\0';
	if(filpart[FPplus] && filpart[FPminus] && filpart[FPside]) strcpy(string,"all");
	else {
		if(filpart[FPplus]) strcat(string,"plus");
		if(filpart[FPminus]) strcat(string,string[0]?",minus":"minus");
		if(filpart[FPside]) strcat(string,string[0]?",side":"side");
		if(!string[0]) strcpy(string,"none"); }
	return string; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* filReadFilName */
int filReadFilName(const simptr sim,const char *str,filamenttypeptr *filtypeptr,filamentptr *filptr,char *filname) {
	int itct,ft,f=-1;
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
	segment->xyzminus=NULL;
	segment->xyzplus=NULL;
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
	filwork->thermtorque=NULL;
	filwork->wqabs=NULL;
	filwork->cidx=NULL;
	filwork->cng=NULL;
	filwork->cg=NULL;

	CHECKMEM(filwork->wnodes=(double***) calloc(3,sizeof(double**)));
	filwork->wnodes[0]=fil->nodes;
	filwork->wnodes[1]=filwork->wnodes[2]=NULL;
	CHECKMEM(filwork->wnodes[1]=(double**) calloc(maxseg+1,sizeof(double*)));
	CHECKMEM(filwork->wnodes[2]=(double**) calloc(maxseg+1,sizeof(double*)));
	CHECKMEM(filwork->thermforce=(double**) calloc(maxseg+1,sizeof(double*)));
	CHECKMEM(filwork->thermtorque=(double*) calloc(maxseg,sizeof(double)));
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

	CHECKMEM(filwork->wqabs=(double**) calloc(3,sizeof(double*)));
	filwork->wqabs[0]=filwork->wqabs[1]=filwork->wqabs[2]=NULL;
	for(i=0;i<3;i++)
		CHECKMEM(filwork->wqabs[i]=(double*) calloc(4*(maxseg>0?maxseg:1),sizeof(double)));

	CHECKMEM(filwork->cidx=(int*) calloc(44*(maxseg>0?maxseg:1),sizeof(int)));		// up to 4 constraints per segment with up to 11 coordinates each
	CHECKMEM(filwork->cng=(int*) calloc(4*(maxseg>0?maxseg:1),sizeof(int)));
	CHECKMEM(filwork->cg=(double*) calloc(44*(maxseg>0?maxseg:1),sizeof(double)));

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

	if(filwork->wqabs) {
		free(filwork->wqabs[0]);
		free(filwork->wqabs[1]);
		free(filwork->wqabs[2]);
		free(filwork->wqabs); }

	free(filwork->cidx);
	free(filwork->cng);
	free(filwork->cg);

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
	free(filwork->thermtorque);
	sparseFreeM(filwork->forcemat);
	free(filwork);
	return; }


/* filAlloc */
filamentptr filAlloc(filamentptr fil,int maxseg,int maxjunct,int maxsequence) {
	int seg,j,seqi;
	segmentptr *newsegments;
	filjunctionptr *newjuncts;
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
		fil->endjunct[0]=fil->endjunct[1]=NULL;
		fil->maxjunct=0;
		fil->njunct=0;
		fil->juncts=NULL;
		fil->maxsequence=0;
		fil->nsequence=0;
		fil->sequence=NULL;
		fil->endstate[0]=fil->endstate[1]=FESopen;
		fil->lenbank[0]=fil->lenbank[1]=0;
		fil->filwork=NULL; }

	if(maxseg>fil->maxseg) {
		CHECKMEM(newsegments=(segmentptr*) calloc(maxseg,sizeof(segmentptr)));
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
			newsegments[seg]->xyzminus=newnodes[seg];
			newsegments[seg]->xyzplus=newnodes[seg+1];
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

	if(maxjunct>fil->maxjunct) {
		CHECKMEM(newjuncts=(filjunctionptr*) calloc(maxjunct,sizeof(filjunctionptr)));
		for(j=0;j<fil->maxjunct;j++)
			newjuncts[j]=fil->juncts[j];
		for(;j<maxjunct;j++)
			newjuncts[j]=NULL;
		free(fil->juncts);
		fil->juncts=newjuncts;
		fil->maxjunct=maxjunct; }

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
	free(fil->juncts);
	free(fil->sequence);
	free(fil);
	return; }


/* filamentTypeAlloc */
filamenttypeptr filamentTypeAlloc(filamenttypeptr filtype,int maxfil,int maxface) {
	int f,fc,end,fes,fes2;
	filamentptr *newfillist;
	char **newfacename,**newfilnames;

	if(!filtype) {
		CHECKMEM(filtype=(filamenttypeptr) malloc(sizeof(struct filamenttypestruct)));
		filtype->filss=NULL;
		filtype->ftname=NULL;
		filtype->dynamics=FDnone;
		filtype->constraintwarn=0;
		filtype->bundlevalue=1;

		filtype->color[0]=filtype->color[1]=filtype->color[2]=0;
		filtype->color[3]=1;
		filtype->edgepts=1;
		filtype->edgestipple[0]=1;
		filtype->edgestipple[1]=0xFFFF;
		filtype->drawmode=DMedge;
		filtype->shiny=0;
		filtype->drawforcescale=0;
		filtype->drawforcecolor[0]=filtype->drawforcecolor[1]=filtype->drawforcecolor[2]=0;
		filtype->drawforcecolor[3]=1;

		filtype->stdlen=1;
		filtype->stdypr[0]=filtype->stdypr[1]=filtype->stdypr[2]=0;
		filtype->klen=1;
		filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=1;
		filtype->kT=1;
		filtype->treadrate=0;
		filtype->mobility=1;
		filtype->filradius=1;

		filtype->branchrate=0;											// branching off by default
		filtype->branchangle=70.0*PI/180.0;			// Arp2/3 ~70 deg
		filtype->branchspread=-1;										// negative = unset; every consumer is gated on >0, so unset behaves as deterministic until derived
		filtype->branchspreadderived=0;
		filtype->branchsegments=1;									// daughters born as a single segment
		filtype->branchend='m';											// daughters attach by their minus end by default
		filtype->branchminspacing=0;								// no minimum branch spacing by default
		filtype->junctforce=0;											// loop-closing junction spring defaults to force_length
		filtype->branchforceangle=0;								// junction torsional spring off by default
		filtype->branchazimuth=-1;									// negative = uniform random birth azimuth
		filtype->branchazimuthfix=0;								// azimuth free by default
		filtype->thickdrag=1;												// thicker filaments have more drag by default
		filtype->branchforceazimuth=0;							// azimuthal spring off by default
		filtype->branchsrf=NULL;										// no location gate by default
		filtype->branchsrfdist=0;
		filtype->branchcmpt=NULL;										// no location gate by default
		filtype->maxsurfint=0;											// no interactions by default
		filtype->nsurfint=0;
		filtype->surfint=NULL;
		filtype->maxmolint=0;
		filtype->nmolint=0;
		filtype->molint=NULL;
		filtype->maxfilint=0;
		filtype->nfilint=0;
		filtype->filint=NULL;

		for(end=0;end<2;end++) {										// ends don't change length or state by default
			filtype->endstate0[end]=FESopen;
			for(fes=0;fes<FESMAX;fes++) {
				filtype->endrate[end][fes]=0;
				for(fes2=0;fes2<FESMAX;fes2++) filtype->endtrans[end][fes][fes2]=0; }}
		filtype->elongmaxlen=0;											// 0 => unbounded growth

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
	int f,fc,i;

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

	for(i=0;i<filtype->nsurfint;i++)
		filSurfInteractFree(filtype->surfint[i]);
	free(filtype->surfint);
	for(i=0;i<filtype->nmolint;i++)							// molecule and filament interactions are not implemented yet
		free(filtype->molint[i]);
	free(filtype->molint);
	for(i=0;i<filtype->nfilint;i++)
		free(filtype->filint[i]);
	free(filtype->filint);

	free(filtype);
	return; }


/* filSurfInteractAlloc */
// Allocates and initializes a filament-surface interaction for filament type filtype,
// with no surface, no filament parts, and no action. Returns it, or NULL if memory
// could not be allocated.
filsurfinteractptr filSurfInteractAlloc(filamenttypeptr filtype) {
	filsurfinteractptr fsi;
	int part;

	fsi=(filsurfinteractptr) malloc(sizeof(struct filsurfinteractstruct));
	if(!fsi) return NULL;
	fsi->filtype=filtype;
	fsi->selfsurf=NULL;
	for(part=0;part<FILPARTMAX;part++) fsi->filpart[part]=0;
	fsi->sequence[0]='\0';
	fsi->filface=-1;
	fsi->othersurf=NULL;
	fsi->surfface=PFfront;
	fsi->compart=NULL;
	fsi->action=FSAnone;
	fsi->kforce=0;
	return fsi; }


/* filSurfInteractFree */
void filSurfInteractFree(filsurfinteractptr fsi) {
	free(fsi);
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
		filss->filtypes=NULL;
		filss->maxjunct=0;
		filss->njunct=0;
		filss->junctlist=NULL;
		filss->nspring=0; }

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
	int ft,j;

	if(!filss) return;

	if(filss->junctlist) {
		for(j=0;j<filss->maxjunct;j++)
			free(filss->junctlist[j]);
		free(filss->junctlist); }

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
	int seg,j,e,seqi,dim;
	char string1[STRCHAR],string2[STRCHAR];
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
			simLog(sim,seg>5?1:2,"    %i length=%1.3g, thick=%1.3g, minus pos.=(%1.3g %1.3g), rel. angle=%1.3g",segment->index,segment->len,segment->thk,segment->xyzminus[0],segment->xyzminus[1],segment->ypr[0]);
			simLog(sim,fil->nodemobility[seg]==1?1:2," minus mobility=%g",fil->nodemobility[seg]);
			simLog(sim,1,", plus pos.=(%1.3g %1.3g), qrel=(%1.3g %1.3g %1.3g %1.3g), qabs=(%1.3g %1.3g %1.3g %1.3g)",segment->xyzplus[0],segment->xyzplus[1],segment->qrel[0],segment->qrel[1],segment->qrel[2],segment->qrel[3],segment->qabs[0],segment->qabs[1],segment->qabs[2],segment->qabs[3]);
			simLog(sim,seg>5?1:2,"\n"); }
		segment=fil->segments[fil->nseg-1];
		simLog(sim,2,"     plus pos.=(%1.3g %1.3g)",segment->xyzplus[0],segment->xyzplus[1]);
		simLog(sim,fil->nodemobility[seg]==1?1:2," plus mobility=%g",fil->nodemobility[seg]);
		simLog(sim,2,"\n"); }
	else {
		for(seg=0;seg<fil->nseg;seg++) {
			segment=fil->segments[seg];
			simLog(sim,seg>5?1:2,"    %i length=%1.3g, thick=%1.3g, minus pos.=(%1.3g %1.3g %1.3g), rel. angle=(%1.3g %1.3g %1.3g)",segment->index,segment->len,segment->thk,segment->xyzminus[0],segment->xyzminus[1],segment->xyzminus[2],segment->ypr[0],segment->ypr[1],segment->ypr[2]);
			simLog(sim,fil->nodemobility[seg]==1?1:2," minus mobility=%g",fil->nodemobility[seg]);
			simLog(sim,1,", plus pos.=(%1.3g %1.3g %1.3g), qrel=(%1.3g %1.3g %1.3g %1.3g), qabs=(%1.3g %1.3g %1.3g %1.3g)",segment->xyzplus[0],segment->xyzplus[1],segment->xyzplus[2],segment->qrel[0],segment->qrel[1],segment->qrel[2],segment->qrel[3],segment->qabs[0],segment->qabs[1],segment->qabs[2],segment->qabs[3]);
			simLog(sim,seg>5?1:2,"\n"); }
		segment=fil->segments[fil->nseg-1];
		simLog(sim,2,"     plus pos.=(%1.3g %1.3g %1.3g)\n",segment->xyzplus[0],segment->xyzplus[1],segment->xyzplus[2]);
		simLog(sim,fil->nodemobility[seg]==1?1:2," plus mobility=%g",fil->nodemobility[seg]); }

	for(e=0;e<2;e++)
		if(fil->endjunct[e])
			simLog(sim,2,"   %s end attached to: %s:%s node %i, %s\n",e?"plus":"minus",fil->endjunct[e]->target->filtype->ftname,fil->endjunct[e]->target->filname,fil->endjunct[e]->node,fil->endjunct[e]->rigid?"rigid":"spring (closes a loop)");

	simLog(sim,(fil->endstate[0]!=FESopen || fil->endstate[1]!=FESopen)?2:1,"   end states: minus %s, plus %s\n",filFES2string(fil->endstate[0],string1),filFES2string(fil->endstate[1],string2));
	simLog(sim,(fil->lenbank[0]!=0 || fil->lenbank[1]!=0)?2:1,"   pending length change: minus %g|L, plus %g|L\n",fil->lenbank[0],fil->lenbank[1]);

	simLog(sim,1,"   allocated junctions: %i\n",fil->maxjunct);
	simLog(sim,fil->njunct>0?2:1,"   number of filament ends attached here: %i\n",fil->njunct);
	for(j=0;j<fil->njunct;j++)
		simLog(sim,2,"    %s:%s %s end at node %i, %s\n",fil->juncts[j]->fil->filtype->ftname,fil->juncts[j]->fil->filname,fil->juncts[j]->end=='p'?"plus":"minus",fil->juncts[j]->node,fil->juncts[j]->rigid?"rigid":"spring (closes a loop)");

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
	char string[STRCHAR],string2[STRCHAR],string3[STRCHAR];
	int fc,f,dim,end,fes,fes2,active,grows,i;
	double rtot;
	simptr sim;
	filsurfinteractptr fsi;

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
	simLog(sim,filtype->branchrate>0?2:1,"  branch rate: %g\n",filtype->branchrate);
	if(filtype->branchrate>0) {
		simLog(sim,2,"  branch angle: %g rad\n",filtype->branchangle);
		if(filtype->branchspread>=0) simLog(sim,filtype->branchspread>0?2:1,"  branch spread: %g\n",filtype->branchspread);
		else simLog(sim,1,"  branch spread: unset\n");
		simLog(sim,2,"  daughter segments at birth: %i\n",filtype->branchsegments);
		simLog(sim,filtype->branchend=='p'?2:1,"  daughters attach by their %s end\n",filtype->branchend=='p'?"plus":"minus");
		if(filtype->branchminspacing>0) simLog(sim,2,"  minimum branch spacing: %g|L\n",filtype->branchminspacing); }
	simLog(sim,filtype->branchforceangle>0?2:1,"  branch junction force constant: %g|E\n",filtype->branchforceangle);
	if(filtype->junctforce>0) simLog(sim,2,"  loop-closing junction force constant: %g|E/L\n",filtype->junctforce);
	else simLog(sim,1,"  loop-closing junction force constant: unset, so force_length is used\n");
	if(filtype->branchforceangle>0 && filtype->kT>0)								// equipartition spread the spring maintains
		simLog(sim,2,"  branch junction equilibrium spread: %g rad, for thickness 1\n",sqrt(filtype->kT/filtype->branchforceangle));
	if(filtype->branchazimuth>=0)
		simLog(sim,2,"  branch azimuth: %g rad\n",filtype->branchazimuth);
	simLog(sim,filtype->branchazimuthfix?2:1,"  branch azimuth fixed: %s\n",filtype->branchazimuthfix?"yes":"no");
	simLog(sim,filtype->thickdrag?1:2,"  thickness affects drag: %s\n",filtype->thickdrag?"yes":"no");
	simLog(sim,filtype->branchforceazimuth>0?2:1,"  branch azimuth force constant: %g|E\n",filtype->branchforceazimuth);
	if(filtype->branchforceazimuth>0 && filtype->kT>0)
		simLog(sim,2,"  branch azimuth equilibrium spread: %g rad, for thickness 1\n",sqrt(filtype->kT/filtype->branchforceazimuth));
	if(filtype->branchsrf)
		simLog(sim,2,"  branching gated to within %g|L of surface %s\n",filtype->branchsrfdist,filtype->branchsrf->sname);
	if(filtype->branchcmpt)
		simLog(sim,2,"  branching gated to inside compartment %s\n",filtype->branchcmpt->cname);
	simLog(sim,filtype->nsurfint>0?2:1,"  surface interactions: %i\n",filtype->nsurfint);
	for(i=0;i<filtype->nsurfint;i++) {
		fsi=filtype->surfint[i];
		simLog(sim,2,"   %s with %s face of surface %s, for filament parts %s",filFSA2string(fsi->action,string),surfface2string(fsi->surfface,string2),fsi->othersurf->sname,filparts2string(fsi->filpart,string3));
		if(fsi->action==FSAforce) {
			simLog(sim,2,", force constant %g|E/L\n",fsi->kforce);
			if(fsi->kforce>0 && filtype->kT>0)						// equipartition penetration depth the wall permits
				simLog(sim,2,"    equilibrium penetration spread: %g|L, for thickness 1\n",sqrt(filtype->kT/fsi->kforce)); }
		else simLog(sim,2,"\n"); }

	grows=0;
	for(end=0;end<2;end++) {														// end dynamics
		active=(filtype->endstate0[end]!=FESopen);
		for(fes=0;fes<FESMAX;fes++) {
			if(filtype->endrate[end][fes]!=0) active=1;
			if(filtype->endrate[end][fes]>0) grows=1;
			for(fes2=0;fes2<FESMAX;fes2++)
				if(filtype->endtrans[end][fes][fes2]>0) active=1; }
		simLog(sim,active?2:1,"  %s end: initial state %s\n",end?"plus":"minus",filFES2string(filtype->endstate0[end],string));
		for(fes=0;fes<FESMAX;fes++)
			if(filtype->endrate[end][fes]!=0)
				simLog(sim,2,"   %s state velocity: %g|L/T\n",filFES2string((enum FilEndState)fes,string),filtype->endrate[end][fes]);
		for(fes=0;fes<FESMAX;fes++)
			for(fes2=0;fes2<FESMAX;fes2++)
				if(filtype->endtrans[end][fes][fes2]>0)
					simLog(sim,2,"   %s to %s transition rate: %g|/T\n",filFES2string((enum FilEndState)fes,string),filFES2string((enum FilEndState)fes2,string2),filtype->endtrans[end][fes][fes2]);
		rtot=0;																						// the analytic target for a growth phase that ends by a transition
		for(fes2=0;fes2<FESMAX;fes2++) rtot+=filtype->endtrans[end][FESopen][fes2];
		if(filtype->endrate[end][FESopen]>0 && rtot>0)
			simLog(sim,2,"   mean length added per open period: %g|L\n",filtype->endrate[end][FESopen]/rtot); }
	if(grows && filtype->elongmaxlen>0)
		simLog(sim,2,"  elongation max length: %g|L\n",filtype->elongmaxlen);

	simLog(sim,2,"  mobility: %g\n",filtype->mobility);
	simLog(sim,2,"  filament radius: %g\n",filtype->filradius);

	if(filtype->nface>0) {
		simLog(sim,2,"  %i faces with twist of %g:",filtype->nface,filtype->facetwist);
		for(fc=0;fc<filtype->nface;fc++)
			simLog(sim,2," %s,",filtype->facename[fc]);
		simLog(sim,2,"\n"); }

	simLog(sim,1,"  allocated filaments: %i, of which %i are unused or removed\n",filtype->maxfil,filtype->maxfil-filtype->nfil);
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
	int error,warn,dim,f,seg,ft,pcount,end,fes,fes2,grows,changes,reach[FESMAX],more,j,j2,i;
	filsurfinteractptr fsi;
	double kappa,theta0,kaz,kj;
	filjunctionptr junct,junct2;
	double fval,ksum,mobility;
	enum PanelShape ps;
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
	for(j=0;j<filss->njunct;j++) {										// loop-closing junctions
		junct=filss->junctlist[j];
		if(junct->rigid) continue;
		filJunctionParams(junct,dim,&kappa,&theta0,&kaz,&kj);
		if(kj<=0) {
			warn++;simLog(sim,5,"WARNING: the %s end of filament %s closes a loop, but nothing holds it in place; set junction_force or force_length for filament type %s\n",junct->end=='p'?"plus":"minus",junct->fil->filname,junct->fil->filtype->ftname);}
		else if(junct->fil->filtype->dynamics==FDnone && junct->target->filtype->dynamics==FDnone) {
			warn++;simLog(sim,5,"WARNING: the %s end of filament %s closes a loop between filaments with dynamics none, so the junction spring has no effect\n",junct->end=='p'?"plus":"minus",junct->fil->filname);}
		else {
			fval=sim->dt*kj*(junct->fil->filtype->mobility>junct->target->filtype->mobility?junct->fil->filtype->mobility:junct->target->filtype->mobility);
			if(fval>0.5) {
				warn++;simLog(sim,5,"WARNING: the loop-closing junction at the %s end of filament %s may be numerically unstable, because time_step * mobility * junction spring constant = %g, which should be well below 1; reduce time_step or junction_force\n",junct->end=='p'?"plus":"minus",junct->fil->filname,fval);}}
		if(dim==3) {																	// azimuthal springs act on the attached filament only, so a loop can turn steadily
			fval=kaz;
			if(filRigidParent(junct->fil)) {
				filJunctionParams(filRigidParent(junct->fil),dim,&kappa,&theta0,&kaz,&kj);
				if(kaz>fval) fval=kaz; }
			if(fval>0) {
				warn++;simLog(sim,5,"WARNING: filament %s is in a loop and has a junction with an azimuthal spring (branch_force_azimuth), which can make the loop rotate steadily because the spring has no reaction on the filament it attaches to\n",junct->fil->filname);}}}
	for(j=0;j<filss->njunct;j++) {										// rigid junctions that transmit loop-closing junction springs
		junct=filss->junctlist[j];
		if(!junct->rigid || filss->nspring==0) continue;
		ksum=0;
		for(j2=0;j2<filss->njunct;j2++) {
			junct2=filss->junctlist[j2];
			if(junct2->rigid || filInSubtree(junct2->fil,junct->fil)==filInSubtree(junct2->target,junct->fil)) continue;	// springs within or outside the subtree aren't transmitted
			filJunctionParams(junct2,dim,&kappa,&theta0,&kaz,&kj);
			ksum+=kj; }
		mobility=junct->fil->filtype->mobility>junct->target->filtype->mobility?junct->fil->filtype->mobility:junct->target->filtype->mobility;
		fval=sim->dt*mobility*ksum;
		if(fval>0.5) {
			warn++;simLog(sim,5,"WARNING: the %s end of filament %s is rigidly attached, and its attachment transmits the forces of loop-closing junction springs with a total spring constant of %g, so time_step * mobility * total = %g, which may be numerically unstable; reduce time_step or junction_force\n",junct->end=='p'?"plus":"minus",junct->fil->filname,ksum,fval);}}
	for(ft=0;ft<filss->ntype;ft++) {
		filtype=filss->filtypes[ft];

		grows=changes=0;
		for(end=0;end<2;end++)
			for(fes=0;fes<FESMAX;fes++) {
				if(filtype->endrate[end][fes]>0) grows=1;
				if(filtype->endrate[end][fes]!=0) changes=1; }
		if(grows && filtype->stdlen<=0) {									// otherwise the banked-growth loop has a zero threshold
			error++;simLog(sim,9,"ERROR: filament type %s has a positive elongation_rate but standard_length<=0; elongation needs a positive standard_length\n",filtype->ftname);}
		if(filtype->treadrate!=0 && changes) {
			warn++;simLog(sim,5,"WARNING: filament type %s has both treadmill_rate and elongation_rate set; these compose as turnover plus net length change\n",filtype->ftname);}
		for(end=0;end<2;end++) {													// end states with velocities that are never reached
			for(fes=0;fes<FESMAX;fes++) reach[fes]=0;
			reach[filtype->endstate0[end]]=1;							// new filaments start here
			for(f=0;f<filtype->nfil;f++)										// existing filaments may have been set to other states
				reach[filtype->fillist[f]->endstate[end]]=1;
			for(more=1;more;) {
				more=0;
				for(fes=0;fes<FESMAX;fes++)
					if(reach[fes])
						for(fes2=0;fes2<FESMAX;fes2++)
							if(!reach[fes2] && filtype->endtrans[end][fes][fes2]>0) reach[fes2]=more=1; }
			for(fes=0;fes<FESMAX;fes++)
				if(!reach[fes] && filtype->endrate[end][fes]!=0) {
					warn++;simLog(sim,5,"WARNING: filament type %s sets an elongation_rate for the %s end in the %s state, but no filament starts in that state and no transition reaches it\n",filtype->ftname,end?"plus":"minus",filFES2string((enum FilEndState)fes,string));} }
		if(filtype->branchforceangle>0 && filtype->kT>0) {	// birth spread should match the spring's equilibrium spread (filUpdateParams derives it when unset)
			fval=sqrt(filtype->kT/filtype->branchforceangle);
			if(filtype->branchspread==0) {
				warn++;simLog(sim,5,"WARNING: filament type %s has deterministic branch births (branch_spread 0) with a junction spring; branches will relax toward spread %g after birth\n",filtype->ftname,fval);}
			else if(filtype->branchspread>2*fval || (filtype->branchspread>0 && filtype->branchspread<0.5*fval)) {
				warn++;simLog(sim,5,"WARNING: filament type %s branch_spread %g differs from the junction spring's equilibrium spread sqrt(kT/branch_force_angle) = %g by more than 2-fold; branches will relax visibly after birth\n",filtype->ftname,filtype->branchspread,fval);}}
		if(dim==2 && (filtype->branchazimuth>=0 || filtype->branchazimuthfix || filtype->branchforceazimuth>0)) {
			warn++;simLog(sim,5,"WARNING: filament type %s branch_azimuth settings have no effect in 2D\n",filtype->ftname);}
		if(filtype->branchazimuthfix && filtype->branchforceazimuth>0) {
			warn++;simLog(sim,5,"WARNING: filament type %s sets both branch_azimuth_fix and branch_force_azimuth; the rigid fix overrides the spring each step\n",filtype->ftname);}
		if(filtype->branchazimuthfix && filtype->dynamics!=FDnone && filtype->kT>0) {
			warn++;simLog(sim,5,"WARNING: filament type %s uses branch_azimuth_fix with thermal dynamics; the rigid pin slaves each branch subtree to its mother's fluctuating tangent and inflates junction-angle spreads in dense networks -- prefer branch_force_azimuth there\n",filtype->ftname);}
		if(filtype->branchforceazimuth>0 && filtype->branchforceangle<=0) {
			warn++;simLog(sim,5,"WARNING: filament type %s has branch_force_azimuth without branch_force_angle; nothing constrains the branch polar angle, and the azimuthal force loses authority (and is capped) as junctions wander toward parallel\n",filtype->ftname);}
		if(filtype->branchsrf && filtype->branchcmpt) {
			error++;simLog(sim,9,"ERROR: filament type %s sets both branch_surface and branch_compartment; use a single location gate\n",filtype->ftname);}
		if((filtype->branchsrf || filtype->branchcmpt) && filtype->branchrate<=0) {
			warn++;simLog(sim,5,"WARNING: filament type %s has a branch location gate but branch_rate is 0, so no branching will occur\n",filtype->ftname);}
		if(filtype->branchminspacing>0 && filtype->branchrate<=0) {
			warn++;simLog(sim,5,"WARNING: filament type %s has branch_min_spacing but branch_rate is 0; the spacing only applies to branch nucleation\n",filtype->ftname);}
		if(filtype->branchminspacing>0) {						// branches entered by the user are kept, but report ones that violate the spacing
			pcount=0;
			for(j=0;j<filss->njunct;j++) {
				junct=filss->junctlist[j];
				if(junct->target->filtype==filtype && !junct->bend && !filBranchNodeSpaced(junct->target,junct->node,filtype->branchminspacing,junct)) pcount++; }
			if(pcount>0) {
				warn++;simLog(sim,5,"WARNING: filament type %s has %i branches that are closer than branch_min_spacing to another branch; these are kept, and the spacing only applies to new branches from branch nucleation\n",filtype->ftname,pcount);}}
		if(filtype->branchsrf) {													// a gate bound to empty geometry closes silently otherwise
			pcount=0;
			for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1)) pcount+=filtype->branchsrf->npanel[ps];
			if(pcount==0) {
				warn++;simLog(sim,5,"WARNING: filament type %s branch_surface %s has no panels; the gate is closed and no branching will occur\n",filtype->ftname,filtype->branchsrf->sname);}}
		if(filtype->branchcmpt && filtype->branchcmpt->npts==0 && filtype->branchcmpt->ncmptl==0) {
			warn++;simLog(sim,5,"WARNING: filament type %s branch_compartment %s has no inside-defining points; the gate is closed and no branching will occur\n",filtype->ftname,filtype->branchcmpt->cname);}
		if(filtype->dynamics==FDeulermat) {					// test methods, not for production simulations
			warn++;simLog(sim,5,"WARNING: filament type %s uses dynamics EulerMat, which is a test method; it gives the same results as Euler but more slowly, so use Euler, RK4, or Implicit for simulations\n",filtype->ftname);}
		if(filtype->dynamics==FDimplicitold) {
			warn++;simLog(sim,5,"WARNING: filament type %s uses dynamics ImplicitOld, which is a test method; it can be unstable even with short time steps, so use Implicit for simulations\n",filtype->ftname);}
		for(i=0;i<filtype->nsurfint;i++) {						// filament-surface interactions
			fsi=filtype->surfint[i];
			pcount=0;
			for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1)) pcount+=fsi->othersurf->npanel[ps];
			if(pcount==0) {
				warn++;simLog(sim,5,"WARNING: filament type %s has a %s interaction with surface %s, which has no panels, so the interaction has no effect\n",filtype->ftname,filFSA2string(fsi->action,string),fsi->othersurf->sname);}
			if(fsi->action==FSAforce && fsi->kforce>0) {
				if(filtype->dynamics==FDnone) {
					warn++;simLog(sim,5,"WARNING: filament type %s has a force interaction with surface %s but dynamics none; the force is never applied\n",filtype->ftname,fsi->othersurf->sname);}
				fval=1;																			// thickest node, since the wall spring scales with node thickness
				for(f=0;f<filtype->nfil;f++)
					for(seg=0;seg<filtype->fillist[f]->nseg;seg++)
						if(filtype->fillist[f]->segments[seg]->thk>fval) fval=filtype->fillist[f]->segments[seg]->thk;
				if(filtype->mobility*fsi->kforce*fval*sim->dt>0.5) {		// explicit relaxation of the wall spring must be resolved; per-node mobility factors multiply this further
					warn++;simLog(sim,5,"WARNING: filament type %s force interaction with surface %s is stiff for this time step (mobility*force*thickness*dt = %g > 0.5, for the thickest node and before any node mobility factors); explicit integration may oscillate or blow up at the wall\n",filtype->ftname,fsi->othersurf->sname,filtype->mobility*fsi->kforce*fval*sim->dt);}}}

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

	else if(!strcmp(param,"junctforce")) {						// loop-closing junction spring; 0 = use klen
		if(value<0) er=2;
		else filtype->junctforce=value; }

	else if(!strcmp(param,"kypr")) {
		if(index<0) filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=value;
		else filtype->kypr[index]=value; }

	else if(!strcmp(param,"kT")) {
		if(value<0) er=2;
		else filtype->kT=value; }

	else if(!strcmp(param,"treadrate")) {
		filtype->treadrate=value; }

	else if(!strcmp(param,"branchrate")) {						// branch nucleation rate
		if(value<0) er=2;
		else filtype->branchrate=value; }

	else if(!strcmp(param,"branchangle"))
		filtype->branchangle=value;

	else if(!strcmp(param,"branchspread")) {					// angular spread of branch
		if(value<0) er=2;
		else {
			filtype->branchspread=value;
			filtype->branchspreadderived=0; }}

	else if(!strcmp(param,"branchsegments")) {				// segments per newborn daughter
		if(value<1) er=2;
		else filtype->branchsegments=(int)(value+0.5); }

	else if(!strcmp(param,"branchminspacing")) {			// minimum contour distance between branch points
		if(value<0) er=2;
		else filtype->branchminspacing=value; }

	else if(!strcmp(param,"branchforceangle")) {			// junction torsional spring constant
		if(value<0) er=2;
		else {
			filtype->branchforceangle=value;
			filSetCondition(filtype->filss,SCparams,0); }}	// so a runtime 'set' still reaches the branch_spread derivation in filUpdateParams

	else if(!strcmp(param,"branchazimuth")) {					// birth azimuth; negative = uniform random
		if(value<0) filtype->branchazimuth=-1;
		else filtype->branchazimuth=fmod(value,2*PI); }

	else if(!strcmp(param,"branchazimuthfix")) {			// hold azimuths at recorded birth values
		if(value!=0 && value!=1) er=2;
		else filtype->branchazimuthfix=(int)value; }

	else if(!strcmp(param,"thickdrag")) {							// thickness divides node mobility factors
		if(value!=0 && value!=1) er=2;
		else filtype->thickdrag=(int)value; }

	else if(!strcmp(param,"branchforceazimuth")) {		// azimuthal spring constant
		if(value<0) er=2;
		else filtype->branchforceazimuth=value; }

	else if(!strcmp(param,"branchsurfdist")) {				// capture distance for the branch_surface gate
		if(value<=0) er=2;
		else filtype->branchsrfdist=value; }

	else if(!strcmp(param,"endrate")) {								// end velocity, length/time; index = end*FESMAX+state
		if(index<0 || index>=2*FESMAX) er=2;
		else filtype->endrate[index/FESMAX][index%FESMAX]=value; }

	else if(!strcmp(param,"endtrans")) {							// end state transition rate, 1/time; index = (end*FESMAX+from)*FESMAX+to
		if(index<0 || index>=2*FESMAX*FESMAX || value<0) er=2;
		else if(index/FESMAX%FESMAX==index%FESMAX) er=2;	// no transition from a state to itself
		else filtype->endtrans[index/(FESMAX*FESMAX)][index/FESMAX%FESMAX][index%FESMAX]=value; }

	else if(!strcmp(param,"endstate0")) {							// initial end state; index = end, value = state
		if(index<0 || index>=2 || value<0 || value>=FESMAX) er=2;
		else filtype->endstate0[index]=(enum FilEndState)(int)value; }

	else if(!strcmp(param,"elongmaxlen")) {						// contour-length ceiling; 0 = unbounded
		if(value<0) er=2;
		else filtype->elongmaxlen=value; }

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


/* filtypeAddSurfInteract */
// Adds a filament-surface interaction to filament type filtype, between the filament
// parts in filpart (indexed by FPplus, FPminus, and FPside) and face face of surface
// srf, which is the side of the surface that the filament is on. action is the type
// of interaction and kforce is its force constant, for FSAforce. If the type already
// has an interaction with the same surface, face, parts, and action, then its
// parameters are replaced instead, so that entering an interaction again changes it.
// Returns the interaction, or NULL if memory could not be allocated.
filsurfinteractptr filtypeAddSurfInteract(filamenttypeptr filtype,surfaceptr srf,enum PanelFace face,const int *filpart,enum FilSurfAction action,double kforce) {
	filsurfinteractptr fsi,*newlist;
	int i,part,newmax;

	fsi=NULL;
	for(i=0;i<filtype->nsurfint && !fsi;i++) {
		fsi=filtype->surfint[i];
		if(fsi->othersurf!=srf || fsi->surfface!=face || fsi->action!=action) fsi=NULL;
		for(part=0;part<FILPARTMAX && fsi;part++)
			if(fsi->filpart[part]!=filpart[part]) fsi=NULL; }

	if(!fsi) {
		if(filtype->nsurfint==filtype->maxsurfint) {
			newmax=2*filtype->maxsurfint+1;
			newlist=(filsurfinteractptr*) calloc(newmax,sizeof(filsurfinteractptr));
			if(!newlist) return NULL;
			for(i=0;i<filtype->nsurfint;i++) newlist[i]=filtype->surfint[i];
			free(filtype->surfint);
			filtype->surfint=newlist;
			filtype->maxsurfint=newmax; }
		fsi=filSurfInteractAlloc(filtype);
		if(!fsi) return NULL;
		fsi->othersurf=srf;
		fsi->surfface=face;
		for(part=0;part<FILPARTMAX;part++) fsi->filpart[part]=filpart[part];
		fsi->action=action;
		filtype->surfint[filtype->nsurfint++]=fsi; }

	fsi->kforce=kforce;
	return fsi; }


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
// Derived parameters. If the junction spring is on and branch_spread was never set,
// derive it as the spring's own equilibrium spread sqrt(kT/branch_force_angle) so
// branches are born in the distribution the spring maintains (equipartition ties the
// two numbers). This runs on every entry path via simupdate -- config file, libsmoldyn,
// Python -- unlike filCheckParams, which only diagnostics paths reach. Idempotent: the
// derivation replaces the negative unset sentinel with a positive value, and an
// explicit branch_spread (including 0 = deterministic births) is never overridden.
int filUpdateParams(simptr sim) {
	int ft;
	filamenttypeptr filtype;

	for(ft=0;ft<sim->filss->ntype;ft++) {
		filtype=sim->filss->filtypes[ft];
		if(filtype->branchspread<0 && filtype->branchforceangle>0 && filtype->kT>0) {
			filtype->branchspread=sqrt(filtype->kT/filtype->branchforceangle);
			filtype->branchspreadderived=1;						// at birth, divided by the square root of the thickness, because the spring scales with thickness
			simLog(sim,2,"branch_spread for filament type %s set to sqrt(kT/branch_force_angle) = %g, for thickness 1\n",filtype->ftname,filtype->branchspread); }}
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
	char nm[STRCHARLONG],nm1[STRCHARLONG],nm2[STRCHARLONG],nm3[STRCHARLONG];
	double fltv1[9],f1;
	enum FilEndState fes1,fes2;
	enum DrawMode dm;
	enum FilamentDynamics fd;
	enum PanelFace pf;
	enum FilSurfAction fsa;
	int filpart[FILPARTMAX];

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
		CHECKM(itct==1,"thickness value is missing. ");
		CHECKS(f1>=0,"thickness value needs to be at least 0");
		er=filtypeSetEdgePts(filtype,f1);
		CHECKS(!er,"BUG: error in filtypeSetEdgePts");
		CHECKS(!strnword(line2,2),"unexpected text following thickness"); }

	else if(!strcmp(word,"stipple")) {					// stipple
		CHECKS(filtype,"need to enter filament type name before stipple");
		itct=strmathsscanf(line2,"%mi %mi",varnames,varvalues,nvar,&i1,&i2);
		CHECKM(itct==2,"stipple format: factor pattern. ");
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
		CHECKM(itct==1,"shininess format: value. ");
		CHECKS(f1>=0 && f1<=128,"shininess value needs to be between 0 and 128");
		er=filtypeSetShiny(filtype,f1);
		CHECKS(!er,"BUG: error in filtypeSetShiny");
		CHECKS(!strnword(line2,2),"unexpected text following shininess"); }

	else if(!strcmp(word,"force_arrows")) {			// force_arrows
		CHECKS(filtype,"need to enter filament type name before force_arrows");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"force_arrows format: scale color. ");
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
		CHECKM(itct==1,"kT format: value. ");
		CHECKS(f1>=0,"kT value needs to be >=0");
		filtypeSetParam(filtype,"kT",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following kT"); }

	else if(!strcmp(word,"treadmill_rate")) {		// treadmill_rate
		CHECKS(filtype,"need to enter filament type name before treadmill_rate");
		itct=strmathsscanf(line2,"%mlg|/T",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"treadmill_rate format: value. ");
		filtypeSetParam(filtype,"treadrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following treadmill_rate"); }

	else if(!strcmp(word,"branch_rate")) {			// branch_rate (per unit length per time)
		CHECKS(filtype,"need to enter filament type name before branch_rate");
		itct=strmathsscanf(line2,"%mlg|/L/T",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_rate format: value. ");
		CHECKS(f1>=0,"branch_rate value needs to be >=0");
		filtypeSetParam(filtype,"branchrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_rate"); }

	else if(!strcmp(word,"branch_angle")) {			// branch_angle: daughter angle off the mother, radians
		CHECKS(filtype,"need to enter filament type name before branch_angle");
		itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_angle format: value. ");
		filtypeSetParam(filtype,"branchangle",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_angle"); }

	else if(!strcmp(word,"branch_spread")) {		// branch_spread: angular std dev, radians
		CHECKS(filtype,"need to enter filament type name before branch_spread");
		itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_spread format: value. ");
		CHECKS(f1>=0,"branch_spread value needs to be >=0");
		filtypeSetParam(filtype,"branchspread",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_spread"); }

	else if(!strcmp(word,"branch_segments")) {	// branch_segments: daughter length in segments
		CHECKS(filtype,"need to enter filament type name before branch_segments");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKM(itct==1,"branch_segments format: number. ");
		CHECKS(i1>=1,"branch_segments value needs to be >=1");
		filtypeSetParam(filtype,"branchsegments",0,(double)i1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_segments"); }

	else if(!strcmp(word,"branch_min_spacing")) {	// branch_min_spacing: minimum contour distance between branch points
		CHECKS(filtype,"need to enter filament type name before branch_min_spacing");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_min_spacing format: value. ");
		CHECKS(f1>=0,"branch_min_spacing value needs to be >=0");
		filtypeSetParam(filtype,"branchminspacing",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_min_spacing"); }

	else if(!strcmp(word,"branch_end")) {				// branch_end: which daughter end attaches to the mother
		CHECKS(filtype,"need to enter filament type name before branch_end");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"branch_end format: minus or plus");
		if(!strcmp(nm1,"minus")) filtype->branchend='m';
		else if(!strcmp(nm1,"plus")) filtype->branchend='p';
		else CHECKS(0,"branch_end options: minus, plus");
		CHECKS(!strnword(line2,2),"unexpected text following branch_end"); }

	else if(!strcmp(word,"branch_force_angle")) {	// branch_force_angle: junction torsional spring constant, energy/rad^2
		CHECKS(filtype,"need to enter filament type name before branch_force_angle");
		itct=strmathsscanf(line2,"%mlg|E",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_force_angle format: value. ");
		CHECKS(f1>=0,"branch_force_angle value needs to be >=0");
		filtypeSetParam(filtype,"branchforceangle",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_force_angle"); }

	else if(!strcmp(word,"branch_azimuth")) {			// branch_azimuth: birth azimuth about the mother axis, radians, or 'random'
		CHECKS(filtype,"need to enter filament type name before branch_azimuth");
		itct=sscanf(line2,"%s",nm1);
		CHECKM(itct==1,"branch_azimuth format: value or random. ");
		if(!strcmp(nm1,"random"))
			filtypeSetParam(filtype,"branchazimuth",0,-1);
		else {
			itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);
			CHECKM(itct==1,"branch_azimuth format: value or random. ");
			CHECKS(f1>=0,"branch_azimuth value needs to be >=0, or the word random");
			filtypeSetParam(filtype,"branchazimuth",0,f1); }
		CHECKS(!strnword(line2,2),"unexpected text following branch_azimuth"); }

	else if(!strcmp(word,"branch_azimuth_fix")) {	// branch_azimuth_fix: hold azimuths at birth values
		CHECKS(filtype,"need to enter filament type name before branch_azimuth_fix");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKM(itct==1,"branch_azimuth_fix format: 0 or 1. ");
		CHECKS(i1==0 || i1==1,"branch_azimuth_fix value needs to be 0 or 1");
		filtypeSetParam(filtype,"branchazimuthfix",0,(double)i1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_azimuth_fix"); }

	else if(!strcmp(word,"thickness_drag")) {				// thickness_drag: whether thickness divides node mobility
		CHECKS(filtype,"need to enter filament type name before thickness_drag");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKM(itct==1,"thickness_drag format: 0 or 1. ");
		CHECKS(i1==0 || i1==1,"thickness_drag value needs to be 0 or 1");
		filtypeSetParam(filtype,"thickdrag",0,(double)i1);
		CHECKS(!strnword(line2,2),"unexpected text following thickness_drag"); }

	else if(!strcmp(word,"branch_force_azimuth")) {	// branch_force_azimuth: azimuthal spring constant, energy/rad^2
		CHECKS(filtype,"need to enter filament type name before branch_force_azimuth");
		itct=strmathsscanf(line2,"%mlg|E",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_force_azimuth format: value. ");
		CHECKS(f1>=0,"branch_force_azimuth value needs to be >=0");
		filtypeSetParam(filtype,"branchforceazimuth",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following branch_force_azimuth"); }

	else if(!strcmp(word,"branch_surface")) {			// branch_surface: location gate, surface name + capture distance
		CHECKS(filtype,"need to enter filament type name before branch_surface");
		CHECKS(line2,"branch_surface format: surface_name distance");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"branch_surface format: surface_name distance");
		CHECKS(sim->srfss,"branch_surface requires that surfaces be defined first");
		i1=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm1);
		CHECKS(i1>=0,"branch_surface surface name not recognized (define the surface before the filament type)");
		line2=strnword(line2,2);
		CHECKS(line2,"branch_surface format: surface_name distance");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"branch_surface format: surface_name distance");
		CHECKS(f1>0,"branch_surface distance needs to be >0");
		CHECKS(!strnword(line2,2),"unexpected text following branch_surface");
		filtype->branchsrf=sim->srfss->srflist[i1];	// parse-time binding, like reaction_cmpt; committed only after full validation so a failed runtime set changes nothing
		filtypeSetParam(filtype,"branchsurfdist",0,f1); }

	else if(!strcmp(word,"branch_compartment")) {	// branch_compartment: location gate, compartment name
		CHECKS(filtype,"need to enter filament type name before branch_compartment");
		CHECKS(line2,"branch_compartment format: compartment_name");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"branch_compartment format: compartment_name");
		CHECKS(sim->cmptss,"branch_compartment requires that compartments be defined first");
		i1=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm1);
		CHECKS(i1>=0,"branch_compartment compartment name not recognized (define the compartment before the filament type)");
		CHECKS(!strnword(line2,2),"unexpected text following branch_compartment");
		filtype->branchcmpt=sim->cmptss->cmptlist[i1]; }	// parse-time binding, like reaction_cmpt

	else if(!strcmp(word,"confine_surface")) {		// confine_surface: harmonic confinement, surface name + spring constant (energy/length^2) + optional face
		CHECKS(filtype,"need to enter filament type name before confine_surface");
		CHECKS(line2,"confine_surface format: surface_name force_constant [front/back]");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"confine_surface format: surface_name force_constant [front/back]");
		CHECKS(sim->srfss,"confine_surface requires that surfaces be defined first");
		i1=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm1);
		CHECKS(i1>=0,"confine_surface surface name not recognized (define the surface before the filament type)");
		line2=strnword(line2,2);
		CHECKS(line2,"confine_surface format: surface_name force_constant [front/back]");
		itct=strmathsscanf(line2,"%mlg|E/L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"confine_surface format: surface_name force_constant [front/back]");
		CHECKS(f1>=0,"confine_surface force constant needs to be >=0");
		pf=PFfront;																	// default face
		line2=strnword(line2,2);
		if(line2) {																	// optional face: which side nodes are confined to
			itct=sscanf(line2,"%s",nm1);
			CHECKS(itct==1,"confine_surface format: surface_name force_constant [front/back]");
			pf=surfstring2face(nm1);
			CHECKS(pf==PFfront || pf==PFback,"confine_surface face options: front, back");
			CHECKS(!strnword(line2,2),"unexpected text following confine_surface"); }
		filpart[FPplus]=filpart[FPminus]=filpart[FPside]=1;	// shorthand for a force interaction on all parts; committed only after full validation so a failed runtime set changes nothing
		CHECKS(filtypeAddSurfInteract(filtype,sim->srfss->srflist[i1],pf,filpart,FSAforce,f1),"out of memory adding confine_surface"); }

	else if(!strcmp(word,"surface_interaction")) {	// surface_interaction: surface face parts action [parameters]
		CHECKS(filtype,"need to enter filament type name before surface_interaction");
		itct=sscanf(line2,"%s %s %s %s",nm1,nm2,nm,nm3);
		CHECKS(itct==4,"surface_interaction format: surface_name face parts action [parameters]");
		CHECKS(sim->srfss,"surface_interaction requires that surfaces be defined first");
		i1=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm1);
		CHECKS(i1>=0,"surface_interaction surface name not recognized (define the surface before the filament type)");
		pf=surfstring2face(nm2);
		CHECKS(pf==PFfront || pf==PFback || pf==PFboth,"surface_interaction face options: front, back, both");
		CHECKS(!filstring2parts(nm,filpart),"surface_interaction parts need to be plus, minus, side, all, or a comma-separated list of them, such as plus,side");
		fsa=filstring2FSA(nm3);
		CHECKS(fsa==FSAforce || fsa==FSAblock,"surface_interaction action options: force, block");
		line2=strnword(line2,5);
		f1=0;
		if(fsa==FSAforce) {
			CHECKS(pf!=PFboth,"surface_interaction force needs a front or back face, which is the side that the filament is on");
			CHECKS(line2,"surface_interaction force format: surface_name face parts force force_constant");
			itct=strmathsscanf(line2,"%mlg|E/L",varnames,varvalues,nvar,&f1);
			CHECKM(itct==1,"surface_interaction force format: surface_name face parts force force_constant");
			CHECKS(f1>=0,"surface_interaction force constant needs to be >=0");
			line2=strnword(line2,2); }
		else if(fsa==FSAblock) {
			CHECKS(!filpart[FPside] || !strcmp(nm,"all"),"surface_interaction block only applies to filament ends, so its parts need to be plus, minus, or all");
			filpart[FPside]=0; }												// all means both ends for blocking
		CHECKS(!line2,"unexpected text following surface_interaction");
		CHECKS(filtypeAddSurfInteract(filtype,sim->srfss->srflist[i1],pf,filpart,fsa,f1),"out of memory adding surface_interaction"); }

	else if(!strcmp(word,"elongation_rate")) {	// elongation_rate: [end state] velocity, length/time
		CHECKS(filtype,"need to enter filament type name before elongation_rate");
		if(!strnword(line2,2)) {										// short form: plus end velocity in the open state
			itct=strmathsscanf(line2,"%mlg|L/T",varnames,varvalues,nvar,&f1);
			CHECKM(itct==1,"elongation_rate format: [end state] value. ");
			CHECKS(f1>=0,"elongation_rate value needs to be >=0");
			i1=1;
			fes1=FESopen; }
		else {
			itct=sscanf(line2,"%s %s",nm,nm1);
			CHECKS(itct==2,"elongation_rate format: [end state] value");
			i1=filstring2end(nm);
			CHECKS(i1>=0,"elongation_rate end needs to be minus or plus");
			fes1=filstring2FES(nm1);
			CHECKS(fes1!=FESnone,"elongation_rate state needs to be open, capped, or shrink");
			line2=strnword(line2,3);
			CHECKS(line2,"elongation_rate format: [end state] value");
			itct=strmathsscanf(line2,"%mlg|L/T",varnames,varvalues,nvar,&f1);
			CHECKM(itct==1,"elongation_rate format: [end state] value. "); }
		filtypeSetParam(filtype,"endrate",i1*FESMAX+fes1,f1);
		CHECKS(!strnword(line2,2),"unexpected text following elongation_rate"); }

	else if(!strcmp(word,"end_transition")) {		// end_transition: end from to rate, 1/time
		CHECKS(filtype,"need to enter filament type name before end_transition");
		itct=sscanf(line2,"%s %s %s",nm,nm1,nm2);
		CHECKS(itct==3,"end_transition format: end from_state to_state rate");
		i1=filstring2end(nm);
		CHECKS(i1>=0,"end_transition end needs to be minus or plus");
		fes1=filstring2FES(nm1);
		fes2=filstring2FES(nm2);
		CHECKS(fes1!=FESnone && fes2!=FESnone,"end_transition states need to be open, capped, or shrink");
		CHECKS(fes1!=fes2,"end_transition states need to be different");
		line2=strnword(line2,4);
		CHECKS(line2,"end_transition format: end from_state to_state rate");
		itct=strmathsscanf(line2,"%mlg|/T",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"end_transition format: end from_state to_state rate. ");
		CHECKS(f1>=0,"end_transition rate needs to be >=0");
		filtypeSetParam(filtype,"endtrans",(i1*FESMAX+fes1)*FESMAX+fes2,f1);
		CHECKS(!strnword(line2,2),"unexpected text following end_transition"); }

	else if(!strcmp(word,"end_state")) {				// end_state: end state, the initial state for new filaments
		CHECKS(filtype,"need to enter filament type name before end_state");
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"end_state format: end state");
		i1=filstring2end(nm);
		CHECKS(i1>=0,"end_state end needs to be minus or plus");
		fes1=filstring2FES(nm1);
		CHECKS(fes1!=FESnone,"end_state state needs to be open, capped, or shrink");
		filtypeSetParam(filtype,"endstate0",i1,(double)fes1);
		CHECKS(!strnword(line2,3),"unexpected text following end_state"); }

	else if(!strcmp(word,"elongation_max_length")) {	// elongation_max_length: contour-length ceiling
		CHECKS(filtype,"need to enter filament type name before elongation_max_length");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"elongation_max_length format: value. ");
		CHECKS(f1>=0,"elongation_max_length value needs to be >=0");
		filtypeSetParam(filtype,"elongmaxlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following elongation_max_length"); }

	else if(!strcmp(word,"capping_rate")) {		// capping_rate: plus end open to capped, 1/time
		CHECKS(filtype,"need to enter filament type name before capping_rate");
		itct=strmathsscanf(line2,"%mlg|/T",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"capping_rate format: value. ");
		CHECKS(f1>=0,"capping_rate value needs to be >=0");
		filtypeSetParam(filtype,"endtrans",(1*FESMAX+FESopen)*FESMAX+FEScapped,f1);
		CHECKS(!strnword(line2,2),"unexpected text following capping_rate"); }

	else if(!strcmp(word,"uncapping_rate")) {	// uncapping_rate: plus end capped to open, 1/time
		CHECKS(filtype,"need to enter filament type name before uncapping_rate");
		itct=strmathsscanf(line2,"%mlg|/T",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"uncapping_rate format: value. ");
		CHECKS(f1>=0,"uncapping_rate value needs to be >=0");
		filtypeSetParam(filtype,"endtrans",(1*FESMAX+FEScapped)*FESMAX+FESopen,f1);
		CHECKS(!strnword(line2,2),"unexpected text following uncapping_rate"); }

	else if(!strcmp(word,"mobility")) {			// mobility
		CHECKS(filtype,"need to enter filament type name before mobility");
		itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&f1);				//?? Units are incorrect
		CHECKM(itct==1,"mobility format: value. ");
		CHECKS(f1>0,"mobility value needs to be greater than 0");
		filtypeSetParam(filtype,"mobility",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following mobility"); }

	else if(!strcmp(word,"standard_length")) {	// standard_length
		CHECKS(filtype,"need to enter filament type name before standard_length");
		itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"standard_length format: value. ");
		CHECKS(f1>=0,"standard_length value needs to be >=0");
		filtypeSetParam(filtype,"stdlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following standard_length"); }

	else if(!strcmp(word,"standard_angle")) {	// standard_angle
		CHECKS(filtype,"need to enter filament type name before standard_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&fltv1[0]);
			CHECKM(itct==1,"standard_angle format: angle. ");
			fltv1[1]=fltv1[2]=0; }
		else {
			itct=strmathsscanf(line2,"%mlg| %mlg| %mlg|",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKM(itct==3,"standard_angle format: yaw pitch roll. "); }
		filtypeSetParam(filtype,"stdypr",0,fltv1[0]);
		filtypeSetParam(filtype,"stdypr",1,fltv1[1]);
		filtypeSetParam(filtype,"stdypr",2,fltv1[2]);
		CHECKS(!strnword(line2,dim==2?2:4),"unexpected text following standard_angle"); }

	else if(!strcmp(word,"force_length")) {		// force_length
		CHECKS(filtype,"need to enter filament type name before force_length");
		itct=strmathsscanf(line2,"%mlg|E/L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"force_length format: value. ");
		filtypeSetParam(filtype,"klen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following force_length"); }

	else if(!strcmp(word,"junction_force")) {		// junction_force: spring for loop-closing junctions
		CHECKS(filtype,"need to enter filament type name before junction_force");
		itct=strmathsscanf(line2,"%mlg|E/L",varnames,varvalues,nvar,&f1);
		CHECKM(itct==1,"junction_force format: value. ");
		CHECKS(f1>=0,"junction_force value needs to be >=0");
		filtypeSetParam(filtype,"junctforce",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following junction_force"); }

	else if(!strcmp(word,"force_angle")) {		// force_angle
		CHECKS(filtype,"need to enter filament type name before force_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|E",varnames,varvalues,nvar,&fltv1[0]);
			CHECKM(itct==1,"force_angle format: value. ");
			fltv1[1]=fltv1[2]=-1; }
		else {
			itct=strmathsscanf(line2,"%mlg|E %mlg|E %mlg|E",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKM(itct==3,"force_angle format: yaw pitch roll. "); }
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
	int itct,er,i1,seg,node;
	char nm[STRCHAR],nm1[STRCHAR],endchar,str1[STRCHAR],str2[STRCHAR],str3[STRCHAR],str4[STRCHAR],str5[STRCHAR],str6[STRCHAR],symbol;
	double fltv1[9],length,angle[3],thick,f1,pos[3];
	enum FilEndState fes;

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
			CHECKM(itct==4,"first_segment format: x y length angle [thickness]. "); }
		else {
			itct=strmathsscanf(line2,"%mlg|L %mlg|L %mlg|L %mlg|L %mlg| %mlg| %mlg|",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2],&length,&angle[0],&angle[1],&angle[2]);
			CHECKM(itct==7,"first_segment format: x y z length yaw pitch roll [thickness]. "); }
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,itct+1);
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			if(dim==2) {CHECKM(itct==1,"first_segment format: x y length angle [thickness]. ");}
			else {CHECKM(itct==1,"first_segment format: x y z length yaw pitch roll [thickness]. ");}
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddSegment(fil,fltv1,length,angle,thick,'p');
		CHECKS(er==0,"BUG: error in filAddsegment");
		CHECKS(!line2,"unexpected text following first_segment"); }

	else if(!strcmp(word,"add_segment")) {			// add_segment
		CHECKS(fil,"need to enter filament name before add_segment");
		CHECKS(fil->nseg>0,"use first_segment to enter the first segment");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg|L %mlg|",varnames,varvalues,nvar,&length,&angle[0]);
			angle[1]=angle[2]=0;
			CHECKM(itct==2,"add_segment format: length angle0 [thickness [plus/minus]]. "); }
		else {
			itct=strmathsscanf(line2,"%mlg|L %mlg| %mlg| %mlg|",varnames,varvalues,nvar,&length,&angle[0],&angle[1],&angle[2]);
			CHECKM(itct==4,"add_segment format: length angle0 angle1 angle2 [thickness [plus/minus]]. "); }
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,itct+1);
		thick=1;
		endchar='p';
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			if(dim==2) {CHECKM(itct==1,"add_segment format: length angle0 [thickness [plus/minus]]. ");}
			else {CHECKM(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [plus/minus]]. ");}
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2);
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				if(dim==2) {CHECKS(itct==1,"add_segment format: length angle0 [thickness [plus/minus]]");}
				else {CHECKS(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [plus/minus]]");}
				if(nm1[0]=='P' || nm1[0]=='p') endchar='p';
				else if(nm1[0]=='M' || nm1[0]=='m') endchar='m';
				else CHECKS(0,"end needs to be 'plus' or 'minus'");
				line2=strnword(line2,2); }}
		er=filAddSegment(fil,NULL,length,angle,thick,endchar);
		CHECKS(er==0,"BUG: error in filAddsegment");
		CHECKS(!line2,"unexpected text following add_segment"); }

	else if(!strcmp(word,"remove_segment")) {			// remove_segment
		CHECKS(fil,"need to enter filament name before remove_segment");
		CHECKS(fil->nseg>0,"filament has no segments to remove");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"remove_segment format: plus/minus");
		endchar='p';
		if(nm1[0]=='P' || nm1[0]=='p') endchar='p';
		else if(nm1[0]=='M' || nm1[0]=='m') endchar='m';
		else CHECKS(0,"end needs to be 'plus' or 'minus'");
		er=filRemoveSegment(fil,endchar);
		CHECKS(!er,"BUG: error in filRemovesegment");
		CHECKS(!strnword(line2,2),"unexpected text following remove_segment"); }

	else if(!strcmp(word,"random_segments")) {		// random_segments
		CHECKS(fil,"need to enter filament name before random_segments");
		CHECKS(fil->filtype,"need to enter filament type before random_segments");
		CHECKS(fil->filtype->klen==-1 || fil->filtype->klen>0,"cannot compute random segments because the filament type has length force constant equal to 0");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKM(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]. ");
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
			if(dim==2) {CHECKM(itct==1,"random_segments format: number [x y phi] [thickness]. ");}
			else {CHECKM(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]. ");}
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
			CHECKM(itct==3,"translate format: symbol x y. ");
			fltv1[2]=0;
			line2=strnword(line2,4); }
		else {
			itct=strmathsscanf(line2,"%c %mlg|L %mlg|L %mlg|L",varnames,varvalues,nvar,&symbol,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKM(itct==4,"translate format: symbol x y z. ");
			line2=strnword(line2,5); }
		CHECKS(symbol=='=' || symbol=='+' || symbol=='-',"symbol needs to be '=', '+', or '-'");
		filTranslate(fil,fltv1,symbol);
		CHECKS(!line2,"unexpected text following translate"); }

	else if(!strcmp(word,"modify_segment")) {				// modify_segment
		CHECKS(fil,"need to enter filament name before modify_segment");
		itct=strmathsscanf(line2,"%mi| %s %c",varnames,varvalues,nvar,&seg,nm,&symbol);
		CHECKM(itct==3,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]. ");
		CHECKS(seg>=0 && seg<fil->nseg,"segment number is not within filament");
		line2=strnword(line2,4);
		CHECKS(line2,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]");
		CHECKS(symbol=='=' || symbol=='+' || symbol=='-',"symbol needs to be '=', '+', or '-'");
		if(!strcmp(nm,"length")) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&length);
			CHECKM(itct==1,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]. ");
			line2=strnword(line2,2);
			endchar='p';
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]");
				line2=strnword(line2,2);
				if(nm1[0]=='P' || nm1[0]=='p') endchar='p';
				else if(nm1[0]=='M' || nm1[0]=='m') endchar='m';
				else CHECKS(0,"end needs to be 'plus' or 'minus'"); }
			er=filLengthenSegment(fil,seg,length,endchar,symbol);
			CHECKS(!er,"Error while trying to change segment length - new length must be positive"); }
		else if(!strcmp(nm,"angle")) {
			angle[0]=angle[1]=angle[2]=0;
			if(dim==2) {
				itct=strmathsscanf(line2,"%mlg|",varnames,varvalues,nvar,&angle[0]);
				CHECKM(itct==1,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]. ");
				line2=strnword(line2,2); }
			else {
				itct=strmathsscanf(line2,"%mlg| %mlg| %mlg|",varnames,varvalues,nvar,&angle[0],&angle[1],&angle[2]);
				CHECKM(itct==3,"insufficient angle values. ");
				line2=strnword(line2,4); }
			endchar='p';
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]");
				line2=strnword(line2,2);
				if(nm1[0]=='P' || nm1[0]=='p') endchar='p';
				else if(nm1[0]=='M' || nm1[0]=='m') endchar='m';
				else CHECKS(0,"end needs to be 'plus' or 'minus'"); }
			filRotateVertex(fil,seg,angle,endchar,symbol); }
		else if(!strcmp(nm,"minus_position")) {
			pos[0]=pos[1]=pos[2]=0;
			if(dim==2) {
				itct=strmathsscanf(line2,"%mlg|L %mlg|L",varnames,varvalues,nvar,&pos[0],&pos[1]);
				CHECKM(itct==2,"insufficient position values. ");
				line2=strnword(line2,3); }
			else {
				itct=strmathsscanf(line2,"%mlg|L %mlg|L %mlg|L",varnames,varvalues,nvar,&pos[0],&pos[1],&pos[2]);
				CHECKM(itct==3,"insufficient position values. ");
				line2=strnword(line2,4); }
			filTranslateNode(fil,seg,pos,symbol); }
		else if(!strcmp(nm,"thickness")) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&thick);
			CHECKM(itct==1,"modify_segment format: segment length/angle/thickness/minus_position +/-/= value [plus/minus]. ");
			line2=strnword(line2,2);
			er=filChangeThickness(fil,seg,thick,symbol);
			CHECKS(!er,"Error while trying to change segment thickness - new value cannot be negative"); }
		else
			CHECKS(0,"modification option needs to be 'length', 'angle', 'minus_position', or 'thickness'");
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

	else if(!strcmp(word,"branch")) {							// branch: daughtername node [minus/plus] [length [yaw pitch roll]]
		CHECKS(fil,"need to enter filament name before branch");
		CHECKS(fil->nseg>0,"mother filament has no segments to branch from");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"branch format: daughter_name node [minus/plus] [length [yaw pitch roll]]");
		line2=strnword(line2,2);
		CHECKS(line2,"branch format: daughter_name node [minus/plus] [length [yaw pitch roll]]");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&node);
		CHECKM(itct==1,"branch format: daughter_name node [minus/plus] [length [yaw pitch roll]]. ");
		CHECKS(node>=0 && node<=fil->nseg,"branch node needs to be from 0 to the number of mother segments");
		angle[0]=fil->filtype->branchangle;						// default to the filament type's branch angle
		angle[1]=angle[2]=0;
		line2=strnword(line2,2);
		endchar=fil->filtype->branchend;							// default to the filament type's branch end
		if(line2) {
			sscanf(line2,"%s",nm);
			if(!strcmp(nm,"minus") || !strcmp(nm,"plus")) {
				endchar=nm[0];
				line2=strnword(line2,2); }}
		length=0;																			// 0 = random length for segment 0
		if(line2) {
			itct=strmathsscanf(line2,"%mlg|L",varnames,varvalues,nvar,&length);
			CHECKM(itct==1,"branch format: daughter_name node [minus/plus] [length [yaw pitch roll]]. ");
			CHECKS(length>0,"branch length needs to be >0");
			line2=strnword(line2,2); }
		if(line2) {
			itct=strmathsscanf(line2,"%mlg| %mlg| %mlg|",varnames,varvalues,nvar,&angle[0],&angle[1],&angle[2]);
			CHECKM(itct==3 || itct==1,"branch angle needs 1 or 3 values. ");
			if(dim==2) angle[1]=angle[2]=0;
			line2=strnword(line2,itct+1); }
		thick=fil->segments[node>0?node-1:0]->thk;
		fil2=filAddBranch(sim,fil,node,endchar,length,angle,thick,nm1);
		CHECKS(fil2,"failed to create branch");
		CHECKS(!line2,"unexpected text following branch"); }

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
		CHECKM(itct==2,"node_mobility format: node value. ");
		CHECKS(i1<=fil->nseg && i1>-fil->nseg,"node number is outside of filment size");
		CHECKS(f1>=0,"mobility value needs to be >=0");
		er=filSetNodeMobility(fil,i1,f1);
		CHECKS(!er,"error while trying to set node mobility");
		CHECKS(!strnword(line2,3),"unexpected text following node_mobility"); }

	else if(!strcmp(word,"attach")) {						// attach: end target node
		CHECKS(fil,"need to enter filament name before attach");
		CHECKS(fil->nseg>0,"filament has no segments to attach");
		itct=sscanf(line2,"%s %s",str1,str2);
		CHECKS(itct==2,"attach format: end target_filament node");
		i1=filstring2end(str1);
		CHECKS(i1>=0,"attach end needs to be minus or plus");
		CHECKS(!fil->endjunct[i1],"this filament end is already attached; detach it first");
		filtype2=NULL;
		fil2=NULL;
		er=filReadFilName(sim,str2,&filtype2,&fil2,NULL);
		CHECKS(er!=-1 && er!=-3,"cannot read target filament name");
		CHECKS(er!=-4,"unknown target filament type");
		CHECKS(er!=-5,"unknown target filament name");
		CHECKS(fil2->nseg>0,"target filament has no segments");
		line2=strnword(line2,3);
		CHECKS(line2,"attach format: end target_filament node");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&node);
		CHECKM(itct==1,"attach format: end target_filament node. ");
		CHECKS(node>=0 && node<=fil2->nseg,"attach node needs to be from 0 to the number of target filament segments");
		CHECKS(!(fil2==fil && node==(i1?fil->nseg:0)),"a filament end cannot attach to itself");
		er=filAttachEnd(fil,filend2char(i1),fil2,node);
		CHECKS(!er,"failed to attach filament end");
		CHECKS(!strnword(line2,2),"unexpected text following attach"); }

	else if(!strcmp(word,"detach")) {						// detach: end
		CHECKS(fil,"need to enter filament name before detach");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"detach format: end");
		i1=filstring2end(str1);
		CHECKS(i1>=0,"detach end needs to be minus or plus");
		CHECKS(fil->endjunct[i1],"this filament end is not attached");
		filRemoveJunction(fil->endjunct[i1]);
		CHECKS(!strnword(line2,2),"unexpected text following detach"); }

	else if(!strcmp(word,"end_state")) {				// end_state: end state
		CHECKS(fil,"need to enter filament name before end_state");
		itct=sscanf(line2,"%s %s",str1,str2);
		CHECKS(itct==2,"end_state format: end state");
		i1=filstring2end(str1);
		CHECKS(i1>=0,"end_state end needs to be minus or plus");
		fes=filstring2FES(str2);
		CHECKS(fes!=FESnone,"end_state state needs to be open, capped, or shrink");
		fil->endstate[i1]=fes;
		CHECKS(!strnword(line2,3),"unexpected text following end_state"); }

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
	if(len+1 > fil->maxsequence) {									// room for the terminating null
		fil=filAlloc(fil,0,0,len+1);
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

	path=filContourLength(fil);							// total path length
	fpath=frac*path;												// desired distance along path
	newpath=0;
	for(seg=0;seg<fil->nseg && newpath<fpath;seg++)			// first endpoint after desired distance
		newpath+=fil->segments[seg]->len;

	segment=fil->segments[seg>0?seg-1:0];
	x=(newpath-fpath)/segment->len;						// interpolate within segment
	pos[0]=x*segment->xyzminus[0]+(1-x)*segment->xyzplus[0];
	pos[1]=x*segment->xyzminus[1]+(1-x)*segment->xyzplus[1];
	pos[2]=x*segment->xyzminus[2]+(1-x)*segment->xyzplus[2];
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
	double *kypr,*stdypr,*ypr,cphi,sphi,ctht,stht,deltaypr[3],reltorque[3],thk;
	int dim;

	if(node<=0 || node>=fil->nseg) {
		torque[0]=torque[1]=torque[2]=0;
		return; }

	filtype=fil->filtype;
	kypr=filtype->kypr;
	stdypr=filtype->stdypr;
	ypr=fil->segments[node]->ypr;
	dim=filtype->filss->sim->dim;
	thk=0.5*(fil->segments[node-1]->thk+fil->segments[node]->thk);	// force constants scale with the average thickness of the two segments, as in filBendEnergy

	if(dim==2) {
		deltaypr[0]=(kypr[0]>0)?thk*kypr[0]*(ypr[0]-stdypr[0]):0;		// a negative constant is a constraint (filProjectConstraints)
		torque[0]=torque[1]=0;
		torque[2]=-deltaypr[0]; }
	else {
		cphi=cos(ypr[0]);
		sphi=sin(ypr[0]);
		ctht=cos(ypr[1]);
		stht=sin(ypr[1]);

		deltaypr[0]=(kypr[0]>0)?thk*kypr[0]*(ypr[0]-stdypr[0]):0;		// a negative constant is a constraint (filProjectConstraints)
		deltaypr[1]=(kypr[1]>0)?thk*kypr[1]*(ypr[1]-stdypr[1]):0;
		deltaypr[2]=(kypr[2]>0)?thk*kypr[2]*(ypr[2]-stdypr[2]):0;

		if(ctht<FILCOSPMIN) ctht=FILCOSPMIN;										// bound the torque near gimbal lock, where yaw is undefined
		reltorque[0]=-deltaypr[0]*stht/ctht*cphi+deltaypr[1]*sphi-deltaypr[2]*cphi/ctht;	// torque in segment reference frame: minus the gradient of the energy with respect to rotation
		reltorque[1]=-deltaypr[0]*stht/ctht*sphi-deltaypr[1]*cphi-deltaypr[2]*sphi/ctht;
		reltorque[2]=-deltaypr[0];

		Sph_QtniRotate(fil->segments[node-1]->qabs,reltorque,torque); }	// convert torque to system reference frame
	return; }


/************************************************************/
/************ Filament updating ************************/
/************************************************************/


/* filNodes2Angles */
void filNodes2Angles(filamentptr fil,int nodemin,int nodemax) {
	int seg,dim;
	segmentptr segment=NULL,segmentm1=NULL;
	double x[3],aphim1,aphi,len;

	if(nodemin<0) nodemin=0;				//?? these are not used
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	dim=fil->filtype->filss->sim->dim;
	if(dim==2) {
		if(nodemin==0) aphim1=0; 									// absolute phi for segment i-1
		else aphim1=Sph_Qtn2Yaw(fil->segments[nodemin-1]->qabs);
		for(seg=nodemin;seg<nodemax;seg++) {
			segment=fil->segments[seg];
			x[0]=segment->xyzplus[0]-segment->xyzminus[0];
			x[1]=segment->xyzplus[1]-segment->xyzminus[1];
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
			x[0]=segment->xyzplus[0]-segment->xyzminus[0];			// displacement of segment
			x[1]=segment->xyzplus[1]-segment->xyzminus[1];
			x[2]=segment->xyzplus[2]-segment->xyzminus[2];
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
			x[0]=segment->xyzplus[0]-segment->xyzminus[0];			// displacement of segment
			x[1]=segment->xyzplus[1]-segment->xyzminus[1];
			x[2]=segment->xyzplus[2]-segment->xyzminus[2];
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
            // segment may be uninitialized!
            Sph_QtnixQtn(segmentm1->qabs,segment->qabs,segment->qrel);	// combine prior and this qabs to get qrel
            Sph_Qtn2Ypr(segment->qrel,segment->ypr);						// use qrel to get ypr
            }}
	return; }


/* filCopyRollToYpr2 */
// Copies the working roll values into the segments' roll angles, ypr[2].
void filCopyRollToYpr2(filamentptr fil) {
	int seg;

	for(seg=0;seg<fil->nseg;seg++)
		fil->segments[seg]->ypr[2]=fil->roll[seg];
	return;	}


/* filCopyYpr2ToRoll */
// Copies the segments' roll angles, ypr[2], into the working roll values.
void filCopyYpr2ToRoll(filamentptr fil) {
	int seg;

	for(seg=0;seg<fil->nseg;seg++)
		fil->roll[seg]=fil->segments[seg]->ypr[2];
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
		if(from==0) filWorkFramesFromSegments(fil);
		for(seg=0;seg<4*fil->nseg;seg++)
			filwork->wqabs[to][seg]=filwork->wqabs[from][seg];
		for(node=0;node<=fil->nseg;node++) {
			nodesto[node][0]=nodesfrom[node][0];
			nodesto[node][1]=nodesfrom[node][1];
			nodesto[node][2]=nodesfrom[node][2]; }}
	return; }


/* filTransportFrames */
// Moves the segment reference frames of filament fil from an old state to a new one, as
// overdamped dynamics moves them. The old state has node positions nodesold and segment
// frames qold, as 4 quaternion values per segment. In the new state, segment seg has
// displacement vector segnew[3*seg..3*seg+2] and is rotated about its own axis by
// dphi[seg*dphistride]. Each segment's frame turns by the minimal rotation that takes
// its old direction to its new one, and then about its new direction by its dphi value,
// so the frame of one segment does not depend on how other segments move. This returns
// the new frames in qnew, and, unless rollnew is NULL, as the segment 0 up vector
// seg0upnew and the relative rolls rollnew, which filNodes2Angles uses with the new node
// positions to reconstruct these frames. qnew may be the same as qold, but nodesold
// needs to hold the old positions.
void filTransportFrames(filamentptr fil,double **nodesold,const double *qold,const double *segnew,const double *dphi,int dphistride,double *qnew,double *seg0upnew,double *rollnew) {
	int seg,d;
	double qnewm1[4],t[3],tn[3],k[3],c1[4],c2[4],q1[4],zold[3],zero[3];
	double len,lenn,cosang,ch,sh,sinang,dq0,dq1,dq2,dq3;

	zero[0]=zero[1]=zero[2]=0;
	for(seg=0;seg<fil->nseg;seg++) {
		len=lenn=0;
		for(d=0;d<3;d++) {
			t[d]=nodesold[seg+1][d]-nodesold[seg][d];
			len+=t[d]*t[d];
			tn[d]=segnew[3*seg+d];
			lenn+=tn[d]*tn[d]; }
		len=sqrt(len);
		lenn=sqrt(lenn);
		for(d=0;d<3;d++) {
			t[d]/=len;
			tn[d]/=lenn; }

		k[0]=t[1]*tn[2]-t[2]*tn[1];											// minimal rotation from t to tn: about k, by the angle between them
		k[1]=t[2]*tn[0]-t[0]*tn[2];
		k[2]=t[0]*tn[1]-t[1]*tn[0];
		cosang=t[0]*tn[0]+t[1]*tn[1]+t[2]*tn[2];
		ch=sqrt(0.5*(1+cosang>0?1+cosang:0));						// cos of half the angle
		if(ch>0.5) {																			// the quaternion for a rotation by a about axis u in the system frame is (cos(a/2),-u sin(a/2)); here sin(a/2)/sin(a) = 1/(2 ch)
			c1[0]=ch;
			for(d=0;d<3;d++) c1[d+1]=-k[d]/(2*ch); }
		else {
			sinang=sqrt(k[0]*k[0]+k[1]*k[1]+k[2]*k[2]);
			sh=sqrt(0.5*(1-cosang));
			if(sinang>1e-12) {
				c1[0]=ch;
				for(d=0;d<3;d++) c1[d+1]=-k[d]*sh/sinang; }
			else {																					// antiparallel: rotate by pi about the old frame's z axis, which is perpendicular to t
				Sph_QtniRotateUnitz(qold+4*seg,zold,zero,1);
				c1[0]=0;
				for(d=0;d<3;d++) c1[d+1]=-zold[d]; }}
		Sph_QtnxQtn(c1,qold+4*seg,q1);

		if(dphi[seg*dphistride]!=0) {											// roll about the new direction
			c2[0]=cos(0.5*dphi[seg*dphistride]);
			sh=sin(0.5*dphi[seg*dphistride]);
			for(d=0;d<3;d++) c2[d+1]=-tn[d]*sh;
			Sph_QtnxQtn(c2,q1,q1); }
		len=sqrt(q1[0]*q1[0]+q1[1]*q1[1]+q1[2]*q1[2]+q1[3]*q1[3]);

		if(seg>0)
			for(d=0;d<4;d++) qnewm1[d]=qnew[4*(seg-1)+d];
		for(d=0;d<4;d++) qnew[4*seg+d]=q1[d]/len;
		if(!rollnew);																			// the caller only needs qnew
		else if(seg==0)
			Sph_QtniRotateUnitz(qnew,seg0upnew,zero,1);
		else {																						// relative roll, from the relative quaternion as in Sph_Qtn2Ypr
			dq0=qnewm1[0]*qnew[4*seg+0]+qnewm1[1]*qnew[4*seg+1]+qnewm1[2]*qnew[4*seg+2]+qnewm1[3]*qnew[4*seg+3];
			dq1=qnewm1[0]*qnew[4*seg+1]-qnewm1[1]*qnew[4*seg+0]+qnewm1[2]*qnew[4*seg+3]-qnewm1[3]*qnew[4*seg+2];
			dq2=qnewm1[0]*qnew[4*seg+2]-qnewm1[1]*qnew[4*seg+3]-qnewm1[2]*qnew[4*seg+0]+qnewm1[3]*qnew[4*seg+1];
			dq3=qnewm1[0]*qnew[4*seg+3]+qnewm1[1]*qnew[4*seg+2]-qnewm1[2]*qnew[4*seg+1]-qnewm1[3]*qnew[4*seg+0];
			rollnew[seg]=atan2(-2*dq0*dq1+2*dq2*dq3,dq0*dq0-dq1*dq1-dq2*dq2+dq3*dq3); }}
	return; }


/* filWorkFramesFromSegments */
// Copies the segments' absolute quaternions into the working frames of state 0.
void filWorkFramesFromSegments(filamentptr fil) {
	int seg,d;
	double *q;

	q=fil->filwork->wqabs[0];
	for(seg=0;seg<fil->nseg;seg++)
		for(d=0;d<4;d++) q[4*seg+d]=fil->segments[seg]->qabs[d];
	return; }


/* filFlattenNodes */
void filFlattenNodes(filamentptr fil,int from,int to) {
	int node,dim;
	double **nodes,*flatnodes;
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
		for(node=0;node<fil->nseg;node++) {
			flatnodes[4*node+0]=nodes[node][0];
			flatnodes[4*node+1]=nodes[node][1];
			flatnodes[4*node+2]=nodes[node][2];
			flatnodes[4*node+3]=0; }																// rotation of the segment about its own axis since the start of the step
		flatnodes[4*node+0]=nodes[node][0];
		flatnodes[4*node+1]=nodes[node][1];
		flatnodes[4*node+2]=nodes[node][2]; }
	return; }


/* filUnflattenNodes */
// Copies the flattened state flatnodes[from] to the working nodes wnodes[to]. In 3D, the
// flattened roll of each segment is its rotation about its own axis since the state was
// flattened, and the segment frames are moved from those of state to, which needs to be
// the flattened state, with filTransportFrames.
void filUnflattenNodes(filamentptr fil,int from,int to) {
	int node,seg,d,dim;
	double **nodes,*flatnodes,*segnew;
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
		if(to==0) filCopyYpr2ToRoll(fil);
		segnew=filwork->flatforces[1];								// scratch space
		for(seg=0;seg<fil->nseg;seg++)
			for(d=0;d<3;d++)
				segnew[3*seg+d]=flatnodes[4*(seg+1)+d]-flatnodes[4*seg+d];
		if(to==0) filWorkFramesFromSegments(fil);
		filTransportFrames(fil,nodes,filwork->wqabs[to],segnew,flatnodes+3,4,filwork->wqabs[to],filwork->wseg0up[to],filwork->wroll[to]);
		for(node=0;node<=fil->nseg;node++) {
			nodes[node][0]=flatnodes[4*node+0];
			nodes[node][1]=flatnodes[4*node+1];
			nodes[node][2]=flatnodes[4*node+2]; }
		if(to==0) filCopyRollToYpr2(fil); }

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
			fil->nodes[i-shift]=newnode; }}

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
		shift=-shift; }

	for(i=0;i<fil->maxseg;i++) {									// every segment, including spares past nseg, must point at its own nodes
		fil->segments[i]->xyzminus=fil->nodes[i];
		fil->segments[i]->xyzplus=fil->nodes[i+1]; }

	filShiftJunctionNodes(fil,shift,fil->nseg+shift);	// nodes were renumbered, so junction nodes move with them

	for(i=0;i<fil->nseg+shift;i++)						// callers update nseg after this, so the new count is nseg+shift
		fil->segments[i]->index=i;

	return; }


/* filShiftJunctionNodes */
// Shift the node of each junction that attaches to fil by shift, and remove any junction
// whose node is no longer from FILJUNCTMINNODE to nseg, where nseg is the filament's new
// segment count. The attached filament end is released.
void filShiftJunctionNodes(filamentptr fil,int shift,int nseg) {
	int j,node;
	filjunctionptr junct;

	for(j=fil->njunct-1;j>=0;j--) {									// backward, since a removal shifts later entries down
		junct=fil->juncts[j];
		node=junct->node+shift;
		if(node>=FILJUNCTMINNODE && node<=nseg) junct->node=node;
		else filRemoveJunction(junct); }
	return; }


/* filAddSegment */
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar) {
	int seg;
	segmentptr segment,segmentm1,segmentp1;
	double zero[3]={0,0,0};

	if(fil->nseg==fil->maxseg) {
		fil=filAlloc(fil,fil->maxseg*2+1,0,0);
		if(!fil) return 1; }		// out of memory

	if(endchar=='p') {
		seg=fil->nseg;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		Sph_Ypr2Ypr(angle,segment->ypr);												// ypr = angle
		Sph_Ypr2Qtn(angle,segment->qrel);												// a = Qtn(angle)
		if(seg==0) {
			segment->xyzminus[0]=x[0];														// x_i = input value
			segment->xyzminus[1]=x[1];
			segment->xyzminus[2]=x[2];
			Sph_Qtn2Qtn(segment->qrel,segment->qabs); 						// b = a
			Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }		// rotate segment z to lab frame
		else {
			segmentm1=fil->segments[seg-1];
			Sph_QtnxQtn(segmentm1->qabs,segment->qrel,segment->qabs);	}	// b_i = b_{i-1} . a_i
		Sph_QtniRotateUnitx(segment->qabs,segment->xyzplus,segment->xyzminus,segment->len);			// x_{i+1} = x_i + l_i * b.xhat.bT
		fil->nseg++; }

	else {
		filArrayShift(fil,1);
		seg=0;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		if(fil->nseg==0) {
			Sph_Ypr2Qtni(angle,segment->qabs);									// b_0
			segment->xyzplus[0]=x[0];														// plus end of segment = input value
			segment->xyzplus[1]=x[1];
			segment->xyzplus[2]=x[2]; }
		else {
			segmentp1=fil->segments[1];
			Sph_Ypr2Ypr(angle,segmentp1->ypr);
			Sph_Ypr2Qtn(angle,segmentp1->qrel);
			Sph_QtnxQtni(segmentp1->qabs,segmentp1->qrel,segment->qabs); }		// b_i = b_{i+1} . a_{i+1}*
		Sph_Qtn2Qtn(segment->qabs,segment->qrel);									// a_i = b_i
		Sph_Qtn2Ypr(segment->qrel,segment->ypr);									// a_0 = Ypr(B_0)
		Sph_QtniRotateUnitx(segment->qabs,segment->xyzminus,segment->xyzplus,-segment->len);
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
			filRandomAngle(filtype,fil->nseg,0.5*(thickness+fil->segments[fil->nseg-1]->thk),1,angle);	// the joint's stiffness uses the average thickness of its two segments
			if(dim==2) angle[1]=angle[2]=0; }
		er=filAddSegment(fil,pos,len,angle,thickness,'p');
		if(er) return er; }

	return 0; }


/* filAddOneRandomSegment */
int filAddOneRandomSegment(simptr sim,filamentptr fil,const double *x,double thickness,char endchar,int constraints) {
	int er,iter,cross,tryagain;
	filamenttypeptr filtype;
	double len,angle[3],jointthk;
	segmentptr segment;
	(void) sim;

	filtype=fil->filtype;

	tryagain=1;
	jointthk=(fil->nseg>0)?0.5*(thickness+fil->segments[(endchar=='m')?0:fil->nseg-1]->thk):thickness;	// the joint's stiffness uses the average thickness of its two segments
	len=filRandomLength(filtype,thickness,1);
	filRandomAngle(filtype,fil->nseg,jointthk,1,angle);
	er=filAddSegment(fil,x,len,angle,thickness,endchar);
	if(er) return er;
	segment=fil->segments[((endchar=='m')?0:fil->nseg-1)];
	if(constraints==0) tryagain=0;
	for(iter=0;iter<FILMAXTRIES && tryagain;iter++) {
		tryagain=0;
		if(constraints & 1) {	// check for blocking by surfaces
			cross=filSurfBlocksSegment(fil,segment,endchar);
			if(cross) {
				len=filRandomLength(filtype,thickness,1);
				filRandomAngle(filtype,fil->nseg,jointthk,1,angle);
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
	er=filAddOneRandomSegment(sim,fil,NULL,fil->segments[(endchar=='p')?fil->nseg-1:0]->thk,endchar,1);
	if(!er)
		filRemoveSegment(fil,(endchar=='p')?'m':'p');
	return er; }



/* filRemoveSegment */
int filRemoveSegment(filamentptr fil,char endchar) {
	int seg;
	segmentptr segment;
	double zero[3]={0,0,0};

	if(fil->nseg==0) return -1;

	if(endchar=='p') {
		fil->nseg--;
		filShiftJunctionNodes(fil,0,fil->nseg); }		// drop any junction on the removed plus-end node
	else {
		seg=1;					// new minus-end segment
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
		shift[0]=vect[0]-segment->xyzminus[0];
		shift[1]=vect[1]-segment->xyzminus[1];
		shift[2]=vect[2]-segment->xyzminus[2]; }
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
	if(endchar=='p') {
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

	if(endchar=='p') {
		for(i=seg;i<fil->nseg;i++) {
			segment=fil->segments[i];
			if(i==0) {
				Sph_Qtn2Qtn(segment->qrel,segment->qabs); 						// b_0 = a_0
				Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }
			else {
				segmentm1=fil->segments[i-1];
				Sph_QtnxQtn(segmentm1->qabs,segment->qrel,segment->qabs); }			// b_i = b_{i-1} a_i
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);
			Sph_QtniRotateUnitx(segment->qabs,segment->xyzplus,segment->xyzminus,segment->len);
			}}
	else {
		for(i=seg;i>=0;i--) {
			segment=fil->segments[i];
			if(i==fil->nseg-1);
			else {
				segmentp1=fil->segments[i+1];
				Sph_QtnxQtni(segmentp1->qabs,segmentp1->qrel,segment->qabs); }		// b_i = b_i a_{i+1)T
			Sph_Qtn2Ypr(segment->qrel,segment->ypr);
			Sph_QtniRotateUnitx(segment->qabs,segment->xyzminus,segment->xyzplus,-segment->len); }
		Sph_QtniRotateUnitz(segment->qabs,fil->seg0up,zero,1); }

	return; }


/* filCopyFilament */
int filCopyFilament(filamentptr filto,const filamentptr filfrom,const filamenttypeptr filtype) {
	int i;
	segmentptr segmentfrom;

	if(!filto || !filfrom) return 2;

	filto->filtype=filtype?filtype:filfrom->filtype;

	filRemoveFilJunctions(filto);										// the copy starts unattached
	filto->nseg=0;
	filto->nsequence=0;
	filto=filAlloc(filto,filfrom->nseg,0,filfrom->nsequence);
	if(!filto) return 1;

	for(i=0;i<filfrom->nseg;i++) {
		segmentfrom=filfrom->segments[i];
		filAddSegment(filto,segmentfrom->xyzminus,segmentfrom->len,segmentfrom->ypr,segmentfrom->thk,'p'); }

	filto->seg0up[0]=filfrom->seg0up[0];
	filto->seg0up[1]=filfrom->seg0up[1];
	filto->seg0up[2]=filfrom->seg0up[2];

	for(i=0;i<filfrom->nsequence;i++)
		filto->sequence[i]=filfrom->sequence[i];
	filto->nsequence=filfrom->nsequence;

	for(i=0;i<2;i++) {
		filto->endstate[i]=filfrom->endstate[i];
		filto->lenbank[i]=filfrom->lenbank[i]; }

	return 0; }


/* filReset */
// Return fil to the state of a newly allocated filament, keeping its allocated memory, its
// filament type, and its name. filAddFilament calls this because it may be reusing a
// removed filament. Segment and node values are not cleared, since they are set as
// segments are added; per-node mobility factors and roll values are, since they persist.
void filReset(filamentptr fil) {
	int seg;

	fil->nseg=0;
	if(fil->maxseg>0) {															// nothing is allocated yet for a never-used filament
		for(seg=0;seg<=fil->maxseg;seg++) fil->nodemobility[seg]=1;
		for(seg=0;seg<fil->maxseg;seg++) fil->roll[seg]=0; }
	fil->seg0up[0]=fil->seg0up[1]=0;
	fil->seg0up[2]=1;
	fil->endjunct[0]=fil->endjunct[1]=NULL;
	fil->njunct=0;
	fil->nsequence=0;
	fil->endstate[0]=fil->filtype->endstate0[0];		// new filaments start in the type's initial end states
	fil->endstate[1]=fil->filtype->endstate0[1];
	fil->lenbank[0]=fil->lenbank[1]=0;
	return; }


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
	strncpy(filtype->filnames[f],filname,STRCHAR);
	filtype->filnames[f][STRCHAR-1]='\0';
	fil=filtype->fillist[f];
	fil->filtype=filtype;
	filReset(fil);																	// the slot may hold a removed filament
	filSetCondition(filtype->filss,SClists,0);

	return fil; }


/* filRemoveFilament */
// Remove filament fil from the simulation, keeping its memory for reuse. All junctions
// that involve fil are removed first, because its memory will be reused for an
// unrelated filament: fil's own ends are detached, and every filament end attached to
// fil is released. Then the filaments after fil in fillist shift down one place, which
// keeps creation order, and fil and its name string move to position nfil-1, at the
// front of the unused filaments that filAddFilament takes next. Returns 0 for success
// or 1 if fil is not in its type's list.
int filRemoveFilament(filamentptr fil) {
	filamenttypeptr filtype;
	char *name;
	int f,f2;

	filtype=fil->filtype;
	for(f=0;f<filtype->nfil && filtype->fillist[f]!=fil;f++);
	if(f==filtype->nfil) return 1;

	filRemoveFilJunctions(fil);

	name=filtype->filnames[f];											// shift later filaments down, keeping creation order
	for(f2=f;f2<filtype->nfil-1;f2++) {
		filtype->fillist[f2]=filtype->fillist[f2+1];
		filtype->filnames[f2]=filtype->filnames[f2+1]; }
	filtype->fillist[f2]=fil;
	filtype->filnames[f2]=name;
	filtype->nfil--;
	filSetCondition(filtype->filss,SClists,0);
	return 0; }


/******************************************************************************/
/**************************** filament branching **************************/
/******************************************************************************/


#define FILJUNCTSINMIN 1e-9			// junctions with sin(theta) below this are degenerate; filBranchAzimuth and filJunctionGeometry must agree on it
#define FILJUNCTSINBOUND 0.1		// floor on sin(theta) in the azimuthal force denominator, capping the 1/sin(theta) divergence near-parallel geometry

/* filBranchJunctionNodes */
// Daughter nodes at a junction, for a daughter that attaches by its end end ('m' or
// 'p'). *nearptr is the daughter end node that is pinned to the mother and *farptr is
// the next node along the daughter, so nodes[far]-nodes[near] points away from the
// mother. Using these nodes for both ends keeps the branch angle and azimuth defined
// the same way whichever end attaches.
void filBranchJunctionNodes(const filamentptr daughter,char end,int *nearptr,int *farptr) {
	if(end=='p') {
		*nearptr=daughter->nseg;
		*farptr=daughter->nseg-1; }
	else {
		*nearptr=0;
		*farptr=1; }
	return; }


/* filRigidParent */
// The junction that rigidly pins fil, which is fil's parent edge in the spanning forest
// of rigid junctions, or NULL if fil is the root of its tree. filClassifyJunctions
// ensures that a filament has at most one.
filjunctionptr filRigidParent(const filamentptr fil) {
	if(fil->endjunct[0] && fil->endjunct[0]->rigid) return fil->endjunct[0];
	if(fil->endjunct[1] && fil->endjunct[1]->rigid) return fil->endjunct[1];
	return NULL; }


/* filTreeRoot */
// The root of the tree of rigid junctions that contains fil.
filamentptr filTreeRoot(filamentptr fil) {
	filjunctionptr junct;

	while((junct=filRigidParent(fil))) fil=junct->target;
	return fil; }


/* filInSubtree */
// Returns 1 if fil is top or is attached to top through a chain of rigid junctions, so
// that it is in the subtree of rigid junctions below top, and 0 otherwise.
int filInSubtree(filamentptr fil,const filamentptr top) {
	filjunctionptr junct;

	while(fil!=top && (junct=filRigidParent(fil))) fil=junct->target;
	return fil==top; }


/* filClassifyJunctions */
// Decide which junctions are rigid, going through them in creation order. A junction is
// rigid, meaning that it is an edge of the spanning forest and is held by translating its
// filament, if its filament is not already rigidly pinned by its other end and the
// target is not in its filament's own tree; otherwise it closes a loop and is held by a
// spring. Every junction of a dendritic network is rigid. Assigning greedily in creation
// order keeps the result deterministic and the rigid junctions acyclic, with at most one
// rigid parent per filament. Called whenever a junction is added or removed.
void filClassifyJunctions(filamentssptr filss) {
	int j;
	filjunctionptr junct;

	for(j=0;j<filss->njunct;j++)
		filss->junctlist[j]->rigid=0;
	filss->nspring=0;
	for(j=0;j<filss->njunct;j++) {
		junct=filss->junctlist[j];
		if(!filRigidParent(junct->fil) && filTreeRoot(junct->target)!=junct->fil)
			junct->rigid=1;
		else
			filss->nspring++; }
	return; }


/* filAddJunction */
// Attach end end ('m' or 'p') of filament fil to node node of filament target, which may
// be fil itself. bend is 1 for an end-to-end join, whose angle is held by the bending
// spring, and 0 for a junction whose angle is held by the branch-angle spring. This only
// records the junction and classifies it as rigid or loop-closing; it does not move any
// filament. Returns the junction, or NULL if that end is already attached or memory
// could not be allocated.
filjunctionptr filAddJunction(filamentptr fil,char end,filamentptr target,int node,int bend) {
	filamentssptr filss;
	filjunctionptr junct,*newlist;
	int j,e,newmax;

	filss=fil->filtype->filss;
	e=(end=='p')?1:0;
	if(fil->endjunct[e]) return NULL;

	if(filss->njunct==filss->maxjunct) {						// grow the junction list
		newmax=2*filss->maxjunct+1;
		newlist=(filjunctionptr*) calloc(newmax,sizeof(filjunctionptr));
		if(!newlist) return NULL;
		for(j=0;j<filss->maxjunct;j++) newlist[j]=filss->junctlist[j];
		for(;j<newmax;j++) {
			newlist[j]=(filjunctionptr) malloc(sizeof(struct filjunctionstruct));
			if(!newlist[j]) return NULL;
			newlist[j]->fil=newlist[j]->target=NULL; }
		free(filss->junctlist);
		filss->junctlist=newlist;
		filss->maxjunct=newmax; }

	if(target->njunct==target->maxjunct)
		if(!filAlloc(target,0,target->maxjunct*2+1,0)) return NULL;

	junct=filss->junctlist[filss->njunct++];
	junct->fil=fil;
	junct->end=end;
	junct->target=target;
	junct->node=node;
	junct->rigid=0;
	junct->bend=bend;
	junct->azim0=-1;
	target->juncts[target->njunct++]=junct;
	fil->endjunct[e]=junct;
	filClassifyJunctions(filss);
	return junct; }


/* filRemoveJunction */
// Remove junction junct, which releases the attached filament end. The other entries
// keep their order, and the junction's memory moves to the unused end of the list for
// reuse. Junctions are reclassified, so a loop-closing junction may become rigid.
void filRemoveJunction(filjunctionptr junct) {
	filamentssptr filss;
	filamentptr target;
	int j;

	filss=junct->fil->filtype->filss;
	target=junct->target;
	for(j=0;j<target->njunct && target->juncts[j]!=junct;j++);
	if(j<target->njunct) {
		for(;j<target->njunct-1;j++) target->juncts[j]=target->juncts[j+1];
		target->njunct--; }
	if(junct->fil->endjunct[0]==junct) junct->fil->endjunct[0]=NULL;
	if(junct->fil->endjunct[1]==junct) junct->fil->endjunct[1]=NULL;
	for(j=0;j<filss->njunct && filss->junctlist[j]!=junct;j++);
	if(j<filss->njunct) {
		for(;j<filss->njunct-1;j++) filss->junctlist[j]=filss->junctlist[j+1];
		filss->junctlist[j]=junct;
		filss->njunct--; }
	junct->fil=junct->target=NULL;
	filClassifyJunctions(filss);
	return; }


/* filRemoveFilJunctions */
// Remove every junction that involves fil, either as the attached filament or as the
// target.
void filRemoveFilJunctions(filamentptr fil) {
	filamentssptr filss;
	filjunctionptr junct;
	int j;

	filss=fil->filtype->filss;
	if(!filss) return;
	for(j=filss->njunct-1;j>=0;j--) {								// backward, since a removal shifts later entries down
		junct=filss->junctlist[j];
		if(junct->fil==fil || junct->target==fil) filRemoveJunction(junct); }
	return; }


/* filAttachEnd */
// Attach end end ('m' or 'p') of fil, which needs to be free, to node node of target,
// which may be fil itself. Attaching at an end node of the target is an end-to-end join,
// whose angle is held by the bending spring; attaching elsewhere uses the branch-angle
// spring. If the junction is rigid, fil is translated so its end sits at the target node,
// and the rest of fil's tree follows; if it closes a loop, only fil's end node is moved
// onto the target node, which stretches its end segment until the springs relax it.
// The current azimuth is recorded for the azimuthal spring. Returns 0 for success or 1
// if the end is already attached or memory could not be allocated.
int filAttachEnd(filamentptr fil,char end,filamentptr target,int node) {
	filjunctionptr junct;
	int jnear,jfar,d;

	junct=filAddJunction(fil,end,target,node,(node==0 || node==target->nseg)?1:0);
	if(!junct) return 1;
	if(junct->rigid) {
		filPinJunction(junct);
		filPinTree(fil); }
	else {
		filBranchJunctionNodes(fil,end,&jnear,&jfar);
		for(d=0;d<3;d++) fil->nodes[jnear][d]=target->nodes[node][d];
		filNodes2Angles(fil,-1,-1); }
	if(!junct->bend) junct->azim0=filBranchAzimuth(junct);
	return 0; }


/* filBranchAzimuth */
// Azimuth of a junction's attached filament about its target: the angle of the attached
// filament's junction segment around the axis of the target's reference segment (the one
// that ends at the junction node, or segment 0 for node 0), measured in that segment's
// material frame (from the frame's y axis toward its z axis), in [0,2*PI). This is the
// quantity that branch_azimuth_fix holds; it is recorded at birth by filAddBranch and
// re-measured by filPinJunction, so the two are consistent by construction whatever the
// frame conventions. Returns -1 where azimuth is undefined: 2D systems, an invalid node,
// or an attached filament parallel to the target.
// SIGN CONTRACT: with this definition, rotating the daughter about +tm (right-handed)
// raises phi, which is what makes filAddJunctionForces' azimuthal gradient
// (tm x td)/(lend*sinth^2) restoring; both rely on Sph_QtnRotate being the lab-to-frame
// map of a proper rotation. If either convention changes, the spring flips sign to
// anti-restoring -- the T6 validation (azimuth drift on a pinned mother) catches it.
double filBranchAzimuth(const filjunctionptr junct) {
	double td[3],v[3],phi;
	segmentptr mseg;
	filamentptr mother,daughter;
	int node,jnear,jfar;

	mother=junct->target;
	daughter=junct->fil;
	node=junct->node;
	if(mother->filtype->filss->sim->dim!=3) return -1;
	if(node<FILJUNCTMINNODE || node>mother->nseg || daughter->nseg<1) return -1;
	mseg=mother->segments[node>0?node-1:0];
	filBranchJunctionNodes(daughter,junct->end,&jnear,&jfar);
	td[0]=daughter->nodes[jfar][0]-daughter->nodes[jnear][0];
	td[1]=daughter->nodes[jfar][1]-daughter->nodes[jnear][1];
	td[2]=daughter->nodes[jfar][2]-daughter->nodes[jnear][2];
	Sph_QtnRotate(mseg->qabs,td,v);								// daughter direction in the mother segment frame; x is the mother axis
	if(v[1]*v[1]+v[2]*v[2]<=FILJUNCTSINMIN*FILJUNCTSINMIN*(v[0]*v[0]+v[1]*v[1]+v[2]*v[2])) return -1;
	phi=atan2(v[2],v[1]);
	if(phi<0) phi+=2*PI;
	return phi; }


/* filBranchDazim */
// Signed deviation of a junction from its recorded azimuth, wrapped to (-PI,PI]. Returns
// 1 and sets *dphiptr when defined; 0 when the azimuth was never recorded or the current
// azimuth is undefined (2D, degenerate geometry). The single definition of the sentinel
// and wrap conventions that the rigid azimuth pin and the azimuthal spring share.
int filBranchDazim(const filjunctionptr junct,double *dphiptr) {
	double phi0,dphi;

	phi0=junct->azim0;
	if(phi0<0) return 0;
	dphi=filBranchAzimuth(junct);
	if(dphi<0) return 0;
	dphi-=phi0;
	if(dphi>PI) dphi-=2*PI;
	else if(dphi<-PI) dphi+=2*PI;
	*dphiptr=dphi;
	return 1; }


/* filAddBranch */
// Nucleate a daughter filament off mother node node, which must be from 1 to nseg. The
// daughter attaches by its end end: 'm' puts its minus end (node 0) at the mother node
// and 'p' puts its plus end (node nseg) there. angle is the direction of the daughter
// away from the mother, as ypr relative to the absolute frame of mother segment node-1,
// the segment that ends at node. length0 is the length of the daughter's segment 0, or
// <=0 for a random length. Returns the daughter, or NULL on error.
filamentptr filAddBranch(simptr sim,filamentptr mother,int node,char end,double length0,const double *angle,double thickness,const char *daughtername) {
	filamenttypeptr mothertype;
	filamentptr daughter;
	filjunctionptr junct;
	segmentptr mseg;
	double branchpos[3],qbranch[4],qdaughter[4],qflip[4],daughterypr[3],len,straight[3]={0,0,0},flip[3]={PI,0,0},shift[3];
	int s,nseg;
	(void) sim;

	if(!mother || mother->nseg==0) return NULL;						// need a real mother
	if(node<0 || node>mother->nseg) return NULL;						// branch node must be a real node
	mothertype=mother->filtype;
	nseg=mothertype->branchsegments;							// daughter length in segments (parser enforces >=1)

	daughter=filAddFilament(mothertype,daughtername);		// daughters share the mother's type; auto-named if NULL
	if(!daughter) return NULL;

	mseg=mother->segments[node>0?node-1:0];					// reference frame = segment that ends at the branch node, or segment 0
	branchpos[0]=mother->nodes[node][0];
	branchpos[1]=mother->nodes[node][1];
	branchpos[2]=mother->nodes[node][2];

	Sph_Ypr2Qtn(angle,qbranch);											// branch rotation in mother's frame
	Sph_QtnxQtn(mseg->qabs,qbranch,qdaughter);				// compose onto mother-segment absolute orientation
	if(end=='p') {																// segments point toward the mother, so turn the daughter around
		Sph_Ypr2Qtn(flip,qflip);
		Sph_QtnxQtn(qdaughter,qflip,qdaughter); }
	Sph_Qtn2Ypr(qdaughter,daughterypr);							// filAddSegment (seg 0) expects a lab-frame ypr

	len=(length0>0)?length0:filRandomLength(mothertype,thickness,1);
	if(filAddSegment(daughter,branchpos,len,daughterypr,thickness,'p')) return NULL;
	for(s=1;s<nseg;s++) {													// extend the daughter straight from the branch
		len=filRandomLength(mothertype,thickness,1);
		if(filAddSegment(daughter,NULL,len,straight,thickness,'p')) return NULL; }
	if(end=='p') {																// built from the branch point toward the mother; slide it out so its plus end is at the branch point
		shift[0]=branchpos[0]-daughter->nodes[daughter->nseg][0];
		shift[1]=branchpos[1]-daughter->nodes[daughter->nseg][1];
		shift[2]=branchpos[2]-daughter->nodes[daughter->nseg][2];
		filTranslate(daughter,shift,'+'); }

	junct=filAddJunction(daughter,end,mother,node,0);	// a new daughter is unattached, so this is a rigid junction
	if(!junct) return NULL;
	junct->azim0=filBranchAzimuth(junct);						// realized birth azimuth, for branch_azimuth_fix

	return daughter; }


/* filBranchPointInRegion */
// Location gate for branch nucleation: returns 1 if pos qualifies under the type's
// branch_surface / branch_compartment restriction, or if no gate is set. The gate is
// an interim geometric proxy for membrane-bound nucleators (Arp2/3 recruited by
// NPFs): it restricts WHERE spontaneous branching may occur but models no nucleator
// molecules, so there is no depletion, saturation, or consumption. It is meant to be
// superseded by explicit filament-molecule binding when the simulator gains that
// capability, at which point a nucleator species on a surface replaces this gate.
int filBranchPointInRegion(simptr sim,const filamenttypeptr filtype,double *pos) {
	double dist;

	if(filtype->branchcmpt)
		return posincompart(sim,pos,filtype->branchcmpt,0);
	if(!filtype->branchsrf) return 1;
	dist=closestsurfacept(filtype->branchsrf,sim->dim,pos,NULL,NULL,NULL);
	return dist>=0 && dist<=filtype->branchsrfdist; }		// -1 means a panel-less surface: gate closed


/* filBranchNodeSpaced */
// Spacing test for branch nucleation: returns 1 if no branch on mother is closer than
// spacing to mother node node, measured along the mother's contour with its current
// segment lengths, and 0 otherwise. Branches are the junctions that target mother other
// than end-to-end joins, so this includes daughters made by branch dynamics, the branch
// statement, and attach to an interior node; junction skip is ignored, or none if NULL.
// Junctions whose node is no longer on the mother are ignored. Distances below spacing
// fail, so any positive spacing keeps two branches off the same node.
int filBranchNodeSpaced(const filamentptr mother,int node,double spacing,const filjunctionptr skip) {
	int j,n,n1,n2;
	double dist;
	filjunctionptr junct;

	if(spacing<=0) return 1;
	for(j=0;j<mother->njunct;j++) {
		junct=mother->juncts[j];
		if(junct==skip || junct->bend) continue;
		if(junct->node<FILJUNCTMINNODE || junct->node>mother->nseg) continue;
		n1=(junct->node<node)?junct->node:node;
		n2=(junct->node<node)?node:junct->node;
		dist=0;
		for(n=n1;n<n2 && dist<spacing;n++) dist+=mother->segments[n]->len;
		if(dist<spacing) return 0; }
	return 1; }


/* filBranchDynamics */
// Poisson branch nucleation for one filament type, at rate branchrate*length*dt, with
// each daughter attached at the plus-end node of a uniformly random segment of its mother. If a location
// gate is set (branch_surface / branch_compartment), drawn events are thinned: an
// event whose branch point falls outside the gated region is discarded, which realizes
// an inhomogeneous Poisson process with density branchrate per unit mother length
// inside the region and zero outside (exact when segment lengths are equal, since the
// node draw is uniform per segment -- the same sampling as the ungated draw). The
// accept test draws no random numbers, so a rejected event consumes only its segment
// draw. With branch_min_spacing, events are also thinned by the spacing test, so
// branch_rate is the rate per unit of mother length that is far enough from existing
// branches; this draws no random numbers either.
void filBranchDynamics(simptr sim,filamenttypeptr filtype) {
	int f,nfil0,node,nbr,i,dim;
	double totallen,angle[3],thick,theta,ratedt,phi,dvec[3];
	filamentptr fil;

	if(filtype->branchrate<=0) return;
	dim=sim->dim;
	ratedt=filtype->branchrate*sim->dt;					// same for every filament this step
	nfil0=filtype->nfil;												// snapshot: don't branch off daughters born this step

	for(f=0;f<nfil0;f++) {
		fil=filtype->fillist[f];
		if(fil->nseg<1) continue;
		totallen=filContourLength(fil);

		nbr=poisrandD(ratedt*totallen);
		for(i=0;i<nbr;i++) {
			node=intrand(fil->nseg)+1;									// uniform random branch node, from 1 to nseg
			if(!filBranchPointInRegion(sim,filtype,fil->nodes[node])) continue;		// interim location gate; accepts everything when no gate is set
			if(!filBranchNodeSpaced(fil,node,filtype->branchminspacing,NULL)) continue;	// too close to an existing branch, including ones born this step

			thick=fil->segments[node-1]->thk;					// the daughter has the thickness of the mother segment that ends at the branch node
			theta=filtype->branchangle;
			if(filtype->branchspread>0)								// jitter the branch angle; a spread derived from the spring is its equilibrium spread for this thickness
				theta=gaussrandtruncOCD(theta,filtype->branchspreadderived?filtype->branchspread/sqrt(thick):filtype->branchspread,-PI,PI);
			if(dim==2) {															// in-plane +/- branch angle
				angle[0]=theta*(coinrandD(0.5)?1:-1);
				angle[1]=angle[2]=0; }
			else {																		// 3D dendritic cone: fixed polar angle theta; uniform azimuth unless branch_azimuth is set
				phi=(filtype->branchazimuth>=0)?filtype->branchazimuth:unirandCOD(0,2*PI);
				dvec[0]=cos(theta);											// daughter direction in the mother segment frame; azimuth from +y toward +z, matching filBranchAzimuth
				dvec[1]=sin(theta)*cos(phi);
				dvec[2]=sin(theta)*sin(phi);
				angle[0]=atan2(dvec[1],dvec[0]);				// yaw-pitch that realize dvec under the segment convention (cos y cos p, sin y cos p, -sin p); filNodes2Angles applies the same inverse
				angle[1]=-asin(dvec[2]);
				angle[2]=0; }														// roll 0: the daughter frame about its own axis is deterministic; anisotropic-bending or intrinsic-twist types get a fixed material-frame orientation per daughter where a random roll would decorrelate it

			filAddBranch(sim,fil,node,filtype->branchend,0,angle,thick,NULL); }}		// realloc-safe: fillist grows, fil ptrs stable

	return; }


/* filPinJunction */
// Hold a rigid junction after the mechanical step by translating its attached filament
// so that its attached end node sits at the target node -- a position constraint, not a
// force. With branch_azimuth_fix on (3D, and not for end-to-end joins): additionally
// rotate the attached filament rigidly about the target's reference segment axis so its
// azimuth returns to the value recorded at birth; the Arp2/3 slot is stereospecific, so
// the branch direction around the mother is a constraint rather than a measured
// compliance. The rotation axis passes through the branch point, so the rotation
// preserves both the pinned position and the mother-daughter angle theta, composing
// cleanly with the branch_force_angle spring. The daughter's seg0up vector co-rotates so
// its own material frame stays consistent and grand-daughter junctions remain valid.
void filPinJunction(filjunctionptr junct) {
	int bnode,node,dim,jnear,jfar;
	double branchpos[3],axis[3],dphi,vect[3],shift[3];
	filamentptr mother,daughter;
	segmentptr mseg;

	mother=junct->target;
	daughter=junct->fil;
	dim=daughter->filtype->filss->sim->dim;
	if(daughter->nseg==0) return;
	bnode=junct->node;
	if(bnode<FILJUNCTMINNODE || bnode>mother->nseg) return;	// mother may have shrunk (treadmilling)
	mseg=mother->segments[bnode>0?bnode-1:0];				// reference segment
	branchpos[0]=mother->nodes[bnode][0];
	branchpos[1]=mother->nodes[bnode][1];
	branchpos[2]=mother->nodes[bnode][2];
	filBranchJunctionNodes(daughter,junct->end,&jnear,&jfar);
	shift[0]=branchpos[0]-daughter->nodes[jnear][0];
	shift[1]=branchpos[1]-daughter->nodes[jnear][1];
	shift[2]=branchpos[2]-daughter->nodes[jnear][2];
	filTranslate(daughter,shift,'+');								// move daughter so its attached end sits at branch point

	if(daughter->filtype->branchazimuthfix && dim==3 && !junct->bend && filBranchDazim(junct,&dphi) && dphi!=0) {	// restore the recorded birth azimuth
		axis[0]=mseg->xyzplus[0]-mseg->xyzminus[0];
		axis[1]=mseg->xyzplus[1]-mseg->xyzminus[1];
		axis[2]=mseg->xyzplus[2]-mseg->xyzminus[2];
		if(axis[0]==0 && axis[1]==0 && axis[2]==0) return;
		for(node=0;node<=daughter->nseg;node++) {
			if(node==jnear) continue;										// the attached node is the branch point, on the axis
			vect[0]=daughter->nodes[node][0]-branchpos[0];
			vect[1]=daughter->nodes[node][1]-branchpos[1];
			vect[2]=daughter->nodes[node][2]-branchpos[2];
			Sph_RotateVectorAxisAngle(vect,axis,-dphi,vect);
			daughter->nodes[node][0]=branchpos[0]+vect[0];
			daughter->nodes[node][1]=branchpos[1]+vect[1];
			daughter->nodes[node][2]=branchpos[2]+vect[2]; }
		Sph_RotateVectorAxisAngle(daughter->seg0up,axis,-dphi,daughter->seg0up);
		filNodes2Angles(daughter,-1,-1); }

	return; }


/* filPinTree */
// Pin every rigid junction in the tree below fil, parents before children, so each
// filament is pinned to its parent's final position.
void filPinTree(filamentptr fil) {
	int j;
	filjunctionptr junct;

	for(j=0;j<fil->njunct;j++) {
		junct=fil->juncts[j];
		if(junct->rigid) {
			filPinJunction(junct);
			filPinTree(junct->fil); }}
	return; }


/* filPinJunctions */
// Pin all rigid junctions after all motion in a time step, starting from the root of
// each tree, across all filament types. Loop-closing junctions are held by springs
// instead (filAddJunctionForces).
void filPinJunctions(filamentssptr filss) {
	int ft,f;
	filamenttypeptr filtype;
	filamentptr fil;

	if(filss->njunct==0) return;
	for(ft=0;ft<filss->ntype;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil;f++) {
			fil=filtype->fillist[f];
			if(!filRigidParent(fil)) filPinTree(fil); }}
	return; }


/******************************************************************************/
/************************ filament end dynamics *******************************/
/******************************************************************************/


/* filContourLength */
// Sum of segment lengths: path length along the polymer, not end-to-end distance.
double filContourLength(const filamentptr fil) {
	int seg;
	double total;

	total=0;
	for(seg=0;seg<fil->nseg;seg++) total+=fil->segments[seg]->len;
	return total; }


/* filEndLengthChange */
// Change the length of end end (0 = minus, 1 = plus) of fil by dlength of contour length:
// grow for dlength>0 and shrink for dlength<0. Segments are added or removed whole, so
// lenbank carries the remainder forward. Growth debits the length actually drawn, which
// keeps total length tracking rate*time with bounded rather than accumulating error; a
// blocked end drops the growth it was denied rather than banking it, so it stalls at zero
// instead of surging when it clears. Shrinking removes the end segment once the bank
// covers that segment's actual length, for the same exact accounting. When the bank
// covers the length of a filament's only remaining segment, the filament has shrunk away;
// this returns FILREMOVED without changing the filament, and the caller removes it.
// Otherwise, returns the number of segments added (positive) or removed (negative), or
// -1000 for a configuration error.
int filEndLengthChange(simptr sim,filamentptr fil,int end,double dlength) {
	filamenttypeptr filtype;
	char endchar;
	double stdlen,maxlen,newlen,curlen;
	int nadd,er;

	filtype=fil->filtype;
	if(fil->nseg<1) return 0;												// nothing to grow from
	endchar=filend2char(end);

	if(dlength<0) {																	// shrinking
		fil->lenbank[end]+=dlength;
		nadd=0;
		while(-fil->lenbank[end]>=fil->segments[(endchar=='p')?fil->nseg-1:0]->len) {
			if(fil->nseg==1) return FILREMOVED;						// the last segment is gone
			fil->lenbank[end]+=fil->segments[(endchar=='p')?fil->nseg-1:0]->len;
			filRemoveSegment(fil,endchar);
			nadd--; }
		return nadd; }

	stdlen=filtype->stdlen;
	if(stdlen<=0) return -1000;											// filCheckParams errors on this; guard the loop anyway

	maxlen=filtype->elongmaxlen;
	curlen=maxlen>0?filContourLength(fil):0;				// only needed to test the ceiling
	if(maxlen>0 && curlen>=maxlen) return 0;				// at the ceiling: no growth, and nothing banked

	fil->lenbank[end]+=dlength;
	nadd=0;
	while(fil->lenbank[end]>=stdlen) {
		if(nadd>=FILMAXGROW) {
			simLog(sim,5,"WARNING: filament %s hit the %i segment per step elongation limit; check elongation_rate against dt\n",fil->filname,FILMAXGROW);
			break; }
		if(maxlen>0 && curlen>=maxlen) {								// crossed the ceiling partway through this step
			if(fil->lenbank[end]>stdlen) fil->lenbank[end]=stdlen;
			break; }

		er=filAddOneRandomSegment(sim,fil,NULL,fil->segments[(endchar=='p')?fil->nseg-1:0]->thk,endchar,1);
		if(er) {																						// blocked end
			fil->lenbank[end]-=dlength;												// this step's growth did not happen
			if(fil->lenbank[end]>stdlen) fil->lenbank[end]=stdlen;	// and never hold more than one segment in reserve
			if(fil->lenbank[end]<0) fil->lenbank[end]=0;
			break; }

		newlen=fil->segments[(endchar=='p')?fil->nseg-1:0]->len;
		fil->lenbank[end]-=newlen;												// pay for the segment actually emitted
		curlen+=newlen;
		nadd++; }

	return nadd; }


/* filEndTransitions */
// Stochastic end state transitions for one filament type. An end in a state with total
// exit rate R leaves it with per-step probability 1-exp(-R*dt), matching the unimolecular
// convention in smolreact.c rather than a linearized R*dt, which is what makes the
// measured rates independent of dt. The destination is chosen in proportion to the
// individual rates. Random numbers are drawn only for ends that can leave their state,
// and the destination is drawn only when there is more than one, so a type that only caps
// and uncaps its plus end draws exactly the random numbers that capping alone did.
void filEndTransitions(simptr sim,filamenttypeptr filtype) {
	int f,end,fes2,ndest;
	double rtot,r;
	filamentptr fil;
	enum FilEndState fes,dest;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		if(fil->nseg<1) continue;
		for(end=0;end<2;end++) {
			fes=fil->endstate[end];
			rtot=0;
			ndest=0;
			dest=fes;
			for(fes2=0;fes2<FESMAX;fes2++)
				if(filtype->endtrans[end][fes][fes2]>0) {
					rtot+=filtype->endtrans[end][fes][fes2];
					ndest++;
					dest=(enum FilEndState)fes2; }
			if(rtot<=0 || !coinrandD(1.0-exp(-rtot*sim->dt))) continue;
			if(ndest>1) {																	// pick a destination in proportion to its rate
				r=unirandCOD(0,rtot);
				for(fes2=0;fes2<FESMAX;fes2++)
					if(filtype->endtrans[end][fes][fes2]>0) {
						dest=(enum FilEndState)fes2;
						r-=filtype->endtrans[end][fes][fes2];
						if(r<0) break; }}
			fil->endstate[end]=dest; }}

	return; }


/* filEndDynamics */
// Length changes at both ends of every filament of one filament type, minus end first,
// each by its current state's velocity times dt. Deterministic; the only length noise is
// the random segment length. A filament that shrinks away is removed.
void filEndDynamics(simptr sim,filamenttypeptr filtype) {
	int f,end;
	double v;
	filamentptr fil;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		for(end=0;end<2;end++) {
			v=filtype->endrate[end][fil->endstate[end]];
			if(v!=0 && filEndLengthChange(sim,fil,end,v*sim->dt)==FILREMOVED) {
				filRemoveFilament(fil);
				f--;																					// the next filament has moved into position f
				break; }}}

	return; }


/******************************************************************************/
/**************************** filament interactions ***********************/
/******************************************************************************/


/* filSurfBlocksSegment */
// Returns 1 if a block interaction of the filament type stops segment segment, which
// was just added at end endchar ('m' or 'p') of filament fil, and 0 if not. A block
// interaction applies if it lists that end. It stops the segment if the segment
// crosses a panel of the interaction's surface from the interaction's face, meaning
// that the existing end node is on that side of the panel, or from either side if the
// face is both. Without block interactions, surfaces do not stop growing ends.
int filSurfBlocksSegment(const filamentptr fil,const segmentptr segment,char endchar) {
	filamenttypeptr filtype;
	filsurfinteractptr fsi;
	surfaceptr srf;
	enum PanelShape ps;
	enum PanelFace face1,face2;
	double *pt1,*pt2,crosspt[3];
	int i,p,part,dim;

	filtype=fil->filtype;
	dim=filtype->filss->sim->dim;
	part=(endchar=='p')?FPplus:FPminus;
	pt1=(endchar=='p')?segment->xyzminus:segment->xyzplus;		// existing end node
	pt2=(endchar=='p')?segment->xyzplus:segment->xyzminus;		// new end node
	for(i=0;i<filtype->nsurfint;i++) {
		fsi=filtype->surfint[i];
		if(fsi->action!=FSAblock || !fsi->filpart[part]) continue;
		srf=fsi->othersurf;
		for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
			for(p=0;p<srf->npanel[ps];p++)
				if(lineXpanel(pt1,pt2,srf->panels[ps][p],dim,crosspt,&face1,&face2,NULL,NULL,NULL,0) && (fsi->surfface==PFboth || face1==fsi->surfface)) return 1; }
	return 0; }


/* filSegmentXFilament */
int filSegmentXFilament(const simptr sim,const segmentptr segment,filamentptr *filptr) {
	int f,i,ft,cross;
	double thk,*ptm,*ptp,dist;
	filamentssptr filss;
	filamenttypeptr filtype;
	filamentptr fil,fil2;
	segmentptr segmentm1,segmentp1,segment2;

	fil=segment->fil;
	ptm=segment->xyzminus;
	ptp=segment->xyzplus;
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
					dist=Geo_NearestSeg2SegDist(ptm,ptp,segment2->xyzminus,segment2->xyzplus);
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

	if(klen<=0) return;															// a negative constant is a constraint (filProjectConstraints)
	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	if(dim==2) {
		for(node=nodemin;node<nodemax;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]);
			sforce=-fil->segments[node]->thk*klen*(len-stdlen)/len;			// this is force/length; the force constant scales with segment thickness
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
			sforce=-fil->segments[node]->thk*klen*(len-stdlen)/len;			// this is force/length; the force constant scales with segment thickness
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
	if(klen<=0) return;															// a negative constant is a constraint (filProjectConstraints)

	if(dim==2) {
		for(node=0;node<fil->nseg;node++) {
			xvect[0]=nodes[node+1][0]-nodes[node][0];
			xvect[1]=nodes[node+1][1]-nodes[node][1];
			len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]);
			sforce=-fil->segments[node]->thk*klen*(len-stdlen)/len;			// this is force/length; the force constant scales with segment thickness
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
			sforce=-fil->segments[node]->thk*klen*(len-stdlen)/len;			// this is force/length; the force constant scales with segment thickness
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


/* filDrawThermalForces */
// Thermal force amplitude from the fluctuation-dissipation theorem for the overdamped
// update x += dt*mobility*nodemobility*F used by filStepDynamics: each Cartesian
// component of each node's thermal force is Gaussian with rms
// sqrt(2*kT/(mobility*nodemobility*dt)), which samples the correct ensemble at
// temperature kT for any choice of dt, mobility, and discretization. Forces are drawn
// once per time step and reused across sub-step force evaluations (Runge-Kutta stages
// and Jacobian calls). Random numbers are always drawn and the amplitude multiplies
// the draw, so the draw sequence is independent of kT and mobility values; immobile
// nodes (nodemobility 0) correctly receive no thermal force. In 3D, each segment also
// receives a thermal torque about its own axis, with rms sqrt(2*kT/(mobility*m*dt)),
// where m is the segment's roll mobility factor, the average of its two nodes' factors
// (see filStepDynamics), so that twist samples the same ensemble. These are drawn after
// the node forces.
void filDrawThermalForces(filamentptr fil) {
	double kT,dt,mobility,famp,nodemob,frms;
	filamenttypeptr filtype;
	filamentworkptr filwork;
	int dim,node,seg;
	simptr sim;

	filtype=fil->filtype;
	filwork=fil->filwork;
	sim=filtype->filss->sim;
	dim=sim->dim;

	if(sim->time>filwork->thermtime) {				// compute random forces on each node
		kT=filtype->kT;
		dt=sim->dt;
		mobility=filtype->mobility;
		famp=(dt>0 && mobility>0)?2*kT/(mobility*dt):0;			// loop-invariant part; per-node frms = sqrt(famp/nodemobility)
		if(dim==2)
			for(node=0;node<=fil->nseg;node++) {
				nodemob=filNodeMobility(fil,node);
				frms=(famp>0 && nodemob>0)?sqrt(famp/nodemob):0;
				filwork->thermforce[node][0]=frms*gaussrandD();
				filwork->thermforce[node][1]=frms*gaussrandD(); }
		else
			for(node=0;node<=fil->nseg;node++) {
				nodemob=filNodeMobility(fil,node);
				frms=(famp>0 && nodemob>0)?sqrt(famp/nodemob):0;
				filwork->thermforce[node][0]=frms*gaussrandD();
				filwork->thermforce[node][1]=frms*gaussrandD();
				filwork->thermforce[node][2]=frms*gaussrandD(); }
		if(dim==3)
			for(seg=0;seg<fil->nseg;seg++) {
				nodemob=0.5*(filNodeMobility(fil,seg)+filNodeMobility(fil,seg+1));	// segment roll mobility factor
				frms=(famp>0 && nodemob>0)?sqrt(famp/nodemob):0;
				filwork->thermtorque[seg]=frms*gaussrandD(); }
		filwork->thermtime=sim->time; }
	return; }


/* filAddThermalForces */
// Adds the thermal forces, and in 3D the thermal torques, for this time step (see
// filDrawThermalForces) to the filament's forces and torques, for nodes nodemin to
// nodemax and the segments between them.
void filAddThermalForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,*torques;
	filamentworkptr filwork;
	int dim,node,seg;

	filwork=fil->filwork;
	dim=fil->filtype->filss->sim->dim;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	filDrawThermalForces(fil);
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
	if(dim==3) {
		torques=filwork->torques;
		for(seg=nodemin;seg<nodemax;seg++)
			torques[seg]+=filwork->thermtorque[seg]; }

	return; }


/* filAddMatrixForceStep */
// Adds the displacements for one time step from the forces that the matrix integrators'
// force matrix doesn't include, which are the thermal, junction, and confinement forces,
// to flattened state flatvect (see filFlattenNodes): dtmu times the node mobility factor
// times the force for each node coordinate, and in 3D, dtmu times the segment roll
// mobility factor times the torque for each absolute segment roll. These are the same
// displacements that filStepDynamics makes for these forces. This overwrites the
// filament's forces and torques.
void filAddMatrixForceStep(filamentptr fil,double dtmu,double *flatvect) {
	filamentworkptr filwork;
	double nodemob,**forces,*torques;
	int dim,node,d;

	filwork=fil->filwork;
	forces=filwork->forces;
	torques=filwork->torques;
	dim=fil->filtype->filss->sim->dim;
	for(node=0;node<=fil->nseg;node++)
		forces[node][0]=forces[node][1]=forces[node][2]=0;
	for(node=0;node<fil->nseg;node++)
		torques[node]=0;
	filAddThermalForces(fil,-1,-1);
	filAddJunctionForces(fil,-1,-1);
	filAddSurfForces(fil,-1,-1);
	if(dim==2) {
		for(node=0;node<=fil->nseg;node++)
			for(d=0;d<2;d++)
				flatvect[2*node+d]+=dtmu*filNodeMobility(fil,node)*forces[node][d]; }
	else {
		for(node=0;node<=fil->nseg;node++) {
			for(d=0;d<3;d++)
				flatvect[4*node+d]+=dtmu*filNodeMobility(fil,node)*forces[node][d];
			if(node<fil->nseg) {
				nodemob=0.5*(filNodeMobility(fil,node)+filNodeMobility(fil,node+1));
				flatvect[4*node+3]+=dtmu*nodemob*torques[node]; }}}
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
			if(node<fil->nseg) {
				xvect[0]=nodes[node+1][0]-nodes[node][0];								// Delta x_i
				xvect[1]=nodes[node+1][1]-nodes[node][1];
				len2=xvect[0]*xvect[0]+xvect[1]*xvect[1];								// length of segment i
				len2inv=1.0/len2;
				bvecti[2]=len2inv*(bendtorqueip1[2]-bendtorquei[2]); }		// b_i
			else
				bvecti[2]=0;																						// there is no segment past the plus end node
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
			if(node<fil->nseg) {
				xvect[0]=nodes[node+1][0]-nodes[node][0];								// Delta x_i
				xvect[1]=nodes[node+1][1]-nodes[node][1];
				xvect[2]=nodes[node+1][2]-nodes[node][2];
				len2=xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2];		// length of segment i
				len2inv=1.0/len2;
				len=sqrt(len2);
				bvecti[0]=len2inv*(bendtorqueip1[0]-bendtorquei[0]);
				bvecti[1]=len2inv*(bendtorqueip1[1]-bendtorquei[1]);
				bvecti[2]=len2inv*(bendtorqueip1[2]-bendtorquei[2]); }		// b_i
			else
				bvecti[0]=bvecti[1]=bvecti[2]=len=0;										// there is no segment past the plus end node
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


/* filJunctionParams */
// Spring parameters for junction junct, from the attached filament's type. For a
// branch-angle junction: kappa and theta0 are branch_force_angle and branch_angle, and
// kaz is branch_force_azimuth, with kappa and kaz times the average thickness of the
// attached filament's end segment and the target's reference segment (see
// filJunctionGeometry). For an end-to-end join (bend): kappa is the yaw bending
// force constant, times the average thickness of the two joined segments, and theta0 is
// the standard bending angle, so the join bends like any other node, and there is no
// azimuthal spring. kj is the spring constant that holds a loop-closing junction's two
// nodes together: junction_force, or force_length if that is unset, times the thickness
// of the attached filament's end segment, as for the segments' own force constants.
void filJunctionParams(const filjunctionptr junct,int dim,double *kappaptr,double *theta0ptr,double *kazptr,double *kjptr) {
	filamenttypeptr jtype;
	filamentptr fil;
	double thk,tthk;

	jtype=junct->fil->filtype;
	fil=junct->fil;
	thk=(fil->nseg>0)?fil->segments[junct->end=='m'?0:fil->nseg-1]->thk:1;	// thickness of the attached filament's end segment
	if(junct->bend) {																// like any other node, the force constant scales with the average thickness of the two segments
		tthk=(junct->target->nseg>0)?junct->target->segments[junct->node==0?0:junct->target->nseg-1]->thk:1;
		*kappaptr=jtype->kypr[0]>0?0.5*(thk+tthk)*jtype->kypr[0]:0;
		*theta0ptr=(dim==3)?sqrt(jtype->stdypr[0]*jtype->stdypr[0]+jtype->stdypr[1]*jtype->stdypr[1]):fabs(jtype->stdypr[0]);
		*kazptr=0; }
	else {																					// branch springs scale with the average thickness of the attached segment and the target's reference segment
		tthk=(junct->target->nseg>0)?junct->target->segments[junct->node>0?junct->node-1:0]->thk:1;
		*kappaptr=0.5*(thk+tthk)*jtype->branchforceangle;
		*theta0ptr=jtype->branchangle;
		*kazptr=0.5*(thk+tthk)*jtype->branchforceazimuth; }
	*kjptr=thk*(jtype->junctforce>0?jtype->junctforce:(jtype->klen>0?jtype->klen:0));
	return; }


/* filJunctionGeometry */
// Shared geometry for the junction angle springs. tm is the unit tangent of the target's
// reference segment, from its node ma to its node mb: for a branch-angle junction, this is
// the segment that ends at the junction node (segment 0 for node 0), pointing toward the
// plus end; for an end-to-end join, it is the target's end segment pointing into the
// junction, so that a straight join has theta = 0. td is the unit tangent of the attached
// filament's junction segment, pointing away from the junction. Also returns their
// cosine and sine and the two segment lengths. Returns 1 when the junction force is well
// defined, 0 when it is degenerate (zero-length segment, or theta at 0 or pi where the
// angle plane is undefined -- >5 sigma from any realistic branch angle, so skipping is
// safe).
int filJunctionGeometry(const filjunctionptr junct,int dim,double *tm,double *td,double *lenmptr,double *lendptr,double *costhptr,double *sinthptr,int *maptr,int *mbptr) {
	double lenm,lend,costh,sinth;
	filamentptr mother,daughter;
	int d,node,ma,mb,jnear,jfar;

	mother=junct->target;
	daughter=junct->fil;
	node=junct->node;
	if(junct->bend && node==0) {ma=1;mb=0;}
	else if(node==0) {ma=0;mb=1;}
	else {ma=node-1;mb=node;}
	lenm=0;
	for(d=0;d<dim;d++) {
		tm[d]=mother->nodes[mb][d]-mother->nodes[ma][d];
		lenm+=tm[d]*tm[d]; }
	filBranchJunctionNodes(daughter,junct->end,&jnear,&jfar);
	lend=0;
	for(d=0;d<dim;d++) {
		td[d]=daughter->nodes[jfar][d]-daughter->nodes[jnear][d];
		lend+=td[d]*td[d]; }
	if(lenm<=0 || lend<=0) return 0;
	lenm=sqrt(lenm);
	lend=sqrt(lend);

	costh=0;
	for(d=0;d<dim;d++) {
		tm[d]/=lenm;
		td[d]/=lend;
		costh+=tm[d]*td[d]; }
	if(costh>1) costh=1;
	else if(costh<-1) costh=-1;
	sinth=sqrt(1-costh*costh);
	if(sinth<FILJUNCTSINMIN) return 0;

	*lenmptr=lenm;
	*lendptr=lend;
	*costhptr=costh;
	*sinthptr=sinth;
	*maptr=ma;
	*mbptr=mb;
	return 1; }


/* filSubtreeSpringForce */
// Adds to sum the net force that the springs of loop-closing junctions exert on fil and
// on the filaments that are rigidly attached to it, directly or through others. A spring
// with both ends in this subtree contributes equal and opposite forces, which cancel.
// filAddJunctionForces transmits this sum through the rigid junction that pins fil.
void filSubtreeSpringForce(filamentptr fil,int dim,double *sum) {
	filjunctionptr junct;
	filamentptr other;
	double kappa,theta0,kaz,kj;
	int e,j,d,jnear,jfar;

	for(e=0;e<2;e++) {															// fil's own ends that close loops
		junct=fil->endjunct[e];
		if(!junct || junct->rigid || fil->nseg<1 || junct->node>junct->target->nseg) continue;
		filJunctionParams(junct,dim,&kappa,&theta0,&kaz,&kj);
		if(kj<=0) continue;
		filBranchJunctionNodes(fil,junct->end,&jnear,&jfar);
		for(d=0;d<dim;d++)
			sum[d]+=kj*(junct->target->nodes[junct->node][d]-fil->nodes[jnear][d]); }

	for(j=0;j<fil->njunct;j++) {										// junctions attached to fil's nodes
		junct=fil->juncts[j];
		other=junct->fil;
		if(junct->rigid) {
			if(other!=fil) filSubtreeSpringForce(other,dim,sum); }	// rigid junctions are acyclic, so this recursion ends
		else if(other->nseg>=1 && junct->node<=fil->nseg) {
			filJunctionParams(junct,dim,&kappa,&theta0,&kaz,&kj);
			if(kj<=0) continue;
			filBranchJunctionNodes(other,junct->end,&jnear,&jfar);
			for(d=0;d<dim;d++)
				sum[d]+=kj*(other->nodes[jnear][d]-fil->nodes[junct->node][d]); }}
	return; }


/* filAddJunctionForces */
// Junction springs, for every junction that involves fil.
// Angle spring: energy 0.5*k*(theta-theta0)^2, where theta is the angle between the
// target's reference segment and the attached filament's junction segment (see
// filJunctionGeometry and filJunctionParams). Each filament applies the exact gradient
// with respect to its OWN nodes during its own force evaluation, reading the partner's
// current geometry as an external field: an attached filament applies a couple on its
// junction nodes, and a target applies the reaction couple on its reference segment
// nodes for each junction attached to it. Both couples are pure torques about the
// junction and sum to zero total torque at any one configuration (the sequential
// per-filament integrator sweep evaluates the two halves at states one substep apart,
// so cancellation over a step is O(dt)), so the pair potential enters both partners'
// dynamics and the junction angle equilibrates at Boltzmann -- a one-way version was
// measured to overheat the angle by (1 + mobility ratio) when the mother is free.
// Azimuthal spring (branch_force_azimuth): restores the attached filament's azimuth
// about the target's axis toward its recorded value with energy 0.5*kaz*(phi-phi0)^2,
// applied as a couple on the attached filament's junction nodes only (one-way: a clean
// mother-side gradient would require differentiating the material frame, and the
// bounded spring torque cannot pump energy the way the rigid azimuth pin can).
// Position spring: a loop-closing junction, which is not held by translation, is held
// by energy 0.5*kj*d^2, where d is the distance between the attached end node and the
// target node; each side applies kj times the separation to its own node. A rigid
// junction transmits the net force of these springs on the attached subtree
// (filSubtreeSpringForce) as a pair of joint forces: the target receives it at its
// junction node, and the attached filament receives the opposite force at its pinned
// node. Without this, pinning would discard the force, and a spring within one tree would
// propel and rotate the tree; with it, the attached filament has no net spring force and
// the spring torque on it acts about the junction.
// No cross-filament force writes occur, per-filament force clearing stays safe, and
// integrators that differentiate forces numerically see everything within a filament
// automatically. Draws no random numbers.
void filAddJunctionForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,kappa,kaz,theta0,kj,tm[3],td[3],lenm,lend,costh,sinth,sinbound,fscale,fvect[3],dphi,cross[3],sum[3];
	filamentptr other;
	filjunctionptr junct;
	int j,d,dim,e,jnear,jfar,ma,mb,node;

	if(!fil->endjunct[0] && !fil->endjunct[1] && fil->njunct==0) return;
	dim=fil->filtype->filss->sim->dim;
	forces=fil->filwork->forces;

	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;

	for(e=0;e<2;e++) {															// attached side: forces on own junction nodes, for each attached end
		junct=fil->endjunct[e];
		if(!junct || fil->nseg<1) continue;
		filBranchJunctionNodes(fil,junct->end,&jnear,&jfar);
		if((jnear<nodemin || jnear>nodemax) && (jfar<nodemin || jfar>nodemax)) continue;	// junction outside the requested node window
		node=junct->node;
		if(node<FILJUNCTMINNODE || node>junct->target->nseg) continue;
		filJunctionParams(junct,dim,&kappa,&theta0,&kaz,&kj);
		if((kappa>0 || (kaz>0 && dim==3)) && filJunctionGeometry(junct,dim,tm,td,&lenm,&lend,&costh,&sinth,&ma,&mb)) {
			if(kappa>0) {
				fscale=kappa*(acos(costh)-theta0)/(lend*sinth);		// F1 = k*(theta-theta0)/lend * (tm-costh*td)/sinth; |tm-costh*td| = sinth, so |F1| = k*|theta-theta0|/lend, bounded
				for(d=0;d<dim;d++) {
					fvect[d]=fscale*(tm[d]-costh*td[d]);
					if(jfar>=nodemin && jfar<=nodemax) forces[jfar][d]+=fvect[d];
					if(jnear>=nodemin && jnear<=nodemax) forces[jnear][d]-=fvect[d]; }}		// couple: equal and opposite on the pinned node
			if(kaz>0 && dim==3 && filBranchDazim(junct,&dphi)) {	// azimuthal spring toward the recorded birth azimuth
				cross[0]=tm[1]*td[2]-tm[2]*td[1];							// dphi/dr1 = (tm x td)/(lend*sinth^2); rotating about +tm raises phi -- this sign ties to the frame convention in filBranchAzimuth, see the contract comment there
				cross[1]=tm[2]*td[0]-tm[0]*td[2];
				cross[2]=tm[0]*td[1]-tm[1]*td[0];
				sinbound=(sinth>FILJUNCTSINBOUND)?sinth:FILJUNCTSINBOUND;	// |cross| = sinth, so |F| = kaz*|dphi|/(lend*sinbound): the true azimuthal gradient diverges as 1/sinth near-parallel geometry, where azimuth stops being meaningful; bounding it keeps a wandering junction from blowing up the integrator
				fscale=-kaz*dphi/(lend*sinth*sinbound);
				for(d=0;d<3;d++) {
					fvect[d]=fscale*cross[d];
					if(jfar>=nodemin && jfar<=nodemax) forces[jfar][d]+=fvect[d];
					if(jnear>=nodemin && jnear<=nodemax) forces[jnear][d]-=fvect[d]; }}}
		if(!junct->rigid && kj>0 && jnear>=nodemin && jnear<=nodemax)	// position spring for a loop-closing junction
			for(d=0;d<dim;d++)
				forces[jnear][d]+=kj*(junct->target->nodes[node][d]-fil->nodes[jnear][d]);
		if(junct->rigid && fil->filtype->filss->nspring>0 && jnear>=nodemin && jnear<=nodemax) {	// reaction at the pinned node to the net spring force on this subtree
			sum[0]=sum[1]=sum[2]=0;
			filSubtreeSpringForce(fil,dim,sum);
			for(d=0;d<dim;d++)
				forces[jnear][d]-=sum[d]; }}

	for(j=0;j<fil->njunct;j++) {										// target side: reaction couple for the polar spring (azimuth is one-way), and the position spring
		junct=fil->juncts[j];
		other=junct->fil;
		node=junct->node;
		if(other->nseg<1) continue;
		if(node<FILJUNCTMINNODE || node>fil->nseg) continue;
		filJunctionParams(junct,dim,&kappa,&theta0,&kaz,&kj);
		if(kappa>0 && filJunctionGeometry(junct,dim,tm,td,&lenm,&lend,&costh,&sinth,&ma,&mb) && !((ma<nodemin || ma>nodemax) && (mb<nodemin || mb>nodemax))) {
			fscale=kappa*(acos(costh)-theta0)/(lenm*sinth);		// F_mother = k*(theta-theta0)/lenm * (td-costh*tm)/sinth, bounded like the attached side
			for(d=0;d<dim;d++) {
				fvect[d]=fscale*(td[d]-costh*tm[d]);
				if(mb>=nodemin && mb<=nodemax) forces[mb][d]+=fvect[d];
				if(ma>=nodemin && ma<=nodemax) forces[ma][d]-=fvect[d]; }}
		if(!junct->rigid && kj>0 && node>=nodemin && node<=nodemax) {
			filBranchJunctionNodes(other,junct->end,&jnear,&jfar);
			for(d=0;d<dim;d++)
				forces[node][d]+=kj*(other->nodes[jnear][d]-fil->nodes[node][d]); }
		if(junct->rigid && fil->filtype->filss->nspring>0 && node>=nodemin && node<=nodemax) {	// net spring force on the rigidly attached subtree acts here
			sum[0]=sum[1]=sum[2]=0;
			filSubtreeSpringForce(other,dim,sum);
			for(d=0;d<dim;d++)
				forces[node][d]+=sum[d]; }}

	return; }


/* filNodeThickness */
// Returns the thickness of node node of filament fil, which is the average thickness of
// the segments on either side of it, or the thickness of the one segment for an end
// node, or 1 if the filament has no segments.
double filNodeThickness(const filamentptr fil,int node) {
	if(fil->nseg<1) return 1;
	if(node<=0) return fil->segments[0]->thk;
	if(node>=fil->nseg) return fil->segments[fil->nseg-1]->thk;
	return 0.5*(fil->segments[node-1]->thk+fil->segments[node]->thk); }


/* filNodeMobility */
// Returns the effective mobility factor of node node of filament fil, which is its node
// mobility factor, divided by its thickness (filNodeThickness) if the filament type's
// thickdrag value is 1, so that thicker filaments have proportionally more drag. The
// mobility of node node is the filament type's mobility times this. Segment rotation
// mobility factors are the averages of these for the segment's two nodes.
double filNodeMobility(const filamentptr fil,int node) {
	if(fil->filtype->thickdrag) return fil->nodemobility[node]/filNodeThickness(fil,node);
	return fil->nodemobility[node]; }


/* filSurfForceOn */
// Returns 1 if filament type filtype has any force interactions with surfaces that have
// positive force constants, and 0 if not.
int filSurfForceOn(const filamenttypeptr filtype) {
	int i;

	for(i=0;i<filtype->nsurfint;i++)
		if(filtype->surfint[i]->action==FSAforce && filtype->surfint[i]->kforce>0) return 1;
	return 0; }


/* filAddSurfForces */
// Forces from the force interactions of the filament type with surfaces (see
// surface_interaction and confine_surface), which are harmonic surface confinement,
// in the spirit of Cytosim's per-model-point "confine" stiffness. Each interaction
// applies to the filament parts that it lists, where the minus and plus parts are
// nodes 0 and nseg and the side part is the interior nodes. Each such node on the
// penalized side of the interaction's surface (the side opposite the interaction's
// face, which is the side the filament is on) feels the restoring force k*d toward the nearest
// point of every panel it violates -- the gradient of a penalty energy 0.5*k*d^2 in
// the penetration depth d past that panel. Summing over violated panels makes box
// corners behave (each wall pushes along its own normal). The constant is per NODE,
// like the other per-element force constants, so refining standard_length stiffens
// the wall per unit contour length, and it is the force constant times the node's thickness,
// which is the average thickness of its adjacent segments, as for the other filament
// force constants; the equilibrium penetration depth of a fluctuating node is the
// half-Gaussian spread sqrt(kT/k). This is a soft steric
// wall for filament mechanics only -- molecules do not see it, and richer
// filament-surface interaction (hard collisions, Brownian-ratchet load feedback)
// remains future work. Two geometric caveats: the side test is panelside's, which
// for rect, tri, and disk panels classifies against the panel's INFINITE plane, so
// a confining surface should span or enclose the region the filaments occupy -- a
// partial patch also penalizes nodes laterally beyond it, pulling them toward the
// patch edge; and coplanar tessellations stack, since the constant is per violated
// panel, so a node behind M coplanar panels feels up to M contributions. Draws no
// random numbers; returns immediately when off.
void filAddSurfForces(filamentptr fil,int nodemin,int nodemax) {
	double **forces,k,kn,pnlpt[3],*nodept,*fnode;
	filamenttypeptr filtype;
	filsurfinteractptr fsi;
	surfaceptr srf;
	panelptr pnl;
	enum PanelShape ps;
	enum PanelFace badface;
	int i,node,p,d,dim,part;

	filtype=fil->filtype;
	if(filtype->nsurfint==0 || fil->nseg<1) return;
	if(nodemin<0) nodemin=0;
	if(nodemax<0 || nodemax>fil->nseg) nodemax=fil->nseg;
	dim=filtype->filss->sim->dim;
	forces=fil->filwork->forces;

	for(i=0;i<filtype->nsurfint;i++) {
		fsi=filtype->surfint[i];
		k=fsi->kforce;
		if(fsi->action!=FSAforce || k<=0) continue;
		srf=fsi->othersurf;
		badface=(fsi->surfface==PFfront)?PFback:PFfront;
		for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
			for(p=0;p<srf->npanel[ps];p++) {
				pnl=srf->panels[ps][p];										// panel-outer: fetch each panel once per node sweep
				for(node=nodemin;node<=nodemax;node++) {
					part=(node==0)?FPminus:((node==fil->nseg)?FPplus:FPside);
					if(!fsi->filpart[part]) continue;
					nodept=fil->nodes[node];
					if(panelside(nodept,pnl,dim,NULL,0,0)!=badface) continue;
					closestpanelpt(pnl,dim,nodept,pnlpt,0);
					kn=k*filNodeThickness(fil,node);
					fnode=forces[node];
					for(d=0;d<dim;d++)
						fnode[d]+=kn*(pnlpt[d]-nodept[d]); }}}
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
	filAddJunctionForces(fil,nodemin,nodemax);
	filAddSurfForces(fil,nodemin,nodemax);
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
// Computes an approximate Jacobian of the forces and segment torques on filament fil, for
// filImplicitDynamics, putting it in filwork->forcemat. The stretching, bending, and
// twisting parts are computed analytically in Gauss-Newton form, which is symmetric and
// negative semidefinite, so the implicit step stays stable; the junction and confinement
// parts are computed by finite differences. It also leaves the flattened state in
// flatnodes[0] and the forces at that state in flatforces[0].
#define FILDERIVDXMIN 1e-7			// smallest finite-difference perturbation, relative to standard_length
void filComputeDerivForceMat(filamentptr fil,double dtmu) {
	int dim,stride,flatsize,node,seg,d,d2,a,i,j,ng,gidx[11],nmin,nmax,side,nrow;
	double **nodes,**flatnodes,**flatforces,**forces,*extforce0;
	double klen,stdlen,*kypr,k,len,u[3],kperp,blk,xm[3],xp[3],lm2,lp2,lm,lp,e[3][3],unit[3],cm[3],cp3[3],g[11];
	double cy,sy,cp,sp,w[3][3],thkavg;
	double dx,dxinv,dxmin,col[2][15],colnorm[2];
	filamenttypeptr filtype;
	sparsematrix forcemat;

	filtype=fil->filtype;
	dim=filtype->filss->sim->dim;
	stride=(dim==2)?2:4;
	flatsize= (dim==2) ? 2*(fil->nseg+1) : 4*(fil->nseg+1)-1;
	forcemat=fil->filwork->forcemat;
	sparseSetSizeM(forcemat,flatsize,flatsize);
	sparseClearM(forcemat);

	nodes=fil->nodes;
	forces=fil->filwork->forces;
	flatforces=fil->filwork->flatforces;
	flatnodes=fil->filwork->flatnodes;
	klen=filtype->klen;
	stdlen=filtype->stdlen;
	kypr=filtype->kypr;

	filFlattenNodes(fil,0,0);																		// flatnodes[0] is original node positions
	filComputeForces(fil,-1,-1);
	filFlattenForces(fil,0,-1,-1);															// flatforces[0] is original forces

	if(klen>0)																									// stretching: Hessian of 0.5*thk*klen*(len-stdlen)^2, without its transverse part for a compressed segment, where that part is destabilizing
		for(seg=0;seg<fil->nseg;seg++) {
			len=0;
			for(d=0;d<dim;d++) {
				u[d]=nodes[seg+1][d]-nodes[seg][d];
				len+=u[d]*u[d]; }
			len=sqrt(len);
			for(d=0;d<dim;d++) u[d]/=len;
			k=fil->segments[seg]->thk*klen;													// the force constant scales with segment thickness
			kperp=(len>stdlen)?k*(1-stdlen/len):0;
			for(d=0;d<dim;d++)
				for(d2=0;d2<dim;d2++) {
					blk=(k-kperp)*u[d]*u[d2]+(d==d2?kperp:0);
					sparseAddToM(forcemat,stride*seg+d,stride*seg+d2,-blk);
					sparseAddToM(forcemat,stride*(seg+1)+d,stride*(seg+1)+d2,-blk);
					sparseAddToM(forcemat,stride*seg+d,stride*(seg+1)+d2,blk);
					sparseAddToM(forcemat,stride*(seg+1)+d,stride*seg+d2,blk); }}

	for(node=1;node<fil->nseg;node++) {													// bending and twisting: Gauss-Newton -k.g.g^T for each angle, with g its gradient
		lm2=lp2=0;
		for(d=0;d<dim;d++) {
			xm[d]=nodes[node][d]-nodes[node-1][d];									// segment node-1
			xp[d]=nodes[node+1][d]-nodes[node][d];									// segment node
			lm2+=xm[d]*xm[d];
			lp2+=xp[d]*xp[d]; }
		thkavg=0.5*(fil->segments[node-1]->thk+fil->segments[node]->thk);	// bending force constants scale with the average thickness of the two segments
		if(dim==2) {
			k=thkavg*kypr[0];
			if(k<=0) continue;
			cm[0]=-xm[1]/lm2;																				// gradient of the bend angle with respect to node-1
			cm[1]=xm[0]/lm2;
			cp3[0]=-xp[1]/lp2;																				// gradient with respect to node+1
			cp3[1]=xp[0]/lp2;
			for(d=0;d<2;d++) {
				g[d]=cm[d];
				g[2+d]=-cm[d]-cp3[d];
				g[4+d]=cp3[d]; }
			ng=6;
			for(i=0;i<ng;i++) gidx[i]=2*(node-1)+i;
			for(i=0;i<ng;i++)
				for(j=0;j<ng;j++)
					sparseAddToM(forcemat,gidx[i],gidx[j],-k*g[i]*g[j]); }
		else {																										// 3D: yaw, pitch, and roll of segment node relative to segment node-1
			lm=sqrt(lm2);
			lp=sqrt(lp2);
			for(a=0;a<3;a++) {
				unit[0]=unit[1]=unit[2]=0;
				unit[a]=1;
				Sph_QtniRotate(fil->segments[node-1]->qabs,unit,e[a]); }	// axes of segment node-1 in the system frame
			cy=cos(fil->segments[node]->ypr[0]);
			sy=sin(fil->segments[node]->ypr[0]);
			cp=cos(fil->segments[node]->ypr[1]);
			sp=sin(fil->segments[node]->ypr[1]);
			if(cp<FILCOSPMIN) cp=FILCOSPMIN;										// bound the gradient near gimbal lock, where yaw is undefined
			for(d=0;d<3;d++) {																			// yaw, pitch, and roll change by w[a].omega, for relative rotation omega (inverse Euler-rate relations, as in filBendTorque)
				w[0][d]=e[2][d]+sp/cp*(cy*e[0][d]+sy*e[1][d]);
				w[1][d]=-sy*e[0][d]+cy*e[1][d];
				w[2][d]=(cy*e[0][d]+sy*e[1][d])/cp; }
			ng=11;
			for(d=0;d<3;d++) {
				gidx[d]=4*(node-1)+d;
				gidx[3+d]=4*node+d;
				gidx[6+d]=4*(node+1)+d; }
			gidx[9]=4*(node-1)+3;																		// absolute roll of segment node-1
			gidx[10]=4*node+3;																			// absolute roll of segment node
			for(a=0;a<3;a++) {
				k=thkavg*kypr[a];
				if(k<=0) continue;
				cm[0]=(w[a][1]*xm[2]-w[a][2]*xm[1])/lm2;								// relative rotation is (xp x dxp)/lp^2+up.dphip-(xm x dxm)/lm^2-um.dphim
				cm[1]=(w[a][2]*xm[0]-w[a][0]*xm[2])/lm2;
				cm[2]=(w[a][0]*xm[1]-w[a][1]*xm[0])/lm2;
				cp3[0]=(w[a][1]*xp[2]-w[a][2]*xp[1])/lp2;
				cp3[1]=(w[a][2]*xp[0]-w[a][0]*xp[2])/lp2;
				cp3[2]=(w[a][0]*xp[1]-w[a][1]*xp[0])/lp2;
				for(d=0;d<3;d++) {
					g[d]=cm[d];
					g[3+d]=-cm[d]-cp3[d];
					g[6+d]=cp3[d]; }
				g[9]=-(w[a][0]*xm[0]+w[a][1]*xm[1]+w[a][2]*xm[2])/lm;		// rolling segment node-1
				g[10]=(w[a][0]*xp[0]+w[a][1]*xp[1]+w[a][2]*xp[2])/lp;		// rolling segment node, which only changes roll
				for(i=0;i<ng;i++)
					for(j=0;j<ng;j++)
						sparseAddToM(forcemat,gidx[i],gidx[j],-k*g[i]*g[j]); }}}

	if(fil->endjunct[0] || fil->endjunct[1] || fil->njunct>0 || filSurfForceOn(filtype)) {	// junction and surface forces: finite differences
		extforce0=flatnodes[2];																		// scratch space, which filImplicitDynamics overwrites later
		for(node=0;node<=fil->nseg;node++)
			for(d=0;d<dim;d++) forces[node][d]=0;
		filAddJunctionForces(fil,-1,-1);
		filAddSurfForces(fil,-1,-1);
		for(node=0;node<=fil->nseg;node++)
			for(d=0;d<dim;d++) extforce0[stride*node+d]=forces[node][d];
		for(node=0;node<=fil->nseg;node++) {
			nmin=(node<2)?0:node-2;
			nmax=(node+2>fil->nseg)?fil->nseg:node+2;
			for(d=0;d<dim;d++) {
				dx=dtmu/1000*flatforces[0][stride*node+d];					// perturbation scaled to the step
				dxmin=FILDERIVDXMIN*stdlen;
				if(fabs(dx)<dxmin) dx=dxmin;													// zero force still needs its column
				for(side=0;side<2;side++) {														// forward and backward differences
					dxinv=1.0/dx;
					nodes[node][d]+=dx;
					for(i=nmin;i<=nmax;i++)
						for(d2=0;d2<dim;d2++) forces[i][d2]=0;
					filAddJunctionForces(fil,nmin,nmax);
					filAddSurfForces(fil,nmin,nmax);
					nodes[node][d]=flatnodes[0][stride*node+d];
					colnorm[side]=0;
					nrow=0;
					for(i=nmin;i<=nmax;i++)
						for(d2=0;d2<dim;d2++) {
							col[side][nrow]=dxinv*(forces[i][d2]-extforce0[stride*i+d2]);
							colnorm[side]+=col[side][nrow]*col[side][nrow];
							nrow++; }
					dx=-dx; }
				side=(colnorm[1]<colnorm[0])?1:0;										// a difference across a force discontinuity is the larger one
				nrow=0;
				for(i=nmin;i<=nmax;i++)
					for(d2=0;d2<dim;d2++)
						sparseAddToM(forcemat,stride*i+d2,stride*node+d,col[side][nrow++]); }}}

	return; }


/******************************************************************************/
/**************************** dynamics functions ***********************/
/******************************************************************************/


/* filStepDynamics */
void filStepDynamics(filamentptr fil,double dtmu,int in,int out) {
	int node,seg,dim,d;
	double **nodesin,**nodesout,*seg0upout,*rollin,*rollout,*segnew,*dphi;
	double **forces,*nodemobility;
	filamentworkptr filwork;

	dim=fil->filtype->filss->sim->dim;
	filwork=fil->filwork;

	nodesin=filwork->wnodes[in];
	rollin=filwork->wroll[in];
	nodesout=filwork->wnodes[out];
	seg0upout=filwork->wseg0up[out];
	rollout=filwork->wroll[out];
	forces=filwork->forces;

	nodemobility=filwork->flatforces[1];						// effective node mobility factors (filNodeMobility), in scratch space
	for(node=0;node<=fil->nseg;node++) nodemobility[node]=filNodeMobility(fil,node);

	if(dim==2) {
		for(node=0;node<=fil->nseg;node++) {
			nodesout[node][0]=nodesin[node][0]+dtmu*nodemobility[node]*forces[node][0];
			nodesout[node][1]=nodesin[node][1]+dtmu*nodemobility[node]*forces[node][1]; }}
	else {
		if(in==0) filCopyYpr2ToRoll(fil);
		segnew=filwork->flatnodes[2];							// scratch space
		dphi=filwork->flatnodes[1];
		for(seg=0;seg<fil->nseg;seg++) {					// new segment vectors, and each segment's rotation about its own axis from its torque
			for(d=0;d<3;d++)
				segnew[3*seg+d]=nodesin[seg+1][d]+dtmu*nodemobility[seg+1]*forces[seg+1][d]-nodesin[seg][d]-dtmu*nodemobility[seg]*forces[seg][d];
			dphi[seg]=dtmu*0.5*(nodemobility[seg]+nodemobility[seg+1])*filwork->torques[seg]; }
		if(in==0) filWorkFramesFromSegments(fil);
		filTransportFrames(fil,nodesin,filwork->wqabs[in],segnew,dphi,1,filwork->wqabs[out],seg0upout,out==0?rollout:NULL);	// a working state other than 0 only needs its frames
		if(fil->nseg>0) rollout[0]=rollin[0];			// segment 0's orientation is held in seg0up
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


/* filMobilityWeights */
// Puts the effective mobility factor of each flattened coordinate of filament fil in wt:
// filNodeMobility for node positions, and in 3D, the average of the two nodes' values for
// segment rotations. Returns the number of flattened coordinates.
int filMobilityWeights(filamentptr fil,double *wt) {
	int dim,stride,node,seg,d;

	dim=fil->filtype->filss->sim->dim;
	stride=(dim==2)?2:4;
	for(node=0;node<=fil->nseg;node++)
		for(d=0;d<dim;d++) wt[stride*node+d]=filNodeMobility(fil,node);
	if(dim==3)
		for(seg=0;seg<fil->nseg;seg++) wt[4*seg+3]=0.5*(filNodeMobility(fil,seg)+filNodeMobility(fil,seg+1));
	return (dim==2)?2*(fil->nseg+1):4*(fil->nseg+1)-1; }


/* filScaleMatrixRows */
// Multiplies row i of banded matrix sprs by wt[i], for its first nrows rows.
void filScaleMatrixRows(sparsematrix sprs,const double *wt,int nrows) {
	int i,j,jmin,jmax;

	for(i=0;i<nrows;i++) {
		jmin=sprs->col0[i]<0?0:sprs->col0[i];
		jmax=sprs->col1[i]>sprs->fcols?sprs->fcols:sprs->col1[i];
		for(j=jmin;j<jmax;j++) sparseMultiplyBy(sprs,i,j,wt[i]); }
	return; }


/* filEulerMatDynamics */
void filEulerMatDynamics(simptr sim,filamenttypeptr filtype) {
	double dtmu;
	int f,flatsize;
	filamentptr fil;
	filamentworkptr filwork;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filwork=fil->filwork;

		filComputeForceMatrix(fil);
		filFlattenNodes(fil,0,0);
		dtmu=filtype->mobility*sim->dt;
		flatsize=filMobilityWeights(fil,filwork->flatforces[1]);
		filScaleMatrixRows(filwork->forcemat,filwork->flatforces[1],flatsize);	// forcemat is W F, with effective mobility factors W
		sparseMultiplyScalarM(filwork->forcemat,dtmu);
		sparseAddIdentityM(filwork->forcemat,1);
		sparseMultiplyMV(filwork->forcemat,filwork->flatnodes[0],filwork->flatnodes[1]);
		filAddMatrixForceStep(fil,dtmu,filwork->flatnodes[1]);	// thermal, junction, and confinement forces
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }


/* filImplicitOldDynamics */
void filImplicitOldDynamics(simptr sim,filamenttypeptr filtype) {
	double dtmu;
	int f,i,flatsize;
	filamentptr fil;
	filamentworkptr filwork;
	sparsematrix forcemat;

	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		filwork=fil->filwork;
		forcemat=filwork->forcemat;
		if(!forcemat->BiCGSTAB)
			sparseAllocBandM(forcemat,-1,-1,1);
		flatsize=(sim->dim==2)?2*(fil->nseg+1):4*(fil->nseg+1)-1;

		filComputeForceMatrix(fil);
		filFlattenNodes(fil,0,0);													// flatnodes[0] is current state
		dtmu=filtype->mobility*sim->dt;
		filMobilityWeights(fil,filwork->flatforces[1]);
		filScaleMatrixRows(forcemat,filwork->flatforces[1],flatsize);	// forcemat is W F, with effective mobility factors W
		sparseMultiplyScalarM(forcemat,dtmu);
		sparseAddIdentityM(forcemat,1);										// forcemat is 1+dtmu*W*force
		sparseMultiplyMV(forcemat,filwork->flatnodes[0],filwork->flatnodes[1]);	// flatnodes[1] is forward Euler next state, without thermal forces
		for(i=0;i<flatsize;i++) filwork->flatnodes[2][i]=filwork->flatnodes[0][i];
		filAddMatrixForceStep(fil,dtmu,filwork->flatnodes[2]);		// flatnodes[2] is current state plus displacements from thermal, junction, and confinement forces
		for(i=0;i<flatsize;i++) filwork->flatnodes[1][i]+=filwork->flatnodes[2][i]-filwork->flatnodes[0][i];	// initial guess, with the same displacements

		sparseMultiplyScalarM(forcemat,-1);
		sparseAddIdentityM(forcemat,2);										// forcemat is 1-dtmu*W*force
		sparseBiCGSTAB(forcemat,filwork->flatnodes[2],filwork->flatnodes[1],1e-9);	// flatnodes[1] is backward Euler next state
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }



/* filImplicitDynamics */
void filImplicitDynamics(simptr sim,filamenttypeptr filtype) {
	double dtmu,**flatnodes,**flatforces,bnorm2,*wt;
	int f,flatsize,dim,i;
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
		filComputeDerivForceMat(fil,dtmu);															// nodes are in flatnodes[0], forces are in flatforces[0], and J is in forcemat
		wt=flatforces[1];																								// effective mobility factors W (filNodeMobility), in scratch space
		filMobilityWeights(fil,wt);
		filScaleMatrixRows(forcemat,wt,flatsize);												// forcemat is W J
		bnorm2=0;
		for(i=0;i<flatsize;i++) {
			flatnodes[2][i]=dtmu*wt[i]*flatforces[0][i];									// flatnodes[2] is the forward Euler displacement
			flatnodes[1][i]=flatnodes[2][i];															// which is the initial guess
			bnorm2+=flatnodes[2][i]*flatnodes[2][i]; }
		if(bnorm2>0) {
			sparseMultiplyScalarM(forcemat,-dtmu);
			sparseAddIdentityM(forcemat,1);																// forcemat is 1-dtmu*W*J
			sparseBiCGSTAB(forcemat,flatnodes[2],flatnodes[1],1e-20*bnorm2); }	// flatnodes[1] is the backward Euler displacement, to relative residual 1e-10
		for(i=0;i<flatsize;i++)
			flatnodes[1][i]+=flatnodes[0][i];															// flatnodes[1] is the next state
		filUnflattenNodes(fil,1,0);
		filNodes2Angles(fil,-1,-1); }

	return; }



/* filConstrained */
// Returns 1 if filament type filtype has any constraints, meaning a negative length or
// angle force constant, and 0 if not.
int filConstrained(filamenttypeptr filtype) {
	if(filtype->klen<0) return 1;
	if(filtype->kypr[0]<0) return 1;
	if(filtype->filss->sim->dim==3 && (filtype->kypr[1]<0 || filtype->kypr[2]<0)) return 1;
	return 0; }


/* filConstraintGrad */
// Computes constraint number c of filament fil, for filProjectConstraints. Constraints
// are numbered by node, with cstride of them per node (2 in 2D, 4 in 3D): number
// cstride*node is the length of segment node, and the next ones are the yaw, pitch, and
// roll of segment node relative to segment node-1. A length is constrained to the
// standard length if the length force constant is negative, and an angle to its standard
// angle if its force constant is negative. This returns 0 if constraint c doesn't apply.
// Otherwise, it puts the constraint's deviation from its target value in res, and its
// gradient with respect to the flattened coordinates (node positions and, in 3D, segment
// rotations about their own axes) in g, with the coordinate indices in idx, and returns
// the number of coordinates, which is at most 11. The angle gradients are the same as for
// filComputeDerivForceMat.
int filConstraintGrad(filamentptr fil,int c,int *idx,double *g,double *res) {
	int dim,cstride,fs,node,a,d,unitdim;
	double **nodes,u[3],len,r,xm[3],xp[3],lm2,lp2,lm,lp,cm[3],cp3[3],e[3][3],unit[3],w[3];
	double cy,sy,cp,sp;
	filamenttypeptr filtype;

	filtype=fil->filtype;
	dim=filtype->filss->sim->dim;
	cstride=(dim==2)?2:4;
	fs=(dim==2)?2:4;
	nodes=fil->nodes;
	node=c/cstride;
	a=c%cstride-1;

	if(a<0) {																					// length of segment node
		if(filtype->klen>=0 || node>=fil->nseg) return 0;
		len=0;
		for(d=0;d<dim;d++) {
			u[d]=nodes[node+1][d]-nodes[node][d];
			len+=u[d]*u[d]; }
		len=sqrt(len);
		*res=len-filtype->stdlen;
		for(d=0;d<dim;d++) {
			idx[d]=fs*node+d;
			g[d]=-u[d]/len;
			idx[dim+d]=fs*(node+1)+d;
			g[dim+d]=u[d]/len; }
		return 2*dim; }

	if(node<1 || node>=fil->nseg || filtype->kypr[a]>=0) return 0;
	r=fil->segments[node]->ypr[a]-filtype->stdypr[a];
	while(r>PI) r-=2*PI;
	while(r<-PI) r+=2*PI;
	*res=r;
	lm2=lp2=0;
	for(d=0;d<dim;d++) {
		xm[d]=nodes[node][d]-nodes[node-1][d];
		xp[d]=nodes[node+1][d]-nodes[node][d];
		lm2+=xm[d]*xm[d];
		lp2+=xp[d]*xp[d]; }
	if(dim==2) {
		cm[0]=-xm[1]/lm2;
		cm[1]=xm[0]/lm2;
		cp3[0]=-xp[1]/lp2;
		cp3[1]=xp[0]/lp2;
		for(d=0;d<2;d++) {
			idx[d]=2*(node-1)+d;
			g[d]=cm[d];
			idx[2+d]=2*node+d;
			g[2+d]=-cm[d]-cp3[d];
			idx[4+d]=2*(node+1)+d;
			g[4+d]=cp3[d]; }
		return 6; }

	lm=sqrt(lm2);
	lp=sqrt(lp2);
	for(unitdim=0;unitdim<3;unitdim++) {
		unit[0]=unit[1]=unit[2]=0;
		unit[unitdim]=1;
		Sph_QtniRotate(fil->segments[node-1]->qabs,unit,e[unitdim]); }
	cy=cos(fil->segments[node]->ypr[0]);
	sy=sin(fil->segments[node]->ypr[0]);
	cp=cos(fil->segments[node]->ypr[1]);
	sp=sin(fil->segments[node]->ypr[1]);
	if(cp<FILCOSPMIN) cp=FILCOSPMIN;
	for(d=0;d<3;d++) {
		if(a==0) w[d]=e[2][d]+sp/cp*(cy*e[0][d]+sy*e[1][d]);
		else if(a==1) w[d]=-sy*e[0][d]+cy*e[1][d];
		else w[d]=(cy*e[0][d]+sy*e[1][d])/cp; }
	cm[0]=(w[1]*xm[2]-w[2]*xm[1])/lm2;
	cm[1]=(w[2]*xm[0]-w[0]*xm[2])/lm2;
	cm[2]=(w[0]*xm[1]-w[1]*xm[0])/lm2;
	cp3[0]=(w[1]*xp[2]-w[2]*xp[1])/lp2;
	cp3[1]=(w[2]*xp[0]-w[0]*xp[2])/lp2;
	cp3[2]=(w[0]*xp[1]-w[1]*xp[0])/lp2;
	for(d=0;d<3;d++) {
		idx[d]=4*(node-1)+d;
		g[d]=cm[d];
		idx[3+d]=4*node+d;
		g[3+d]=-cm[d]-cp3[d];
		idx[6+d]=4*(node+1)+d;
		g[6+d]=cp3[d]; }
	idx[9]=4*(node-1)+3;
	g[9]=-(w[0]*xm[0]+w[1]*xm[1]+w[2]*xm[2])/lm;
	idx[10]=4*node+3;
	g[10]=(w[0]*xp[0]+w[1]*xp[1]+w[2]*xp[2])/lp;
	return 11; }


/* filConstraintStart */
// Stores the gradients of the constraints of filament fil (see filConstraintGrad) at the
// current state, in filwork->cidx, cng, and cg, for filProjectConstraints. This is called
// at the start of each dynamics step.
void filConstraintStart(filamentptr fil) {
	int c,nc;
	double res;
	filamentworkptr filwork;

	filwork=fil->filwork;
	nc=((fil->filtype->filss->sim->dim==2)?2:4)*fil->nseg;
	for(c=0;c<nc;c++)
		filwork->cng[c]=filConstraintGrad(fil,c,filwork->cidx+11*c,filwork->cg+11*c,&res);
	return; }


/* filProjectConstraints */
// Moves filament fil onto its constraints (see filConstraintGrad), which are a fixed
// length for each segment if the length force constant is negative and fixed relative
// angles for each angle whose force constant is negative. This is done after each
// dynamics step. The coordinates move by -W G0^T lambda, where G0 is the constraint
// gradient matrix at the start of the step (from filConstraintStart) and W is the
// diagonal matrix of mobility factors (node mobility factors for node positions, and
// their averages for segment rotations), so nodes with mobility factor 0 don't move.
// Projecting along the start-of-step gradients, as in SHAKE, makes the dynamics sample
// the Boltzmann distribution of the rigidly constrained system; projecting along the
// gradients at the projected point instead gives a bias that doesn't vanish with the
// time step. Newton iterations find lambda by solving (G W G0^T) dlambda = r, where r is
// the list of constraint deviations and G is the current gradient matrix. This matrix is
// banded, so this uses the filament's force matrix and BiCGSTAB. Segment frames are moved with
// filTransportFrames. Iterations stop when every length deviation is less than
// FILCONSTRAINTTOL times the standard length and every angle deviation is less than
// FILCONSTRAINTTOL. Returns 0 for success, 1 if the iterations did not converge, and 2 for
// a memory allocation failure.
int filProjectConstraints(filamentptr fil) {
	int dim,cstride,fs,nc,flatsize,c,c2,i,k,l,iter,band,*idx,*ng,*idx0,*ng0,er,seg,node,d;
	double *g,*g0,*res,*lambda,*delta,*segnew,*wt,sum,maxres,res2,dev;
	filamentworkptr filwork;
	sparsematrix forcemat;

	if(fil->nseg<1) return 0;
	filwork=fil->filwork;
	forcemat=filwork->forcemat;
	dim=fil->filtype->filss->sim->dim;
	cstride=(dim==2)?2:4;
	fs=(dim==2)?2:4;
	band=(dim==2)?5:11;															// constraints that share a coordinate are at most this far apart
	nc=cstride*fil->nseg;
	flatsize=(dim==2)?2*(fil->nseg+1):4*(fil->nseg+1)-1;
	if(!forcemat->BiCGSTAB)
		sparseAllocBandM(forcemat,-1,-1,1);

	idx0=filwork->cidx;															// gradients at the start of the step
	ng0=filwork->cng;
	g0=filwork->cg;
	res=filwork->flatforces[0];											// scratch space
	lambda=filwork->flatforces[1];
	delta=filwork->flatnodes[2];
	segnew=filwork->flatnodes[1];
	idx=(int*) malloc(11*nc*sizeof(int));
	ng=(int*) malloc(nc*sizeof(int));
	g=(double*) malloc(11*nc*sizeof(double));
	wt=(double*) malloc(flatsize*sizeof(double));
	if(!idx || !ng || !g || !wt) {
		free(idx);
		free(ng);
		free(g);
		free(wt);
		return 2; }

	for(node=0;node<=fil->nseg;node++)							// mobility weights
		for(d=0;d<dim;d++) wt[fs*node+d]=filNodeMobility(fil,node);
	if(dim==3)
		for(seg=0;seg<fil->nseg;seg++) wt[4*seg+3]=0.5*(filNodeMobility(fil,seg)+filNodeMobility(fil,seg+1));

	er=1;
	for(iter=0;iter<FILCONSTRAINTITER;iter++) {
		maxres=res2=0;
		for(c=0;c<nc;c++) {													// deviations and gradients
			ng[c]=filConstraintGrad(fil,c,idx+11*c,g+11*c,&res[c]);
			if(ng[c]) {
				sum=0;
				for(k=0;k<ng[c];k++) sum+=g[11*c+k]*g[11*c+k]*wt[idx[11*c+k]];
				if(sum<=0) ng[c]=0; }													// only immobile coordinates, so it can't be satisfied by moving
			if(!ng[c]) res[c]=0;
			dev=fabs(res[c]);
			if(c%cstride==0) dev/=fil->filtype->stdlen;
			if(dev>maxres) maxres=dev;
			res2+=res[c]*res[c]; }
		if(maxres<FILCONSTRAINTTOL) {
			er=0;
			break; }

		sparseSetSizeM(forcemat,nc,nc);									// G W G0^T
		sparseClearM(forcemat);
		for(c=0;c<nc;c++) {
			lambda[c]=0;
			if(!ng[c] || !ng0[c]) {
				sparseAddToM(forcemat,c,c,1);
				res[c]=0;
				continue; }
			for(c2=(c-band<0?0:c-band);c2<=c+band && c2<nc;c2++) {
				if(!ng[c2] || !ng0[c2]) continue;
				sum=0;
				for(k=0;k<ng[c];k++)
					for(l=0;l<ng0[c2];l++)
						if(idx[11*c+k]==idx0[11*c2+l]) sum+=g[11*c+k]*wt[idx[11*c+k]]*g0[11*c2+l];
				if(sum!=0) sparseAddToM(forcemat,c,c2,sum); }}
		sparseBiCGSTAB(forcemat,res,lambda,1e-24*res2);

		for(i=0;i<flatsize;i++) delta[i]=0;							// delta = -W G0^T lambda
		for(c=0;c<nc;c++)
			if(ng[c])
				for(k=0;k<ng0[c];k++)
					delta[idx0[11*c+k]]-=wt[idx0[11*c+k]]*g0[11*c+k]*lambda[c];

		if(dim==2)
			for(node=0;node<=fil->nseg;node++) {
				fil->nodes[node][0]+=delta[2*node];
				fil->nodes[node][1]+=delta[2*node+1]; }
		else {
			filCopyYpr2ToRoll(fil);
			filWorkFramesFromSegments(fil);
			for(seg=0;seg<fil->nseg;seg++)
				for(d=0;d<3;d++)
					segnew[3*seg+d]=fil->nodes[seg+1][d]+delta[4*(seg+1)+d]-fil->nodes[seg][d]-delta[4*seg+d];
			filTransportFrames(fil,fil->nodes,filwork->wqabs[0],segnew,delta+3,4,filwork->wqabs[0],fil->seg0up,fil->roll);
			for(node=0;node<=fil->nseg;node++)
				for(d=0;d<3;d++) fil->nodes[node][d]+=delta[4*node+d];
			filCopyRollToYpr2(fil); }
		filNodes2Angles(fil,-1,-1); }

	free(idx);
	free(ng);
	free(g);
	free(wt);
	return er; }


/* filUnstable */
// Returns 1 if the mechanics of filament type filtype have become numerically unstable
// and 0 if not. It is unstable if some node coordinate is not finite or some segment is
// longer than 100 times the system diagonal plus the standard length. With a length
// spring, it is also unstable if some segment is longer than 100 times the standard
// length plus the thermal spread of segment lengths. This stops a blown-up simulation
// before branch nucleation, which scales with contour length, creates an unbounded
// number of filaments.
int filUnstable(simptr sim,filamenttypeptr filtype) {
	int f,node,d,dim;
	double maxlen,maxlen2,sprlen,seglen2,len2,dx;
	filamentptr fil;

	dim=sim->dim;
	maxlen=100*(systemdiagonal(sim)+filtype->stdlen);
	maxlen2=maxlen*maxlen;
	for(f=0;f<filtype->nfil;f++) {
		fil=filtype->fillist[f];
		for(node=0;node<=fil->nseg;node++) {
			len2=0;
			for(d=0;d<dim;d++) {
				if(!isfinite(fil->nodes[node][d])) return 1;
				if(node>0) {
					dx=fil->nodes[node][d]-fil->nodes[node-1][d];
					len2+=dx*dx; }}
			if(node==0) continue;
			seglen2=maxlen2;
			if(filtype->klen>0) {
				sprlen=100*(filtype->stdlen+sqrt(filtype->kT/(fil->segments[node-1]->thk*filtype->klen)));
				if(sprlen*sprlen<seglen2) seglen2=sprlen*sprlen; }
			if(!(len2<=seglen2)) return 1; }}
	return 0; }


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
				if(fil->endstate[filtype->treadrate>0?1:0]!=FESopen) continue;	// only an open end takes a monomer
				treadnum=poisrandD(fabs(filtype->treadrate)*sim->dt);
				for(i=0;i<treadnum;i++)
					filTreadmill(sim,fil,filtype->treadrate>0?'p':'m'); }}

		filBranchDynamics(sim,filtype);									// Arp2/3-style branch nucleation
		filEndTransitions(sim,filtype);									// before length changes: an end capped now doesn't also grow now
		filEndDynamics(sim,filtype);										// before the integrator: a new segment relaxes this step

		if(filtype->dynamics!=FDnone && filConstrained(filtype))		// constraint gradients at the start of the step
			for(f=0;f<filtype->nfil;f++)
				filConstraintStart(filtype->fillist[f]);
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

		if(filtype->dynamics!=FDnone && filConstrained(filtype))		// negative force constants are constraints
			for(f=0;f<filtype->nfil;f++) {
				i=filProjectConstraints(filtype->fillist[f]);
				if(i==2) {
					simLog(sim,10,"Out of memory projecting filaments onto constraints\n");
					return 1; }
				if(i==1 && !filtype->constraintwarn) {
					simLog(sim,5,"WARNING: filament %s did not converge onto its constraints (negative force constants) at time %g; the time step may be too long\n",filtype->fillist[f]->filname,sim->time);
					filtype->constraintwarn=1; }}

		if(filtype->dynamics!=FDnone && filUnstable(sim,filtype)) {
			simLog(sim,9,"ERROR: filament type %s became numerically unstable at time %g; reduce time_step, mobility, or force constants, or use dynamics implicit\n",filtype->ftname,sim->time);
			return 1; }}

	filPinJunctions(filss);													// re-pin rigid junctions after all motion this step

	return 0; }




