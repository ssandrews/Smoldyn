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

#include "smoldyn.h"
#include "smoldynfuncs.h"


#define FILMAXTRIES 100


/******************************************************************************/
/********************************** Filaments *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types
char *filFB2string(enum FilamentBiology fb,char *string);
enum FilamentBiology filstring2FB(const char *string);
char *filFD2string(enum FilamentDynamics fd,char *string);
enum FilamentDynamics filstring2FD(const char *string);

// low level utilities
double filRandomLength(const filamenttypeptr filtype,double thickness,double sigmamult);
double *filRandomAngle(const filamenttypeptr filtype,int dim,int n,double thickness,double sigmamult,double *angle);

// computations on filaments
double filStretchEnergy(const filamentptr fil,int seg1,int seg2);
double filBendEnergy(const filamentptr fil,int seg1,int seg2);
void filBendTorque(const filamentptr fil,int seg,double *torque);

// memory management
segmentptr segmentalloc();
void segmentfree(segmentptr segment);
filamentptr filalloc(filamentptr fil,int maxseg,int maxbranch,int maxmonomer);
void filfree(filamentptr fil);
filamenttypeptr filamenttypealloc(filamenttypeptr filtype,int maxfil,int maxface);
void filamenttypefree(filamenttypeptr filtype);
filamentssptr filssalloc(filamentssptr filss,int maxtype);
void filssfree(filamentssptr filss);

// data structure output
void filoutput(const filamentptr fil);
void filtypeoutput(const filamenttypeptr filtype);
void filssoutput(const simptr sim);
void filwrite(const simptr sim,FILE *fptr);
int filcheckparams(const simptr sim,int *warnptr);

// filaments
void filUpdateSegmentIndices(filamentptr fil);
void filArrayShift(filamentptr fil,int shift);
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar);
int filRemoveSegment(filamentptr fil,char endchar);
void filTranslate(filamentptr fil,const double *vect,char func);
void filRotateVertex(filamentptr fil,int seg,const double *angle,char endchar,char func);
void filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func);
void filReverseFilament(filamentptr fil);
int filCopyFilament(filamentptr filto,const filamentptr filfrom);

// filament type
int filtypeSetParam(filamenttypeptr filtype,const char *param,int index,double value);
int filtypeSetColor(filamenttypeptr filtype,const double *rgba);
int filtypeSetEdgePts(filamenttypeptr filtype,double value);
int filtypeSetStipple(filamenttypeptr filtype,int factor,int pattern);
int filtypeSetDrawmode(filamenttypeptr filtype,enum DrawMode dm);
int filtypeSetShiny(filamenttypeptr filtype,double shiny);
int filtypeSetDynamics(filamenttypeptr filtype,enum FilamentDynamics fd);
int filtypeSetBiology(filamenttypeptr filtype,enum FilamentBiology fb);
int filtypeAddFace(filamenttypeptr filtype,const char* facename);
filamenttypeptr filAddFilamentType(simptr sim,const char *ftname);

// filament superstructure
void filsetcondition(filamentssptr filss,enum StructCond cond,int upgrade);
int filenablefilaments(simptr sim);
int filupdateparams(simptr sim);
int filupdatelists(simptr sim);
int filupdate(simptr sim);


// user input





/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* filFB2string */
char *filFB2string(enum FilamentBiology fb,char *string) {
	if(fb==FBactin) strcpy(string,"actin");
	else if(fb==FBmicrotubule) strcpy(string,"microtubule");
	else if(fb==FBintermediate) strcpy(string,"intermediate");
	else if(fb==FBdsDNA) strcpy(string,"dsDNA");
	else if(fb==FBssDNA) strcpy(string,"ssDNA");
	else if(fb==FBother) strcpy(string,"other");
	else strcpy(string,"none");
	return string; }


/* filstring2FB */
enum FilamentBiology filstring2FB(const char *string) {
	enum FilamentBiology ans;

	if(strbegin(string,"actin",0)) ans=FBactin;
	else if(strbegin(string,"microtubule",0)) ans=FBmicrotubule;
	else if(strbegin(string,"intermediate",0)) ans=FBintermediate;
	else if(strbegin(string,"dsDNA",0)) ans=FBdsDNA;
	else if(strbegin(string,"ssDNA",0)) ans=FBssDNA;
	else if(strbegin(string,"other",0)) ans=FBother;
	else ans=FBnone;
	return ans; }


/* filFD2string */
char *filFD2string(enum FilamentDynamics fd,char *string) {
	if(fd==FDeuler) strcpy(string,"euler");
	else strcpy(string,"none");
	return string; }


/* filstring2FD */
enum FilamentDynamics filstring2FD(const char *string) {
	enum FilamentDynamics ans;

	if(strbegin(string,"none",0)) ans=FDnone;
	else if(strbegin(string,"euler",0)) ans=FDeuler;
	else ans=FDnone;
	return ans; }


/******************************************************************************/
/****************************** low level utilities ***************************/
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
double *filRandomAngle(const filamenttypeptr filtype,int dim,int n,double thickness,double sigmamult,double *angle) {
	int d;

	if(n>0 && dim==3) {										// 3D not first segment
		for(d=0;d<3;d++){
			if(filtype->kypr[d]<0)
				angle[d]=filtype->stdypr[d];
			else if(filtype->kypr[d]==0)
				angle[d]=unirandOCD(-PI,PI);
			else
				angle[d]=filtype->stdypr[d]+sigmamult*sqrt(filtype->kT/(thickness*filtype->kypr[d]))*gaussrandD(); }}
	else if(n>0) {												// 2D not first segment
		if(filtype->kypr[0]<0)
			angle[0]=filtype->stdypr[0];
		else if(filtype->kypr[0]==0)
			angle[0]=unirandOCD(-PI,PI);
		else
			angle[0]=filtype->stdypr[0]+sigmamult*sqrt(filtype->kT/(thickness*filtype->kypr[0]))*gaussrandD();
		angle[1]=angle[2]=0; }
	else if(dim==3) {											// 3D first segment
		angle[0]=thetarandCCD();
		angle[1]=unirandCOD(0,2*PI);
		angle[2]=unirandCOD(0,2*PI);
		Sph_Eax2Xyz(angle,angle); }
	else {																// 2D first segment
		angle[0]=unirandOCD(-PI,PI);
		angle[1]=angle[2]=0; }
	return angle; }


/******************************************************************************/
/*************************** computations on filaments ************************/
/******************************************************************************/


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
	if(seg1==-1) seg1=fil->frontseg;
	if(seg2==-1) seg2=fil->frontseg+fil->nseg;
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
	if(seg1==-1) seg1=fil->frontseg;
	if(seg2==-1) seg2=fil->frontseg+fil->nseg-1;
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
	double *kypr,*stdypr,*ypr,cphi,sphi,ctht,stht,deltaypr[3];

	if(node<=0 || node>=fil->nseg) {
		torque[0]=torque[1]=torque[2]=0;
		return; }

	filtype=fil->filtype;
	kypr=filtype->kypr;
	stdypr=filtype->stdypr;
	ypr=fil->segments[fil->frontseg+node]->ypr;

	cphi=cos(ypr[0]);
	sphi=sin(ypr[0]);
	ctht=cos(ypr[1]);
	stht=sin(ypr[1]);

	deltaypr[0]=kypr[0]*(ypr[0]-stdypr[0]);
	deltaypr[1]=kypr[1]*(ypr[1]-stdypr[1]);
	deltaypr[2]=kypr[2]*(ypr[2]-stdypr[2]);

	torque[0]=-deltaypr[2]*ctht*cphi-deltaypr[1]*sphi;
	torque[1]=-deltaypr[1]*cphi+deltaypr[2]*sphi;
	torque[2]=-deltaypr[0]-deltaypr[2]*cphi*stht;

	Sph_DcmtxCart(fil->segments[fil->frontseg+node-1]->adcm,torque,torque);
	return; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/


