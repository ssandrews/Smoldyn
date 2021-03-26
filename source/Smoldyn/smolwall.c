/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <math.h>
#include "random2.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"

/******************************************************************************/
/************************************ Walls ***********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// low level utilities

// memory management
wallptr wallalloc(void);
void wallfree(wallptr wptr);
wallptr *wallsalloc(int dim);

// data structure output

// structure setup

// core simulation functions


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/

/* systemrandpos */
void systemrandpos(simptr sim,double *pos) {
	int d;

	for(d=0;d<sim->dim;d++) pos[d]=unirandOOD(sim->wlist[2*d]->pos,sim->wlist[2*d+1]->pos);
	return; }


/* systemvolume */
double systemvolume(simptr sim) {
	int d;
	double vol;

	vol=1.0;
	for(d=0;d<sim->dim;d++) vol*=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;
	return vol; }


/* systemcorners */
void systemcorners(simptr sim,double *poslo,double *poshi) {
	int d;

	for(d=0;d<sim->dim;d++) {
		if(poslo) poslo[d]=sim->wlist[2*d]->pos;
		if(poshi) poshi[d]=sim->wlist[2*d+1]->pos; }
	return; }


/* systemcenter */
void systemcenter(simptr sim,double *center) {
	int d;

	for(d=0;d<sim->dim;d++)
		center[d]=0.5*(sim->wlist[2*d]->pos+sim->wlist[2*d+1]->pos);
	return; }


/* systemdiagonal */
double systemdiagonal(simptr sim) {
	int d;
	double diagonal;

	diagonal=0;
	for(d=0;d<sim->dim;d++)
		diagonal+=(sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos)*(sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos);
	return sqrt(diagonal); }


/* posinsystem */
int posinsystem(simptr sim,double *pos) {
	int d;

	for(d=0;d<sim->dim;d++) {
		if(pos[d]<sim->wlist[2*d]->pos) return 0;
		if(pos[d]>sim->wlist[2*d+1]->pos) return 0; }
	return 1; }


