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


/******************************************************************************/
/********************************** Filaments *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types


// low level utilities
double filStretchEnergy(filamentptr fil,int seg1,int seg2);
double filBendEnergy(filamentptr fil,int seg1,int seg2);

// memory management
void beadfree(beadptr bead);
void segmentfree(segmentptr segment);
void filamenttypefree(filamenttypeptr filtype);
filamentptr filalloc(filamentptr fil,int isbead,int maxbs,int maxbranch,int maxmonomer);
void filfree(filamentptr fil);


// data structure output

// structure set up
int filtypeSetColor(filamenttypeptr filtype,const double *rgba);
int filtypeSetEdgePts(filamenttypeptr filtype,double value);
int filtypeSetStipple(filamenttypeptr filtype,int factor,int pattern);
int filtypeSetDrawmode(filamenttypeptr filtype,enum DrawMode dm);
int filtypeSetShiny(filamenttypeptr filtype,double shiny);
int filtypeSetBiology(filamenttypeptr filtype,enum FilamentBiology fb);

int filupdateparams(simptr sim);
int filupdatelists(simptr sim);



/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* filFD2string */
char *filFD2string(enum FilamentDynamics fd,char *string) {
	if(fd==FDnone) strcpy(string,"none");
	else if(fd==FDrouse) strcpy(string,"rouse");
	else if(fd==FDalberts) strcpy(string,"alberts");
	else if(fd==FDnedelec) strcpy(string,"nedelec");
	else strcpy(string,"none");
	return string; }


/* filstring2FD */
enum FilamentDynamics filstring2FD(const char *string) {
	enum FilamentDynamics ans;

	if(strbegin(string,"none",0)) ans=FDnone;
	else if(strbegin(string,"rouse",0)) ans=FDrouse;
	else if(strbegin(string,"alberts",0)) ans=FDalberts;
	else if(strbegin(string,"nedelec",0)) ans=FDnedelec;
	else ans=FDnone;
	return ans; }