/* segmentalloc */
segmentptr segmentalloc() {
	segmentptr segment;

	CHECKMEM(segment=(segmentptr) malloc(sizeof(struct segmentstruct)));
	segment->fil=NULL;
	segment->index=0;
	segment->xyzfront=NULL;
	segment->xyzback=NULL;
	segment->len=0;
	segment->thk=1;
	segment->ypr[0]=segment->ypr[1]=segment->ypr[2]=0;
	Sph_One2Dcm(segment->dcm);
	Sph_One2Dcm(segment->adcm);
	return segment;

 failure:
	return NULL; }


/* segmentfree */
void segmentfree(segmentptr segment) {
	if(!segment) return;
	free(segment);
	return; }


/* filalloc */
filamentptr filalloc(filamentptr fil,int maxseg,int maxbranch,int maxmonomer) {
	int seg,br,mn;
	segmentptr *newsegments;
	int *newbranchspots;
	filamentptr *newbranches;
	char *newmonomers;
	double **newnodes,**newforces,*newtorques;

	if(!fil) {
		CHECKMEM(fil=(filamentptr) malloc(sizeof(struct filamentstruct)));
		fil->filtype=NULL;
		fil->filname=NULL;
		fil->maxseg=0;
		fil->nseg=0;
		fil->frontseg=0;
		fil->segments=NULL;
		fil->nodes=NULL;
		fil->forces=NULL;
		fil->torques=NULL;
		fil->frontend=NULL;
		fil->backend=NULL;
		fil->maxbranch=0;
		fil->nbranch=0;
		fil->branchspots=NULL;
		fil->branches=NULL;
		fil->maxmonomer=0;
		fil->nmonomer=0;
		fil->frontmonomer=0;
		fil->monomers=NULL; }

	if(maxseg>fil->maxseg) {
		CHECKMEM(newsegments=(segmentptr*) calloc(maxseg,sizeof(struct segmentstruct)));
		CHECKMEM(newnodes=(double**) calloc(maxseg+1,sizeof(double*)));
		CHECKMEM(newforces=(double**) calloc(maxseg+1,sizeof(double*)));
		CHECKMEM(newtorques=(double*) calloc(maxseg,sizeof(double)));

		for(seg=0;seg<=maxseg;seg++) newnodes[seg]=newforces[seg]=NULL;		// nodes and forces
		if(fil->maxseg>0)
			for(seg=0;seg<=fil->maxseg;seg++) {
				newnodes[seg]=fil->nodes[seg];
				newforces[seg]=fil->forces[seg]; }
		else
			seg=0;
		for(;seg<=maxseg;seg++) {
			CHECKMEM(newnodes[seg]=calloc(3,sizeof(double)));
			CHECKMEM(newforces[seg]=calloc(3,sizeof(double)));
			newnodes[seg][0]=newnodes[seg][1]=newnodes[seg][2]=0;
			newforces[seg][0]=newforces[seg][1]=newforces[seg][2]=0; }

		for(seg=0;seg<maxseg;seg++) {													// segments and torques
			newsegments[seg]=NULL;
			newtorques[seg]=0; }
		for(seg=0;seg<fil->maxseg;seg++) {
			newsegments[seg]=fil->segments[seg];
			newtorques[seg]=fil->torques[seg]; }
		for(;seg<maxseg;seg++) {
			CHECKMEM(newsegments[seg]=segmentalloc());
			newsegments[seg]->fil=fil;
			newsegments[seg]->xyzfront=newnodes[seg];
			newsegments[seg]->xyzback=newnodes[seg+1]; }

		free(fil->segments);
		free(fil->nodes);
		free(fil->forces);
		free(fil->torques);
		fil->segments=newsegments;
		fil->nodes=newnodes;
		fil->forces=newforces;
		fil->torques=newtorques;
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

	if(maxmonomer>fil->maxmonomer) {
		CHECKMEM(newmonomers=(char*) calloc(maxmonomer,sizeof(char)));
		for(mn=0;mn<fil->maxmonomer;mn++)
			newmonomers[mn]=fil->monomers[mn];
		for(;mn<maxmonomer;mn++)
			newmonomers[mn]='\0';
		free(fil->monomers);
		fil->monomers=newmonomers;
		fil->maxmonomer=maxmonomer; }

	return fil;

failure:		// no memory is freed, which causes leaks, but otherwise risk segfault
	return NULL; }


/* filfree */
void filfree(filamentptr fil) {
	int seg;

	if(!fil) return;
	if(fil->segments) {
		for(seg=0;seg<fil->maxseg;seg++)
			segmentfree(fil->segments[seg]);
		free(fil->segments); }
	if(fil->nodes) {
		for(seg=0;seg<=fil->maxseg;seg++)
			free(fil->nodes[seg]);
		free(fil->nodes); }
	if(fil->forces) {
		for(seg=0;seg<=fil->maxseg;seg++)
			free(fil->forces[seg]);
		free(fil->forces); }

	free(fil->torques);
	free(fil->branchspots);
	free(fil->branches);
	free(fil->monomers);
	free(fil);
	return; }


/* filamemttypealloc */
filamenttypeptr filamenttypealloc(filamenttypeptr filtype,int maxfil,int maxface) {
	int f,fc;
	filamentptr *newfillist;
	char **newfacename,**newfilnames;

	if(!filtype) {
		CHECKMEM(filtype=(filamenttypeptr) malloc(sizeof(struct filamenttypestruct)));
		filtype->filss=NULL;
		filtype->ftname=NULL;
		filtype->biology=FBnone;
		filtype->dynamics=FDnone;
		filtype->bundlevalue=1;

		filtype->color[0]=filtype->color[1]=filtype->color[2]=0;
		filtype->color[3]=1;
		filtype->edgepts=1;
		filtype->edgestipple[0]=1;
		filtype->edgestipple[1]=0xFFFF;
		filtype->drawmode=DMedge;
		filtype->shiny=0;

		filtype->stdlen=1;
		filtype->stdypr[0]=filtype->stdypr[1]=filtype->stdypr[2]=0;
		filtype->klen=1;
		filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=1;
		filtype->kT=1;
		filtype->treadrate=0;
		filtype->viscosity=1;
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
			CHECKMEM(newfillist[f]=filalloc(NULL,0,0,0));
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


/* filamenttypefree */
void filamenttypefree(filamenttypeptr filtype) {
	if(!filtype) return;
	int f,fc;

	if(filtype->fillist && filtype->filnames) {
		for(f=0;f<filtype->maxfil;f++) {
			filfree(filtype->fillist[f]);
			free(filtype->filnames[f]); }
		free(filtype->fillist);
		free(filtype->filnames); }

	if(filtype->facename) {
		for(fc=0;fc<filtype->maxface;fc++)
			free(filtype->facename[fc]);
		free(filtype->facename); }

	free(filtype);
	return; }


/* filssalloc */
filamentssptr filssalloc(filamentssptr filss,int maxtype) {
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
			CHECKMEM(newfiltypes[ft]=filamenttypealloc(NULL,0,0));
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


/* filssfree */
void filssfree(filamentssptr filss) {
	int ft;

	if(!filss) return;

	if(filss->filtypes) {
		for(ft=0;ft<filss->maxtype;ft++)
			filamenttypefree(filss->filtypes[ft]);
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


/* filoutput */
void filoutput(const filamentptr fil) {
	int seg,br,mn,dim;
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

	simLog(sim,2," Filament: %s\n",fil->filname);
	simLog(sim,1,"  type: %s\n",fil->filtype?fil->filtype->ftname:"None (assuming dim=3)");
	simLog(sim,1,"  allocated segments: %i\n",fil->maxseg);
	simLog(sim,2,"  number of segments: %i\n",fil->nseg);
	simLog(sim,1,"  front index: %i\n",fil->frontseg);

	simLog(sim,2,"  segment, length, thickness, front position, relative angle:\n");
	for(seg=0;seg<fil->nseg;seg++) {
		segment=fil->segments[seg+fil->frontseg];
		if(dim==2)
			simLog(sim,seg>5?1:2,"   %i length=%1.3g, thick=%1.3g, front pos.=(%1.3g %1.3g), rel. angle=%1.3g\n",segment->index,segment->len,segment->thk,segment->xyzfront[0],segment->xyzfront[1],segment->ypr[0]);
		else
			simLog(sim,seg>5?1:2,"   %i length=%1.3g, thick=%1.3g, front pos.=(%1.3g %1.3g %1.3g), rel. angle=(%1.3g %1.3g %1.3g)\n",segment->index,segment->len,segment->thk,segment->xyzfront[0],segment->xyzfront[1],segment->xyzfront[2],segment->ypr[0],segment->ypr[1],segment->ypr[2]); }

	if(fil->frontend)
		simLog(sim,2,"  front branched from: %s\n",fil->frontend->filname);
	if(fil->backend)
		simLog(sim,2,"  back branched from: %s\n",fil->backend->filname);

	simLog(sim,1,"  allocated branches: %i\n",fil->maxbranch);
	simLog(sim,fil->nbranch>0?2:1,"  number of branches: %i\n",fil->nbranch);
	for(br=0;br<fil->nbranch;br++)
		simLog(sim,2,"   %s at %i\n",fil->branches[br]->filname,fil->branchspots[br]);

	simLog(sim,1,"  monomer codes: %i of %i allocated,",fil->nmonomer,fil->maxmonomer);
	simLog(sim,1," starting index: %i\n",fil->frontmonomer);
	if(fil->nmonomer) {
		simLog(sim,2,"  monomer code: ");
		for(mn=0;mn<fil->nmonomer;mn++)
			simLog(sim,2,"%c",fil->monomers[mn]);
		simLog(sim,2,"\n"); }

	if(fil->filtype->klen>0)
		simLog(sim,2," stretching energy: %g\n",filStretchEnergy(fil,-1,-1));
	if(fil->filtype->kypr[0]>0 || fil->filtype->kypr[1]>0 || fil->filtype->kypr[2]>0)
		simLog(sim,2," bending energy: %g\n",filBendEnergy(fil,-1,-1));

	return; }


/* filtypeoutput */
void filtypeoutput(const filamenttypeptr filtype) {
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
	simLog(sim,2,"  biology: %s\n",filFB2string(filtype->biology,string));
	simLog(sim,2,"  dynamics: %s\n",filFD2string(filtype->dynamics,string));
	simLog(sim,filtype->bundlevalue!=1?2:1,"  bundle value: %g\n",filtype->bundlevalue);

	simLog(sim,2,"  color: %g %g %g %g\n",filtype->color[0],filtype->color[1],filtype->color[2],filtype->color[3]);
	simLog(sim,2,"  edge points: %g, polygon mode: %s\n",filtype->edgepts,surfdm2string(filtype->drawmode,string));
	if(filtype->edgestipple[1]!=0xFFFF) simLog(sim,2,"  edge stippling: %ui %X\n",filtype->edgestipple[0],filtype->edgestipple[1]);
	if(filtype->shiny!=0) simLog(sim,2,"  shininess: %g\n",filtype->shiny);

	simLog(sim,2,"  %s length: %g\n",filtype->klen>=0?"standard":"fixed",filtype->stdlen);
	if(filtype->klen>0) simLog(sim,2,"  length force constant: %g\n",filtype->klen);

	if(dim==2) {
		simLog(sim,2,"  %s angle: %g\n",filtype->kypr[0]>=0?"standard":"fixed",filtype->stdypr[0]);
		simLog(sim,2,"  bending force constant: %g\n",filtype->kypr[0]); }
	else {
		simLog(sim,2,"  standard angles: %g, %g, %g\n",filtype->stdypr[0],filtype->stdypr[1],filtype->stdypr[2]);
		simLog(sim,2,"  bending force constants: %g, %g, %g\n",filtype->kypr[0],filtype->kypr[1],filtype->kypr[2]); }

	simLog(sim,2,"  kT: %g\n",filtype->kT);
	simLog(sim,filtype->treadrate!=0?2:1,"  treadmilling rate: %g\n",filtype->treadrate);
	simLog(sim,2,"  viscosity: %g\n",filtype->viscosity);
	simLog(sim,2,"  filament radius: %g\n",filtype->filradius);

	if(filtype->nface>0) {
		simLog(sim,2,"  %i faces with twist of %g:",filtype->nface,filtype->facetwist);
		for(fc=0;fc<filtype->nface;fc++)
			simLog(sim,2," %s,",filtype->facename[fc]);
		simLog(sim,2,"\n"); }

	simLog(sim,2,"  %i filaments:\n",filtype->nfil);
	for(f=0;f<filtype->nfil;f++)
		filoutput(filtype->fillist[f]);

	return; }


/* filssoutput */
void filssoutput(const simptr sim) {
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
		filtypeoutput(filss->filtypes[ft]);

	simLog(sim,2,"\n");
	return; }


/* filwrite */
void filwrite(const simptr sim,FILE *fptr) {
	filamentssptr filss;

	filss=sim->filss;
	if(!filss) return;
	fprintf(fptr,"# filament parameters\n");
	fprintf(fptr,"# ERROR: code not written yet\n\n");
	//??TODO: write filwrite
	return; }


/* filcheckparams */
int filcheckparams(const simptr sim,int *warnptr) {
	int error,warn;
	filamentssptr filss;
	char string[STRCHAR];

	error=warn=0;
	filss=sim->filss;
	if(!filss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(filss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: filament structure %s\n",simsc2string(filss->condition,string)); }

	//TODO: write checkparams

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/********************************** Filaments *****************************/
/******************************************************************************/


/* filUpdateSegmentIndices */
void filUpdateSegmentIndices(filamentptr fil) {
	int i;

	for(i=0;i<fil->nseg;i++)
		fil->segments[i+fil->frontseg]->index=i;
	return; }


/* filGetFilIndex */
int filGetFilIndex(simptr sim,const char *name,int *ftptr) {
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


/* filArrayShift */
void filArrayShift(filamentptr fil,int shift) {
	int i,frontseg,backseg;
	segmentptr newsegment;
	double *newnode,*newforce,newtorque;

	if(!shift)
		shift=(fil->maxseg-fil->nseg)/2-fil->frontseg;

	frontseg=fil->frontseg;
	backseg=fil->frontseg+fil->nseg;			// one past back segment

	if(shift>0) {
		if(backseg+shift>fil->maxseg) shift=fil->maxseg-backseg;
		for(i=backseg+shift-1;i>=frontseg+shift;i--) {			// i is new index
			newsegment = fil->segments[i];
			newtorque = fil->torques[i];
			fil->segments[i]=fil->segments[i-shift];
			fil->torques[i]=fil->torques[i-shift];
			fil->segments[i-shift] = newsegment;
			fil->torques[i-shift] = newtorque; }
		for(i=backseg+shift;i>=frontseg+shift;i--) {
			newnode = fil->nodes[i];
			newforce = fil->forces[i];
			fil->nodes[i]=fil->nodes[i-shift];
			fil->forces[i]=fil->forces[i-shift];
			fil->nodes[i-shift]=newnode;
			fil->forces[i-shift]=newforce; }
		fil->frontseg+=shift; }

	else if(shift<0) {
		shift=-shift;								// now shift is positive
		if(frontseg-shift<0) shift=frontseg;
		for(i=frontseg-shift;i<backseg-shift;i++) {					// i is new index
			newsegment = fil->segments[i];
			newtorque = fil->torques[i];
			fil->segments[i]=fil->segments[i+shift];
			fil->torques[i]=fil->torques[i+shift];
			fil->segments[i+shift] = newsegment;
			fil->torques[i+shift] = newtorque; }
		for(i=frontseg-shift;i<=backseg-shift;i++) {
			newnode = fil->nodes[i];
			newforce = fil->forces[i];
			fil->nodes[i]=fil->nodes[i+shift];
			fil->forces[i]=fil->forces[i+shift];
			fil->nodes[i+shift]=newnode;
			fil->forces[i+shift]=newforce; }
		fil->frontseg-=shift; }

	return; }


/* filAddSegment */
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar) {
	int seg;
	segmentptr segment,segmentm1,segmentp1;
	double dcm[9];

//	printf("filAddSegment. end=%c",endchar);//??
//	if(x) printf(" x=(%g,%g,%g)",x[0],x[1],x[2]);//??
//	else printf(" x=NULL");//??
//	printf(" length=%g, angle=(%g,%g,%g), thickness=%g\n",length,angle[0],angle[1],angle[2],thickness);//??

	if(fil->nseg==fil->maxseg) {
		fil=filalloc(fil,fil->maxseg*2+1,0,0); //?? too big
		if(!fil) return 1; }		// out of memory

	if(endchar=='b') {
		if(fil->frontseg+fil->nseg==fil->maxseg) filArrayShift(fil,0);
		seg=fil->nseg;
		segment=fil->segments[seg+fil->frontseg];
		segment->index=seg;
		segment->len=length;
		segment->thk=thickness;
		segment->xyzfront=fil->nodes[seg+fil->frontseg];
		segment->xyzback=fil->nodes[seg+1+fil->frontseg];
		Sph_Xyz2Xyz(angle,segment->ypr);												// ypr = angle
		Sph_Xyz2Dcm(angle,segment->dcm);												// A = Dcm(angle)
		if(seg==0) {
			segment->xyzfront[0]=x[0];														// x_i = input value
			segment->xyzfront[1]=x[1];
			segment->xyzfront[2]=x[2];
			Sph_Dcm2Dcm(segment->dcm,segment->adcm); }						// B = A
		else {
			segmentm1=fil->segments[seg-1+fil->frontseg];
			Sph_DcmxDcm(segment->dcm,segmentm1->adcm,segment->adcm);	}	// B_i = A_i . B_{i-1}
		Sph_DcmtxUnit(segment->adcm,'x',segment->xyzback,segment->xyzfront,segment->len);		// x_{i+1} = x_i + l_i * BT_i . xhat
		fil->nseg++; }

	else {
		if(fil->frontseg==0) filArrayShift(fil,0);
		if(fil->frontseg==0) filArrayShift(fil,1);								// used if nseg=maxseg-1
		seg=-1;
		segment=fil->segments[seg+fil->frontseg];
		segment->index=seg;
		segment->len=length;
		segment->thk=thickness;
		segment->xyzfront=fil->nodes[-1+fil->frontseg];
		segment->xyzback=fil->nodes[fil->frontseg];
		if(fil->nseg==0) {
			Sph_Xyz2Dcmt(angle,segment->adcm);									// B_0 = Dcmt(angle)
			segment->xyzback[0]=x[0];														// back of segment = input value
			segment->xyzback[1]=x[1];
			segment->xyzback[2]=x[2]; }
		else {
			segmentp1=fil->segments[fil->frontseg];
			Sph_Xyz2Dcm(angle,dcm);
			Sph_DcmtxDcm(dcm,segmentp1->adcm,segment->adcm);
			Sph_Dcm2Dcm(dcm,segmentp1->dcm); }
		Sph_Dcm2Dcm(segment->adcm,segment->dcm);									// A_i = B_i
		Sph_Dcm2Xyz(segment->dcm,segment->ypr);										// a_0 = Xyz(B_0)
		Sph_DcmtxUnit(segment->adcm,'x',segment->xyzfront,segment->xyzback,-segment->len);	// x_i = x_{i+1} - l_i * BT_i . xhat
		fil->frontseg--;
		fil->nseg++;
		filUpdateSegmentIndices(fil); }
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
		else if(strmathsscanf(xstr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[0]=f1;
		else return 2;
		if(!strcmp(ystr,"u"));
		else if(strmathsscanf(ystr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[1]=f1;
		else return 2;
		if(dim==2) pos[2]=0;
		else if(!strcmp(zstr,"u"));
		else if(strmathsscanf(zstr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[2]=f1;
		else return 2;

		angle[0]=(dim==2 ? unirandCCD(-PI,PI) : thetarandCCD());
		angle[1]=unirandCOD(0,2*PI);
		angle[2]=unirandCOD(0,2*PI);
		if(dim==3) Sph_Eax2Xyz(angle,angle);
		if(!strcmp(phistr,"u"));
		else if(strmathsscanf(phistr,"%mlg",varnames,varvalues,nvar,&f1)==1) angle[0]=f1;
		else return 3;
		if(dim==2) angle[1]=0;
		else if(!strcmp(thtstr,"u"));
		else if(strmathsscanf(thtstr,"%mlg",varnames,varvalues,nvar,&f1)==1) angle[1]=f1;
		else return 3;
		if(dim==2) angle[2]=0;
		else if(!strcmp(psistr,"u"));
		else if(strmathsscanf(psistr,"%mlg",varnames,varvalues,nvar,&f1)==1) angle[2]=f1;
		else return 3; }
	else {
		pos[0]=pos[1]=pos[2]=0;
		angle[0]=angle[1]=angle[2]=0; }

	for(i=0;i<number;i++) {
		len=filRandomLength(filtype,thickness,1);
		if(fil->nseg>0) {
			filRandomAngle(filtype,dim,fil->nseg,thickness,1,angle);
			if(dim==2) angle[1]=angle[2]=0; }
		er=filAddSegment(fil,pos,len,angle,thickness,'b');
		if(er) return er; }

	return 0; }


/* filRemoveSegment */
int filRemoveSegment(filamentptr fil,char endchar) {
	int seg;
	segmentptr segment;

	if(fil->nseg==0) return -1;

	if(endchar=='b')
		fil->nseg--;
	else {
		seg=++fil->frontseg;					// new front segment
		fil->nseg--;
		segment=fil->segments[seg];
		Sph_Dcm2Dcm(segment->adcm,segment->dcm);
		Sph_Dcm2Xyz(segment->dcm,segment->ypr);
		filUpdateSegmentIndices(fil); }
	return 0; }


/* filTranslate */
void filTranslate(filamentptr fil,const double *vect,char func) {
	int seg;
	double shift[3],*node;
	segmentptr segment;

	seg=fil->frontseg;
	if(func=='=') {
		segment=fil->segments[seg];
		shift[0]=segment->xyzfront[0]-vect[0];
		shift[1]=segment->xyzfront[1]-vect[1];
		shift[2]=segment->xyzfront[2]-vect[2];}
	else if(func=='-') {
		shift[0]=-vect[0];
		shift[1]=-vect[1];
		shift[2]=-vect[2]; }
	else {
		shift[0]=vect[0];
		shift[1]=vect[1];
		shift[2]=vect[2]; }

	for(seg=0;seg<=fil->nseg;seg++) {
		node=fil->nodes[seg+fil->frontseg];
		node[0]+=shift[0];
		node[1]+=shift[1];
		node[2]+=shift[2]; }

	return; }


/* filLengthenSegment */
void filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func) {
	int i;
	double lenold,lendelta,xdelta[3],**nodes;
	segmentptr segment;

	segment=fil->segments[fil->frontseg+seg];
	lenold=segment->len;
	if(func=='=') lendelta=length-lenold;
	else if(func=='+') lendelta=length;
	else lendelta=lenold-length;

	xdelta[0]=lendelta*segment->adcm[0];		// same as SphDcmtxUnit(segment->adcm,'x',xdelta,NULL,lendelta);
	xdelta[1]=lendelta*segment->adcm[1];
	xdelta[2]=lendelta*segment->adcm[2];
	nodes=fil->nodes;
	if(endchar=='b') {
		for(i=seg+1;i<=fil->nseg;i++) {
			nodes[i+fil->frontseg][0]+=xdelta[0];
			nodes[i+fil->frontseg][1]+=xdelta[1];
			nodes[i+fil->frontseg][2]+=xdelta[2]; }}
	else {
		for(i=seg;i>=0;i--) {
			nodes[i+fil->frontseg][0]-=xdelta[0];
			nodes[i+fil->frontseg][1]-=xdelta[1];
			nodes[i+fil->frontseg][2]-=xdelta[2]; }}

	return; }


/* filRotateVertex */
void filRotateVertex(filamentptr fil,int seg,const double *angle,char endchar,char func) {
	int i;
	double dcmdelta[9];
	segmentptr segment,segmentm1,segmentp1;

	segment=fil->segments[fil->frontseg+seg];
	Sph_Xyz2Dcm(angle,dcmdelta);
	if(func=='=') Sph_Dcm2Dcm(dcmdelta,segment->dcm);
	else if(func=='+') Sph_DcmxDcm(dcmdelta,segment->dcm,segment->dcm);
	else Sph_DcmtxDcm(dcmdelta,segment->dcm,segment->dcm);

	if(endchar=='b') {
		for(i=seg;i<fil->nseg;i++) {
			segment=fil->segments[fil->frontseg+i];
			if(i==0) Sph_Dcm2Dcm(segment->dcm,segment->adcm);
			else {
				segmentm1=fil->segments[fil->frontseg+i-1];
				Sph_DcmxDcm(segment->dcm,segmentm1->adcm,segment->adcm); }
			Sph_Dcm2Xyz(segment->dcm,segment->ypr);
			segment->xyzback[0]=segment->xyzfront[0]+segment->len*segment->adcm[0];
			segment->xyzback[1]=segment->xyzfront[1]+segment->len*segment->adcm[1];
			segment->xyzback[2]=segment->xyzfront[2]+segment->len*segment->adcm[2]; }}
	else {
		for(i=seg;i>=0;i--) {
			segment=fil->segments[fil->frontseg+i];
			if(i==fil->nseg-1);
			else {
				segmentp1=fil->segments[fil->frontseg+i+1];
				Sph_DcmtxDcm(segmentp1->dcm,segmentp1->adcm,segment->adcm); }
			Sph_Dcm2Xyz(segment->dcm,segment->ypr);
			segment->xyzfront[0]=segment->xyzback[0]-segment->len*segment->adcm[0];
			segment->xyzfront[1]=segment->xyzback[1]-segment->len*segment->adcm[1];
			segment->xyzfront[2]=segment->xyzback[2]-segment->len*segment->adcm[2]; }}

	return; }


/* filCopyFilament */
int filCopyFilament(filamentptr filto,const filamentptr filfrom) {
	int i;
	segmentptr segmentto,segmentfrom;

	if(!filto || !filfrom) return 2;

	filto->filtype=filfrom->filtype;

	filto->nseg=0;
	filto->frontseg=0;
	filto->nbranch=0;
	filto->nmonomer=0;
	filto->frontmonomer=0;
	filto=filalloc(filto,filfrom->nseg,filfrom->nbranch,filfrom->nmonomer);
	if(!filto) return 1;

	for(i=0;i<filfrom->nseg;i++) {
		segmentto=filto->segments[i];
		segmentfrom=filfrom->segments[i+filfrom->frontseg];
		segmentto->index=segmentfrom->index;
		copyVD(segmentfrom->xyzfront,segmentto->xyzfront,3);
		copyVD(filfrom->forces[i+filfrom->frontseg],filto->forces[i],3);
		segmentto->len=segmentfrom->len;
		segmentto->thk=segmentfrom->thk;
		copyVD(segmentfrom->ypr,segmentto->ypr,3);
		copyVD(segmentfrom->dcm,segmentto->dcm,9);
		copyVD(segmentfrom->adcm,segmentto->adcm,9); }
	if(filfrom->nseg>0) {
		copyVD(segmentfrom->xyzback,segmentto->xyzback,3);
		i=filfrom->nseg;
		copyVD(filfrom->forces[i+filfrom->frontseg],filto->forces[i],3); }
	filto->nseg=filfrom->nseg;

	filto->frontend=filfrom->frontend;
	filto->backend=filfrom->backend;
	for(i=0;i<filfrom->nbranch;i++) {
		filto->branchspots[i]=filfrom->branchspots[i];
		filto->branches[i]=filfrom->branches[i]; }
	filto->nbranch=filfrom->nbranch;

	for(i=0;i<filfrom->nmonomer;i++)
		filto->monomers[i]=filfrom->monomers[i+filfrom->frontmonomer];
	filto->nmonomer=filfrom->nmonomer;

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
		filtype=filamenttypealloc(filtype,filtype->maxfil*2+1,0);
		if(!filtype) return NULL; }
	f=filtype->nfil++;
	strncpy(filtype->filnames[f],filname,STRCHAR-1);
	filtype->filnames[f][STRCHAR-1]='\0';
	fil=filtype->fillist[f];
	filsetcondition(filtype->filss,SClists,0);

	return fil; }


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

	else if(!strcmp(param,"viscosity")) {
		if(value<=0) er=2;
		else filtype->viscosity=value; }

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


/* filtypeSetBiology */
int filtypeSetBiology(filamenttypeptr filtype,enum FilamentBiology fb) {
	filtype->biology=fb;	//?? set parameters to match
	return 0; }


/* filtypeAddFace */
int filtypeAddFace(filamenttypeptr filtype,const char* facename) {
	if(filtype->nface==filtype->maxface) {
		filtype=filamenttypealloc(filtype,0,filtype->maxface*2+1);
		if(!filtype) return -1; }
	strcpy(filtype->facename[filtype->nface++],facename);
	return 0; }


/* filAddFilamentType */
filamenttypeptr filAddFilamentType(simptr sim,const char *ftname) {
	int ft,er;
	filamentssptr filss;
	filamenttypeptr filtype;

	if(!sim->filss) {
		er=filenablefilaments(sim);
		if(er) return NULL; }
	filss=sim->filss;

	ft=stringfind(filss->ftnames,filss->ntype,ftname);
	if(ft<0) {
		if(filss->ntype==filss->maxtype) {
			filss=filssalloc(filss,filss->maxtype*2+1);
			if(!filss) return NULL; }
		ft=filss->ntype++;
		strncpy(filss->ftnames[ft],ftname,STRCHAR-1);
		filss->ftnames[ft][STRCHAR-1]='\0';
		filtype=filss->filtypes[ft];
		filsetcondition(filss,SClists,0); }
	else
		filtype=filss->filtypes[ft];

	return filtype; }



/******************************************************************************/
/**************************** filament superstructure *************************/
/******************************************************************************/


/* filsetcondition */
void filsetcondition(filamentssptr filss,enum StructCond cond,int upgrade) {
	if(!filss) return;
	if(upgrade==0 && filss->condition>cond) filss->condition=cond;
	else if(upgrade==1 && filss->condition<cond) filss->condition=cond;
	else if(upgrade==2) filss->condition=cond;
	if(filss->sim && filss->condition<filss->sim->condition) {
		cond=filss->condition;
		simsetcondition(filss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* filenablefilaments */
int filenablefilaments(simptr sim) {
	filamentssptr filss;

	if(sim->filss) return 0;

	filss=filssalloc(sim->filss,0);
	if(!filss) return 1;
	sim->filss=filss;
	filss->sim=sim;
	filsetcondition(sim->filss,SClists,0);
	return 0; }


/* filupdateparams */
int filupdateparams(simptr sim) {
	(void) sim;
	return 0; }


/* filupdatelists */
int filupdatelists(simptr sim) {
	(void) sim;
	return 0; }


/* filupdate */
int filupdate(simptr sim) {
	int er;
	filamentssptr filss;

	filss=sim->filss;
	if(filss) {
		if(filss->condition<=SClists) {
			er=filupdatelists(sim);
			if(er) return er;
			filsetcondition(filss,SCparams,1); }
		if(filss->condition==SCparams) {
			er=filupdateparams(sim);
			if(er) return er;
			filsetcondition(filss,SCok,1); }}
	return 0; }


/******************************************************************************/
/********************************** user input ********************************/
/******************************************************************************/


/* filtypereadstring */
filamenttypeptr filtypereadstring(simptr sim,ParseFilePtr pfp,filamenttypeptr filtype,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar,dim;

	int itct,er,i1,i2;
	char nm[STRCHARLONG],nm1[STRCHARLONG];
	double fltv1[9],f1;
	enum DrawMode dm;
	enum FilamentDynamics fd;
	enum FilamentBiology fb;

	printf("%s\n",word);//?? debug
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
		CHECKS(itct==1,"dynamics options: none, euler");
		fd=filstring2FD(nm1);
//		CHECKS(fd!=FDnone,"dynamics options: none, euler");		//?? fix this
		er=filtypeSetDynamics(filtype,fd);
		CHECKS(!er,"BUG: error setting filament dynamics");
		CHECKS(!strnword(line2,2),"unexpected text following dynamics"); }

	else if(!strcmp(word,"biology")) {				// biology
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"biology options: actin, microtubule, intermediate, dsDNA, ssDNA, other");
		fb=filstring2FB(nm);
		CHECKS(fb!=FBnone,"biology options: actin, microtubule, intermediate, dsDNA, ssDNA, other");
		er=filtypeSetBiology(filtype,fb);
		CHECKS(!er,"error setting filament biology");
		CHECKS(!strnword(line2,2),"unexpected text following biology"); }

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
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
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
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"shininess format: value");
		CHECKS(f1>=0 && f1<=128,"shininess value needs to be between 0 and 128");
		er=filtypeSetShiny(filtype,f1);
		CHECKS(!er,"BUG: error in filtypeSetShiny");
		CHECKS(!strnword(line2,2),"unexpected text following shininess"); }

	else if(!strcmp(word,"kT")) {								// kT
		CHECKS(filtype,"need to enter filament type name before kT");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"kT format: value");
		CHECKS(f1>=0,"kT value needs to be >=0");
		filtypeSetParam(filtype,"kT",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following kT"); }

	else if(!strcmp(word,"treadmill_rate")) {		// treadmill_rate
		CHECKS(filtype,"need to enter filament type name before treadmill_rate");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"treadmill_rate format: value");
		filtypeSetParam(filtype,"treadrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following treadmill_rate"); }

	else if(!strcmp(word,"viscosity")) {			// viscosity
		CHECKS(filtype,"need to enter filament type name before viscosity");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"viscosity format: value");
		CHECKS(f1>0,"viscosity value needs to be greater than 0");
		filtypeSetParam(filtype,"viscosity",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following viscosity"); }

	else if(!strcmp(word,"standard_length")) {	// standard_length
		CHECKS(filtype,"need to enter filament type name before standard_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"standard_length format: value");
		CHECKS(f1>=0,"standard_length value needs to be >=0");
		filtypeSetParam(filtype,"stdlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following standard_length"); }

	else if(!strcmp(word,"standard_angle")) {	// standard_angle
		CHECKS(filtype,"need to enter filament type name before standard_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&fltv1[0]);
			CHECKS(itct==1,"standard_angle format: angle");
			fltv1[1]=fltv1[2]=0; }
		else {
			itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
			CHECKS(itct==3,"standard_angle format: yaw pitch roll"); }
		filtypeSetParam(filtype,"stdypr",0,fltv1[0]);
		filtypeSetParam(filtype,"stdypr",1,fltv1[1]);
		filtypeSetParam(filtype,"stdypr",2,fltv1[2]);
		CHECKS(!strnword(line2,dim==2?2:4),"unexpected text following standard_angle"); }

	else if(!strcmp(word,"force_length")) {		// force_length
		CHECKS(filtype,"need to enter filament type name before force_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"force_length format: value");
		filtypeSetParam(filtype,"klen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following force_length"); }

	else if(!strcmp(word,"force_angle")) {		// force_angle
		CHECKS(filtype,"need to enter filament type name before force_angle");
		if(dim==2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&fltv1[0]);
			CHECKS(itct==1,"force_angle format: value");
			fltv1[1]=fltv1[2]=-1; }
		else {
			itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
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


/* filreadstring */
filamentptr filreadstring(simptr sim,ParseFilePtr pfp,filamentptr fil,filamenttypeptr filtype,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar;

	filamentptr fil2;
	filamentssptr filss;
	int itct,er,i1;
	char nm[STRCHAR],nm1[STRCHAR],endchar,str1[STRCHAR],str2[STRCHAR],str3[STRCHAR],str4[STRCHAR],str5[STRCHAR],str6[STRCHAR],symbol;
	double fltv1[9],length,angle[3],thick;

	printf("%s\n",word);//?? debug
	filss=sim->filss;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(!strcmp(word,"name")) {
		itct=sscanf(line2,"%s %s",nm1,nm);
		CHECKS(itct==2,"correct format: filament_type filament_name");
		i1=stringfind(filss->ftnames,filss->ntype,nm1);
		CHECKS(i1>=0,"filament type not recognized");
		filtype=filss->filtypes[i1];
		fil=filAddFilament(filtype,nm);
		CHECKS(fil,"failed to add filament");
		CHECKS(!strnword(line2,3),"unexpected text following filament name"); }

	else if(!strcmp(word,"first_segment")) {		// first_segment
		CHECKS(fil,"need to enter filament name before first_segment");
		CHECKS(fil->nseg==0,"filament already has segments in it");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg %mlg %mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2],&length,&angle[0],&angle[1],&angle[2]);
		CHECKS(itct==7,"first_segment format: x y z length angle0 angle1 angle2 [thickness]");
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,8);
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&thick);
			CHECKS(itct==1,"first_segment format: x y z length angle0 angle1 angle2 [thickness]");
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddSegment(fil,fltv1,length,angle,thick,'b');
		CHECKS(er==0,"BUG: error in filAddsegment");
		CHECKS(!line2,"unexpected text following first_segment"); }

	else if(!strcmp(word,"add_segment")) {			// add_segment
		CHECKS(fil,"need to enter filament name before add_segment");
		CHECKS(fil->nseg>0,"use first_segment to enter the first segment");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg %mlg",varnames,varvalues,nvar,&length,&angle[0],&angle[1],&angle[2]);
		CHECKS(itct==4,"add_segment format: length angle0 angle1 angle2 [thickness [end]]");
		CHECKS(length>0,"length needs to be >0");
		line2=strnword(line2,5);
		thick=1;
		endchar='b';
		if(line2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&thick);
			CHECKS(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [end]]");
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2);
			if(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"add_segment format: length angle0 angle1 angle2 [thickness [end]]");
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
		CHECKS(fil->filtype->klen==-1 || fil->filtype->klen>0,"cannot compute random segments because the filament type has length force constant equals 0");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]");
		CHECKS(i1>0,"number needs to be >0");
		line2=strnword(line2,2);
		if(fil->nseg==0) {
			CHECKS(line2,"missing position and angle information");
			itct=sscanf(line2,"%s %s %s %s %s %s",str1,str2,str3,str4,str5,str6);
			CHECKS(itct==6,"random_segments format: number [x y z phi theta psi] [thickness]");
			line2=strnword(line2,7); }
		else {
			sprintf(str1,"%i",0);
			sprintf(str2,"%i",0);
			sprintf(str3,"%i",0);
			sprintf(str4,"%i",0);
			sprintf(str5,"%i",0);
			sprintf(str6,"%i",0); }
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&thick);
			CHECKS(itct==1,"random_segments format: number [x y z phi theta psi] [thickness]");
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddRandomSegments(fil,i1,str1,str2,str3,str4,str5,str6,thick);
		CHECKS(er!=2,"random_segments positions need to be 'u' or value");
		CHECKS(er!=3,"random_segments angles need to be 'u' or value");
		CHECKS(er==0,"BUG: error in filAddRandomsegments");
		CHECKS(!line2,"unexpected text following random_segments"); }

	else if(!strcmp(word,"translate")) {					// translate
		CHECKS(fil,"need to enter filament name before translate");
		itct=strmathsscanf(line2,"%c %mlg %mlg %mlg",varnames,varvalues,nvar,&symbol,&fltv1[0],&fltv1[1],&fltv1[2]);
		CHECKS(itct==4,"translate format: symbol x y z");
		CHECKS(symbol=='=' || symbol=='+' || symbol=='-',"symbol needs to be '=', '+', or '-'");
		filTranslate(fil,fltv1,symbol);
		CHECKS(!strnword(line2,5),"unexpected text following translate"); }

	else if(!strcmp(word,"copy")) {								// copy
		CHECKS(fil,"need to enter filament name before copy");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading filament name");
		fil2=filAddFilament(fil->filtype,nm);
		CHECKS(fil2,"failed to add filament");
		er=filCopyFilament(fil2,fil);
		CHECKS(!strnword(line2,2),"unexpected text following copy"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within filament_type block: statement not recognized"); }

	return fil;

failure:
	simParseError(sim,pfp);
	return NULL; }


/* filloadtype */
int filloadtype(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHARLONG];
	int done,pfpcode,firstline2;
	filamenttypeptr filtype;

	pfp=*pfpptr;
	done=0;
	filtype=NULL;
	firstline2=line2?1:0;
	filenablefilaments(sim);

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
            filtype=filtypereadstring(sim,pfp,filtype,word,line2);
            CHECK(filtype); }}													// failed but error has already been sent

	CHECKS(0,"end of file encountered before end_filament_type statement");	// end of file

	failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* filloadfil */
int filloadfil(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHARLONG];
	int done,pfpcode,firstline2;
	filamentptr fil;
	filamenttypeptr filtype;

	pfp=*pfpptr;
	done=0;
	firstline2=line2?1:0;
	filenablefilaments(sim);
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
		else if(!strcmp(word,"end_filament")) {				// end_filament_type
				CHECKS(!line2,"unexpected text following end_filament");
				return 0; }
		else if(!line2) {															// just word
				CHECKS(0,"unknown word or missing parameter"); }
		else {
				fil=filreadstring(sim,pfp,fil,filtype,word,line2);
				CHECK(fil); 															// failed but error has already been sent
				filtype=fil->filtype; }}

	CHECKS(0,"end of file encountered before end_filament statement");	// end of file

	failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/******************************************************************************/
/**************************** core simulation functions ***********************/
/******************************************************************************/


/* filNodes2Angles */
void filNodes2Angles(filamentptr fil) {	//?? Not finished
	int seg;
	segmentptr segment;
	double x[3],len,cy,sy,cq,sq,cf,sf,*Dcm;

	for(seg=0;seg<fil->nseg;seg++) {
		segment=fil->segments[fil->frontseg+seg];
		x[0]=segment->xyzback[0]-segment->xyzfront[0];
		x[1]=segment->xyzback[1]-segment->xyzfront[1];
		x[2]=segment->xyzback[2]-segment->xyzfront[2];
		len=sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
		segment->len=len;
		x[0]/=len;
		x[1]/=len;
		x[2]/=len;
		cy=cos(segment->ypr[2]);	//?? cos and sin of absolute ypr angles. First are wrong because I want aypr and this is local ypr
		sy=sin(segment->ypr[2]);
		sf=sy*x[2]-cy*x[1];
		cf=sqrt(1-sf*sf);
		sq=(cy*x[2]+sy*x[1])/cf;
		cq=sqrt(1-sq*sq);

		Dcm=segment->adcm;
		Dcm[0]=cq*cf;							// absolute dcm copied from Sph_Xyz2Dcm.
		Dcm[1]=cq*sf;
		Dcm[2]=-sq;
		Dcm[3]=sy*sq*cf-cy*sf;
		Dcm[4]=sy*sq*sf+cy*cf;
		Dcm[5]=cq*sy;
		Dcm[6]=cy*sq*cf+sy*sf;
		Dcm[7]=cy*sq*sf-sy*cf;
		Dcm[8]=cq*cy;

		if(seg==0)
			Sph_Dcm2Dcm(Dcm,segment->dcm);
		else
			Sph_DcmtxDcm(fil->segments[fil->frontseg+seg-1]->adcm,Dcm,segment->dcm);

		Sph_Dcm2Xyz(segment->dcm,segment->ypr); }

	return; }


/* filSegmentXSurface */
int filSegmentXSurface(simptr sim,segmentptr segment,panelptr *pnlptr) {
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
int filSegmentXFilament(simptr sim,segmentptr segment,filamentptr *filptr) {
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
	segmentm1=(segment->index==0) ? NULL:fil->segments[fil->frontseg+segment->index-1];
	segmentp1=(segment->index==fil->nseg-1) ? NULL:fil->segments[fil->frontseg+segment->index+1];

	cross=0;
	filss=sim->filss;
	for(ft=0;ft<filss->ntype && !cross;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil && !cross;f++) {
			fil2=filtype->fillist[f];
			for(i=0;i<fil2->nseg && !cross;i++) {
				segment2=fil2->segments[i+fil2->frontseg];
				if(!(segment2==segment || segment2==segmentm1 || segment2==segmentp1)) {
					dist=Geo_NearestSeg2SegDist(ptf,ptb,segment2->xyzfront,segment2->xyzback);
					if(dist<thk+segment2->thk) cross=1; }}}}
	if(cross && filptr)
		*filptr=fil2;
	return cross; }


/* filAddOneRandomSegment */
int filAddOneRandomSegment(simptr sim,filamentptr fil,const double *x,double thickness,char endchar,int constraints) {
	int dim,er,iter,cross,tryagain;
	filamenttypeptr filtype;
	double len,angle[3];
	panelptr pnl;
	segmentptr segment;

	filtype=fil->filtype;
	dim=sim->dim;

	tryagain=1;
	len=filRandomLength(filtype,thickness,1);
	filRandomAngle(filtype,dim,fil->nseg,thickness,1,angle);
	er=filAddSegment(fil,x,len,angle,thickness,endchar);
	if(er) return er;
	segment=fil->segments[fil->frontseg+((endchar=='f')?0:fil->nseg-1)];
	if(constraints==0) tryagain=0;
	for(iter=0;iter<FILMAXTRIES && tryagain;iter++) {
		tryagain=0;
		if(constraints & 1) {	// check for surface crossing
			cross=filSegmentXSurface(sim,segment,&pnl);
			if(cross) {
				len=filRandomLength(filtype,thickness,1);
				filRandomAngle(filtype,dim,fil->nseg,thickness,1,angle);
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
	er=filAddOneRandomSegment(sim,fil,NULL,fil->segments[fil->frontseg+((endchar=='b')?fil->nseg-1:0)]->thk,endchar,1);
	if(!er)
		filRemoveSegment(fil,(endchar=='b')?'f':'b');
	return er; }


void filAddStretchForces(filamentptr fil) {
	double **forces,klen,stdlen,force,len,xvect[3];
	segmentptr segment;
	int seg;

	forces=fil->forces;
	klen=fil->filtype->klen;
	stdlen=fil->filtype->stdlen;

	for(seg=0;seg<fil->nseg;seg++) {
		segment=fil->segments[fil->frontseg+seg];
		xvect[0]=segment->xyzback[0]-segment->xyzfront[0];
		xvect[1]=segment->xyzback[1]-segment->xyzfront[1];
		xvect[2]=segment->xyzback[2]-segment->xyzfront[2];
		len=sqrt(xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2]);
		force=-klen*(len-stdlen)/len;			// this is force/length
		forces[seg][0]-=force*xvect[0];
		forces[seg][1]-=force*xvect[1];
		forces[seg][2]-=force*xvect[2];
		forces[seg+1][0]+=force*xvect[0];
		forces[seg+1][1]+=force*xvect[1];
		forces[seg+1][2]+=force*xvect[2]; }
	return; }


void filAddRandomForces(filamentptr fil) {	//??
	(void) fil;
	return; }


void filAddBendForces(filamentptr fil) {
	double **forces,*torques,torque[3],forcem1[3],forcep1[3],force[3];
	double xvect[3],xvectm1[3],len2,len2inv,len2m1,len2m1inv;
	int node;
	segmentptr segmentm1,segment;

	forces=fil->forces;
	torques=fil->torques;

	for(node=1;node<fil->nseg;node++) {								// compute bending forces
		filBendTorque(fil,node,torque);
		segmentm1=fil->segments[fil->frontseg+node-1];
		segment=fil->segments[fil->frontseg+node];

		xvect[0]=segment->xyzback[0]-segment->xyzfront[0];
		xvect[1]=segment->xyzback[1]-segment->xyzfront[1];
		xvect[2]=segment->xyzback[2]-segment->xyzfront[2];
		xvectm1[0]=segmentm1->xyzback[0]-segmentm1->xyzfront[0];
		xvectm1[1]=segmentm1->xyzback[1]-segmentm1->xyzfront[1];
		xvectm1[2]=segmentm1->xyzback[2]-segmentm1->xyzfront[2];

		len2=xvect[0]*xvect[0]+xvect[1]*xvect[1]+xvect[2]*xvect[2];
		len2inv=1.0/len2;
		len2m1=xvectm1[0]*xvectm1[0]+xvectm1[1]*xvectm1[1]+xvectm1[2]*xvectm1[2];
		len2m1inv=1.0/len2m1;

		forcep1[0]=len2inv*(torque[1]*xvect[2]-torque[2]*xvect[1]);	// torque x xvect / len^2
		forcep1[1]=len2inv*(torque[2]*xvect[0]-torque[0]*xvect[2]);
		forcep1[2]=len2inv*(torque[0]*xvect[1]-torque[1]*xvect[0]);
		crossVVD(torque,xvectm1,forcem1);
		forcem1[0]=-len2m1inv*(torque[1]*xvectm1[2]-torque[2]*xvectm1[1]);		// -torque x xvectm1 / lenm1^2
		forcem1[1]=-len2m1inv*(torque[2]*xvectm1[0]-torque[0]*xvectm1[2]);
		forcem1[2]=-len2m1inv*(torque[0]*xvectm1[1]-torque[1]*xvectm1[0]);
		force[0]=-(forcep1[0]+forcem1[0]);
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

		torques[node]+=(torque[0]*xvect[0]+torque[1]*xvect[1]+torque[2]*xvect[2])*sqrt(len2inv);
		torques[node-1]-=(torque[0]*xvectm1[0]+torque[1]*xvectm1[1]+torque[2]*xvectm1[2])*sqrt(len2m1inv); }

	return; }


void filComputeForces(filamentptr fil) {
	double **forces,*torques;
	int node;

	forces=fil->forces;
	torques=fil->torques;

	for(node=0;node<=fil->nseg;node++) {							// clear all forces
		forces[node][0]=forces[node][1]=forces[node][2]=0;
		torques[node]=0; }
	filAddStretchForces(fil);
	filAddBendForces(fil);

	return; }


/* filEulerDynamics */
void filEulerDynamics(simptr sim,filamentptr fil) {
	int node,seg;
	double **nodes,mobility;
	segmentptr segment;

	mobility=0.1*fil->filtype->kT*sim->dt;										//?? needs fixing
	filComputeForces(fil);
	nodes=fil->nodes;
	for(node=fil->frontseg;node<=fil->frontseg+fil->nseg;node++) {
		nodes[node][0]+=mobility*fil->forces[node][0];
		nodes[node][1]+=mobility*fil->forces[node][1];
		nodes[node][2]+=mobility*fil->forces[node][2]; }
	for(seg=fil->frontseg;seg<fil->frontseg+fil->nseg;seg++) {
		segment=fil->segments[seg];
		segment->ypr[2]+=mobility*fil->torques[seg]; }
	filNodes2Angles(fil);

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

		if(filtype->dynamics==FDeuler) {
			for(f=0;f<filtype->nfil;f++) {
				fil=filtype->fillist[f];
				filEulerDynamics(sim,fil); }}}


/*
			if(filtype->dynamics==FDrouse) {
				k1 = 3*filtype->kT*sim->dt/(6*PI*filtype->viscosity*filtype->filradius*filtype->stdlen*filtype->stdlen); //?? Double check this is kuhn length squared
				k2 = sqrt(2*filtype->kT/(6*PI*filtype->viscosity*filtype->filradius));

				for(b=fil->frontseg;b<=fil->nseg+fil->frontseg;b++){
					for(d=0;d<dim;d++){
						fil->beads[b]->xyzold[d]=fil->beads[b]->xyz[d];}}
					//?? PERHAPS include a check for number of segments to be >= 2

				b=fil->frontseg;
				bead=fil->beads[b];
				beadplus=fil->beads[b+1];
				for(d=0;d<dim;d++){
					bead->xyz[d]=bead->xyzold[d]-k1*(bead->xyzold[d]-beadplus->xyzold[d])+k2*gaussrandD();}

				for(b=fil->frontseg+1;b<fil->nseg+fil->frontseg;b++){
					beadminus=fil->beads[b-1];
					bead=fil->beads[b];
					beadplus=fil->beads[b+1];
					for(d=0;d<dim;d++){
						bead->xyz[d]=bead->xyzold[d]-k1*(2*bead->xyzold[d]-beadminus->xyzold[d]-beadplus->xyzold[d])+k2*gaussrandD();}}

				b=fil->nseg+fil->frontseg;
				beadminus=fil->beads[b-1];
				bead=fil->beads[b];
				for(d=0;d<dim;d++){
					bead->xyz[d]=bead->xyzold[d]-k1*(bead->xyzold[d]-beadminus->xyzold[d])+k2*gaussrandD();}
				}*/

	return 0; }


