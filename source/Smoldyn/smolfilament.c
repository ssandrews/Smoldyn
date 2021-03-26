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
#include "random2.h"
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
filamentptr filalloc(int maxseg);
void filfree(filamentptr fil);


// data structure output

// structure set up
void filSetParam(filamentptr fil,const char *param,int index,double value);
int filsetcolor(filamentptr fil,double *rgba);
int filsetedgepts(filamentptr fil,double value);
int filsetstipple(filamentptr fil,int factor,int pattern);
int filsetdrawmode(filamentptr fil,enum DrawMode dm);
int filsetshiny(filamentptr fil,double shiny);
int filupdateparams(simptr sim);
int filupdatelists(simptr sim);
	


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/







/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* filRandomLength */
double filRandomLength(const filamentptr fil,double thickness,double sigmamult) {
	double lsigma=0;
	double length;

	if(fil->klen<=0) return fil->stdlen;
	lsigma=sigmamult*sqrt(fil->kT/(thickness*fil->klen));
	length=0;
	while(length<=0)
		length=fil->stdlen+lsigma*gaussrandD();
	return length; }


/* filRandomAngle */
double *filRandomAngle(const filamentptr fil,double thickness,double *angle,double sigmamult) {
	static double ysigma,psigma,rsigma;

	ysigma=fil->kypr[0]>0?sigmamult*sqrt(fil->kT/(thickness*fil->kypr[0])):0;
	psigma=fil->kypr[1]>0?sigmamult*sqrt(fil->kT/(thickness*fil->kypr[1])):0;
	rsigma=fil->kypr[2]>0?sigmamult*sqrt(fil->kT/(thickness*fil->kypr[2])):0;
	angle[0]=fil->stdypr[0]+(ysigma>0?ysigma*gaussrandD():0);
	angle[1]=fil->stdypr[1]+(psigma>0?psigma*gaussrandD():0);
	angle[2]=fil->stdypr[2]+(rsigma>0?rsigma*gaussrandD():0);
	return angle; }


// filStretchEnergy
double filStretchEnergy(const filamentptr fil,int seg1,int seg2) {
	double *sthk,klen,*slen,stdlen,energy;
	int seg;
	
	sthk=fil->sthk;
	klen=fil->klen;
	slen=fil->slen;
	stdlen=fil->stdlen;
	energy=0;
	if(seg1==-1) seg1=fil->front;
	if(seg2==-1) seg2=fil->back;
	for(seg=seg1;seg<seg2;seg++)
		energy+=0.5*sthk[seg]*klen*(slen[seg]-stdlen)*(slen[seg]-stdlen);
	return energy; }


// filBendEnergy
double filBendEnergy(const filamentptr fil,int seg1,int seg2) {
	double *kypr,*stdypr,**sypr,*sthk,energy;
	double tk;
	int seg;
	
	kypr=fil->kypr;
	stdypr=fil->stdypr;
	sypr=fil->sypr;
	sthk=fil->sthk;
	energy=0;
	if(seg1==-1) seg1=fil->front;
	if(seg2==-1) seg2=fil->back;
	for(seg=seg1+1;seg<=seg2;seg++) {
		tk=0.5*(sthk[seg-1]+sthk[seg]);
		energy+=0.5*tk*kypr[1]*(sypr[seg][1]-stdypr[1])*(sypr[seg][1]-stdypr[1]);
		energy+=0.5*tk*kypr[2]*(sypr[seg][2]-stdypr[2])*(sypr[seg][2]-stdypr[2]);
		energy+=0.5*tk*kypr[3]*(sypr[seg][3]-stdypr[3])*(sypr[seg][3]-stdypr[3]); }
	return energy; }


