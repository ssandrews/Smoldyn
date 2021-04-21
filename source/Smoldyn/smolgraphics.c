/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Geometry.h"
#include "math2.h"
#include "opengl2.h"
#include "Rn.h"
#include "string2.h"
#include "../libSteve/SimCommand.h"

#ifdef OPTION_NSV
  #include "nsvc.h"
#endif

#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smoldynconfigure.h"

/******************************************************************************/
/*********************************** Graphics *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types
char *graphicslp2string(enum LightParam lp,char *string);

// low level utilities

// memory management
graphicsssptr graphssalloc(void);

// data structure output

// structure setup

// structure update functions
int graphicsupdateinit(simptr sim);
int graphicsupdatelists(simptr sim);
int graphicsupdateparams(simptr sim);

// core simulation functions
void RenderSurfaces(simptr sim);
void RenderMolecs(simptr sim);
void RenderText(simptr sim);
void RenderSim(simptr sim);

// top level OpenGL functions

/******************************************************************************/
/******************************* enumerated types *****************************/
/******************************************************************************/

/* graphicsstring2lp */
enum LightParam graphicsstring2lp(char *string) {
	enum LightParam ans;
	
	if(strbegin(string,"ambient",0)) ans=LPambient;
	else if(strbegin(string,"diffuse",0)) ans=LPdiffuse;
	else if(strbegin(string,"specular",0)) ans=LPspecular;
	else if(strbegin(string,"position",0)) ans=LPposition;
	else if(strbegin(string,"on",0)) ans=LPon;
	else if(strbegin(string,"off",0)) ans=LPoff;
	else if(strbegin(string,"auto",0)) ans=LPauto;
	else ans=LPnone;
	return ans; }