/* filFB2string */
char *filFB2string(enum FilamentBiology fb,char *string) {
	if(fb==FBactin) strcpy(string,"actin");
	else if(fb==FBmicrotubule) strcpy(string,"microtubule");
	else if(fb==FBintermediate) strcpy(string,"intermediate");
	else if(fb==FBdsDNA) strcpy(string,"dsDNA");
	else if(fb==FBssDNA) strcpy(string,"ssDNA");
	else strcpy(string,"other");
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



/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* filRandomLength */
double filRandomLength(const filamenttypeptr filtype,double thickness,double sigmamult) {
	double lsigma;
	double length;

	if(filtype->klen<=0) return filtype->stdlen;
	lsigma=sigmamult*sqrt(filtype->kT/(thickness*filtype->klen));
	length=0;
	while(length<=0)
		length=filtype->stdlen+lsigma*gaussrandD();
	return length; }


/* filRandomAngle */
double *filRandomAngle(const filamenttypeptr filtype,double thickness,double *angle,double sigmamult) {
	double sigma;
	int d;

	for(d=0;d<3;d++){
		if(filtype->kypr[d]>0)
			sigma=sigmamult*sqrt(filtype->kT/(thickness*filtype->kypr[d]));
		else if(filtype->kypr[d]==0)
			sigma=unirandOCD(-PI,PI);
		else
			sigma=0;
		angle[d]=filtype->stdypr[d]+(sigma>0?sigma*gaussrandD():0); }
	return angle; }


/******************************************************************************/
/*************************** computations on filaments ************************/
/******************************************************************************/


// filStretchEnergy
double filStretchEnergy(const filamentptr fil,int seg1,int seg2) {
	double thk,klen,len,stdlen,energy,*xyz,*xyzplus;
	filamenttypeptr filtype;
	int bs;

	filtype=fil->filtype;
	stdlen=filtype->stdlen;
	klen=filtype->klen;
	if(klen<=0) return 0;

	energy=0;
	if(seg1==-1) seg1=fil->frontbs;
	if(seg2==-1) seg2=fil->frontbs+fil->nbs;
	if(filtype->isbead) {
		for(bs=seg1;bs<seg2-1;bs++) {
			xyz=fil->beads[bs]->xyz;
			xyzplus=fil->beads[bs+1]->xyz;
			len=sqrt((xyzplus[0]-xyz[0])*(xyzplus[0]-xyz[0])+(xyzplus[1]-xyz[1])*(xyzplus[1]-xyz[1])+(xyzplus[2]-xyz[2])*(xyzplus[2]-xyz[2]));
			energy+=0.5*klen*(len-stdlen)*(len-stdlen); }}
	else {
		for(bs=seg1;bs<seg2;bs++){
			thk=fil->segments[bs]->thk;
			len=fil->segments[bs]->len;
			energy+=0.5*thk*klen*(len-stdlen)*(len-stdlen); }}
	return energy; }


// filBendEnergy
double filBendEnergy(const filamentptr fil,int seg1,int seg2) {
	double *kypr,*stdypr,*ypr,thk,thkminus,energy;
	filamenttypeptr filtype;
	segmentptr segptr,segptrminus;
	double tk;
	int seg;

	filtype=fil->filtype;
	if(filtype->isbead) return 0;

	kypr=filtype->kypr;
	stdypr=filtype->stdypr;

	energy=0;
	if(seg1==-1) seg1=fil->frontbs;
	if(seg2==-1) seg2=fil->frontbs+fil->nbs;
	for(seg=seg1+1;seg<seg2;seg++) {
		segptr = fil->segments[seg];
		segptrminus = fil->segments[seg-1];
		ypr = segptr->ypr;
		thk = segptr->thk;
		thkminus = segptrminus->thk;
		tk=0.5*(thkminus+thk);
		if(kypr[0]>0) energy+=0.5*tk*kypr[0]*(ypr[0]-stdypr[0])*(ypr[0]-stdypr[0]);
		if(kypr[1]>0) energy+=0.5*tk*kypr[1]*(ypr[1]-stdypr[1])*(ypr[1]-stdypr[1]);
		if(kypr[2]>0) energy+=0.5*tk*kypr[2]*(ypr[2]-stdypr[2])*(ypr[2]-stdypr[2]); }
	return energy; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* beadalloc */
beadptr beadalloc() {
	beadptr bead;

	CHECKMEM(bead=(beadptr) malloc(sizeof(struct beadstruct)));
	bead->xyz[0]=bead->xyz[1]=bead->xyz[2]=0;
	bead->xyzold[0]=bead->xyzold[1]=bead->xyzold[2]=0;
	return bead;

 failure:
	return NULL; }


/* beadfree */
void beadfree(beadptr bead) {
	if(!bead) return;
	free(bead);
	return; }


/* segmentalloc */
segmentptr segmentalloc() {
	segmentptr segment;

	CHECKMEM(segment=(segmentptr) malloc(sizeof(struct segmentstruct)));
	segment->xyzfront[0]=segment->xyzfront[1]=segment->xyzfront[2]=0;
	segment->xyzback[0]=segment->xyzback[1]=segment->xyzback[2]=0;
	segment->len=0;
	segment->ypr[0]=segment->ypr[1]=segment->ypr[2]=0;
	Sph_One2Dcm(segment->dcm);
	Sph_One2Dcm(segment->adcm);
	segment->thk=1;
	return segment;

 failure:
	return NULL; }


/* segmentfree */
void segmentfree(segmentptr segment) {
	if(!segment) return;
	free(segment);
	return; }


/* filalloc */
filamentptr filalloc(filamentptr fil,int isbead,int maxbs,int maxbranch,int maxmonomer) {
	int bs,br,mn;
	beadptr *newbeads;
	segmentptr *newsegments;
	int *newbranchspots;
	filamentptr *newbranches;
	char *newmonomers;

	if(!fil) {
		CHECKMEM(fil=(filamentptr) malloc(sizeof(struct filamentstruct)));
		fil->filtype=NULL;
		fil->filname=NULL;
		fil->maxbs=0;
		fil->nbs=0;
		fil->frontbs=0;
		fil->beads=NULL;
		fil->segments=NULL;
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

	if(maxbs>fil->maxbs) {
		if(isbead) {
			CHECKMEM(newbeads=(beadptr*) calloc(maxbs,sizeof(struct beadstruct)));
			for(bs=0;bs<maxbs;bs++)
				newbeads[bs]=NULL;
			for(bs=0;bs<fil->maxbs;bs++)
				newbeads[bs]=fil->beads[bs];
			for(;bs<maxbs;bs++)
				CHECKMEM(newbeads[bs]=beadalloc());
			free(fil->beads);
			fil->beads=newbeads; }
		else {
			CHECKMEM(newsegments=(segmentptr*) calloc(maxbs,sizeof(struct segmentstruct)));
			for(bs=0;bs<maxbs;bs++)
				newsegments[bs]=NULL;
			for(bs=0;bs<fil->maxbs;bs++)
				newsegments[bs]=fil->segments[bs];
			for(;bs<maxbs;bs++)
				CHECKMEM(newsegments[bs]=segmentalloc());
			free(fil->segments);
			fil->segments=newsegments; }
		fil->maxbs=maxbs; }

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
	int bs;

	if(!fil) return;
	if(fil->beads) {
		for(bs=0;bs<fil->maxbs;bs++)
			beadfree(fil->beads[bs]);
		free(fil->beads); }
	if(fil->segments) {
		for(bs=0;bs<fil->maxbs;bs++)
			segmentfree(fil->segments[bs]);
		free(fil->segments); }

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
		filtype->dynamics=FDnone;
		filtype->isbead=0;
		filtype->biology=FBother;
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
		filtype->kT=0;
		filtype->treadrate=0;
		filtype->viscosity=1;
		filtype->filradius=1;

		filtype->maxface=0;
		filtype->nface=0;
		filtype->facename=NULL;
		filtype->facetwist=0;

		filtype->maxfil=0;
		filtype->nfil=0;
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
			CHECKMEM(newfillist[f]=filalloc(NULL,0,0,0,0));
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
	int bs,br,mn,isbead,dim;
	simptr sim;
	beadptr bead;
	segmentptr segment;

	if(!fil) {
		simLog(NULL,2," NULL filament\n");
		return; }

	isbead=fil->filtype?fil->filtype->isbead:0;
	if(fil->filtype && fil->filtype->filss) {
		sim=fil->filtype->filss->sim;
		dim=sim->dim; }
	else {
		sim=NULL;
		dim=3; }

	simLog(sim,2," Filament: %s\n",fil->filname);
	simLog(sim,1,"  allocated beads or segments: %i\n",fil->maxbs);
	simLog(sim,2,"  number of %s: %i\n",isbead?"beads":"segments",fil->nbs);
	simLog(sim,1,"  front index: %i\n",fil->frontbs);

	if(isbead)
		simLog(sim,2,"  bead, position\n");
	else
		simLog(sim,2,"  segment, thickness, length, front position, relative angle\n");
	for(bs=0;bs<fil->nbs;bs++) {
		if(isbead) {
			bead=fil->beads[bs+fil->frontbs];
			if(dim==2)
				simLog(sim,bs>5?1:2,"   %i pos.=(%1.3g %1.3g)\n",bs,bead->xyz[0],bead->xyz[1]);
			else
				simLog(sim,bs>5?1:2,"   %i x=(%1.3g %1.3g %1.3g)\n",bs,bead->xyz[0],bead->xyz[1],bead->xyz[2]); }
		else {
			segment=fil->segments[bs+fil->frontbs];
			if(dim==2)
				simLog(sim,bs>5?1:2,"   %i thick=%1.3g, length=%1.3g, front pos.=(%1.3g %1.3g), rel. angle=%1.3g\n",bs,segment->thk,segment->len,segment->xyzfront[0],segment->xyzfront[1],segment->ypr[0]);
			else
				simLog(sim,bs>5?1:2,"   %i thick=%1.3g, length=%1.3g, front pos.=(%1.3g %1.3g %1.3g), rel. angle=(%1.3g %1.3g %1.3g)\n",bs,segment->thk,segment->len,segment->xyzfront[0],segment->xyzfront[1],segment->xyzfront[2],segment->ypr[0],segment->ypr[1],segment->ypr[2]); }}

	if(fil->frontend)
		simLog(sim,2,"  front branched from: %s\n",fil->frontend->filname);
	if(fil->backend)
		simLog(sim,2,"  back branched from: %s\n",fil->backend->filname);
	if(fil->nbranch>0) {
		simLog(sim,2,"  number of branches: %i\n",fil->nbranch);
		for(br=0;br<fil->nbranch;br++)
			simLog(sim,2,"   %s at %i\n",fil->branches[br]->filname,fil->branchspots[br]); }

	if(fil->nmonomer) {
		simLog(sim,1,"  monomer codes: %i of %i allocated,",fil->nmonomer,fil->maxmonomer);
		simLog(sim,1," starting index: %i\n",fil->frontmonomer);
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
	int isbead,fc,f,dim;
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
	isbead=filtype->isbead;

	simLog(sim,2," Filament type: %s\n",filtype->ftname);
	simLog(sim,2,"  dynamics: %s using %s\n",filFD2string(filtype->dynamics,string),isbead?"beads":"segments");
	simLog(sim,2,"  biology: %s\n",filFB2string(filtype->biology,string));
	simLog(sim,filtype->bundlevalue!=1?2:1,"  bundle value: %g\n",filtype->bundlevalue);

	simLog(sim,2,"  color: %g %g %g %g\n",filtype->color[0],filtype->color[1],filtype->color[2],filtype->color[3]);
	simLog(sim,2,"  edge points: %g, polygon mode: %s\n",filtype->edgepts,surfdm2string(filtype->drawmode,string));
	if(filtype->edgestipple[1]!=0xFFFF) simLog(sim,2,"  edge stippling: %ui %X\n",filtype->edgestipple[0],filtype->edgestipple[1]);
	if(filtype->shiny!=0) simLog(sim,2,"  shininess: %g\n",filtype->shiny);

	simLog(sim,2,"  %s length: %g\n",filtype->klen>0?"standard":"fixed",filtype->stdlen);
	if(filtype->klen>0) simLog(sim,2,"  length force constant: %g\n",filtype->klen);

	if(!isbead) {
		if(dim==2) {
			simLog(sim,2,"  standard angle: %g\n",filtype->stdypr[0]);
			simLog(sim,2,"  bending force constant: %g\n",filtype->kypr[0]); }
		else {
			simLog(sim,2,"  standard angles: %g, %g, %g\n",filtype->stdypr[0],filtype->stdypr[1],filtype->stdypr[2]);
			simLog(sim,2,"  bending force constants: %g, %g, %g\n",filtype->kypr[0],filtype->kypr[1],filtype->kypr[2]); }}

	simLog(sim,2,"  kT: %g\n",filtype->kT);
	simLog(sim,filtype->treadrate>0?2:1,"  treadmilling rate: %g\n",filtype->treadrate);
	simLog(sim,2,"  viscosity: %g\n",filtype->viscosity);
	simLog(sim,2,"  %s radius: %g\n",isbead?"bead":"segment",filtype->filradius);

	if(filtype->nface>0) {
		simLog(sim,2,"  %i faces with twist of %g:",filtype->nface,filtype->facetwist);
		for(fc=0;fc<filtype->nface;fc++)
			simLog(sim,2," %s",filtype->facename[fc]);
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
/****************************** filament manipulation *************************/
/******************************************************************************/


/* filArrayShift */
void filArrayShift(filamentptr fil,int shift) {
	int i,frontbs,backbs;
	int isbead;
	beadptr nullbead;
	segmentptr nullsegment;

	isbead=fil->filtype->isbead;
	if(!shift)
		shift=(fil->maxbs-fil->nbs)/2-fil->frontbs;

	frontbs=fil->frontbs;
	backbs=fil->frontbs+fil->nbs;

	if(shift>0) {
		if(backbs+shift>fil->maxbs) shift=fil->maxbs-backbs;
		if(isbead) {
			for(i=backbs+shift-1;i>=frontbs+shift;i--) {		// i is destination
				nullbead = fil->beads[i];
				fil->beads[i] = fil->beads[i-shift];
				fil->beads[i-shift] = nullbead; }}
		else {
			for(i=backbs+shift-1;i>=frontbs+shift;i--) {
				nullsegment = fil->segments[i];
				fil->segments[i]=fil->segments[i-shift];
				fil->segments[i-shift] = nullsegment; }}
		fil->frontbs+=shift; }

	else if(shift<0) {
		shift=-shift;								// now shift is positive
		if(frontbs-shift<0) shift=frontbs;
		if(isbead) {
			for(i=frontbs-shift;i<backbs-shift;i++) {				// i is destination
				nullbead = fil->beads[i];
				fil->beads[i]=fil->beads[i+shift];
				fil->beads[i+shift] = nullbead;}}
		else {
			for(i=frontbs-shift;i<backbs-shift;i++) {
				nullsegment = fil->segments[i];
				fil->segments[i]=fil->segments[i+shift];
				fil->segments[i+shift] = nullsegment; }}
		fil->frontbs-=shift; }

	return; }


/* filAddBead */
int filAddBead(filamentptr fil,const double *x,double length,char endchar) {
	int seg,dim;
	double xyz[3];
	filamenttypeptr filtype;
	beadptr bead,beadminus,beadplus;

	filtype=fil->filtype;
	dim=filtype->filss->sim->dim;
	if(!filtype->isbead) return -2;	// can't add a bead to a non-bead filament type

	if(fil->nbs==fil->maxbs) {
		fil=filalloc(fil,1,fil->maxbs*2+1,0,0);
		if(!fil) return -1; }		// out of memory

	if(endchar=='b') {				// add to back end
		if(fil->frontbs+fil->nbs==fil->maxbs)
			filArrayShift(fil,0);
		seg=fil->frontbs+fil->nbs;
		bead=fil->beads[seg];
		if(fil->nbs==0) {
			bead->xyz[0]=bead->xyzold[0]=x[0];
			bead->xyz[1]=bead->xyzold[1]=x[1];
			bead->xyz[2]=bead->xyzold[2]=x[2]; }		// xyzold = xyz = x
		else {
			beadminus=fil->beads[seg-1];
			if(dim==2) {
				circlerandD(xyz,length);
				xyz[2]=0; }
			else
				sphererandCCD(xyz,length,length);
			bead->xyz[0]=bead->xyzold[0]=beadminus->xyz[0]+xyz[0];
			bead->xyz[1]=bead->xyzold[1]=beadminus->xyz[1]+xyz[1];
			bead->xyz[2]=bead->xyzold[2]=beadminus->xyz[2]+xyz[2]; }
		fil->nbs++; }

	else {									// add to front end
		if(fil->frontbs==0) filArrayShift(fil,0);
		if(fil->frontbs==0) filArrayShift(fil,1);								// used if nbs=maxbs-1
		seg=fil->frontbs-1;
		bead=fil->beads[seg];
		if(fil->nbs==0) {
			bead->xyz[0]=bead->xyzold[0]=x[0];
			bead->xyz[1]=bead->xyzold[1]=x[1];
			bead->xyz[2]=bead->xyzold[2]=x[2]; }
		else {
			beadplus=fil->beads[seg+1];
			if(dim==2) {
				circlerandD(xyz,length);
				xyz[2]=0; }
			else
				sphererandCCD(xyz,length,length);
			bead->xyz[0]=bead->xyzold[0]=beadplus->xyz[0]+xyz[0];
			bead->xyz[1]=bead->xyzold[1]=beadplus->xyz[1]+xyz[1];
			bead->xyz[2]=bead->xyzold[2]=beadplus->xyz[2]+xyz[2]; }
		fil->frontbs--;
		fil->nbs++; }

	return 0; }


/* filAddSegment */
int filAddSegment(filamentptr fil,const double *x,double length,const double *angle,double thickness,char endchar) {
	int seg,dim;
    UNUSED(dim);
	filamenttypeptr filtype;
	segmentptr segment,segmentminus,segmentplus;

	filtype=fil->filtype;
	dim=filtype->filss->sim->dim;
	if(filtype->isbead) return -2;	// can't add a segment to a bead filament type

	if(fil->nbs==fil->maxbs) {
		fil=filalloc(fil,0,fil->maxbs*2+1,0,0);
		if(!fil) return -1; }		// out of memory

	if(endchar=='b') {
		if(fil->frontbs+fil->nbs==fil->maxbs) filArrayShift(fil,0);
		seg=fil->frontbs+fil->nbs;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		Sph_Xyz2Xyz(angle,segment->ypr);												// ypr = angle
		Sph_Xyz2Dcm(angle,segment->dcm);												// A = Dcm(angle)
		if(fil->nbs==0) {
			segment->xyzfront[0]=x[0];														// x_i = input value
			segment->xyzfront[1]=x[1];
			segment->xyzfront[2]=x[2];
			Sph_Dcm2Dcm(segment->dcm,segment->adcm); }						// B = A
		else {
			segmentminus=fil->segments[seg-1];
			segment->xyzfront[0]=segmentminus->xyzback[0];				// x_i = prior end
			segment->xyzfront[1]=segmentminus->xyzback[1];
			segment->xyzfront[2]=segmentminus->xyzback[2];
			Sph_DcmxDcm(segment->dcm,segmentminus->adcm,segment->adcm);	}	// B_i = A_i . B_{i-1}
		Sph_DcmtxUnit(segment->adcm,'x',segment->xyzback,segment->xyzfront,segment->len);		// x_{i+1} = x_i + l_i * BT_i . xhat
		fil->nbs++; }

	else {
		if(fil->frontbs==0) filArrayShift(fil,0);
		if(fil->frontbs==0) filArrayShift(fil,1);								// used if nbs=maxbs-1
		seg=fil->frontbs;
		segment=fil->segments[seg];
		segment->len=length;
		segment->thk=thickness;
		if(fil->nbs==0) {
			Sph_Xyz2Dcmt(angle,segment->adcm);									// B_0 = Dcmt(angle)
			segment->xyzback[0]=x[0];														// back of segment = input value
			segment->xyzback[1]=x[1];
			segment->xyzback[2]=x[2]; }
		else {
			segmentplus=fil->segments[seg+1];
			segment->xyzback[0]=segmentplus->xyzfront[0];				// back of segment = next front
			segment->xyzback[1]=segmentplus->xyzfront[1];
			segment->xyzback[2]=segmentplus->xyzfront[2];
			Sph_DcmtxDcm(segmentplus->dcm,segmentplus->adcm,segment->adcm); }	// B_i = AT_{i+1} . B_{i+1}
		Sph_Dcm2Dcm(segment->adcm,segment->dcm);									// A_i = B_i
		Sph_Dcm2Xyz(segment->dcm,segment->ypr);										// a_0 = Xyz(B_0)
		Sph_DcmtxUnit(segment->adcm,'x',segment->xyzfront,segment->xyzback,-segment->len);	// x_i = x_{i+1} - l_i * BT_i . xhat
		fil->frontbs--;
		fil->nbs++; }
	return 0; }


/* filAddRandomSegments */
int filAddRandomSegments(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr,double thickness) {
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

	if(fil->nbs==0) {								// new filament
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
		else return 2; }
	else
		pos[0]=pos[1]=pos[2]=0;

	for(i=0;i<number;i++) {
		len=filRandomLength(filtype,thickness,1);
		filRandomAngle(filtype,thickness,angle,1);
		if(dim==2) angle[1]=angle[2]=0;
		er=filAddSegment(fil,pos,len,angle,thickness,'b');
		if(er) return er; }

	return 0; }


/* filAddRandomBeads */
int filAddRandomBeads(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr) {
	int i,dim,er;
	double f1,pos[3],len;
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

	if(fil->nbs==0) {
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
		else return 2; }
	else
		pos[0]=pos[1]=pos[2]=0;

	for(i=0;i<number;i++) {
		len=filRandomLength(filtype,1,1);
		er=filAddBead(fil,pos,len,'b');
		if(er) return er; }

	return 0; }


/* filRemoveSegment */
int filRemoveSegment(filamentptr fil,char endchar) {
	int seg;
	segmentptr segment;

	if(fil->nbs==0) return -1;

	if(endchar=='b')
		fil->nbs--;
	else {
		seg=++fil->frontbs;
		fil->nbs--;
		if(!fil->filtype->isbead) {
			segment=fil->segments[seg];
			Sph_Dcm2Dcm(segment->adcm,segment->dcm);
			Sph_Dcm2Xyz(segment->dcm,segment->ypr); }}
	return 0; }


/* filTranslate */
void filTranslate(filamentptr fil,const double *vect,char func) {
	int seg;
	double shift[3];
	segmentptr segment;
	beadptr bead;

	seg=fil->frontbs;
	if(func=='=') {
		if(fil->filtype->isbead){
			bead=fil->beads[seg];
			shift[0]=bead->xyz[0]-vect[0];
			shift[1]=bead->xyz[1]-vect[1];
			shift[2]=bead->xyz[2]-vect[2]; }
		else{
			segment=fil->segments[seg];
			shift[0]=segment->xyzfront[0]-vect[0];
			shift[1]=segment->xyzfront[1]-vect[1];
			shift[2]=segment->xyzfront[2]-vect[2];}}
	else if(func=='-') {
		shift[0]=-vect[0];
		shift[1]=-vect[1];
		shift[2]=-vect[2]; }
	else {
		shift[0]=vect[0];
		shift[1]=vect[1];
		shift[2]=vect[2]; }

	if(fil->filtype->isbead){
		for(seg=0;seg<fil->nbs;seg++) {
			bead = fil->beads[seg+fil->frontbs];
			bead->xyz[0]+=shift[0];
			bead->xyz[1]+=shift[1];
			bead->xyz[2]+=shift[2];
			bead->xyzold[0]+=shift[0];
			bead->xyzold[1]+=shift[1];
			bead->xyzold[2]+=shift[2]; }}
	else {
		for(seg=0;seg<fil->nbs;seg++) {
			segment=fil->segments[seg+fil->frontbs];
			segment->xyzfront[0]+=shift[0];
			segment->xyzfront[1]+=shift[1];
			segment->xyzfront[2]+=shift[2];
			segment->xyzback[0]+=shift[0];
			segment->xyzback[1]+=shift[1];
			segment->xyzback[2]+=shift[2]; }}

	return; }


/* filRotateVertex */
void filRotateVertex(filamentptr fil,int seg,const double *angle,char endchar,char func) {
	(void)fil;
	(void)seg;
	(void)angle;
	(void)endchar;
	(void)func;
	return; }


/* filLengthenSegment */
void filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func) {
	(void)fil;
	(void)seg;
	(void)length;
	(void)endchar;
	(void)func;
	return; }


/* filReverseFilament */
void filReverseFilament(filamentptr fil) {
	(void)fil;
	return; }


/* filCopyFilament */
int filCopyFilament(filamentptr filto,const filamentptr filfrom) {
	int isbead,i;
	beadptr beadto,beadfrom;
	segmentptr segmentto,segmentfrom;

	if(!filto || !filfrom) return 2;

	if(filfrom->filtype) isbead=filfrom->filtype->isbead;
	else if(filfrom->beads) isbead=1;
	else isbead=0;

	filto->filtype=filfrom->filtype;

	filto->nbs=0;
	filto->frontbs=0;
	filto->nbranch=0;
	filto->nmonomer=0;
	filto->frontmonomer=0;
	filto=filalloc(filto,isbead,filfrom->nbs,filfrom->nbranch,filfrom->nmonomer);
	if(!filto) return 1;

	if(isbead) {
		for(i=0;i<filfrom->nbs;i++) {
			beadto=filto->beads[i];
			beadfrom=filfrom->beads[i+filfrom->frontbs];
			copyVD(beadfrom->xyz,beadto->xyz,3);
			copyVD(beadfrom->xyzold,beadto->xyzold,3); }}
	else {
		for(i=0;i<filfrom->nbs;i++) {
			segmentto=filto->segments[i];
			segmentfrom=filfrom->segments[i+filfrom->frontbs];
			copyVD(segmentfrom->xyzfront,segmentto->xyzfront,3);
			copyVD(segmentfrom->xyzback,segmentto->xyzback,3);
			segmentto->len=segmentfrom->len;
			copyVD(segmentfrom->ypr,segmentto->ypr,3);
			copyVD(segmentfrom->dcm,segmentto->dcm,9);
			copyVD(segmentfrom->adcm,segmentto->adcm,9);
			segmentto->thk=segmentfrom->thk; }}
	filto->nbs=filfrom->nbs;

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


/******************************************************************************/
/********************************* filament type ******************************/
/******************************************************************************/


/* filtypeSetParam */
int filtypeSetParam(filamenttypeptr filtype,const char *param,int index,double value) {
	int er;

	er=0;
	if(!strcmp(param,"stdlen")) {
		if(value<=0) er=2;
		else filtype->stdlen=value; }

	else if(!strcmp(param,"stdypr")) {
		if(index<0)
			filtype->stdypr[0]=filtype->stdypr[1]=filtype->stdypr[2]=value;
		else filtype->stdypr[index]=value; }

	else if(!strcmp(param,"klen"))
		filtype->klen=value;

	else if(!strcmp(param,"kypr")) {
		if(index<0) filtype->kypr[0]=filtype->kypr[1]=filtype->kypr[2]=value;
		else filtype->kypr[index]=value; }

	else if(!strcmp(param,"kT")) {
		if(value<=0) er=2;
		else filtype->kT=value; }

	else if(!strcmp(param,"treadrate"))
		filtype->treadrate=value;

	else if(!strcmp(param,"viscosity")) {
		if(value<=0) er=2;
		else filtype->viscosity=value; }

	else if(!strcmp(param,"beadradius")) {
		if(value<=0) er=2;
		else filtype->filradius=value; }

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
	filtype->dynamics=fd;			//?? check for pre-existing dynamics and convert if needed
	if(fd==FDrigidbeads) filtype->isbead=1;
	else if(fd==FDrigidsegments) filtype->isbead=0;
	else if(fd==FDrouse) filtype->isbead=1;
	else if(fd==FDalberts) filtype->isbead=0;
	else if(fd==FDnedelec) filtype->isbead=0;
	return 0; }


/* filtypeSetBiology */
int filtypeSetBiology(filamenttypeptr filtype,enum FilamentBiology fb) {
	filtype->biology=fb;	//?? set parameters to match
	return 0; }


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

	filss=filssalloc(sim->filss,1);
	if(!filss) return 1;
	sim->filss=filss;
	filss->sim=sim;
	filsetcondition(sim->filss,SClists,0);
	return 0; }


/* filAddFilament */
filamentptr filAddFilament(filamenttypeptr filtype,filamentptr fil,const char *filname) {
	int f;
	filamentptr fil2;

	if(!filtype && fil)											// no filtype, yes fil
		return fil;

	else if(!filtype) {											// no filtype, no fil
		fil=filalloc(NULL,0,0,0,0);
		if(!fil) return NULL;
		fil->filname=EmptyString();
		if(!fil->filname) return NULL; }

	else {																	// yes filtype, yes or no fil
		f=stringfind(filtype->filnames,filtype->nfil,filname);
		if(f<0) {
			if(filtype->nfil==filtype->maxfil) {
				filtype=filamenttypealloc(filtype,filtype->maxfil*2+1,0);
				if(!filtype) return NULL; }
			f=filtype->nfil++;
			strncpy(filtype->filnames[f],filname,STRCHAR-1);
			filtype->filnames[f][STRCHAR-1]='\0';
			fil2=filtype->fillist[f];
			if(fil) {
				filCopyFilament(fil2,fil);
				fil2->filtype=filtype;
				free(fil->filname);
				filfree(fil); }
			fil=fil2;
			filsetcondition(filtype->filss,SClists,0); }
		else
			fil=filtype->fillist[f]; }

	return fil; }


/* filAddFilamentType */
filamenttypeptr filAddFilamentType(simptr sim,const char *ftname) {
	int ft;
	filamentssptr filss;
	filamenttypeptr filtype;
	filamentssptr fssptr;

	filss=sim->filss;

	ft=stringfind(filss->ftnames,filss->ntype,ftname);
	if(ft<0) {
		if(filss->ntype==filss->maxtype) {
			fssptr=filssalloc(filss,filss->maxtype*2+1);
			if(!fssptr) return NULL; }
		ft=filss->ntype++;
		strncpy(filss->ftnames[ft],ftname,STRCHAR-1);
		filss->ftnames[ft][STRCHAR-1]='\0';
		filtype=filss->filtypes[ft];
		filsetcondition(filss,SClists,0); }
	else
		filtype=filss->filtypes[ft];

	return filtype; }


/******************************************************************************/
/********************************** user input ********************************/
/******************************************************************************/


/* filtypereadstring */
filamenttypeptr filtypereadstring(simptr sim,ParseFilePtr pfp,filamenttypeptr filtype,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar;

	filamentssptr filss;
    UNUSED(filss);
	int itct,er,i1,i2;
	char nm[STRCHAR],nm1[STRCHAR];
	double fltv1[9],f1;
	enum DrawMode dm;
	enum FilamentDynamics fd;
	enum FilamentBiology fb;

	printf("%s\n",word);//?? debug
	filss=sim->filss;

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
		CHECKS(itct==1,"dynamics options: RigidBeads, RigidSegements, Rouse, Alberts, Nedelec");
		fd=filstring2FD(nm1);
		CHECKS(fd!=FDnone,"dynamics options: RigidBeads, RigidSegements, Rouse, Alberts, Nedelec");
		er=filtypeSetDynamics(filtype,fd); //?? beware of changing between beads and segments
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
		CHECKS(f1>0,"kT value needs to be greater than 0");
		filtypeSetParam(filtype,"kT",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following kT"); }

	else if(!strcmp(word,"treadmill_rate")) {		// treadmill_rate
		CHECKS(filtype,"need to enter filament type name before treadmill_rate");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"treadmill_rate format: value");
		filtypeSetParam(filtype,"treadrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following treadrate"); }

	else if(!strcmp(word,"viscosity")) {			// viscosity
		CHECKS(filtype,"need to enter filament type name before viscosity");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"viscosity format: value");
		CHECKS(f1>0,"viscosity value needs to be greater than 0");
		filtypeSetParam(filtype,"viscosity",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following viscosity"); }

	else if(!strcmp(word,"bead_radius")) {			// beadradius
		CHECKS(filtype,"need to enter filament type name before bead radius");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"bead_radius format: value");
		filtypeSetParam(filtype,"beadradius",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following bead radius"); }

	else if(!strcmp(word,"standard_length")) {	// standard_length
		CHECKS(filtype,"need to enter filament type name before standard_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"standard_length format: value");
		CHECKS(f1>0,"standard_length value needs to be greater than 0");
		filtypeSetParam(filtype,"stdlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following standard_length"); }

	else if(!strcmp(word,"standard_angle")) {	// standard_angle
		CHECKS(filtype,"need to enter filament type name before standard_angle");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
		CHECKS(itct==3,"standard_angle format: yaw pitch roll");
		filtypeSetParam(filtype,"stdypr",0,fltv1[0]);
		filtypeSetParam(filtype,"stdypr",1,fltv1[1]);
		filtypeSetParam(filtype,"stdypr",2,fltv1[2]);
		CHECKS(!strnword(line2,4),"unexpected text following standard_angle"); }

	else if(!strcmp(word,"force_length")) {		// force_length
		CHECKS(filtype,"need to enter filament type name before force_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"force_length format: value");
		filtypeSetParam(filtype,"klen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following force_length"); }

	else if(!strcmp(word,"force_angle")) {		// force_angle
		CHECKS(filtype,"need to enter filament type name before force_angle");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
		CHECKS(itct==3,"force_angle format: yaw pitch roll");
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
	char nm[STRCHAR],nm1[STRCHAR],endchar,str1[STRCHAR],str2[STRCHAR],str3[STRCHAR],symbol;
	double fltv1[9],length,angle[3],thick;

	printf("%s\n",word);//?? debug
	filss=sim->filss;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(!strcmp(word,"name")) {								// name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading filament name");
		fil=filAddFilament(filtype,fil,nm);
		CHECKS(fil,"failed to add filament");
		CHECKS(!strnword(line2,2),"unexpected text following filament name"); }

	else if(!strcmp(word,"type")) {						// type
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"error reading filament type");
		i1=stringfind(filss->ftnames,filss->ntype,nm1);
		CHECKS(i1>=0,"filament type not recognized");
		filtype=filss->filtypes[i1];
		if(fil) {
			CHECKS(fil->filtype==NULL || fil->filtype==filtype,"filament type was already defined and cannot be changed");
			fil=filAddFilament(filtype,fil,fil->filname);
			CHECKS(fil,"failed to add filament"); }
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"first_segment")) {		// first_segment
		CHECKS(fil,"need to enter filament name before first_segment");
		CHECKS(fil->nbs==0,"filament already has segments in it");
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
		CHECKS(fil->nbs>0,"use first_segment to enter the first segment");
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
		CHECKS(fil->nbs>0,"filament has no segments to remove");
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
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"random_segments format: number [x y z] [thickness]");
		CHECKS(i1>0,"number needs to be >0");
		line2=strnword(line2,2);
		if(fil->nbs==0) {
			CHECKS(line2,"missing x y z position information");
			itct=sscanf(line2,"%s %s %s",str1,str2,str3);
			CHECKS(itct==3,"random_segments format: number [x y z] [thickness]");
			line2=strnword(line2,4); }
		else {
			sprintf(str1,"%i",0);
			sprintf(str2,"%i",0);
			sprintf(str3,"%i",0); }
		thick=1;
		if(line2) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&thick);
			CHECKS(itct==1,"random_segments format: number [x y z] [thickness]");
			CHECKS(thick>0,"thickness needs to be >0");
			line2=strnword(line2,2); }
		er=filAddRandomSegments(fil,i1,str1,str2,str3,thick);
		CHECKS(er!=2,"random_segments positions need to be 'u' or value");
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
		fil2=filAddFilament(fil->filtype,NULL,nm);
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
	char word[STRCHAR],errstring[STRCHAR];
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
int filloadfil(simptr sim,ParseFilePtr *pfpptr,char *line2,filamenttypeptr filtype) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	filamentptr fil;

	pfp=*pfpptr;
	done=0;
	firstline2=line2?1:0;
	filenablefilaments(sim);
	fil=NULL;

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
/******************************* structure updating ***************************/
/******************************************************************************/


/* filupdateparams */
int filupdateparams(simptr sim) {
	(void) sim;
	return 0; }


/* filupdatelists */
int filupdatelists(simptr sim) {
	(void) sim;
	return 0; }


/* filupdate */
int filsupdate(simptr sim) {
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
/**************************** core simulation functions ***********************/
/******************************************************************************/


/* filSegmentXSurface */
int filSegmentXSurface(simptr sim,filamentptr fil,char endchar) { //?? Currently only suitable for treadmilling, not dynamics
	int s,p,mxs;
	surfaceptr srf;
	panelptr pnl;
	double *pt1,*pt2,crosspt[3];
	enum PanelShape ps;
	filamenttypeptr filtype;
	beadptr bead,beadplus;
	segmentptr segment;

	filtype = fil->filtype;

	if(!sim->srfss) return 0;
	if(endchar=='f') {
		if(filtype->isbead){
			bead=fil->beads[fil->frontbs];
			beadplus=fil->beads[fil->frontbs+1];
			pt1=bead->xyz;
			pt2=beadplus->xyz; }
		else{
			segment=fil->segments[fil->frontbs];
			pt1=segment->xyzfront;
			pt2=segment->xyzback; }}
	else {
		if(filtype->isbead){
			bead=fil->beads[fil->nbs+fil->frontbs-1];
			beadplus=fil->beads[fil->nbs+fil->frontbs];
			pt1=bead->xyz;
			pt2=beadplus->xyz; }
		else{
			segment=fil->segments[fil->nbs+fil->frontbs];
			pt1=segment->xyzfront;
			pt2=segment->xyzback; }}

	mxs=0;
	for(s=0;s<sim->srfss->nsrf && !mxs;s++) {
		srf=sim->srfss->srflist[s];
		for(ps=(enum PanelShape)0;ps<PSMAX && !mxs;ps=(enum PanelShape)(ps+1))
			for(p=0;p<srf->npanel[ps] && !mxs;p++) {
				pnl=srf->panels[ps][p];
				mxs=lineXpanel(pt1,pt2,pnl,3,crosspt,NULL,NULL,NULL,NULL,NULL,0); }}
//				printf("pt1=(%g %g %g), pt2=(%g %g %g), mxs=%i\n",pt1[0],pt1[1],pt1[2],pt2[0],pt2[1],pt2[2],mxs); }}

	return mxs; }


/* filSegmentXFilament */
int filSegmentXFilament(simptr sim,filamentptr fil,char endchar,filamentptr *filptr) { //?? Currently only supported for segments not beads
	int f,i,mxf,mn,mn1,ft;
	double thk=0.0,*pt1,*pt2,dist=0.0;
	filamentssptr filss;
	filamenttypeptr filtype;
	filamentptr fil2;
	segmentptr segment;

	if(endchar=='f') {
		segment=fil->segments[fil->frontbs];
		pt1=segment->xyzfront;
		pt2=segment->xyzback;
		thk=segment->thk;
		mn=mn1=fil->frontbs;
		if(fil->nbs>1) mn1=fil->frontbs+1; }
	else {
		segment=fil->segments[fil->nbs+fil->frontbs];
		pt1=segment->xyzfront;
		pt2=segment->xyzback;
		mn=mn1=fil->nbs+fil->frontbs-1;
		if(fil->nbs>1) mn1=fil->nbs+fil->frontbs-2; }

	mxf=0;
	filss=sim->filss;
	fil2=NULL;
	for(ft=0;ft<filss->ntype && !mxf;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil && !mxf;f++) {
			fil2=filtype->fillist[f];
			for(i=fil2->frontbs;i<fil2->nbs+fil2->frontbs && !mxf;i++) {
				if(!(fil2==fil && (i==mn || i==mn1))) {
					dist=Geo_NearestSeg2SegDist(pt1,pt2,fil2->segments[i]->xyzfront,fil2->segments[i]->xyzfront);		//?? check
					if(dist<thk+fil2->segments[i]->thk) mxf=1; }}}}
	if(mxf && filptr)
		*filptr=fil2;
	return mxf; }


/* filTreadmill */
void filTreadmill(simptr sim,filamentptr fil,int steps) {		//?? Currently for segments only
	int i,mxs,done,iter;
	double thk,length,angle[3],sigmamult,angletry[3],dcm[9];
	filamentptr fil2;
	filamenttypeptr filtype;

	filtype=fil->filtype;
	for(i=0;i<steps;i++) {
		thk=fil->segments[0]->thk;
		done=0;
		sigmamult=1;
		angletry[0]=angletry[1]=angletry[2]=0;
		for(iter=0;iter<500 && !done;iter++) {
			length=filRandomLength(filtype,thk,sigmamult);
			filRandomAngle(filtype,thk,angle,sigmamult);
			if(iter>0 && coinrandD(0.5))
				filAddSegment(fil,NULL,length,angletry,thk,'b');
			else
				filAddSegment(fil,NULL,length,angle,thk,'b');
			mxs=filSegmentXSurface(sim,fil,'b');
			if(!mxs) {
				mxs=filSegmentXFilament(sim,fil,'b',&fil2);
				if(mxs) {
					mxs=coinrandD(0.995);
					Sph_DcmxDcmt(fil2->segments[fil2->nbs+fil2->frontbs-1]->adcm,fil->segments[fil->nbs+fil->frontbs-2]->adcm,dcm);
					Sph_Dcm2Xyz(dcm,angletry); }}
			if(mxs) {
				filRemoveSegment(fil,'b');
				sigmamult*=1.01; }
			else done=1; }
		if(done)
			filRemoveSegment(fil,'f'); }

	return; }


/* filDynamics */
int filDynamics(simptr sim) {
	filamentssptr filss;
	filamentptr fil;
	filamenttypeptr filtype;
	beadptr bead,beadplus,beadminus;
	int f,b,d,ft;
	double k1,k2; // Force & gaussian constants
	int dim;

	filss=sim->filss;
	if(!filss) return 0;

	dim=sim->dim;

	for(ft=0;ft<filss->ntype;ft++) {
		filtype=filss->filtypes[ft];
		for(f=0;f<filtype->nfil;f++) {
			fil=filtype->fillist[f];
			if(filtype->treadrate>0)
				filTreadmill(sim,fil,poisrandD(filtype->treadrate*sim->dt));

			if(filtype->dynamics==FDrouse) {
				k1 = 3*filtype->kT*sim->dt/(6*PI*filtype->viscosity*filtype->filradius*filtype->stdlen*filtype->stdlen); //?? Double check this is kuhn length squared
				k2 = sqrt(2*filtype->kT/(6*PI*filtype->viscosity*filtype->filradius));

				for(b=fil->frontbs;b<=fil->nbs+fil->frontbs;b++){
					for(d=0;d<dim;d++){
						fil->beads[b]->xyzold[d]=fil->beads[b]->xyz[d];}}
					//?? PERHAPS include a check for number of segments to be >= 2

				b=fil->frontbs;
				bead=fil->beads[b];
				beadplus=fil->beads[b+1];
				for(d=0;d<dim;d++){
					bead->xyz[d]=bead->xyzold[d]-k1*(bead->xyzold[d]-beadplus->xyzold[d])+k2*gaussrandD();}

				for(b=fil->frontbs+1;b<fil->nbs+fil->frontbs;b++){
					beadminus=fil->beads[b-1];
					bead=fil->beads[b];
					beadplus=fil->beads[b+1];
					for(d=0;d<dim;d++){
						bead->xyz[d]=bead->xyzold[d]-k1*(2*bead->xyzold[d]-beadminus->xyzold[d]-beadplus->xyzold[d])+k2*gaussrandD();}}

				b=fil->nbs+fil->frontbs;
				beadminus=fil->beads[b-1];
				bead=fil->beads[b];
				for(d=0;d<dim;d++){
					bead->xyz[d]=bead->xyzold[d]-k1*(bead->xyzold[d]-beadminus->xyzold[d])+k2*gaussrandD();}
				}}}

	return 0; }