/* filArrayShift */
void filArrayShift(filamentptr fil,int shift) {
	int i;
	
	if(!shift)
		shift=(fil->maxseg-fil->nseg)/2-fil->front;

	if(shift>0) {
		i=fil->back+shift;
		fil->sxyz[i][0]=fil->sxyz[i-shift][0];
		fil->sxyz[i][1]=fil->sxyz[i-shift][1];
		fil->sxyz[i][2]=fil->sxyz[i-shift][2];
		for(i=fil->back+shift-1;i>=fil->front+shift;i--) {
			fil->sxyz[i][0]=fil->sxyz[i-shift][0];
			fil->sxyz[i][1]=fil->sxyz[i-shift][1];
			fil->sxyz[i][2]=fil->sxyz[i-shift][2];
			fil->slen[i]=fil->slen[i-shift];
			fil->sypr[i][0]=fil->sypr[i-shift][0];
			fil->sypr[i][1]=fil->sypr[i-shift][1];
			fil->sypr[i][2]=fil->sypr[i-shift][2];
			Sph_Dcm2Dcm(fil->sdcm[i-shift],fil->sdcm[i]);
			Sph_Dcm2Dcm(fil->sadcm[i-shift],fil->sadcm[i]);
			fil->sthk[i]=fil->sthk[i-shift]; }}
	else if(shift<0) {
		for(i=fil->front+shift;i<fil->back+shift;i++) {
			fil->sxyz[i][0]=fil->sxyz[i-shift][0];
			fil->sxyz[i][1]=fil->sxyz[i-shift][1];
			fil->sxyz[i][2]=fil->sxyz[i-shift][2];
			fil->slen[i]=fil->slen[i-shift];
			fil->sypr[i][0]=fil->sypr[i-shift][0];
			fil->sypr[i][1]=fil->sypr[i-shift][1];
			fil->sypr[i][2]=fil->sypr[i-shift][2];
			Sph_Dcm2Dcm(fil->sdcm[i-shift],fil->sdcm[i]);
			Sph_Dcm2Dcm(fil->sadcm[i-shift],fil->sadcm[i]);
			fil->sthk[i]=fil->sthk[i-shift]; }
		fil->sxyz[i][0]=fil->sxyz[i-shift][0];
		fil->sxyz[i][1]=fil->sxyz[i-shift][1];
		fil->sxyz[i][2]=fil->sxyz[i-shift][2]; }
	fil->front+=shift;
	fil->back+=shift;
	return; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/


/* filalloc */
filamentptr filalloc(int maxseg) {
	int i;
	filamentptr fil;
	
	fil=NULL;
	CHECKMEM(fil=(filamentptr) malloc(sizeof(struct filamentstruct)));
	fil->filss=NULL;
	fil->fname=NULL;
	fil->color[0]=fil->color[1]=fil->color[2]=0;
	fil->color[3]=1;
	fil->edgepts=1;
	fil->edgestipple[0]=1;
	fil->edgestipple[1]=0xFFFF;
	fil->drawmode=DMedge;
	fil->shiny=0;
	fil->maxseg=maxseg;
	fil->nseg=0;
	fil->front=0;
	fil->back=0;
	fil->sxyz=NULL;
	fil->slen=NULL;
	fil->sypr=NULL;
	fil->sdcm=NULL;
	fil->sadcm=NULL;
	fil->sthk=NULL;
	fil->stdlen=1.0;
	fil->stdypr[0]=0;
	fil->stdypr[1]=0;
	fil->stdypr[2]=0;
	fil->klen=1;
	fil->kypr[0]=1;
	fil->kypr[1]=1;
	fil->kypr[2]=1;
	fil->kT=0;
	fil->treadrate=0;

	CHECKMEM(fil->sxyz=(double**)calloc(maxseg+1,sizeof(double*)));
	for(i=0;i<=maxseg;i++) fil->sxyz[i]=NULL;
	for(i=0;i<=maxseg;i++) CHECK(fil->sxyz[i]=(double*)calloc(3,sizeof(double)));
	for(i=0;i<=maxseg;i++) fil->sxyz[i][0]=fil->sxyz[i][1]=fil->sxyz[i][2]=0;
	
	CHECKMEM(fil->slen=(double*)calloc(maxseg,sizeof(double)));
	for(i=0;i<maxseg;i++) fil->slen[i]=1;
	
	CHECKMEM(fil->sypr=(double**)calloc(maxseg,sizeof(double*)));
	for(i=0;i<maxseg;i++) fil->sypr[i]=NULL;
	for(i=0;i<maxseg;i++) CHECK(fil->sypr[i]=(double*)calloc(3,sizeof(double)));
	for(i=0;i<maxseg;i++) fil->sypr[i][0]=fil->sypr[i][1]=fil->sypr[i][2]=0;
	
	CHECKMEM(fil->sdcm=(double**)calloc(maxseg,sizeof(double*)));
	for(i=0;i<maxseg;i++) fil->sdcm[i]=NULL;
	for(i=0;i<maxseg;i++) CHECK(fil->sdcm[i]=(double*)calloc(9,sizeof(double)));
	for(i=0;i<maxseg;i++) Sph_One2Dcm(fil->sdcm[i]);
	
	CHECKMEM(fil->sadcm=(double**)calloc(maxseg,sizeof(double*)));
	for(i=0;i<maxseg;i++) fil->sadcm[i]=NULL;
	for(i=0;i<maxseg;i++) CHECK(fil->sadcm[i]=(double*)calloc(9,sizeof(double)));
	for(i=0;i<maxseg;i++) Sph_One2Dcm(fil->sadcm[i]);
	
	CHECKMEM(fil->sthk=(double*)calloc(maxseg,sizeof(double)));
	for(i=0;i<maxseg;i++) fil->sthk[i]=1;
	
	return fil;
	
failure:
	if(fil) filfree(fil);
	return NULL; }


/* filfree */
void filfree(filamentptr fil) {
	int i,maxseg;
	
	if(!fil) return;
	maxseg=fil->maxseg;
	if(fil->sthk) free(fil->sthk);
	if(fil->sadcm) {
		for(i=0;i<maxseg;i++) if(fil->sadcm[i]) free(fil->sadcm[i]);
		free(fil->sadcm); }
	if(fil->sdcm) {
		for(i=0;i<maxseg;i++) if(fil->sdcm[i]) free(fil->sdcm[i]);
		free(fil->sdcm); }
	if(fil->sypr) {
		for(i=0;i<maxseg;i++) if(fil->sypr[i]) free(fil->sypr[i]);
		free(fil->sypr); }
	if(fil->slen) free(fil->slen);
	if(fil->sxyz) {
		for(i=0;i<maxseg+1;i++) if(fil->sxyz[i]) free(fil->sxyz[i]);
		free(fil->sxyz); }
	free(fil);
	return; }


/* filssalloc */
filamentssptr filssalloc(filamentssptr filss,int maxfil) {
	int f;
	char **newnames;
	filamentptr *newfillist;
	
	if(maxfil<1) return NULL;
	
	newnames=NULL;
	newfillist=NULL;
	
	if(!filss) {																			// new allocation
		filss=(filamentssptr) malloc(sizeof(struct filamentsuperstruct));
		CHECKMEM(filss);
		filss->condition=SCinit;
		filss->sim=NULL;
		filss->maxfil=0;
		filss->nfil=0;
		filss->fnames=NULL;
		filss->fillist=NULL; }
	else {																						// minor check
		if(maxfil<filss->maxfil) return NULL; }
	
	if(maxfil>filss->maxfil) {											// allocate new fil names and fils
		CHECKMEM(newnames=(char**) calloc(maxfil,sizeof(char*)));
		for(f=0;f<maxfil;f++) newnames[f]=NULL;
		for(f=0;f<filss->maxfil;f++) newnames[f]=filss->fnames[f];
		for(;f<maxfil;f++)
			CHECKMEM(newnames[f]=EmptyString());
		
		CHECKMEM(newfillist=(filamentptr*) calloc(maxfil,sizeof(filamentptr)));	// fil list
		for(f=0;f<maxfil;f++) newfillist[f]=NULL;
		for(f=0;f<filss->maxfil;f++) newfillist[f]=filss->fillist[f];
		for(;f<maxfil;f++) {
			CHECKMEM(newfillist[f]=filalloc(100));		//?? fixed number of segments for now at 100
			newfillist[f]->filss=filss;
			newfillist[f]->fname=newnames[f]; }}
	
	filss->maxfil=maxfil;
	free(filss->fnames);
	filss->fnames=newnames;
	free(filss->fillist);
	filss->fillist=newfillist;
	
	return filss;
	
failure:
	filssfree(filss);
	simLog(NULL,10,"Unable to allocate memory in filssalloc");
	return NULL; }


/* filssfree */
void filssfree(filamentssptr filss) {
	int f;

	if(!filss) return;
	if(filss->maxfil && filss->fillist)
	for(f=0;f<filss->maxfil;f++) filfree(filss->fillist[f]);
	free(filss->fillist);
	if(filss->maxfil && filss->fnames)
	for(f=0;f<filss->maxfil;f++) free(filss->fnames[f]);
	free(filss->fnames);
	free(filss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* filoutput */
void filoutput(filamentptr fil) {
	int i;
	char string[STRCHAR];
	
	simLog(NULL,2," Filament: %s\n",fil->fname);
	if(!fil) {
		simLog(NULL,2," NULL pointer\n");
		return; }
	simLog(NULL,2," allocated size: %i, segments: %i\n",fil->maxseg,fil->nseg);
	simLog(NULL,2," standard length: %g\n",fil->stdlen);
	simLog(NULL,2," standard angles: %g, %g, %g\n",fil->stdypr[0],fil->stdypr[1],fil->stdypr[2]);
	simLog(NULL,2," length force constant: %g\n",fil->klen);
	simLog(NULL,2," bending force constants: %g, %g, %g\n",fil->kypr[0],fil->kypr[1],fil->kypr[2]);
	simLog(NULL,2," kT: %g\n",fil->kT);
	simLog(NULL,2," stretching energy: %g\n",filStretchEnergy(fil,-1,-1));
	simLog(NULL,2," bending energy: %g\n",filBendEnergy(fil,-1,-1));
	if(fil->treadrate>0)
		simLog(NULL,2," treadmilling rate: %g\n",fil->treadrate);

	simLog(NULL,2," color: %g %g %g %g\n",fil->color[0],fil->color[1],fil->color[2],fil->color[3]);
	simLog(NULL,2," edge points: %g, polygon mode: %s\n",fil->edgepts,surfdm2string(fil->drawmode,string));
	if(fil->edgestipple[1]!=0xFFFF) simLog(NULL,2,"  edge stippling: %ui %X\n",fil->edgestipple[0],fil->edgestipple[1]);
	if(fil->shiny!=0) simLog(NULL,2," shininess: %g\n",fil->shiny);

	simLog(NULL,2," segment, thickness, length, angle, positions\n");
	for(i=fil->front;i<fil->back;i++)
		simLog(NULL,2," %i w=%1.3f l=%1.3f x=(%1.3f %1.3f %1.3f) a=(%1.3f %1.3f %1.3f)\n",i-fil->front,fil->sthk[i],fil->slen[i],fil->sxyz[i][0],fil->sxyz[i][1],fil->sxyz[i][2],fil->sypr[i][0],fil->sypr[i][1],fil->sypr[i][2]);
	simLog(NULL,2,"    end position=(%1.3f %1.3f %1.3f)\n",fil->sxyz[i][0],fil->sxyz[i][1],fil->sxyz[i][2]);
	return; }


/* filssoutput */
void filssoutput(simptr sim) {
	filamentssptr filss;
	filamentptr fil;
	int f;
	
	filss=sim->filss;
	if(!filss) return;
	simLog(sim,2,"FILAMENT PARAMETERS\n");
	simLog(sim,2," Filaments allocated: %i, filaments defined: %i\n",filss->maxfil,filss->nfil);
	for(f=0;f<filss->nfil;f++) {
		fil=filss->fillist[f];
		filoutput(fil); }
	simLog(sim,2,"\n");
	return; }


/* filwrite */
void filwrite(simptr sim,FILE *fptr) {
	filamentssptr filss;
	filamentptr fil;
	int f;
	
	filss=sim->filss;
	if(!filss) return;
	fprintf(fptr,"# filament parameters\n");
	fprintf(fptr,"max_filament %i\n",filss->maxfil);
	for(f=0;f<filss->nfil;f++) {
		fil=filss->fillist[f];
		fprintf(fptr,"start_filament %s\n",fil->fname);
		//TODO: write filwrite
		fprintf(fptr,"end_filament\n\n"); }
	return; }


/* filcheckparams */
int filcheckparams(simptr sim,int *warnptr) {
	int error,warn;
	filamentssptr filss;
//	filamentptr fil;
	char string[STRCHAR];
	
	error=warn=0;
	filss=sim->filss;
	if(!filss) {
		if(warnptr) *warnptr=warn;
		return 0; }
	
	if(filss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: filament structure %s\n",simsc2string(filss->condition,string)); }
	
//	for(f=0;f<filss->nfil;f++) {
//		fil=filss->fillist[f];
		//TODO: write checkparams
//	}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/****************************** filament manipulation *************************/
/******************************************************************************/


/* filAddSegment */
int filAddSegment(filamentptr fil,double *x,double length,double *angle,double thickness,char endchar) {
	int seg,nseg;
	
	nseg=fil->nseg;
	if(nseg==fil->maxseg) return -1;								//?? should automatically lengthen
	
	if(endchar=='b') {
		if(fil->back==fil->maxseg) filArrayShift(fil,0);
		seg=fil->back++;
		fil->nseg++;
		fil->slen[seg]=length;
		fil->sthk[seg]=thickness;
		Sph_Xyz2Xyz(angle,fil->sypr[seg]);												// a[seg] = angle
		Sph_Xyz2Dcm(angle,fil->sdcm[seg]);												// A[seg] = Dcm(angle)
		if(nseg==0) {
			fil->sxyz[seg][0]=x[0];
			fil->sxyz[seg][1]=x[1];
			fil->sxyz[seg][2]=x[2];
			Sph_Dcm2Dcm(fil->sdcm[seg],fil->sadcm[seg]);	}								// B[0] = A[0]
		else
			Sph_DcmxDcm(fil->sdcm[seg],fil->sadcm[seg-1],fil->sadcm[seg]);		// B[seg] = A[seg].B[seg-1]
		Sph_DcmtxUnit(fil->sadcm[seg],'x',fil->sxyz[seg+1],fil->sxyz[seg],fil->slen[seg]); }
	else {
		if(fil->front==0) filArrayShift(fil,0);
		if(fil->front==0) filArrayShift(fil,1);								// used if nseg=maxseg-1
		seg=--fil->front;
		fil->nseg++;
		fil->slen[seg]=length;
		fil->sthk[seg]=thickness;
		if(nseg==0) {
			Sph_Xyz2Dcmt(angle,fil->sadcm[seg]);											// B[0] = Dcmt(angle)
			fil->sxyz[seg+1][0]=x[0];
			fil->sxyz[seg+1][1]=x[1];
			fil->sxyz[seg+1][2]=x[2]; }
		else {
			Sph_Xyz2Xyz(angle,fil->sypr[seg+1]);										// a[i+1] = angle
			Sph_Xyz2Dcm(angle,fil->sdcm[seg+1]);										// A[seg+1] = Dcm(angle)
			Sph_DcmtxDcm(fil->sdcm[seg+1],fil->sadcm[seg+1],fil->sadcm[seg]); }	// B[seg] = AT[seg+1].B[seg+1]
		Sph_Dcm2Dcm(fil->sadcm[seg],fil->sdcm[seg]);										// A[seg] = B[seg]
		Sph_Dcm2Xyz(fil->sdcm[seg],fil->sypr[seg]);										// a[seg] = Xyz(B[0])
		Sph_DcmtxUnit(fil->sadcm[seg],'x',fil->sxyz[seg],fil->sxyz[seg+1],-fil->slen[seg]); }
	return 0; }


/* filRemoveSegment */
int filRemoveSegment(filamentptr fil,char endchar) {
	int seg;
	
	if(fil->nseg==0) return -1;
	
	if(endchar=='b') {
		fil->nseg--;
		fil->back--; }
	else {
		seg=++fil->front;
		fil->nseg--;
		Sph_Dcm2Dcm(fil->sadcm[seg],fil->sdcm[seg]);
		Sph_Dcm2Xyz(fil->sdcm[seg],fil->sypr[seg]); }
	return 0; }


/* filTranslate */
void filTranslate(filamentptr fil,const double *vect,char func) {
	int seg;
	double shift[3];

	seg=fil->front;
	if(func=='=') {
		shift[0]=fil->sxyz[seg][0]-vect[0];
		shift[1]=fil->sxyz[seg][1]-vect[1];
		shift[2]=fil->sxyz[seg][2]-vect[2]; }
	else if(func=='-') {
		shift[0]=-vect[0];
		shift[1]=-vect[1];
		shift[2]=-vect[2]; }
	else {
		shift[0]=vect[0];
		shift[1]=vect[1];
		shift[2]=vect[2]; }

	for(seg=fil->front;seg<=fil->back;seg++) {
		fil->sxyz[seg][0]+=shift[0];
		fil->sxyz[seg][1]+=shift[1];
		fil->sxyz[seg][2]+=shift[2]; }

	return; }
	

/* filRotateVertex */
void filRotateVertex(filamentptr fil,int seg,double *angle,char endchar,char func) {

	return; }


/* filLengthenSegment */
void filLengthenSegment(filamentptr fil,int seg,double length,char endchar,char func) {

	return; }


/* filReverseFilament */
void filReverseFilament(filamentptr fil) {

	return; }


/* filCopyFilament */
int filCopyFilament(filamentptr filfrom,filamentptr filto,const char *fname) {
	int i,er;

	filto->nseg=0;
	filto->back=filto->front;

	filto->filss=filfrom->filss;
	if(fname) strncpy(filto->fname,fname,STRCHAR);
	filto->color[0]=filfrom->color[0];
	filto->color[1]=filfrom->color[1];
	filto->color[2]=filfrom->color[2];
	filto->color[3]=filfrom->color[3];
	filto->edgepts=filfrom->edgepts;
	filto->edgestipple[0]=filfrom->edgestipple[0];
	filto->edgestipple[1]=filfrom->edgestipple[1];
	filto->drawmode=filfrom->drawmode;
	filto->shiny=filfrom->shiny;
	filto->stdlen=filfrom->stdlen;
	filto->stdypr[0]=filfrom->stdypr[0];
	filto->stdypr[1]=filfrom->stdypr[1];
	filto->stdypr[2]=filfrom->stdypr[2];
	filto->klen=filfrom->klen;
	filto->kypr[0]=filfrom->kypr[0];
	filto->kypr[1]=filfrom->kypr[1];
	filto->kypr[2]=filfrom->kypr[2];
	filto->kT=filfrom->kT;
	filto->treadrate=filfrom->treadrate;
	for(i=filfrom->front;i<filfrom->back;i++) {
		er=filAddSegment(filto,filfrom->sxyz[i],filfrom->slen[i],filfrom->sypr[i],filfrom->sthk[i],'b');
		if(er) return 1; }
	return 0; }


/******************************************************************************/
/******************************** structure set up ****************************/
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


/* filSetParam */
void filSetParam(filamentptr fil,const char *param,int index,double value) {
	if(!strcmp(param,"stdlen")) fil->stdlen=value;
	else if(!strcmp(param,"stdypr")) {
		if(index<0) fil->stdypr[0]=fil->stdypr[1]=fil->stdypr[2]=value;
		else fil->stdypr[index]=value; }
	else if(!strcmp(param,"klen")) fil->klen=value;
	else if(!strcmp(param,"kypr")) {
		if(index<0) fil->kypr[0]=fil->kypr[1]=fil->kypr[2]=value;
		else fil->kypr[index]=value; }
	else if(!strcmp(param,"kT")) fil->kT=value;
	else if(!strcmp(param,"treadrate")) fil->treadrate=value;
	return; }


/* filsetcolor */
int filsetcolor(filamentptr fil,double *rgba) {
	int col;
	
	if(!fil) return 1;
	for(col=0;col<4;col++)
		if(rgba[col]<0 || rgba[col]>1) return 2;
	
	for(col=0;col<4;col++) fil->color[col]=rgba[col];
	return 0; }


/* filsetedgepts */
int filsetedgepts(filamentptr fil,double value) {
	if(!fil) return 1;
	if(value<0) return 2;
	fil->edgepts=value;
	return 0; }


/* filsetstipple */
int filsetstipple(filamentptr fil,int factor,int pattern) {
	if(!fil) return 1;
	if(factor>=0) {
		if(factor==0) return 2;
		fil->edgestipple[0]=(unsigned int) factor; }
	if(pattern>=0) {
		if(pattern>0xFFFF) return 2;
		fil->edgestipple[1]=(unsigned int) pattern; }
	return 0; }


/* filsetdrawmode */
int filsetdrawmode(filamentptr fil,enum DrawMode dm) {
	if(!fil) return 1;
	if(dm==DMnone) return 2;
	fil->drawmode=dm;
	return 0; }


/* filsetshiny */
int filsetshiny(filamentptr fil,double shiny) {
	if(!fil) return 1;
	if(shiny<0 || shiny>128) return 2;
	fil->shiny=shiny;
	return 0; }





/* filenablefilaments */
int filenablefilaments(simptr sim,int maxfil) {
	filamentssptr filss;

	if(sim->filss)									// check for redundant function call
		if(maxfil==-1 || sim->filss->maxfil==maxfil)
			return 0;
	filss=filssalloc(sim->filss,maxfil<0?5:maxfil);
	if(!filss) return 1;
	sim->filss=filss;
	filss->sim=sim;
	filsetcondition(sim->filss,SClists,0);
	return 0; }


/* filaddfilament */
filamentptr filaddfilament(simptr sim,const char *fnames) {
	int er,f;
	filamentssptr filss;
	filamentptr fil;
	
	if(!sim->filss) {
		er=filenablefilaments(sim,-1);
		if(er) return NULL; }
	filss=sim->filss;
	
	f=stringfind(filss->fnames,filss->nfil,fnames);
	if(f<0) {
		if(filss->nfil==filss->maxfil) {
			er=filenablefilaments(sim,filss->nfil*2+1);
			if(er) return NULL; }
		f=filss->nfil++;
		strncpy(filss->fnames[f],fnames,STRCHAR-1);
		filss->fnames[f][STRCHAR-1]='\0';
		fil=filss->fillist[f]; }
	else
		fil=filss->fillist[f];

	filsetcondition(filss,SClists,0);
	return fil; }


/* filAddRandomSegments */
int filAddRandomSegments(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr,double thickness) {
	int i;
	double f1,pos[3],angle[3];
	char **varnames;
	double *varvalues;
	int nvar;

	varnames=fil->filss->sim->varnames;
	varvalues=fil->filss->sim->varvalues;
	nvar=fil->filss->sim->nvar;

	if(fil->nseg+number>fil->maxseg-1) return -1;

	if(fil->nseg==0) {
		systemrandpos(fil->filss->sim,pos);
		if(!strcmp(xstr,"u"));
		else if(strmathsscanf(xstr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[0]=f1;
		else return 2;
		if(!strcmp(ystr,"u"));
		else if(strmathsscanf(ystr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[1]=f1;
		else return 2;
		if(!strcmp(zstr,"u"));
		else if(strmathsscanf(zstr,"%mlg",varnames,varvalues,nvar,&f1)==1) pos[2]=f1;
		else return 2; }
	else
		pos[0]=pos[1]=pos[2]=0;

	for(i=0;i<number;i++) {
		filAddSegment(fil,pos,filRandomLength(fil,thickness,1),filRandomAngle(fil,thickness,angle,1),thickness,i==0?'1':'b'); }
	return 0; }


/* filreadstring */
filamentptr filreadstring(simptr sim,ParseFilePtr pfp,filamentptr fil,const char *word,char *line2) {
	char **varnames;
	double *varvalues;
	int nvar;

	int itct,er,i1,i2;
	char nm[STRCHAR],nm1[STRCHAR],endchar,str1[STRCHAR],str2[STRCHAR],str3[STRCHAR],symbol;
	double fltv1[9],f1,length,angle[3],thick;
	enum DrawMode dm;
	filamentptr fil2;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	if(!strcmp(word,"name")) {								// name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading filament name");
		fil=filaddfilament(sim,nm);
		CHECKS(fil,"failed to add filament");
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"color") || !strcmp(word,"colour")) {		// color
		CHECKS(fil,"need to enter filament name before color");
		er=graphicsreadcolor(&line2,fltv1);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=filsetcolor(fil,fltv1);
		CHECKS(!er,"BUG: error in filsetcolor");
		CHECKS(!line2,"unexpected text following color"); }
	
	else if(!strcmp(word,"thickness")) {				// thickness
		CHECKS(fil,"need to enter filament name before thickness");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"thickness value is missing");
		CHECKS(f1>=0,"thickness value needs to be at least 0");
		er=filsetedgepts(fil,f1);
		CHECKS(!er,"BUG: error in filsetedgepts");
		CHECKS(!strnword(line2,2),"unexpected text following thickness"); }
	
	else if(!strcmp(word,"stipple")) {					// stipple
		CHECKS(fil,"need to enter filament name before stipple");
		itct=strmathsscanf(line2,"%mi %mi",varnames,varvalues,nvar,&i1,&i2);
		CHECKS(itct==2,"stipple format: factor pattern");
		CHECKS(i1>=1,"stipple factor needs to be >=1");
		CHECKS(i2>=0 && i2 <=0xFFFF,"stipple pattern needs to be between 0x00 and 0xFFFF");
		er=filsetstipple(fil,i1,i2);
		CHECKS(!er,"BUG: error in filsetstipple");
		CHECKS(!strnword(line2,3),"unexpected text following stipple"); }
	
	else if(!strcmp(word,"polygon")) {					// polygon
		CHECKS(fil,"need to enter filament name before polygon");
		itct=sscanf(line2,"%s",nm1);
		CHECKS(itct==1,"polygon format: drawmode");
		dm=surfstring2dm(nm1);
		CHECKS(dm!=DMnone,"in polygon, drawing mode is not recognized");
		er=filsetdrawmode(fil,dm);
		CHECKS(!er,"BUG: error in filsetdrawmode");
		CHECKS(!strnword(line2,2),"unexpected text following polygon"); }

	else if(!strcmp(word,"shininess")) {				// shininess
		CHECKS(fil,"need to enter filament name before shininess");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"shininess format: value");
		CHECKS(f1>=0 && f1<=128,"shininess value needs to be between 0 and 128");
		er=filsetshiny(fil,f1);
		CHECKS(!er,"BUG: error in filsetshiny");
		CHECKS(!strnword(line2,2),"unexpected text following shininess"); }

	// max_segments value

	else if(!strcmp(word,"kT")) {								// kT
		CHECKS(fil,"need to enter filament name before kT");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"kT format: value");
		CHECKS(f1>0,"kT value needs to be greater than 0");
		filSetParam(fil,"kT",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following kT"); }
	
	else if(!strcmp(word,"treadmill_rate")) {		// treadmill_rate
		CHECKS(fil,"need to enter filament name before treadmill_rate");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"treadmill_rate format: value");
		filSetParam(fil,"treadrate",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following treadrate"); }
	
	else if(!strcmp(word,"standard_length")) {	// standard_length
		CHECKS(fil,"need to enter filament name before standard_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"standard_length format: value");
		CHECKS(f1>0,"standard_length value needs to be greater than 0");
		filSetParam(fil,"stdlen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following standard_length"); }
	
	else if(!strcmp(word,"standard_angle")) {	// standard_angle
		CHECKS(fil,"need to enter filament name before standard_angle");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
		CHECKS(itct==3,"standard_angle format: yaw pitch roll");
		filSetParam(fil,"stdypr",0,fltv1[0]);
		filSetParam(fil,"stdypr",1,fltv1[1]);
		filSetParam(fil,"stdypr",2,fltv1[2]);
		CHECKS(!strnword(line2,4),"unexpected text following standard_angle"); }
	
	else if(!strcmp(word,"force_length")) {		// standard_length
		CHECKS(fil,"need to enter filament name before force_length");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&f1);
		CHECKS(itct==1,"force_length format: value");
		filSetParam(fil,"klen",0,f1);
		CHECKS(!strnword(line2,2),"unexpected text following force_length"); }
	
	else if(!strcmp(word,"force_angle")) {		// force_angle
		CHECKS(fil,"need to enter filament name before force_angle");
		itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&fltv1[0],&fltv1[1],&fltv1[2]);
		CHECKS(itct==3,"force_angle format: yaw pitch roll");
		filSetParam(fil,"kypr",0,fltv1[0]);
		filSetParam(fil,"kypr",1,fltv1[1]);
		filSetParam(fil,"kypr",2,fltv1[2]);
		CHECKS(!strnword(line2,4),"unexpected text following force_angle"); }
	
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
		CHECKS(fil,"need to enter filament name before add_segment");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"random_segments format: number [x y z] [thickness]");
		CHECKS(i1>0,"number needs to be >0");
		line2=strnword(line2,2);
		if(fil->nseg==0) {
			CHECKS(line2,"missing x y z position information");
			itct=sscanf(line2,"%s %s %s",str1,str2,str3);
			CHECKS(itct==3,"random_segments format: number [x y z] [thickness]");
			line2=strnword(line2,4); }
		else {
			snprintf(str1,STRCHAR,"%i",0);
			snprintf(str2,STRCHAR,"%i",0);
			snprintf(str3,STRCHAR,"%i",0); }
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
		fil2=filaddfilament(sim,nm);
		CHECKS(fil2,"failed to add filament");
		er=filCopyFilament(fil,fil2,NULL);
		CHECKS(!strnword(line2,2),"unexpected text following copy"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within fil block: statement not recognized"); }
	
	return fil;
	
failure:
	simParseError(sim,pfp);
	return NULL; }


/* filload */
int filload(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	filamentptr fil;

	pfp=*pfpptr;
	done=0;
	fil=NULL;
	firstline2=line2?1:0;

	while(!done)
		if(pfp->lctr==0) {
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
			fil=filreadstring(sim,pfp,fil,word,line2);
			CHECK(fil); }}															// failed but error has already been sent

	CHECKS(0,"end of file encountered before end_filament statement");	// end of file

	failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* filupdateparams */
int filupdateparams(simptr sim) {
	return 0; }

/* filupdatelists */
int filupdatelists(simptr sim) {
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


int filSegmentXSurface(simptr sim,filamentptr fil,char endchar) {
	int s,p,mxs;
	surfaceptr srf;
	panelptr pnl;
	double *pt1,*pt2,crosspt[3];
	enum PanelShape ps;

	if(!sim->srfss) return 0;
	if(endchar=='f') {
		pt1=fil->sxyz[fil->front];
		pt2=fil->sxyz[fil->front+1]; }
	else {
		pt1=fil->sxyz[fil->back-1];
		pt2=fil->sxyz[fil->back]; }

	mxs=0;
	for(s=0;s<sim->srfss->nsrf && !mxs;s++) {
		srf=sim->srfss->srflist[s];
		for(ps=(enum PanelShape)0;ps<PSMAX && !mxs;ps=(enum PanelShape)(ps+1))
			for(p=0;p<srf->npanel[ps] && !mxs;p++) {
				pnl=srf->panels[ps][p];
				mxs=lineXpanel(pt1,pt2,pnl,3,crosspt,NULL,NULL,NULL,NULL,NULL,0); }}
//				printf("pt1=(%g %g %g), pt2=(%g %g %g), mxs=%i\n",pt1[0],pt1[1],pt1[2],pt2[0],pt2[1],pt2[2],mxs); }}

	return mxs; }


int filSegmentXFilament(simptr sim,filamentptr fil,char endchar,filamentptr *filptr) {
	int f,i,mxf,mn,mn1;
	double thk,*pt1,*pt2,dist;
	filamentssptr filss;
	filamentptr fil2;

	if(endchar=='f') {
		pt1=fil->sxyz[fil->front];
		pt2=fil->sxyz[fil->front+1];
		thk=fil->sthk[fil->front];
		mn=mn1=fil->front;
		if(fil->nseg>1) mn1=fil->front+1; }
	else {
		pt1=fil->sxyz[fil->back-1];
		pt2=fil->sxyz[fil->back];
		thk=fil->sthk[fil->back-1];
		mn=mn1=fil->back-1;
		if(fil->nseg>1) mn1=fil->back-2; }

	mxf=0;
	filss=sim->filss;
	fil2=NULL;
	for(f=0;f<filss->nfil && !mxf;f++) {
		fil2=filss->fillist[f];
		for(i=fil2->front;i<fil2->back && !mxf;i++) {
			if(!(fil2==fil && (i==mn || i==mn1))) {
				dist=Geo_NearestSeg2SegDist(pt1,pt2,fil2->sxyz[i],fil2->sxyz[i+1]);
				if(dist<thk+fil2->sthk[i]) mxf=1; }}}
	if(mxf && filptr)
		*filptr=fil2;
	return mxf; }


void filTreadmill(simptr sim,filamentptr fil,int steps) {
	int i,mxs,done,iter;
	double thk,length,angle[3],sigmamult,angletry[3],dcm[9];
	filamentptr fil2;

	for(i=0;i<steps;i++) {
		thk=fil->sthk[0];
		done=0;
		sigmamult=1;
		angletry[0]=angletry[1]=angletry[2]=0;
		for(iter=0;iter<500 && !done;iter++) {
			length=filRandomLength(fil,thk,sigmamult);
			filRandomAngle(fil,thk,angle,sigmamult);
			if(iter>0 && coinrandD(0.5))
				filAddSegment(fil,NULL,length,angletry,thk,'b');
			else
				filAddSegment(fil,NULL,length,angle,thk,'b');
			mxs=filSegmentXSurface(sim,fil,'b');
			if(!mxs) {
				mxs=filSegmentXFilament(sim,fil,'b',&fil2);
				if(mxs) {
					mxs=coinrandD(0.995);
					Sph_DcmxDcmt(fil2->sadcm[fil2->back-1],fil->sadcm[fil->back-2],dcm);
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
	int f;

	filss=sim->filss;
	if(!filss) return 0;
	for(f=0;f<filss->nfil;f++) {
		fil=filss->fillist[f];
		if(fil->treadrate>0)
			filTreadmill(sim,fil,poisrandD(fil->treadrate*sim->dt)); }
	return 0; }