/* graphicslp2string */
char *graphicslp2string(enum LightParam lp,char *string) {
	if(lp==LPambient) strcpy(string,"ambient");
	else if(lp==LPdiffuse) strcpy(string,"diffuse");
	else if(lp==LPspecular) strcpy(string,"specular");
	else if(lp==LPposition) strcpy(string,"position");
	else if(lp==LPon) strcpy(string,"on");
	else if(lp==LPoff) strcpy(string,"off");
	else if(lp==LPauto) strcpy(string,"auto");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/

/* graphicsreadcolor */
int graphicsreadcolor(char **stringptr,double *rgba) {
	char *string,name[STRCHAR];
	double r,g,b,a;
	int itct,er;

	if(!stringptr) return 1;
	string=*stringptr;
	if(!string) return 1;
	itct=sscanf(string,"%s",name);
	if(itct!=1) return 1;

	er=0;
	r=g=b=0;
	a=1;
	if((name[0]>='0' && name[0]<='9') || name[0]=='.') {	// numbers
		itct=sscanf(string,"%lg %lg %lg",&r,&g,&b);
		if(itct!=3) er=2;
		else {
			if(r<0 || r>1 || g<0 ||g>1 || b<0 || b>1) er=3;
			string=strnword(string,4); }}
	else {																								// word
		if(!strcmp(name,"maroon"))			{r=0.5;g=0;b=0;}
		else if(!strcmp(name,"red"))		{r=1;g=0;b=0;}
		else if(!strcmp(name,"scarlet")) {r=1;g=0.14;b=0;}
		else if(!strcmp(name,"rose"))		{r=1;g=0;b=0.5;}
		else if(!strcmp(name,"brick"))	{r=0.7;g=0.13;b=0.13;}
		else if(!strcmp(name,"pink"))		{r=1;g=0.08;b=0.58;}
		else if(!strcmp(name,"brown"))	{r=0.54;g=0.27;b=0.07;}
		else if(!strcmp(name,"tan"))		{r=0.86;g=0.58;b=0.44;}
		else if(!strcmp(name,"sienna"))	{r=0.56;g=0.42;b=0.14;}
		else if(!strcmp(name,"orange")) {r=1;g=0.65;b=0;}
		else if(!strcmp(name,"salmon")) {r=0.91;g=0.59;b=0.48;}
		else if(!strcmp(name,"coral"))	{r=0.94;g=0.42;b=0.31;}
		else if(!strcmp(name,"yellow")) {r=1;g=1;b=0;}
		else if(!strcmp(name,"gold"))		{r=1;g=0.84;b=0;}
		else if(!strcmp(name,"olive"))	{r=0.5;g=0.5;b=0;}
		else if(!strcmp(name,"green"))	{r=0;g=0.5;b=0;}
		else if(!strcmp(name,"chartrouse")) {r=0.5;g=1;b=0;}
		else if(!strcmp(name,"khaki"))	{r=0.8;g=0.77;b=0.45;}
		else if(!strcmp(name,"purple")) {r=0.5;g=0;b=0.5;}
		else if(!strcmp(name,"magenta"))	{r=1;g=0;b=1;}
		else if(!strcmp(name,"fuchsia"))	{r=1;g=0;b=1;}
		else if(!strcmp(name,"lime"))		{r=0;g=1;b=0;}
		else if(!strcmp(name,"teal"))		{r=0;g=0.5;b=0.5;}
		else if(!strcmp(name,"aqua"))		{r=0;g=1;b=1;}
		else if(!strcmp(name,"cyan"))		{r=0;g=1;b=1;}
		else if(!strcmp(name,"blue"))		{r=0;g=0;b=1;}
		else if(!strcmp(name,"navy"))		{r=0;g=0;b=0.5;}
		else if(!strcmp(name,"turquoise")) {r=0.25;g=0.88;b=0.82;}
		else if(!strcmp(name,"royal"))	{r=0;g=0.14;b=0.4;}
		else if(!strcmp(name,"sky"))		{r=0.53;g=0.81;b=0.92;}
		else if(!strcmp(name,"aquamarine")) {r=0.5;g=1;b=0.83;}
		else if(!strcmp(name,"indigo"))	{r=0.29;g=0;b=0.51;}
		else if(!strcmp(name,"violet"))	{r=0.58;g=0;b=0.83;}
		else if(!strcmp(name,"mauve"))	{r=0.88;g=0.69;b=1;}
		else if(!strcmp(name,"orchid")) {r=0.73;g=0.33;b=0.83;}
		else if(!strcmp(name,"plum"))		{r=0.6;g=0.12;b=0.5;}
		else if(!strcmp(name,"azure"))	{r=0;g=0.5;b=1;}
		else if(!strcmp(name,"black"))	{r=0;g=0;b=0;}
		else if(!strcmp(name,"gray"))		{r=0.5;g=0.5;b=0.5;}
		else if(!strcmp(name,"grey"))		{r=0.5;g=0.5;b=0.5;}
		else if(!strcmp(name,"silver")) {r=0.75;g=0.75;b=0.75;}
		else if(!strcmp(name,"slate"))	{r=0.44;g=0.5;b=0.56;}
		else if(!strcmp(name,"white"))	{r=1;g=1;b=1;}

		else if(!strcmp(name,"darkred"))	{r=0.55;g=0;b=0;}
		else if(!strcmp(name,"darkorange"))	{r=1;g=0.55;b=0;}
		else if(!strcmp(name,"darkyellow"))	{r=1;g=0.84;b=0;}
		else if(!strcmp(name,"darkgreen"))	{r=0;g=0.39;b=0;}
		else if(!strcmp(name,"darkblue"))	{r=0;g=0;b=0.55;}
		else if(!strcmp(name,"darkviolet"))	{r=0.29;g=0;b=0.51;}
		else if(!strcmp(name,"darkgrey"))	{r=0.25;g=0.25;b=0.25;}
		else if(!strcmp(name,"darkgray"))	{r=0.25;g=0.25;b=0.25;}
		else if(!strcmp(name,"lightred"))	{r=1;g=0.39;b=0.28;}
		else if(!strcmp(name,"lightorange"))	{r=1;g=0.75;b=0.2;}
		else if(!strcmp(name,"lightyellow"))	{r=1;g=1;b=0.5;}
		else if(!strcmp(name,"lightgreen"))	{r=0.56;g=1;b=0.5;}
		else if(!strcmp(name,"lightblue"))	{r=0.53;g=0.81;b=0.98;}
		else if(!strcmp(name,"lightviolet"))	{r=0.68;g=0.1;b=1;}
		else if(!strcmp(name,"lightgrey"))	{r=0.75;g=0.75;b=0.75;}
		else if(!strcmp(name,"lightgray"))	{r=0.75;g=0.75;b=0.75;}

		else er=4;
		string=strnword(string,2); }

	if(string) {
		itct=sscanf(string,"%lg",&a);
		if(itct!=1) er=5;
		if(a<0 || a>1) er=6;
		string=strnword(string,2); }

	if(rgba) {
		rgba[0]=r;
		rgba[1]=g;
		rgba[2]=b;
		rgba[3]=a; }
	*stringptr=string;
	return er; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* graphssalloc */
graphicsssptr graphssalloc(void) {
	graphicsssptr graphss;
	int lt;

	graphss=NULL;
	CHECKMEM(graphss=(graphicsssptr) malloc(sizeof(struct graphicssuperstruct)));

	graphss->condition=SCinit;
	graphss->sim=NULL;
	graphss->graphics=0;
	graphss->currentit=0;
	graphss->graphicit=20;
	graphss->graphicdelay=0;
	graphss->tiffit=0;
	graphss->framepts=2;
	graphss->gridpts=0;

	graphss->framecolor[0]=0;		// black frame
	graphss->framecolor[1]=0;
	graphss->framecolor[2]=0;
	graphss->framecolor[3]=1;

	graphss->gridcolor[0]=0;		// black grid
	graphss->gridcolor[1]=0;
	graphss->gridcolor[2]=0;
	graphss->gridcolor[3]=1;

	graphss->backcolor[0]=1;		// white background
	graphss->backcolor[1]=1;
	graphss->backcolor[2]=1;
	graphss->backcolor[3]=1;
	
	graphss->textcolor[0]=0;		// black text
	graphss->textcolor[1]=0;
	graphss->textcolor[2]=0;
	graphss->textcolor[3]=0;

	graphss->maxtextitems=0;
	graphss->ntextitems=0;
	graphss->textitems=NULL;
	
	graphicssetlight(NULL,graphss,-1,LPauto,NULL);
	for(lt=0;lt<MAXLIGHTS;lt++) graphicssetlight(NULL,graphss,lt,LPauto,NULL);

	return graphss;
	
failure:
	graphssfree(graphss);
	simLog(NULL,10,"Failed to allocate memory in graphssalloc");
	return NULL; }


/* graphssfree */
void graphssfree(graphicsssptr graphss) {
	int item;

	if(!graphss) return;
	for(item=0;item<graphss->maxtextitems;item++) free(graphss->textitems[item]);
	free(graphss->textitems);
	free(graphss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* graphssoutput */
void graphssoutput(simptr sim) {
	graphicsssptr graphss;
	char string1[STRCHAR],string2[STRCHAR];
	int i1,i2,lt;

	graphss=sim->graphss;
	simLog(sim,2,"GRAPHICS PARAMETERS\n");
	if(!graphss || graphss->graphics==0) {
		simLog(sim,2," No graphical output\n\n");
		return; }

	simLog(sim,2," display: ");
	if(graphss->graphics==1) simLog(sim,2,"OpenGL");
	else if(graphss->graphics==2) simLog(sim,2,"OpenGL_good");
	else if(graphss->graphics==3) simLog(sim,2,"OpenGL_better");
	simLog(sim,2,", every %i iterations\n",graphss->graphicit);
	if(graphss->graphicdelay>0) simLog(sim,2," delay per frame: %ui ms\n",graphss->graphicdelay);

	simLog(sim,2," frame thickness: %g",graphss->framepts);
	if(graphss->gridpts) simLog(sim,2,", grid thickness: %g",graphss->gridpts);
	simLog(sim,2,"\n");
	if(graphss->framepts) simLog(sim,2," frame color: %g,%g,%g,%g\n",graphss->framecolor[0],graphss->framecolor[1],graphss->framecolor[2],graphss->framecolor[3]);
	if(graphss->gridpts) simLog(sim,2," grid color: %g,%g,%g,%g\n",graphss->gridcolor[0],graphss->gridcolor[1],graphss->gridcolor[2],graphss->gridcolor[3]);
	simLog(sim,2," background color: %g,%g,%g,%g\n",graphss->backcolor[0],graphss->backcolor[1],graphss->backcolor[2],graphss->backcolor[3]);

	if(graphss->ntextitems) {
		simLog(sim,2," text color: %g,%g,%g,%g\n",graphss->textcolor[0],graphss->textcolor[1],graphss->textcolor[2],graphss->textcolor[3]);
		simLog(sim,2," text items:");
		for(i1=0;i1<graphss->ntextitems;i1++)
			simLog(sim,2," %s",graphss->textitems[i1]);
		simLog(sim,2,"\n"); }

	if(graphss->graphics>=3)
		simLog(sim,2," ambient light (%s): %g %g %g %g\n",graphicslp2string(graphss->roomstate,string1),graphss->ambiroom[0],graphss->ambiroom[1],graphss->ambiroom[2],graphss->ambiroom[3]);
	for(lt=0;lt<MAXLIGHTS;lt++)
		if(graphss->lightstate[lt]!=LPauto) {
			simLog(sim,2," light %i: %s\n",lt,graphicslp2string(graphss->lightstate[lt],string1));
			simLog(sim,2,"  position: %g %g %g %g\n",graphss->lightpos[lt][0],graphss->lightpos[lt][1],graphss->lightpos[lt][2],graphss->lightpos[lt][3]);
			simLog(sim,2,"  ambient: %g %g %g %g\n",graphss->ambilight[lt][0],graphss->ambilight[lt][1],graphss->ambilight[lt][2],graphss->ambilight[lt][3]);
			simLog(sim,2,"  diffuse: %g %g %g %g\n",graphss->difflight[lt][0],graphss->difflight[lt][1],graphss->difflight[lt][2],graphss->difflight[lt][3]);
			simLog(sim,2,"  specular: %g %g %g %g\n",graphss->speclight[lt][0],graphss->speclight[lt][1],graphss->speclight[lt][2],graphss->speclight[lt][3]); }

	gl2GetString("TiffName",string1);
	gl2GetString("TiffNameDefault",string2);
	i1=(int)gl2GetNumber("TiffNumber");
	i2=(int)gl2GetNumber("TiffNumMax");
	if(strcmp(string1,string2)) simLog(sim,2," TIFF name: %s\n",string1);
	if(i1!=(int)gl2GetNumber("TiffNumberDefault") || i2!=(int)gl2GetNumber("TiffNumMaxDefault"))
		simLog(sim,2," TIFFs numbered from %i to %i\n",i1,i2);
	simLog(sim,2,"\n");
	return; }


/* writegraphss */
void writegraphss(simptr sim,FILE *fptr) {
	graphicsssptr graphss;
	char string[STRCHAR];
	int lt,item;

	graphss=sim->graphss;
	if(!graphss) return;

	fprintf(fptr,"# Graphics parameters\n");
	if(graphss->graphics==0) fprintf(fptr,"graphics none\n");
	else if(graphss->graphics==1) fprintf(fptr,"graphics opengl\n");
	else if(graphss->graphics==2) fprintf(fptr,"graphics opengl_good\n");
	else if(graphss->graphics==3) fprintf(fptr,"graphics opengl_better\n");
	if(graphss->graphicit>1) fprintf(fptr,"graphic_iter %i\n",graphss->graphicit);
	if(graphss->graphicdelay>0) fprintf(fptr,"graphic_delay %ui\n",graphss->graphicdelay);

	if(graphss->tiffit>0) fprintf(fptr,"tiff_iter %i\n",graphss->tiffit);
	fprintf(fptr,"tiff_name %s\n",gl2GetString("TiffName",string));
	fprintf(fptr,"tiff_min %i\n",gl2SetOptionInt("TiffNumber",-1));
	fprintf(fptr,"tiff_max %i\n",gl2SetOptionInt("TiffNumMax",-1));

	fprintf(fptr,"frame_thickness %g\n",graphss->framepts);
	fprintf(fptr,"frame_color %g %g %g %g\n",graphss->framecolor[0],graphss->framecolor[1],graphss->framecolor[2],graphss->framecolor[3]);
	fprintf(fptr,"grid_thickness %g\n",graphss->gridpts);
	fprintf(fptr,"grid_color %g %g %g %g\n",graphss->gridcolor[0],graphss->gridcolor[1],graphss->gridcolor[2],graphss->gridcolor[3]);
	fprintf(fptr,"background_color %g %g %g %g\n",graphss->backcolor[0],graphss->backcolor[1],graphss->backcolor[2],graphss->backcolor[3]);
	fprintf(fptr,"text_color %g %g %g %g\n",graphss->textcolor[0],graphss->textcolor[1],graphss->textcolor[2],graphss->textcolor[3]);

	for(item=0;item<graphss->ntextitems;item++)
		fprintf(fptr,"text_display %s\n",graphss->textitems[item]);
	
	if(graphss->roomstate!=LPauto) {
		fprintf(fptr,"light global ambient %g %g %g %g\n",graphss->ambiroom[0],graphss->ambiroom[1],graphss->ambiroom[2],graphss->ambiroom[3]);
		fprintf(fptr,"light global %s\n",graphicslp2string(graphss->roomstate,string)); }
	for(lt=0;lt<MAXLIGHTS;lt++)
		if(graphss->lightstate[lt]!=LPauto) {
			fprintf(fptr,"light %i position %g %g %g\n",lt,graphss->lightpos[lt][0],graphss->lightpos[lt][1],graphss->lightpos[lt][2]);
			fprintf(fptr,"light %i ambient %g %g %g %g\n",lt,graphss->ambilight[lt][0],graphss->ambilight[lt][1],graphss->ambilight[lt][2],graphss->ambilight[lt][3]);
			fprintf(fptr,"light %i diffuse %g %g %g %g\n",lt,graphss->difflight[lt][0],graphss->difflight[lt][1],graphss->difflight[lt][2],graphss->difflight[lt][3]);
			fprintf(fptr,"light %i specular %g %g %g %g\n",lt,graphss->speclight[lt][0],graphss->speclight[lt][1],graphss->speclight[lt][2],graphss->speclight[lt][3]);
			fprintf(fptr,"light %i %s\n",lt,graphicslp2string(graphss->lightstate[lt],string)); }

	fprintf(fptr,"\n");
	return; }


/* checkgraphicsparams */
int checkgraphicsparams(simptr sim,int *warnptr) {
	int error,warn;
	graphicsssptr graphss;
	char string[STRCHAR];

	error=warn=0;
	graphss=sim->graphss;
	if(!graphss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(graphss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: graphics structure %s\n",simsc2string(graphss->condition,string)); }

	if(warnptr) *warnptr=warn;
	return error; }

/******************************************************************************/
/******************************* structure setup ******************************/
/******************************************************************************/


/* graphicssetcondition */
void graphicssetcondition(graphicsssptr graphss,enum StructCond cond,int upgrade) {
	if(!graphss) return;
	if(upgrade==0 && graphss->condition>cond) graphss->condition=cond;
	else if(upgrade==1 && graphss->condition<cond) graphss->condition=cond;
	else if(upgrade==2) graphss->condition=cond;
	if(graphss->sim && graphss->condition<graphss->sim->condition) {
		cond=graphss->condition;
		simsetcondition(graphss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* graphicsenablegraphics */
int graphicsenablegraphics(simptr sim,const char *type) {
	graphicsssptr graphss;
	int code;

	if(!sim) return 2;

	if(type) {
		if(!strcmp(type,"none")) code=0;
		else if(!strcmp(type,"opengl")) code=1;
		else if(!strcmp(type,"opengl_good")) code=2;
		else if(!strcmp(type,"opengl_better")) code=3;
		else return 3; }
	else code=-1;

	if(sim->graphss && (code==-1 || sim->graphss->graphics==code)) return 0;
	if(!sim->graphss && code==0) return 0;

	if(!sim->graphss) {
		graphss=graphssalloc();
		if(!graphss) return 1;
		sim->graphss=graphss;
		graphss->sim=sim; }
	else graphss=sim->graphss;
	if(code>=0) graphss->graphics=code;
	graphicssetcondition(graphss,SClists,0);
	return 0; }


/* graphicssetiter */
int graphicssetiter(simptr sim,int iter) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(iter<1) return 3;
	sim->graphss->graphicit=iter;
	return 0; }


/* graphicssettiffiter */
int graphicssettiffiter(simptr sim,int iter) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(iter<1) return 3;
	sim->graphss->tiffit=iter;
	return 0; }


/* graphicssetdelay */
int graphicssetdelay(simptr sim,int delay) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(delay<0) return 3;
	sim->graphss->graphicdelay=delay;
	return 0; }


/* graphicssetframethickness */
int graphicssetframethickness(simptr sim,double thickness) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(thickness<0) return 3;
	sim->graphss->framepts=thickness;
	return 0; }


/* graphicssetframecolor */
int graphicssetframecolor(simptr sim,double *color) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(color[0]<0 || color[0]>1) return 3;
	if(color[1]<0 || color[1]>1) return 3;
	if(color[2]<0 || color[2]>1) return 3;
	if(color[3]<0 || color[3]>1) return 3;
	sim->graphss->framecolor[0]=color[0];
	sim->graphss->framecolor[1]=color[1];
	sim->graphss->framecolor[2]=color[2];
	sim->graphss->framecolor[3]=color[3];
	return 0; }


/* graphicssetgridthickness */
int graphicssetgridthickness(simptr sim,double thickness) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(thickness<0) return 3;
	sim->graphss->gridpts=thickness;
	return 0; }


/* graphicssetgridcolor */
int graphicssetgridcolor(simptr sim,double *color) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(color[0]<0 || color[0]>1) return 3;
	if(color[1]<0 || color[1]>1) return 3;
	if(color[2]<0 || color[2]>1) return 3;
	if(color[3]<0 || color[3]>1) return 3;
	sim->graphss->gridcolor[0]=color[0];
	sim->graphss->gridcolor[1]=color[1];
	sim->graphss->gridcolor[2]=color[2];
	sim->graphss->gridcolor[3]=color[3];
	return 0; }


/* graphicssetbackcolor */
int graphicssetbackcolor(simptr sim,double *color) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(color[0]<0 || color[0]>1) return 3;
	if(color[1]<0 || color[1]>1) return 3;
	if(color[2]<0 || color[2]>1) return 3;
	if(color[3]<0 || color[3]>1) return 3;
	sim->graphss->backcolor[0]=color[0];
	sim->graphss->backcolor[1]=color[1];
	sim->graphss->backcolor[2]=color[2];
	sim->graphss->backcolor[3]=color[3];

	graphicssetcondition(sim->graphss,SCparams,0);
	return 0; }


/* graphicssettextcolor */
int graphicssettextcolor(simptr sim,double *color) {
	int er;
	
	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	if(color[0]<0 || color[0]>1) return 3;
	if(color[1]<0 || color[1]>1) return 3;
	if(color[2]<0 || color[2]>1) return 3;
	if(color[3]<0 || color[3]>1) return 3;
	sim->graphss->textcolor[0]=color[0];
	sim->graphss->textcolor[1]=color[1];
	sim->graphss->textcolor[2]=color[2];
	sim->graphss->textcolor[3]=color[3];
	return 0; }


/* graphicssettextitem */
int graphicssettextitem(simptr sim,char *itemname) {
	graphicsssptr graphss;
	int er,i,newmaxtextitems,item;
	char **newtextitems;
	enum MolecState ms;

	er=graphicsenablegraphics(sim,NULL);
	if(er) return er;
	graphss=sim->graphss;

	if(graphss->ntextitems==graphss->maxtextitems) {
		newmaxtextitems=2*graphss->maxtextitems+1;
		newtextitems=(char **) calloc(newmaxtextitems,sizeof(char *));
		if(!newtextitems) return 1;
		for(item=0;item<graphss->maxtextitems;item++)
			newtextitems[item]=graphss->textitems[item];
		for(;item<newmaxtextitems;item++) {
			newtextitems[item]=EmptyString();
			if(!newtextitems[item]) return 1; }

		free(graphss->textitems);
		graphss->maxtextitems=newmaxtextitems;
		graphss->textitems=newtextitems; }

	if(!strcmp(itemname,"time"));		// supported items
	else if(((sim->mols && ((i=molstring2index1(sim,itemname,&ms,NULL))>=0 || i==-5)) || sim->ruless) && ms!=MSbsoln);
	else return 2;									// unrecognized item name

	for(item=0;item<graphss->ntextitems;item++)
		if(!strcmp(itemname,graphss->textitems[item])) return 3;
	strncpy(graphss->textitems[graphss->ntextitems++],itemname,STRCHAR);
	return 0; }

/* graphicssetlight */
int graphicssetlight(simptr sim,graphicsssptr graphss,int lt,enum LightParam ltparam,double *value) {
	int i,er;

	if(!graphss) {
		er=graphicsenablegraphics(sim,NULL);
		if(er) return er;
		graphss=sim->graphss; }

	if(lt==-1) {										// room lights
		if(ltparam==LPambient) {
			if(graphss->roomstate==LPauto) graphss->roomstate=LPon;
			for(i=0;i<4;i++) graphss->ambiroom[i]=value[i]; }
		else if(ltparam==LPon)
			graphss->roomstate=LPon;
		else if(ltparam==LPoff)
			graphss->roomstate=LPoff;
		else if(ltparam==LPauto) {
			graphss->roomstate=LPauto;
			graphss->ambiroom[0]=0.2;		// low white ambient light
			graphss->ambiroom[1]=0.2;
			graphss->ambiroom[2]=0.2;
			graphss->ambiroom[3]=1; }}

	else if(ltparam==LPambient) {
		if(graphss->lightstate[lt]==LPauto) graphss->lightstate[lt]=LPon;
		for(i=0;i<4;i++) graphss->ambilight[lt][i]=value[i]; }
	else if(ltparam==LPdiffuse) {
		if(graphss->lightstate[lt]==LPauto) graphss->lightstate[lt]=LPon;
		for(i=0;i<4;i++) graphss->difflight[lt][i]=value[i]; }
	else if(ltparam==LPspecular) {
		if(graphss->lightstate[lt]==LPauto) graphss->lightstate[lt]=LPon;
		for(i=0;i<4;i++) graphss->speclight[lt][i]=value[i]; }
	else if(ltparam==LPposition) {
		if(graphss->lightstate[lt]==LPauto) graphss->lightstate[lt]=LPon;
		for(i=0;i<4;i++) graphss->lightpos[lt][i]=value[i]; }
	else if(ltparam==LPon)
		graphss->lightstate[lt]=LPon;
	else if(ltparam==LPoff)
		graphss->lightstate[lt]=LPoff;
	else if(ltparam==LPauto) {
		graphss->lightstate[lt]=LPauto;
		graphss->ambilight[lt][0]=0;	// no ambient lightsource light
		graphss->ambilight[lt][1]=0;
		graphss->ambilight[lt][2]=0;
		graphss->ambilight[lt][3]=1;
		graphss->difflight[lt][0]=1;	// white diffuse lightsource light
		graphss->difflight[lt][1]=1;
		graphss->difflight[lt][2]=1;
		graphss->difflight[lt][3]=1;
		graphss->speclight[lt][0]=1;	// white specular lightsource light
		graphss->speclight[lt][1]=1;
		graphss->speclight[lt][2]=1;
		graphss->speclight[lt][3]=1;
		graphss->lightpos[lt][0]=1;		// lightsource at (1,1,0)
		graphss->lightpos[lt][1]=1;
		graphss->lightpos[lt][2]=0;
		graphss->lightpos[lt][3]=0; }

	graphicssetcondition(graphss,SCparams,0);
	return 0; }


/******************************************************************************/
/************************** structure update functions ************************/
/******************************************************************************/


/* graphicsupdateinit */
int graphicsupdateinit(simptr sim) {
#ifdef __gl_h_
	graphicsssptr graphss;
	int qflag,tflag,dim;
	wallptr *wlist;

	graphss=sim->graphss;
	tflag=strchr(sim->flags,'t')?1:0;
	if(tflag || graphss->graphics==0) return 0;

	qflag=strchr(sim->flags,'q')?1:0;
	gl2glutInit(NULL,NULL);
	gl2SetOptionInt("Fix2DAspect",1);
	gl2SetOptionVoid("FreeFunc",(void*) &simfree);
	gl2SetOptionVoid("FreePointer",(void*)sim);
	if(!qflag) simLog(sim,2,"Starting simulation\n");
	dim=sim->dim;
	wlist=sim->wlist;
	if(dim==1) gl2Initialize(sim->filename,(float)wlist[0]->pos,(float)wlist[1]->pos,0,0,0,0);
	else if(dim==2) gl2Initialize(sim->filename,(float)wlist[0]->pos,(float)wlist[1]->pos,(float)wlist[2]->pos,(float)wlist[3]->pos,0,0);
	else {
		gl2Initialize(sim->filename,(float)wlist[0]->pos,(float)wlist[1]->pos,(float)wlist[2]->pos,(float)wlist[3]->pos,(float)wlist[4]->pos,(float)wlist[5]->pos);
		if(sim->srfss) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);}}