/* wallcalcdist2 */
double wallcalcdist2(simptr sim,double *pos1,double *pos2,int wpcode,double *vect) {
	double dist2,syssize;
	int d,dim;

	dim=sim->dim;
	dist2=0;
	for(d=0;d<dim;d++) {
		if((wpcode>>(2*d)&3)==0)							// no wrapping
			vect[d]=pos2[d]-pos1[d];
		else if((wpcode>>(2*d)&3)==1) {				// wrap towards low side, so pos1 is small and pos2 is big
			syssize=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;
			vect[d]=pos2[d]-pos1[d]-syssize; }
		else {																// wrap towards high side, so pos1 is big and pos2 is small
			syssize=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;
			vect[d]=pos2[d]-pos1[d]+syssize; }
		dist2+=vect[d]*vect[d]; }
	return dist2; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* wallalloc */
wallptr wallalloc(void) {
	wallptr wptr;

	CHECKMEM(wptr=(wallptr) malloc(sizeof(struct wallstruct)));
	wptr->wdim=0;
	wptr->side=0;
	wptr->pos=0;
	wptr->type='r';
	wptr->opp=NULL;
	return wptr;
failure:
	simLog(NULL,10,"Unable to allocate memory in wallalloc");
	return NULL; }


/* wallfree */
void wallfree(wallptr wptr) {
	if(!wptr) return;
	free(wptr);
	return; }


/* wallsalloc */
wallptr *wallsalloc(int dim) {
	int w,d;
	wallptr *wlist;

	if(dim<1) return NULL;
	CHECKMEM(wlist=(wallptr *) calloc(2*dim,sizeof(wallptr)));
	for(w=0;w<2*dim;w++) wlist[w]=NULL;
	for(w=0;w<2*dim;w++)
		CHECKMEM(wlist[w]=wallalloc());
	for(d=0;d<dim;d++) {
		wlist[2*d]->wdim=wlist[2*d+1]->wdim=d;
		wlist[2*d]->side=0;
		wlist[2*d+1]->side=1;
		wlist[2*d]->pos=0;
		wlist[2*d+1]->pos=1;
		wlist[2*d]->type=wlist[2*d+1]->type='r';
		wlist[2*d]->opp=wlist[2*d+1];
		wlist[2*d+1]->opp=wlist[2*d]; }
	return wlist;
failure:
	wallsfree(wlist,dim);
	simLog(NULL,10,"Unable to allocate memory in wallsalloc");
	return NULL; }


/* wallsfree */
void wallsfree(wallptr *wlist,int dim) {
	if(!wlist||dim<1) return;
	for(dim--;dim>=0;dim--) {
		wallfree(wlist[2*dim+1]);
		wallfree(wlist[2*dim]); }
	free(wlist);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/

/* walloutput */
void walloutput(simptr sim) {
	int w,d,dim;
	wallptr *wlist,wptr;
	double vol,poslo[DIMMAX],poshi[DIMMAX];
	char dimchar;

	dim=sim->dim;
	wlist=sim->wlist;
	simLog(sim,2,"WALL PARAMETERS\n");
	if(!wlist) {
		simLog(sim,2," No walls defined for simulation\n\n");
		return; }

	for(w=0;w<2*dim;w++) {
		wptr=wlist[w];
		if(wptr->wdim==0) dimchar='x';
		else if(wptr->wdim==1) dimchar='y';
		else if(wptr->wdim==2) dimchar='z';
		else dimchar='?';
		simLog(sim,2," wall %i: dimension %c, at %g",w,dimchar,wptr->pos);
		if(sim->srfss) {
			simLog(sim,1,", non-interacting because surfaces are defined");
			simLog(sim,2,"\n"); }
		else if(wptr->type=='r') simLog(sim,2,", reflecting\n");
		else if(wptr->type=='p') simLog(sim,2,", periodic\n");
		else if(wptr->type=='a') simLog(sim,2,", absorbing\n");
		else if(wptr->type=='t') simLog(sim,2,", transparent\n");
		if(wlist[w+1-2*(w%2)]!=wptr->opp) simLog(sim,10," ERROR: opposing wall is incorrect\n"); }

	vol=systemvolume(sim);
	if(dim==1) simLog(sim,2," system length: %g\n",vol);
	else if(dim==2) simLog(sim,2," system area: %g\n",vol);
	else simLog(sim,2," system volume: %g\n",vol);

	systemcorners(sim,poslo,poshi);
	simLog(sim,2," system corners: (%g",poslo[0]);
	for(d=1;d<dim;d++) simLog(sim,2,",%g",poslo[d]);
	simLog(sim,2,") and (%g",poshi[0]);
	for(d=1;d<dim;d++) simLog(sim,2,",%g",poshi[d]);
	simLog(sim,2,")\n");
	simLog(sim,2,"\n");

	return; }


/* writewalls */
void writewalls(simptr sim,FILE *fptr) {
	int d,dim;

	fprintf(fptr,"# Boundary parameters\n");
	dim=sim->dim;
	for(d=0;d<dim;d++) {
		fprintf(fptr,"low_wall %i %g %c\n",d,sim->wlist[2*d]->pos,sim->wlist[2*d]->type);
		fprintf(fptr,"high_wall %i %g %c\n",d,sim->wlist[2*d+1]->pos,sim->wlist[2*d+1]->type); }
	fprintf(fptr,"\n");
	return; }


/* checkwallparams */
int checkwallparams(simptr sim,int *warnptr) {
	int d,dim,warn,error;
	wallptr *wlist;
	double lowwall[DIMMAX],highwall[DIMMAX],syslen;

	error=warn=0;
	dim=sim->dim;
	wlist=sim->wlist;

	systemcorners(sim,lowwall,highwall);
	syslen=0;
	for(d=0;d<dim;d++) syslen+=(highwall[d]-lowwall[d])*(highwall[d]-lowwall[d]);
	syslen=sqrt(syslen);
	if(syslen<=0) {error++;simLog(sim,10," ERROR: Total system size is zero\n");}

	for(d=0;d<dim;d++)
		if(lowwall[d]>=highwall[d]) {
			simLog(sim,10," ERROR: low_wall positions need to be smaller than high_wall positions");
			error++; }

	if(!sim->srfss) {
		for(d=0;d<dim;d++)								// check for asymmetric periodic boundary condition
			if(wlist[2*d]->type=='p'&&wlist[2*d+1]->type!='p') {
				simLog(sim,5," WARNING: only one wall on dimension %i has a periodic boundary condition\n",d);
				warn++; }}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/******************************** structure setup *****************************/
/******************************************************************************/


/* walladd */
int walladd(simptr sim,int d,int highside,double pos,char type) {
	if(!sim->wlist) {
		if(!sim->dim) return 2;
		sim->wlist=wallsalloc(sim->dim);
		if(!sim->wlist) return 1; }
	d=2*d+highside;
	sim->wlist[d]->pos=pos;
	sim->wlist[d]->type=type;
	boxsetcondition(sim->boxs,SClists,0);
	return 0; }


/* wallsettype */
int wallsettype(simptr sim,int d,int highside,char type) {
	int d2;

	if(!sim->wlist) return 1;
	if(d<0)
		for(d2=0;d2<sim->dim;d2++) {
			if(highside<0) {
				sim->wlist[2*d2]->type=type;
				sim->wlist[2*d2+1]->type=type; }
			else
				sim->wlist[2*d2+highside]->type=type; }
	else {
		if(highside<0) {
			sim->wlist[2*d]->type=type;
			sim->wlist[2*d+1]->type=type; }
		else
			sim->wlist[2*d+highside]->type=type; }

	boxsetcondition(sim->boxs,SClists,0);
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* checkwalls */
int checkwalls(simptr sim,int ll,int reborn,boxptr bptr) {
	int nmol,w,d,m;
	moleculeptr *mlist;
	double pos2,diff,difi,step,**difstep;
	wallptr wptr;

	if(sim->srfss) return 0;
	if(bptr) {
		nmol=bptr->nmol[ll];
		mlist=bptr->mol[ll]; }
	else {
		nmol=sim->mols->nl[ll];
		mlist=sim->mols->live[ll]; }
	if(!reborn) m=0;
	else if(reborn&&!bptr) m=sim->mols->topl[ll];
	else {m=0;simLog(sim,10,"SMOLDYN ERROR: in checkwalls, both bptr and reborn are defined");}

	for(w=0;w<2*sim->dim;w++) {
		wptr=sim->wlist[w];
		d=wptr->wdim;
		if(wptr->type=='r'&&wptr->side==0) {			// reflective
			pos2=2*wptr->pos;
			for(m=0;m<nmol;m++)
				if(mlist[m]->pos[d]<wptr->pos) {
					sim->eventcount[ETwall]++;
					mlist[m]->pos[d]=pos2-mlist[m]->pos[d];}}
		else if(wptr->type=='r') {
			pos2=2*wptr->pos;
			for(m=0;m<nmol;m++)
				if(mlist[m]->pos[d]>wptr->pos) {
					sim->eventcount[ETwall]++;
					mlist[m]->pos[d]=pos2-mlist[m]->pos[d];}}
		else if(wptr->type=='p'&&wptr->side==0) {	// periodic
			pos2=wptr->opp->pos-wptr->pos;
			for(m=0;m<nmol;m++)
				if(mlist[m]->pos[d]<wptr->pos) {
					sim->eventcount[ETwall]++;
					mlist[m]->pos[d]+=pos2;
					mlist[m]->posoffset[d]-=pos2; }}
		else if(wptr->type=='p') {
			pos2=wptr->opp->pos-wptr->pos;
			for(m=0;m<nmol;m++)
				if(mlist[m]->pos[d]>wptr->pos) {
					sim->eventcount[ETwall]++;
					mlist[m]->pos[d]+=pos2;
					mlist[m]->posoffset[d]-=pos2; }}
		else if(wptr->type=='a') {								// absorbing
			difstep=sim->mols->difstep;
			for(m=0;m<nmol;m++) {
				diff=wptr->pos-mlist[m]->pos[d];
				difi=wptr->pos-mlist[m]->posx[d];
				step=difstep[mlist[m]->ident][MSsoln];
				if((!(wptr->side)&&diff>0)||(wptr->side&&diff<0)||coinrandD(exp(-2*difi*diff/step/step))) {
					sim->eventcount[ETwall]++;
					molkill(sim,mlist[m],ll,-1); }}}}
	sim->mols->touch++;
	return 0; }