#endif
	return 0; }


/* graphicsupdatelists */
int graphicsupdatelists(simptr sim) {
#ifdef __gl_h_
	graphicsssptr graphss;
	int tflag;
	GLfloat f1[4];

	graphss=sim->graphss;
	tflag=strchr(sim->flags,'t')?1:0;
	if(tflag || graphss->graphics==0) return 0;

	if(graphss->graphics>=3) {
		glEnable(GL_LIGHTING);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,gl2Double2GLfloat(graphss->ambiroom,f1,4));
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE); }
#endif
	return 0; }


/* graphicsupdateparams */
int graphicsupdateparams(simptr sim) {
#ifdef __gl_h_
	graphicsssptr graphss;
	int lt,tflag;
	GLenum gllightnum;
	GLfloat glf1[4];

	graphss=sim->graphss;
	tflag=strchr(sim->flags,'t')?1:0;
	if(tflag || graphss->graphics==0) return 0;

	glClearColor((GLclampf)graphss->backcolor[0],(GLclampf)graphss->backcolor[1],(GLclampf)graphss->backcolor[2],(GLclampf)graphss->backcolor[3]);
	if(graphss->graphics>=3) {
		for(lt=0;lt<MAXLIGHTS;lt++)
			if(graphss->lightstate[lt]==LPon) {
				if(lt==0) gllightnum=GL_LIGHT0;
				else if(lt==1) gllightnum=GL_LIGHT1;
				else if(lt==2) gllightnum=GL_LIGHT2;
				else if(lt==3) gllightnum=GL_LIGHT3;
				else if(lt==4) gllightnum=GL_LIGHT4;
				else if(lt==5) gllightnum=GL_LIGHT5;
				else if(lt==6) gllightnum=GL_LIGHT6;
				else gllightnum=GL_LIGHT7;
				glLightfv(gllightnum,GL_AMBIENT,gl2Double2GLfloat(graphss->ambilight[lt],glf1,4));
				glLightfv(gllightnum,GL_DIFFUSE,gl2Double2GLfloat(graphss->difflight[lt],glf1,4));
				glLightfv(gllightnum,GL_SPECULAR,gl2Double2GLfloat(graphss->speclight[lt],glf1,4));
				glLightfv(gllightnum,GL_POSITION,gl2Double2GLfloat(graphss->lightpos[lt],glf1,4));
				glEnable(gllightnum); }}
#endif
	return 0; }


/* graphicsupdate */
int graphicsupdate(simptr sim) {
	int er;
	graphicsssptr graphss;

	graphss=sim->graphss;
	if(graphss) {
		if(graphss->condition==SCinit) {
			er=graphicsupdateinit(sim);
			if(er) return er;
			graphicssetcondition(graphss,SClists,1); }
		if(graphss->condition==SClists) {
			er=graphicsupdatelists(sim);
			if(er) return er;
			graphicssetcondition(graphss,SCparams,1); }
		if(graphss->condition==SCparams) {
			er=graphicsupdateparams(sim);
			if(er) return er;
			graphicssetcondition(graphss,SCok,1); }}
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* RenderSurfaces */
void RenderSurfaces(simptr sim) {
#ifdef __gl_h_
	surfacessptr srfss;
	surfaceptr srf;
	int s,p,graphics,fdone,bdone,c;
	double **point,*front;
	double xlo,xhi,ylo,yhi,xpix,ypix,ymid,zmid;
	double delta,deltax,deltay,theta,vect[3],vect2[3],axis[3],height;
	double flcolor[4],blcolor[4];
	enum DrawMode fdrawmode,bdrawmode,fdm,bdm;
	GLdouble gldvect[3];
	GLfloat glfvect[4];

	srfss=sim->srfss;
	graphics=sim->graphss->graphics;
	if(!srfss) return;
	xlo=gl2GetNumber("ClipLeft");
	xhi=gl2GetNumber("ClipRight");
	ylo=gl2GetNumber("ClipBot");
	yhi=gl2GetNumber("ClipTop");
	xpix=gl2GetNumber("PixWide");
	ypix=gl2GetNumber("PixHigh");
	ymid=gl2GetNumber("ClipMidy");
	zmid=gl2GetNumber("ClipMidz");
	
	if(sim->dim==1) {
		for(s=0;s<srfss->nsrf;s++) {
			srf=srfss->srflist[s];
			if(srf->fdrawmode!=DMno) {
				glLineWidth((GLfloat)srf->edgepts);
				delta=srf->edgepts*(xhi-xlo)/xpix/2;
				glColor4fv(gl2Double2GLfloat(srf->fcolor,glfvect,4));
				glBegin(GL_LINES);
				for(p=0;p<srf->npanel[0];p++) {		// 1-D rectangles front
					point=srf->panels[0][p]->point;
					front=srf->panels[0][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[1];p++) {		// 1-D triangles front
					point=srf->panels[1][p]->point;
					front=srf->panels[1][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[2];p++) {		// 1-D spheres front
					point=srf->panels[2][p]->point;
					front=srf->panels[2][p]->front;
					glVertex3d((GLdouble)(point[0][0]+point[1][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+point[1][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-point[1][0]-front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-point[1][0]-front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				glEnd();
				
				delta*=-1;
				glColor4fv(gl2Double2GLfloat(srf->bcolor,glfvect,4));
				glBegin(GL_LINES);
				for(p=0;p<srf->npanel[0];p++) {		// 1-D rectangles back
					point=srf->panels[0][p]->point;
					front=srf->panels[0][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[1];p++) {		// 1-D triangles back
					point=srf->panels[1][p]->point;
					front=srf->panels[1][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[2];p++) {		// 1-D spheres back
					point=srf->panels[2][p]->point;
					front=srf->panels[2][p]->front;
					glVertex3d((GLdouble)(point[0][0]+point[1][0]+front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]+point[1][0]+front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-point[1][0]-front[0]*delta),(GLdouble)(ymid-(yhi-ylo)/20),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-point[1][0]-front[0]*delta),(GLdouble)(ymid+(yhi-ylo)/20),(GLdouble)zmid); }
				glEnd(); }}}
	
	else if(sim->dim==2) {
		for(s=0;s<srfss->nsrf;s++) {
			srf=srfss->srflist[s];
			fdrawmode=srf->fdrawmode;
			bdrawmode=srf->bdrawmode;
			if(fdrawmode!=DMno) {
				glColor4fv(gl2Double2GLfloat(srf->fcolor,glfvect,4));
				if(fdrawmode&DMedge || fdrawmode&DMface) {
					glLineWidth((GLfloat)srf->edgepts);
					deltax=srf->edgepts*(xhi-xlo)/xpix/2.5;
					deltay=srf->edgepts*(yhi-ylo)/ypix/2.5;
					glBegin(GL_LINES); }
				else {
					glPointSize((GLfloat)srf->edgepts);
					deltax=deltay=0;
					glBegin(GL_POINTS); }
				
				for(p=0;p<srf->npanel[0];p++) {		// 2-D rectangles front
					point=srf->panels[0][p]->point;
					front=srf->panels[0][p]->front;
					if(front[1]==0) {
						glVertex3d((GLdouble)(point[0][0]+front[0]*deltax),(GLdouble)(point[0][1]),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[1][0]+front[0]*deltax),(GLdouble)(point[1][1]),(GLdouble)zmid); }
					else {
						glVertex3d((GLdouble)(point[0][0]),(GLdouble)(point[0][1]+front[0]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[1][0]),(GLdouble)(point[1][1]+front[0]*deltay),(GLdouble)zmid); }}
				for(p=0;p<srf->npanel[1];p++) {		// 2-D triangles front
					point=srf->panels[1][p]->point;
					front=srf->panels[1][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*deltax),(GLdouble)(point[0][1]+front[1]*deltay),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[1][0]+front[0]*deltax),(GLdouble)(point[1][1]+front[1]*deltay),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[3];p++) {		// 2-D cylinders front
					point=srf->panels[3][p]->point;
					front=srf->panels[3][p]->front;
					glVertex3d((GLdouble)(point[0][0]+front[0]*point[2][0]+front[2]*front[0]*deltax),(GLdouble)(point[0][1]+front[1]*point[2][0]+front[2]*front[1]*deltay),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[1][0]+front[0]*point[2][0]+front[2]*front[0]*deltax),(GLdouble)(point[1][1]+front[1]*point[2][0]+front[2]*front[1]*deltay),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-front[0]*point[2][0]-front[2]*front[0]*deltax),(GLdouble)(point[0][1]-front[1]*point[2][0]-front[2]*front[1]*deltay),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[1][0]-front[0]*point[2][0]-front[2]*front[0]*deltax),(GLdouble)(point[1][1]-front[1]*point[2][0]-front[2]*front[1]*deltay),(GLdouble)zmid); }
				for(p=0;p<srf->npanel[5];p++) {		// 2-D disks front
					point=srf->panels[5][p]->point;
					front=srf->panels[5][p]->front;
					glVertex3d((GLdouble)(point[0][0]+point[1][0]*front[1]+front[0]*deltax),(GLdouble)(point[0][1]-point[1][0]*front[0]+front[1]*deltay),(GLdouble)zmid);
					glVertex3d((GLdouble)(point[0][0]-point[1][0]*front[1]+front[0]*deltax),(GLdouble)(point[0][1]+point[1][0]*front[0]+front[1]*deltay),(GLdouble)zmid); }
				glEnd();
				for(p=0;p<srf->npanel[2];p++) {		// 2-D spheres front
					point=srf->panels[2][p]->point;
					front=srf->panels[2][p]->front;
					if(fdrawmode&DMvert) gl2DrawCircleD(point[0],point[1][0],(int)point[1][1],'v',2);
					if(fdrawmode&DMface) gl2DrawCircleD(point[0],point[1][0],(int)point[1][1],'f',2);
					if(fdrawmode&DMedge) gl2DrawCircleD(point[0],point[1][0]+front[0]*deltax,(int)point[1][1],'e',2); }
				for(p=0;p<srf->npanel[4];p++) {		// 2-D hemispheres front
					point=srf->panels[4][p]->point;
					front=srf->panels[4][p]->front;
					theta=atan2(point[2][1],point[2][0])+PI/2.0;
					if(fdrawmode&DMvert) gl2DrawArcD(point[0],point[1][0],theta,theta+PI,(int)point[1][1],'v',2);
					if(fdrawmode&DMface) gl2DrawArcD(point[0],point[1][0],theta,theta+PI,(int)point[1][1],'f',2);
					if(fdrawmode&DMedge) gl2DrawArcD(point[0],point[1][0]+front[0]*deltax,theta,theta+PI,(int)point[1][1],'e',2); }
				
				if(fdrawmode&DMedge || fdrawmode&DMface) {
					deltax*=-1;
					deltay*=-1;
					glColor4fv(gl2Double2GLfloat(srf->bcolor,glfvect,4));
					glBegin(GL_LINES);
					for(p=0;p<srf->npanel[0];p++) {		// 2-D rectangles back
						point=srf->panels[0][p]->point;
						front=srf->panels[0][p]->front;
						if(front[1]==0) {
							glVertex3d((GLdouble)(point[0][0]+front[0]*deltax),(GLdouble)(point[0][1]),(GLdouble)zmid);
							glVertex3d((GLdouble)(point[1][0]+front[0]*deltax),(GLdouble)(point[1][1]),(GLdouble)zmid); }
						else {
							glVertex3d((GLdouble)(point[0][0]),(GLdouble)(point[0][1]+front[0]*deltay),(GLdouble)zmid);
							glVertex3d((GLdouble)(point[1][0]),(GLdouble)(point[1][1]+front[0]*deltay),(GLdouble)zmid); }}
					for(p=0;p<srf->npanel[1];p++) {		// 2-D triangles back
						point=srf->panels[1][p]->point;
						front=srf->panels[1][p]->front;
						glVertex3d((GLdouble)(point[0][0]+front[0]*deltax),(GLdouble)(point[0][1]+front[1]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[1][0]+front[0]*deltax),(GLdouble)(point[1][1]+front[1]*deltay),(GLdouble)zmid); }
					for(p=0;p<srf->npanel[3];p++) {		// 2-D cylinders back
						point=srf->panels[3][p]->point;
						front=srf->panels[3][p]->front;
						glVertex3d((GLdouble)(point[0][0]+front[0]*point[2][0]+front[2]*front[0]*deltax),(GLdouble)(point[0][1]+front[1]*point[2][0]+front[2]*front[1]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[1][0]+front[0]*point[2][0]+front[2]*front[0]*deltax),(GLdouble)(point[1][1]+front[1]*point[2][0]+front[2]*front[1]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[0][0]-front[0]*point[2][0]-front[2]*front[0]*deltax),(GLdouble)(point[0][1]-front[1]*point[2][0]-front[2]*front[1]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[1][0]-front[0]*point[2][0]-front[2]*front[0]*deltax),(GLdouble)(point[1][1]-front[1]*point[2][0]-front[2]*front[1]*deltay),(GLdouble)zmid); }
					for(p=0;p<srf->npanel[5];p++) {		// 2-D disks back
						point=srf->panels[5][p]->point;
						front=srf->panels[5][p]->front;
						glVertex3d((GLdouble)(point[0][0]+point[1][0]*front[1]+front[0]*deltax),(GLdouble)(point[0][1]-point[1][0]*front[0]+front[1]*deltay),(GLdouble)zmid);
						glVertex3d((GLdouble)(point[0][0]-point[1][0]*front[1]+front[0]*deltax),(GLdouble)(point[0][1]+point[1][0]*front[0]+front[1]*deltay),(GLdouble)zmid); }
					glEnd();
					for(p=0;p<srf->npanel[2];p++) {		// 2-D spheres back
						point=srf->panels[2][p]->point;
						front=srf->panels[2][p]->front;
						if(bdrawmode&DMedge) gl2DrawCircleD(point[0],point[1][0]+front[0]*deltax,(int)point[1][1],'e',2); }
					for(p=0;p<srf->npanel[4];p++) {		// 2-D hemispheres back
						point=srf->panels[4][p]->point;
						front=srf->panels[4][p]->front;
						theta=atan2(point[2][1],point[2][0])+PI/2.0;
						if(bdrawmode&DMedge) gl2DrawArcD(point[0],point[1][0]+front[0]*deltax,theta,theta+PI,(int)point[1][1],'e',2); }}}}}
	
	else if(sim->dim==3) {
		for(s=0;s<srfss->nsrf;s++) {
			srf=srfss->srflist[s];
			fdrawmode=srf->fdrawmode;
			bdrawmode=srf->bdrawmode;
			for(c=0;c<4;c++) flcolor[c]=srf->fcolor[c];
			for(c=0;c<4;c++) blcolor[c]=srf->bcolor[c];

			if(fdrawmode&DMface) fdm=DMface;
			else if(fdrawmode&DMedge) fdm=DMedge;
			else if(fdrawmode&DMvert) fdm=DMvert;
			else fdm=DMno;

			if(bdrawmode&DMface) bdm=DMface;
			else if(bdrawmode&DMedge) bdm=DMedge;
			else if(bdrawmode&DMvert) bdm=DMvert;
			else bdm=DMno;

			while(fdm || bdm) {
				if(fdm==DMface) glPolygonMode(GL_FRONT,GL_FILL);
				else if(fdm==DMedge) glPolygonMode(GL_FRONT,GL_LINE);
				else if(fdm==DMvert) glPolygonMode(GL_FRONT,GL_POINT);
				else glCullFace(GL_FRONT);

				if(bdm==DMface) glPolygonMode(GL_BACK,GL_FILL);
				else if(bdm==DMedge) glPolygonMode(GL_BACK,GL_LINE);
				else if(bdm==DMvert) glPolygonMode(GL_BACK,GL_POINT);
				else glCullFace(GL_BACK);

				glColor4fv(gl2Double2GLfloat(flcolor,glfvect,4));
				glLineWidth((GLfloat)srf->edgepts);
				if(graphics>=2 && srf->edgestipple[1]!=0xFFFF) {
					glEnable(GL_LINE_STIPPLE);
					glLineStipple((GLint)srf->edgestipple[0],(GLushort)srf->edgestipple[1]); }

				if(graphics>=3) {
					glMaterialfv(GL_FRONT,GL_SPECULAR,gl2Double2GLfloat(flcolor,glfvect,4));
					glMaterialfv(GL_BACK,GL_SPECULAR,gl2Double2GLfloat(blcolor,glfvect,4));
					glMateriali(GL_FRONT,GL_SHININESS,(GLint)srf->fshiny);
					glMateriali(GL_BACK,GL_SHININESS,(GLint)srf->bshiny); }

				if(srf->npanel[PSrect]) {
					glBegin(GL_QUADS);									// 3-D rectangles
					for(p=0;p<srf->npanel[PSrect];p++) {
						if(graphics>=3) {
							gldvect[0]=gldvect[1]=gldvect[2]=0;
							front=srf->panels[PSrect][p]->front;
							gldvect[(int)front[1]]=(GLdouble)front[0];
							glNormal3dv(gldvect); }
						point=srf->panels[PSrect][p]->point;
						glVertex3d((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
						glVertex3d((GLdouble)(point[1][0]),(GLdouble)(point[1][1]),(GLdouble)(point[1][2]));
						glVertex3d((GLdouble)(point[2][0]),(GLdouble)(point[2][1]),(GLdouble)(point[2][2]));
						glVertex3d((GLdouble)(point[3][0]),(GLdouble)(point[3][1]),(GLdouble)(point[3][2])); }
					glEnd(); }
				
				if(srf->npanel[PStri]) {
					glBegin(GL_TRIANGLES);							// 3-D triangles
					for(p=0;p<srf->npanel[PStri];p++) {
						if(graphics>=3) glNormal3fv(gl2Double2GLfloat(srf->panels[PStri][p]->front,glfvect,4));
						point=srf->panels[PStri][p]->point;
						glVertex3d((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
						glVertex3d((GLdouble)(point[1][0]),(GLdouble)(point[1][1]),(GLdouble)(point[1][2]));
						glVertex3d((GLdouble)(point[2][0]),(GLdouble)(point[2][1]),(GLdouble)(point[2][2])); }
					glEnd(); }
				
				for(p=0;p<srf->npanel[PSsph];p++) {		// 3-D spheres
					point=srf->panels[PSsph][p]->point;
					front=srf->panels[PSsph][p]->front;
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslated((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
					gl2DrawSphere(point[1][0],(int)point[1][1],(int)point[1][2],front[0]>0?0:1,graphics>=3?1:0);
					glPopMatrix(); }
				
				for(p=0;p<srf->npanel[PScyl];p++) {			// 3-D cylinders
					point=srf->panels[PScyl][p]->point;
					front=srf->panels[PScyl][p]->front;
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslated((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
					vect[0]=vect[1]=0;
					vect[2]=1;
					vect2[0]=point[1][0]-point[0][0];
					vect2[1]=point[1][1]-point[0][1];
					vect2[2]=point[1][2]-point[0][2];
					height=sqrt(vect2[0]*vect2[0]+vect2[1]*vect2[1]+vect2[2]*vect2[2]);
					normalizeVD(vect2,3);
					theta=gl2FindRotateD(vect,vect2,axis);
					glRotated((GLdouble)theta,(GLdouble)(axis[0]),(GLdouble)(axis[1]),(GLdouble)(axis[2]));
					gl2DrawCylinder(point[2][0],point[2][0],height,(int)point[2][1],(int)point[2][2],front[0]>0?0:1,graphics>=3?1:0);
					glPopMatrix(); }
				
				for(p=0;p<srf->npanel[PShemi];p++) {			// 3-D hemispheres
					point=srf->panels[PShemi][p]->point;
					front=srf->panels[PShemi][p]->front;
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslated((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
					vect[0]=vect[1]=0;
					vect[2]=-1;
					theta=gl2FindRotateD(vect,point[2],axis);
					glRotated((GLdouble)theta,(GLdouble)(axis[0]),(GLdouble)(axis[1]),(GLdouble)(axis[2]));
					gl2DrawHemisphere(point[1][0],(int)point[1][1],(int)point[1][2],front[0]>0?0:1,graphics>=3?1:0);
					glPopMatrix(); }
				
				for(p=0;p<srf->npanel[PSdisk];p++) {			// 3-D disks
					point=srf->panels[PSdisk][p]->point;
					front=srf->panels[PSdisk][p]->front;
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glTranslated((GLdouble)(point[0][0]),(GLdouble)(point[0][1]),(GLdouble)(point[0][2]));
					vect[0]=vect[1]=0;
					vect[2]=-1;
					theta=gl2FindRotateD(vect,front,axis);
					glRotated((GLdouble)theta,(GLdouble)(axis[0]),(GLdouble)(axis[1]),(GLdouble)(axis[2]));
					vect2[0]=vect2[1]=vect2[2]=0;
					gl2DrawCircleD(vect2,point[1][0],(int)point[1][1],'f',3);		//?? 'f' isn't right
					glPopMatrix(); }

				fdone=0;
				if(fdm==DMface && fdrawmode&DMedge) fdm=DMedge;
				else if((fdm==DMface || fdm==DMedge) && fdrawmode&DMvert) fdm=DMvert;
				else fdone=1;

				bdone=0;
				if(bdm==DMface && bdrawmode&DMedge) bdm=DMedge;
				else if((bdm==DMface || bdm==DMedge) && bdrawmode&DMvert) bdm=DMvert;
				else bdone=1;

				if(fdone && bdone) fdm=bdm=DMno;
				else {
					for(c=0;c<3;c++) flcolor[c]=blcolor[c]=0;
					flcolor[3]=blcolor[3]=1; }}

			if((fdrawmode || bdrawmode) && glIsEnabled(GL_LINE_STIPPLE))
				glDisable(GL_LINE_STIPPLE); }}
#endif
	return; }


/* RenderFilaments */
void RenderFilaments(simptr sim) {
#ifdef __gl_h_
	filamentssptr filss;
	filamentptr fil;
	int f,vtx,graphics;
	double *point;
	enum DrawMode drawmode;
	GLfloat glfvect[4];
	
	filss=sim->filss;
	if(!filss) return;
	graphics=sim->graphss->graphics;

	for(f=0;f<filss->nfil;f++) {
		fil=filss->fillist[f];
		drawmode=fil->drawmode;
		if(drawmode==DMno);

		else if(drawmode&DMvert || drawmode&DMedge) {
			glColor4fv(gl2Double2GLfloat(fil->color,glfvect,4));
			if(graphics>=2 && fil->edgestipple[1]!=0xFFFF) {
				glEnable(GL_LINE_STIPPLE);
				glLineStipple((GLint)fil->edgestipple[0],(GLushort)fil->edgestipple[1]); }
			if(drawmode&DMedge) {
				glLineWidth((GLfloat)fil->edgepts);
				glBegin(GL_LINE_STRIP); }
			else {
				glPointSize((GLfloat)fil->edgepts);
				glBegin(GL_POINTS); }

			for(vtx=fil->front;vtx<=fil->back;vtx++) {
				point=fil->sxyz[vtx];
				glVertex3d((GLdouble)(point[0]),(GLdouble)(point[1]),(GLdouble)(point[2])); }
			glEnd(); }

		else if(drawmode&DMface) {
			glPolygonMode(GL_FRONT,GL_FILL);
			glCullFace(GL_BACK);
			if(graphics>=3) {
				//glMaterialfv(GL_FRONT,GL_SPECULAR,gl2Double2GLfloat(srf->fcolor,glfvect,4));
				//glMaterialfv(GL_BACK,GL_SPECULAR,gl2Double2GLfloat(srf->bcolor,glfvect,4));
				glMateriali(GL_FRONT,GL_SHININESS,(GLint)fil->shiny); }
			for(vtx=fil->front;vtx<fil->back;vtx++)
				/*gl2drawtwistprism(fil->px[vtx],fil->px[vtx+1],fil->nface,fil->po[vtx],twist,fil->radius,fil->facecolor)*/; }
		if(glIsEnabled(GL_LINE_STIPPLE))
			glDisable(GL_LINE_STIPPLE); }

#endif
	return; }


/* RenderMolecs */
void RenderMolecs(simptr sim) {
#ifdef __gl_h_
	molssptr mols;
	moleculeptr mptr;
	int ll,m,i,dim;
	double ymid,zmid;
	enum MolecState ms;
	GLfloat whitecolor[]={1,1,1,1};
	GLfloat glf1[4];

	dim=sim->dim;
	mols=sim->mols;
	if(!mols) return;
	ymid=gl2GetNumber("ClipMidy");
	zmid=gl2GetNumber("ClipMidz");

	if(sim->graphss->graphics==1) {
		for(ll=0;ll<sim->mols->nlist;ll++)
			if(sim->mols->listtype[ll]==MLTsystem)
				for(m=0;m<mols->nl[ll];m++) {
					mptr=mols->live[ll][m];
					i=mptr->ident;
					ms=mptr->mstate;
					if(mols->display[i][ms]>0) {
						glPointSize((GLfloat)mols->display[i][ms]);
						glColor3fv(gl2Double2GLfloat(mols->color[i][ms],glf1,3));
						glBegin(GL_POINTS);
						if(dim==1) glVertex3d((GLdouble)mptr->pos[0],(GLdouble)ymid,(GLdouble)zmid);
						else if(dim==2) glVertex3d((GLdouble)(mptr->pos[0]),(GLdouble)(mptr->pos[1]),(GLdouble)zmid);
						else glVertex3fv(gl2Double2GLfloat(mptr->pos,glf1,3));
						glEnd(); }}}

	else if(sim->graphss->graphics>=2) {
		glMatrixMode(GL_MODELVIEW);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		if(sim->graphss->graphics>=3) {
			glMaterialfv(GL_FRONT,GL_SPECULAR,whitecolor);
			glMateriali(GL_FRONT,GL_SHININESS,30); }
		for(ll=0;ll<sim->mols->nlist;ll++)
			if(sim->mols->listtype[ll]==MLTsystem)
				for(m=0;m<mols->nl[ll];m++) {
					mptr=mols->live[ll][m];
					i=mptr->ident;
					ms=mptr->mstate;
					if(mols->display[i][ms]>0) {
						glColor3fv(gl2Double2GLfloat(mols->color[i][ms],glf1,3));
						glPushMatrix();
						if(dim==1) glTranslated((GLdouble)(mptr->pos[0]),(GLdouble)ymid,(GLdouble)zmid);
						else if(dim==2) glTranslated((GLdouble)(mptr->pos[0]),(GLdouble)(mptr->pos[1]),(GLdouble)zmid);
						else glTranslated((GLdouble)(mptr->pos[0]),(GLdouble)(mptr->pos[1]),(GLdouble)(mptr->pos[2]));
						glutSolidSphere((GLdouble)(mols->display[i][ms]),15,15);
						glPopMatrix(); }}}

#endif
	return; }


/* RenderLattice */
void RenderLattice(simptr sim) {
#ifdef __gl_h_
	latticeptr lattice;
	int lat,n,ilat,ismol,i,dim;
	const int *copy_numbers;
	const double* positions;
	molssptr mols;
	GLfloat glf1[4];
	double poslo[3],poshi[3],deltay;

	mols=sim->mols;
	dim=sim->dim;
	poslo[0]=poshi[0]=gl2GetNumber("ClipMidx");
	poslo[1]=poshi[1]=gl2GetNumber("ClipMidy");
	poslo[2]=poshi[2]=gl2GetNumber("ClipMidz");
	for(lat=0;lat<sim->latticess->nlattice;lat++) {
		lattice = sim->latticess->latticelist[lat];
		positions=NULL;
		copy_numbers=NULL;
		n=0;
		for(ilat=0;ilat<lattice->nspecies;ilat++) {		// ilat is the species identity
			ismol=lattice->species_index[ilat];
			NSV_CALL(n = nsv_get_species_copy_numbers(lattice->nsv, ismol,&copy_numbers,&positions));
			for (i=0;i<n;++i) {						// n is the total number of molecules of this species, copy_numbers is how many in each site, and positions are the site positions
				if((mols->display[ismol][MSsoln]>0)&&(copy_numbers[i]>0)) {
					poslo[0]=positions[3*i+0]-0.5*lattice->dx[0];
					poshi[0]=positions[3*i+0]+0.5*lattice->dx[0];
					if(dim==1) {
						deltay=0.025*(gl2GetNumber("ClipTop")-gl2GetNumber("ClipBot"));
						poslo[1]-=deltay;
						poshi[1]+=deltay; }
					else if(dim>1) {
						poslo[1]=positions[3*i+1]-0.5*lattice->dx[1];
						poshi[1]=positions[3*i+1]+0.5*lattice->dx[1]; }
					if(dim>2) {
						poslo[2]=positions[3*i+2]-0.5*lattice->dx[2];
						poshi[2]=positions[3*i+2]+0.5*lattice->dx[2]; }
					glColor3fv(gl2Double2GLfloat(mols->color[ismol][MSsoln],glf1,3));
					gl2DrawBoxFaceD(poslo,poshi,dim==3?3:2); }}}}
#endif
	return; }


/* RenderText */
void RenderText(simptr sim) {
#ifdef __gl_h_
	graphicsssptr graphss;
	int item,i,*index;
	char *itemname,string[STRCHAR],string2[STRCHAR];
	enum MolecState ms;

	graphss=sim->graphss;
	string2[0]='\0';
	for(item=0;item<graphss->ntextitems;item++) {
		itemname=graphss->textitems[item];
		if(!strcmp(itemname,"time"))
			 snprintf(string,STRCHAR,"time: %g",sim->time);
		else if((i=molstring2index1(sim,itemname,&ms,&index))>=0 || i==-5)
			snprintf(string,STRCHAR,"%s: %i",itemname,molcount(sim,i,index,ms,-1));
		else if(sim->ruless)
			snprintf(string,STRCHAR,"%s: 0",itemname);
		else
			snprintf(string,STRCHAR,"syntax error");

		if(STRCHAR-strlen(string2)>strlen(string))
			strcat(string2,string);
		if(item+1<graphss->ntextitems)
			strncat(string2,", ",STRCHAR-3); }
	gl2DrawTextD(5,95,graphss->textcolor,GLUT_BITMAP_HELVETICA_12,string2,-1);
#endif
	return; }


/* RenderSim */
void RenderSim(simptr sim) {
#ifdef __gl_h_
	graphicsssptr graphss;
	double pt1[DIMMAX],pt2[DIMMAX];
	int dim;
	wallptr *wlist;
	GLfloat glf1[4];

	graphss=sim->graphss;
	if(!graphss || graphss->graphics==0) return;
	dim=sim->dim;
	wlist=sim->wlist;
	if(dim<3) glClear(GL_COLOR_BUFFER_BIT);
	else glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if(dim==3) RenderMolecs(sim);

	if(graphss->framepts) {														// draw bounding box
		pt1[0]=wlist[0]->pos;
		pt2[0]=wlist[1]->pos;
		pt1[1]=dim>1?wlist[2]->pos:0;
		pt2[1]=dim>1?wlist[3]->pos:0;
		pt1[2]=dim>2?wlist[4]->pos:0;
		pt2[2]=dim>2?wlist[5]->pos:0;
		glColor4fv(gl2Double2GLfloat(graphss->framecolor,glf1,4));
		glLineWidth((GLfloat)graphss->framepts);
		gl2DrawBoxD(pt1,pt2,dim); }

	if(graphss->gridpts) {
		pt1[0]=sim->boxs->min[0];
		pt2[0]=pt1[0]+sim->boxs->size[0]*sim->boxs->side[0];
		pt1[1]=dim>1?sim->boxs->min[1]:0;
		pt2[1]=dim>1?pt1[1]+sim->boxs->size[1]*sim->boxs->side[1]:0;
		pt1[2]=dim>2?sim->boxs->min[2]:0;
		pt2[2]=dim>2?pt1[2]+sim->boxs->size[2]*sim->boxs->side[2]:0;
		glColor4fv(gl2Double2GLfloat(graphss->gridcolor,glf1,4));
		if(dim==1) glPointSize((GLfloat)graphss->gridpts);
		else glLineWidth((GLfloat)graphss->gridpts);
		gl2DrawGridD(pt1,pt2,sim->boxs->side,dim); }

	if(dim<3) RenderMolecs(sim);

	if(sim->srfss) RenderSurfaces(sim);
	if(sim->filss) RenderFilaments(sim);
	if(sim->latticess) RenderLattice(sim);
	if(graphss->ntextitems) RenderText(sim);

	glutSwapBuffers();
#endif
	return; }


/******************************************************************************/
/************************** Top level OpenGL functions ************************/
/******************************************************************************/


void RenderScene(void);
void TimerFunction(int state);

simptr Sim;


/* smolPostRedisplay */
void smolPostRedisplay(void) {	//??
#ifdef __gl_h_
	glutPostRedisplay();
#endif
	return; }


/* RenderScene */
void RenderScene(void) {
	RenderSim(Sim);
	return; }


/* TimerFunction */
void TimerFunction(int state) {
#ifdef __gl_h_
	static int oldstate=0;
	unsigned int delay;
	int it;
	simptr sim;
	graphicsssptr graphss;
	
	sim=Sim;
	graphss=sim->graphss;
	//qflag=strchr(sim->flags,'q')?1:0;
	delay=graphss->graphicdelay;

	if(oldstate==1 && gl2State(-1)==0) {							// leave pause state
		oldstate=0;
		sim->clockstt=time(NULL);
		simLog(sim,2,"Simulation running\n"); }

	if(state==0 && gl2State(-1)==0) {										// normal run mode
		it=graphss->currentit;
		if(!(it%graphss->graphicit)) glutPostRedisplay();
		if(graphss->tiffit>0 && it>0 && !((it-1)%graphss->tiffit)) gl2SetKeyPush('T');
		state=simulatetimestep(sim);
		graphss->currentit++; }
	else if(state>0 || (state==0 && gl2State(-1)==2)) {			// stop the simulation
		if(oldstate==0) sim->elapsedtime+=difftime(time(NULL),sim->clockstt);
		scmdpop(sim->cmds,sim->tmax);
		scmdexecute(sim->cmds,sim->time,sim->dt,-1,1);
		scmdsetcondition(sim->cmds,0,0);
		endsimulate(sim,state);
		if(sim->quitatend) gl2SetKeyPush('Q');	//??
		state=-1; }
	else if(oldstate==0 && gl2State(-1)==1) {					// enter pause state
		sim->elapsedtime+=difftime(time(NULL),sim->clockstt);
		oldstate=1;
		delay=20;
		simLog(sim,2,"Simulation paused at simulation time: %g\n",sim->time); }
	else {																						// still in pause state or simulation is over
		glutPostRedisplay();
		delay=20; }

	glutTimerFunc(delay,TimerFunction,state);
#endif
	return; }



/* smolsimulategl */
void smolsimulategl(simptr sim) {
#ifdef __gl_h_
	int er;

	glutTimerFunc((unsigned int)0,TimerFunction,0);
	Sim=sim;
	sim->clockstt=time(NULL);
	er=simdocommands(sim);
	if(er) endsimulate(sim,er);
	glutDisplayFunc(RenderScene);
	glutMainLoop();
#else
	simLog(sim,5,"Graphics are unavailable, so performing non-graphics simulation.\n");
	smolsimulate(sim);
#endif
	return; }
