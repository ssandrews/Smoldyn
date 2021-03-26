/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <float.h>
#include <stdio.h>
#include <string.h>
#include "Geometry.h"
#include "math2.h"
#include "random2.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "Zn.h"

/******************************************************************************/
/******************************** Virtual boxes *******************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// low level utilities
int panelinbox(simptr sim,panelptr pnl,boxptr bptr);

// memory management
boxptr boxalloc(int dim,int nlist);
int expandbox(boxptr bptr,int n,int ll);
int expandboxpanels(boxptr bptr,int n);
void boxfree(boxptr bptr,int nlist);
boxptr *boxesalloc(int nbox,int dim,int nlist);
void boxesfree(boxptr *blist,int nbox,int nlist);
boxssptr boxssalloc(int dim);

// data structure output

// structure set up
int boxesupdateparams(simptr sim);
int boxesupdatelists(simptr sim);

// core simulation functions


/******************************************************************************/
/***************************** low level utilities ****************************/
/******************************************************************************/


/* box2pos */
void box2pos(simptr sim,boxptr bptr,double *poslo,double *poshi) {
	int d,dim;
	double *size,*min;

	dim=sim->dim;
	size=sim->boxs->size;
	min=sim->boxs->min;
	if(poslo) for(d=0;d<dim;d++) poslo[d]=min[d]+bptr->indx[d]*size[d];
	if(poshi) for(d=0;d<dim;d++) poshi[d]=min[d]+(bptr->indx[d]+1)*size[d];
	return; }


/* pos2box */
boxptr pos2box(simptr sim,const double *pos) {
	int b,d,indx,dim;
	boxssptr boxs;

	dim=sim->dim;
	boxs=sim->boxs;
	b=0;
	for(d=0;d<dim;d++) {
		indx=(int)((pos[d]-boxs->min[d])/boxs->size[d]);
		if(indx<0) indx=0;
		else if(indx>=boxs->side[d]) indx=boxs->side[d]-1;
		b=boxs->side[d]*b+indx; }
	return boxs->blist[b]; }


/* boxrandpos */
void boxrandpos(simptr sim,double *pos,boxptr bptr) {
	int d;
	double *min,*size;

	min=sim->boxs->min;
	size=sim->boxs->size;
	for(d=0;d<sim->dim;d++)
		pos[d]=unirandCCD(min[d]+size[d]*bptr->indx[d],min[d]+size[d]*(bptr->indx[d]+1));
	return; }


/* panelinbox */
int panelinbox(simptr sim,panelptr pnl,boxptr bptr) {
	int dim,d,cross;
	double v1[DIMMAX],v2[DIMMAX],v3[DIMMAX],v4[DIMMAX],**point,*front;

	dim=sim->dim;
	box2pos(sim,bptr,v1,v2);							// v1 and v2 are set to corners of box
	for(d=0;d<dim;d++) {
		if(bptr->indx[d]==0) v1[d]=-VERYLARGE;
		if(bptr->indx[d]==sim->boxs->side[d]-1) v2[d]=VERYLARGE; }
	point=pnl->point;
	front=pnl->front;

	if(pnl->ps==PSrect) {
		if(dim==1) cross=Geo_PtInSlab(v1,v2,point[0],dim);
		else if(dim==2) {
			v3[0]=front[1]==0?1:0;
			v3[1]=front[1]==0?0:1;
			cross=Geo_LineXaabb2(point[0],point[1],v3,v1,v2); }
		else cross=Geo_RectXaabb3(point[0],point[1],point[3],point[0],v1,v2); }
	else if(pnl->ps==PStri) {
		if(dim==1) cross=Geo_PtInSlab(v1,v2,point[0],dim);
		else if(dim==2) cross=Geo_LineXaabb2(point[0],point[1],front,v1,v2);
		else cross=Geo_TriXaabb3(point[0],point[1],point[2],front,v1,v2); }
	else if(pnl->ps==PSsph) {
		if(dim==1) {
			if((point[0][0]-point[1][0]<v1[0] || point[0][0]-point[1][0]>=v2[0]) && (point[0][0]+point[1][0]<v1[0] || point[0][0]+point[1][0]>=v2[0])) cross=0;
			else cross=1; }
		else if(dim==2) cross=Geo_CircleXaabb2(point[0],point[1][0],v1,v2);
		else cross=Geo_SphsXaabb3(point[0],point[1][0],v1,v2); }
	else if(pnl->ps==PScyl) {
		if(dim==2) {
			v3[0]=point[0][0]+point[2][0]*front[0];
			v3[1]=point[0][1]+point[2][0]*front[1];
			v4[0]=point[1][0]+point[2][0]*front[0];
			v4[1]=point[1][1]+point[2][0]*front[1];
			cross=Geo_LineXaabb2(v3,v4,front,v1,v2);
			if(!cross) {
				v3[0]=point[0][0]-point[2][0]*front[0];
				v3[1]=point[0][1]-point[2][0]*front[1];
				v4[0]=point[1][0]-point[2][0]*front[0];
				v4[1]=point[1][1]-point[2][0]*front[1];
				cross=Geo_LineXaabb2(v3,v4,front,v1,v2); }}
		else {
			cross=Geo_CylsXaabb3(point[0],point[1],point[2][0],v1,v2); }}
	else if(pnl->ps==PShemi) {
		if(dim==2) cross=Geo_SemicXaabb2(point[0],point[1][0],point[2],v1,v2);
		else cross=Geo_HemisXaabb3(point[0],point[1][0],point[2],v1,v2); }
	else if(pnl->ps==PSdisk) {
		if(dim==2) {
			v3[0]=point[0][0]+point[1][0]*front[1];
			v3[1]=point[0][1]-point[1][0]*front[0];
			v4[0]=point[0][0]-point[1][0]*front[1];
			v4[1]=point[0][1]+point[1][0]*front[0];
			cross=Geo_LineXaabb2(v3,v4,front,v1,v2); }
		else cross=Geo_DiskXaabb3(point[0],point[1][0],front,v1,v2); }
	else cross=0;

	return cross; }


/* boxaddmol */
int boxaddmol(moleculeptr mptr,int ll) {
	boxptr bptr;

	bptr=mptr->box;
	if(bptr->nmol[ll]==bptr->maxmol[ll])
		if(expandbox(bptr,bptr->maxmol[ll]+1,ll)) return 1;
	bptr->mol[ll][bptr->nmol[ll]++]=mptr;
	return 0; }


/* boxremovemol */
void boxremovemol(moleculeptr mptr,int ll) {
	int m;
	boxptr bptr;

	bptr=mptr->box;
	for(m=bptr->nmol[ll]-1;m>=0 && bptr->mol[ll][m]!=mptr;m--);
	if(m>=0) bptr->mol[ll][m]=bptr->mol[ll][--bptr->nmol[ll]];
	mptr->box=NULL;
	return; }


/* boxscansphere */
boxptr boxscansphere(simptr sim,const double *pos,double radius,boxptr bptr,int *wrap) {
	boxssptr boxs;
	double boxmin[DIMMAX],boxmax[DIMMAX],v1[DIMMAX],dist;
	int dim,d,b,index[DIMMAX],done,diam,keepgoing;
	static int boxdiameter,startindex[DIMMAX],deltaindex[DIMMAX];

	boxs=sim->boxs;
	dim=sim->dim;

	if(bptr==NULL) {		// first call
		boxdiameter=0;
		for(d=0;d<dim;d++) {
			startindex[d]=(int) floor(((pos[d]-boxs->min[d])-radius)/boxs->size[d]);			// first box to consider
			diam=(int) ceil(((pos[d]-boxs->min[d])+radius)/boxs->size[d])-startindex[d];
			if(diam>boxdiameter) boxdiameter=diam;	// largest diameter in boxes, giving count range
			deltaindex[d]=0; }}											// working index relative to startindex
	else {							// subsequent calls
		done=Zn_incrementcounter(deltaindex,dim,boxdiameter);
		if(done) return NULL; }

	keepgoing=1;
	while(keepgoing) {
		keepgoing=0;	// expect a good box
		for(d=0;d<dim;d++) {
			index[d]=startindex[d]+deltaindex[d];			// index of box, ignoring edges and wrapping
			boxmin[d]=boxs->min[d]+boxs->size[d]*index[d];			// min corner of box, ignoring edges and wrapping
			boxmax[d]=boxs->min[d]+boxs->size[d]*(index[d]+1);	// max corner of box, ignoring edges and wrapping
			wrap[d]=0;
			if(index[d]<0) {
				if(sim->wlist[2*d]->type=='p')
					while(index[d]<0) {
						index[d]+=boxs->side[d];
						wrap[d]--; }
				else
					keepgoing=1; }
			else if(index[d]>=boxs->side[d]) {
				if(sim->wlist[2*d+1]->type=='p')
					while(index[d]>=boxs->side[d]) {
						index[d]-=boxs->side[d];
						wrap[d]++; }
				else
					keepgoing=1; }}
		dist=Geo_NearestAabbPt(boxmin,boxmax,dim,pos,v1);
		if(dist>radius)
			keepgoing=1;
		if(keepgoing) {
			done=Zn_incrementcounter(deltaindex,dim,boxdiameter);
			if(done) return NULL; }}

	b=0;																				// compute the box number with this index
	for(d=0;d<dim;d++)
		b=boxs->side[d]*b+index[d];
	bptr=boxs->blist[b];

	return bptr; }


/* boxdebug */
int boxdebug(simptr sim) {
	int er,ll,m,mb,b;
	molssptr mols;
	boxssptr boxs;
	moleculeptr mptr;
	boxptr bptr;
	char string[STRCHAR];

	er=0;
	mols=sim->mols;
	boxs=sim->boxs;

	for(b=0;b<boxs->nbox;b++) {
		bptr=boxs->blist[b];
		for(ll=0;ll<mols->nlist;ll++) {
			printf("Box %p list %i:",bptr,ll);
			for(mb=0;mb<bptr->nmol[ll];mb++)
				printf(" %s",molserno2string(bptr->mol[ll][mb]->serno,string));
			printf("\n"); }}


	for(ll=0;ll<mols->nlist;ll++)		// check that molecules are in the boxes they think they are in
		for(m=0;m<mols->nl[ll];m++) {
			mptr=mols->live[ll][m];
			bptr=mptr->box;
			if(!bptr) {er++;printf("BUG: molecule %s has box value set to NULL\n",molserno2string(mptr->serno,string)); }
			else {
				for(mb=0;mb<bptr->nmol[ll] && bptr->mol[ll][mb]!=mptr;mb++);
				if(mb==bptr->nmol[ll]) {er++;printf("BUG: molecule %s thinks it's in box %p but isn't\n",molserno2string(mptr->serno,string),bptr); } }}

	for(b=0;b<boxs->nbox;b++) {
		bptr=boxs->blist[b];
		for(ll=0;ll<mols->nlist;ll++)
			for(mb=0;mb<bptr->nmol[ll];mb++) {
				mptr=bptr->mol[ll][mb];
				if(mptr->box!=bptr) {er++;printf("BUG: molecule %s thinks it's in box %p but is really in box %p\n",molserno2string(mptr->serno,string),mptr->box,bptr);} }}

	return er; }


/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/

/* boxalloc */
boxptr boxalloc(int dim,int nlist) {
	boxptr bptr;
	int d,ll;

	bptr=NULL;
	CHECKMEM(bptr=(boxptr) malloc(sizeof(struct boxstruct)));
	bptr->indx=NULL;
	bptr->nneigh=0;
	bptr->midneigh=0;
	bptr->neigh=NULL;
	bptr->wpneigh=NULL;
	bptr->nwall=0;
	bptr->wlist=NULL;
	bptr->maxpanel=0;
	bptr->npanel=0;
	bptr->panel=NULL;
	bptr->maxmol=NULL;
	bptr->nmol=NULL;
	bptr->mol=NULL;

	CHECKMEM(bptr->indx=(int*) calloc(dim,sizeof(int)));
	for(d=0;d<dim;d++) bptr->indx[d]=0;
	if(nlist) {
		CHECKMEM(bptr->maxmol=(int*) calloc(nlist,sizeof(int)));
		for(ll=0;ll<nlist;ll++) bptr->maxmol[ll]=0;
		CHECKMEM(bptr->nmol=(int*) calloc(nlist,sizeof(int)));
		for(ll=0;ll<nlist;ll++) bptr->nmol[ll]=0;
		CHECKMEM(bptr->mol=(moleculeptr**) calloc(nlist,sizeof(moleculeptr*)));
		for(ll=0;ll<nlist;ll++) bptr->mol[ll]=NULL; }

	return bptr;

 failure:
	boxfree(bptr,nlist);
	simLog(NULL,10,"Failed to allocate memory in boxalloc");
	return NULL; }


/* expandbox */
int expandbox(boxptr bptr,int n,int ll) {
	moleculeptr *mlist;
	int m,maxmol,mn;

	maxmol=bptr->maxmol[ll]+n;
	if(maxmol>0) {
		mlist=(moleculeptr*) calloc(maxmol,sizeof(moleculeptr));
		if(!mlist) return 1;
		mn=(n>0)?bptr->maxmol[ll]:maxmol;
		for(m=0;m<mn;m++) mlist[m]=bptr->mol[ll][m]; }
	else {
		maxmol=0;
		mlist=NULL; }
	free(bptr->mol[ll]);
	bptr->mol[ll]=mlist;
	bptr->maxmol[ll]=maxmol;
	if(bptr->nmol[ll]>maxmol) bptr->nmol[ll]=maxmol;
	return 0;}


/* expandboxpanels */
int expandboxpanels(boxptr bptr,int n) {
	int maxpanel,p;
	panelptr *panel;

	if(n<=0) return 0;
	maxpanel=n+bptr->maxpanel;
	panel=(panelptr*) calloc(maxpanel,sizeof(panelptr));
	if(!panel) return 1;
	for(p=0;p<bptr->npanel;p++)
		panel[p]=bptr->panel[p];
	for(;p<maxpanel;p++)
		panel[p]=NULL;
	free(bptr->panel);
	bptr->panel=panel;
	bptr->maxpanel=maxpanel;
	return 0; }


/* boxfree */
void boxfree(boxptr bptr,int nlist) {
	int ll;

	if(!bptr) return;
	if(bptr->mol) {
		for(ll=0;ll<nlist;ll++)
			free(bptr->mol[ll]); }
	free(bptr->mol);
	free(bptr->nmol);
	free(bptr->maxmol);
	free(bptr->panel);
	free(bptr->wlist);
	free(bptr->wpneigh);
	free(bptr->neigh);
	free(bptr->indx);
	free(bptr);
	return; }


/* boxesalloc */
boxptr *boxesalloc(int nbox,int dim,int nlist) {
	int b;
	boxptr *blist;

	blist=NULL;
	CHECKMEM(blist=(boxptr*) calloc(nbox,sizeof(boxptr)));
	for(b=0;b<nbox;b++) blist[b]=NULL;
	for(b=0;b<nbox;b++)
		CHECKMEM(blist[b]=boxalloc(dim,nlist));
	return blist;

 failure:
	boxesfree(blist,nbox,nlist);
	simLog(NULL,10,"Failed to allocate memory in boxesalloc");
	return NULL; }


/* boxesfree */
void boxesfree(boxptr *blist,int nbox,int nlist) {
	int b;

	if(!blist) return;
	for(b=0;b<nbox;b++) boxfree(blist[b],nlist);
	free(blist);
	return; }


/* boxssalloc */
boxssptr boxssalloc(int dim) {
	boxssptr boxs;
	int d;

	boxs=NULL;
	CHECKMEM(boxs=(boxssptr) malloc(sizeof(struct boxsuperstruct)));
	boxs->condition=SCinit;
	boxs->sim=NULL;
	boxs->nlist=0;
	boxs->mpbox=0;
	boxs->boxsize=0;
	boxs->boxvol=0;
	boxs->nbox=0;
	boxs->side=NULL;
	boxs->min=NULL;
	boxs->size=NULL;
	boxs->blist=NULL;

	CHECKMEM(boxs->side=(int*) calloc(dim,sizeof(int)));
	for(d=0;d<dim;d++) boxs->side[d]=0;
	CHECKMEM(boxs->min=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++) boxs->min[d]=0;
	CHECKMEM(boxs->size=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++) boxs->size[d]=0;
	return boxs;

 failure:
	boxssfree(boxs);
	simLog(NULL,10,"Failed to allocate memory in boxssalloc");
	return NULL; }


/* boxssfree */
void boxssfree(boxssptr boxs) {
	if(!boxs) return;
	boxesfree(boxs->blist,boxs->nbox,boxs->nlist);
	free(boxs->size);
	free(boxs->min);
	free(boxs->side);
	free(boxs);
	return; }


/******************************************************************************/
/*************************** data structure output ****************************/
/******************************************************************************/


/* boxoutput */
void boxoutput(boxssptr boxs,int blo,int bhi,int dim) {
	int b,b2,b3,p,d,ll;
	boxptr bptr;
	simptr sim;

	sim=boxs->sim;
	simLog(sim,2,"INDIVIDUAL BOX PARAMETERS\n");
	if(!boxs) {
		simLog(sim,2," No box superstructure defined\n\n");
		return; }
	if(bhi<0 || bhi>boxs->nbox) bhi=boxs->nbox;
	for(b=blo;b<bhi;b++) {
		bptr=boxs->blist[b];
		simLog(sim,2," Box %i: indx=(",b);
		for(d=0;d<dim-1;d++) simLog(sim,2,"%i,",bptr->indx[d]);
		simLog(sim,2,"%i), nwall=%i\n",bptr->indx[d],bptr->nwall);

		simLog(sim,2,"  nneigh=%i midneigh=%i\n",bptr->nneigh,bptr->midneigh);
		if(bptr->neigh) {
			simLog(sim,2,"   neighbors:");
			for(b2=0;b2<bptr->nneigh;b2++) {
				b3=indx2addZV(bptr->neigh[b2]->indx,boxs->side,dim);
				simLog(sim,2," %i",b3); }
			simLog(sim,2,"\n"); }
		if(bptr->wpneigh) {
			simLog(sim,2,"  wrap code:");
			for(b2=0;b2<bptr->nneigh;b2++) simLog(sim,2," %i",bptr->wpneigh[b2]);
			simLog(sim,2,"\n"); }

		simLog(sim,2,"  %i panels",bptr->npanel);
		if(bptr->npanel) {
			simLog(sim,2,": ");
			for(p=0;p<bptr->npanel;p++) {
				simLog(sim,2," %s",bptr->panel[p]->pname); }}
		simLog(sim,2,"\n");

		simLog(sim,2,"  %i live lists:\n",boxs->nlist);
		simLog(sim,2,"   max:");
		for(ll=0;ll<boxs->nlist;ll++) simLog(sim,2," %i",bptr->maxmol[ll]);
		simLog(sim,2,"\n   size:");
		for(ll=0;ll<boxs->nlist;ll++) simLog(sim,2," %i",bptr->nmol[ll]);
		simLog(sim,2,"\n"); }

	if(b<boxs->nbox) simLog(sim,2," ...\n");
	simLog(sim,2,"\n");
	return; }


/* boxssoutput */
void boxssoutput(simptr sim) {
	int dim,d,ll;
	boxssptr boxs;
	double flt1;

	simLog(sim,2,"VIRTUAL BOX PARAMETERS\n");
	if(!sim || !sim->boxs) {
		simLog(sim,2," No box superstructure defined\n\n");
		return; }
	dim=sim->dim;
	boxs=sim->boxs;
	simLog(sim,2," %i boxes\n",boxs->nbox);
	simLog(sim,2," Number of boxes on each side:");
	for(d=0;d<dim;d++) simLog(sim,2," %i",boxs->side[d]);
	simLog(sim,2,"\n");
	simLog(sim,1," Minimum box position: ");
	for(d=0;d<dim;d++) simLog(sim,1," %g",boxs->min[d]);
	simLog(sim,1,"\n");
	if(boxs->boxsize) simLog(sim,2," Requested box width: %g\n",boxs->boxsize);
	if(boxs->mpbox) simLog(sim,2," Requested molecules per box: %g\n",boxs->mpbox);
	simLog(sim,2," Box dimensions: ");
	for(d=0;d<dim;d++) simLog(sim,2," %g",boxs->size[d]);
	simLog(sim,2,"\n");
	if(boxs->boxvol>0)
		simLog(sim,2," Box volumes: %g\n",boxs->boxvol);
	else
		simLog(sim,2," Box volumes not computed\n");

	if(sim->mols) {
		flt1=0;
		for(ll=0;ll<sim->mols->nlist;ll++)
			if(sim->mols->listtype[ll]==MLTsystem) flt1+=sim->mols->nl[ll];
		flt1/=boxs->nbox;
		simLog(sim,2," Molecules per box= %g\n",flt1);
		simLog(sim,2,"\n"); }

	return; }


/* checkboxparams */
int checkboxparams(simptr sim,int *warnptr) {
	int error,warn,b,dim,nmolec,ll;
	boxssptr boxs;
	double mpbox;
	char string[STRCHAR];
	boxptr bptr;

	error=warn=0;
	boxs=sim->boxs;
	dim=sim->dim;
	if(!boxs) {
		warn++;
		simLog(sim,9," WARNING: no box structure defined\n");
		if(warnptr) *warnptr=warn;
		return 0; }

	if(boxs->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: box structure %s\n",simsc2string(boxs->condition,string)); }

	if(boxs->mpbox>100) {																// mpbox value
		warn++;
		simLog(sim,5," WARNING: requested molecules per box, %g, is very high\n",boxs->mpbox); }
	else if(boxs->mpbox>0 && boxs->mpbox<1) {
		warn++;
		simLog(sim,5," WARNING: requested molecules per box, %g, is very low\n",boxs->mpbox); }
	mpbox=boxs->mpbox>0?boxs->mpbox:10;

	for(b=0;b<boxs->nbox;b++) {
		bptr=boxs->blist[b];
		if(sim->mols) {
			nmolec=0;																					// actual molecs per box
			for(ll=0;ll<sim->mols->nlist;ll++) nmolec+=bptr->nmol[ll];
			if(nmolec>10*mpbox) {
				warn++;
				simLog(sim,5," WARNING: box (%s) has %i molecules in it, which is very high\n",Zn_vect2csvstring(bptr->indx,dim,string),nmolec); }}

		if(bptr->npanel>20) {
			warn++;
			simLog(sim,5," WARNING: box (%s) has %i panels in it, which is very high\n",Zn_vect2csvstring(bptr->indx,dim,string),bptr->npanel); }}

	if(warnptr) *warnptr=warn;
	return error; }



/******************************************************************************/
/********************************* structure set up ***************************/
/******************************************************************************/


/* boxsetcondition */
void boxsetcondition(boxssptr boxs,enum StructCond cond,int upgrade) {
	if(!boxs) return;
	if(upgrade==0 && boxs->condition>cond) boxs->condition=cond;
	else if(upgrade==1 && boxs->condition<cond) boxs->condition=cond;
	else if(upgrade==2) boxs->condition=cond;
	if(boxs->sim && boxs->condition<boxs->sim->condition) {
		cond=boxs->condition;
		simsetcondition(boxs->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* boxsetsize */
int boxsetsize(simptr sim,const char *info,double val) {
	boxssptr boxs;

	if(val<=0) return 2;
	if(!sim->boxs) {
		if(!sim->dim) return 3;
		boxs=boxssalloc(sim->dim);
		if(!boxs) return 1;
		boxs->sim=sim;
		sim->boxs=boxs;
		boxsetcondition(boxs,SCinit,0); }
	else
		boxs=sim->boxs;
	if(!strcmp(info,"molperbox")) boxs->mpbox=val;
	else if(!strcmp(info,"boxsize")) boxs->boxsize=val;
	else return 2;
	boxsetcondition(boxs,SClists,0);
	return 0; }


/* boxesupdateparams */
int boxesupdateparams(simptr sim) {
	int m,mlo,mhi,nbox,b,ll,ll1,mxml,er,npanel;
	boxssptr boxs;
	boxptr *blist,bptr;
	int nsrf,s,p;
	surfaceptr srf;
	moleculeptr mptr,*mlist;
	enum PanelShape ps;
	panelptr pnl;

	boxs=sim->boxs;
	nbox=boxs->nbox;
	blist=boxs->blist;

	if(sim->srfss) {
		for(b=0;b<nbox;b++)
			blist[b]->npanel=0;
		nsrf=sim->srfss->nsrf;
		for(b=0;b<nbox;b++) {														// box->npanel, panel
			bptr=blist[b];
			npanel=0;
			for(s=0;s<nsrf;s++) {
				srf=sim->srfss->srflist[s];
				for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
					for(p=0;p<srf->npanel[ps];p++) {
						pnl=srf->panels[ps][p];
						if(panelinbox(sim,pnl,bptr)) npanel++; }}
			if(npanel && npanel>bptr->maxpanel) {
				er=expandboxpanels(bptr,npanel-bptr->maxpanel);
				if(er) return 1; }
			if(npanel) {
				for(s=0;s<nsrf;s++) {
					srf=sim->srfss->srflist[s];
					for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
						for(p=0;p<srf->npanel[ps];p++)
							if(panelinbox(sim,srf->panels[ps][p],bptr))
								bptr->panel[bptr->npanel++]=srf->panels[ps][p]; }}}}

	if(sim->mols) {												// mptr->box, box->maxmol, nmol, mol
		if(sim->mols->condition<SCparams) return 2;
		for(b=0;b<nbox;b++)									// clear out molecule lists in boxes
			for(ll=0;ll<boxs->nlist;ll++)
				blist[b]->nmol[ll]=0;
		for(ll1=-1;ll1<boxs->nlist;ll1++) {
			if(ll1==-1) {
				mlo=sim->mols->topd;
				mhi=sim->mols->nd;
				mlist=sim->mols->dead; }
			else {
				mlo=0;
				mhi=sim->mols->nl[ll1];
				mlist=sim->mols->live[ll1]; }
			for(m=mlo;m<mhi;m++) {
				mptr=mlist[m];
				if(mptr->ident>0) {
					bptr=pos2box(sim,mptr->pos);
					mptr->box=bptr;
					ll=sim->mols->listlookup[mptr->ident][mptr->mstate];
					bptr->nmol[ll]++; }}}

		for(b=0;b<nbox;b++) {
			bptr=blist[b];
			for(ll=0;ll<boxs->nlist;ll++) {
				mxml=bptr->nmol[ll];
				bptr->nmol[ll]=0;
				if(bptr->maxmol[ll]<mxml) {
					er=expandbox(bptr,(int)(mxml*1.5-bptr->maxmol[ll]),ll);
					if(er) return 1; }}}
		for(ll1=0;ll1<boxs->nlist;ll1++) {
			mlo=0;
			mhi=sim->mols->nl[ll1];
			mlist=sim->mols->live[ll1];
			for(m=mlo;m<mhi;m++) {
				mptr=mlist[m];
				if(mptr->ident>0) {
					ll=sim->mols->listlookup[mptr->ident][mptr->mstate];
					bptr=mptr->box;
					bptr->mol[ll][bptr->nmol[ll]++]=mptr; }}}}

	return 0; }


/* boxesupdatelists */
int boxesupdatelists(simptr sim) {
	int dim,d,nbox,b,b2,w,er,nneigh,nwall;
	int *side,*indx;
	boxssptr boxs;
	boxptr *blist,bptr;
	double flt1,flt2,mpbox;
	int *ntemp,*wptemp;

	dim=sim->dim;
	boxs=sim->boxs;

	if(!boxs) {																			// create superstructure if needed
		er=boxsetsize(sim,"molperbox",4);
		if(er) return 1;
		boxs=sim->boxs; }
	
	if(sim->mols && sim->mols->condition<SCparams) return 2;
	if(boxs->blist) {																// box superstructure
		boxesfree(boxs->blist,boxs->nbox,boxs->nlist);
		boxs->nbox=0; }
	side=boxs->side;
	mpbox=boxs->mpbox;
	if(mpbox<=0 && boxs->boxsize<=0) mpbox=5;
	if(mpbox>0) {
		flt1=systemvolume(sim);
		flt2=(double)molcount(sim,-5,NULL,MSall,-1);
		flt1=pow(flt2/mpbox/flt1,1.0/dim); }
	else flt1=1.0/boxs->boxsize;
	nbox=1;
	for(d=0;d<dim;d++) {
		boxs->min[d]=sim->wlist[2*d]->pos;
		side[d]=(int)ceil((sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos)*flt1);
		if(!side[d]) side[d]=1;
		boxs->size[d]=(sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos)/side[d];
		nbox*=side[d]; }
	boxs->boxvol=1.0;
	for(d=0;d<dim;d++) boxs->boxvol*=boxs->size[d];

	boxs->nlist=sim->mols?sim->mols->nlist:0;					// individual boxes
	boxs->nbox=nbox;
	blist=boxs->blist=boxesalloc(nbox,dim,boxs->nlist);
	if(!blist) return 1;

	for(b=0;b<nbox;b++) add2indxZV(b,blist[b]->indx,side,dim);	// box->indx

	if(sim->accur>=3) {
		ntemp=allocZV(intpower(3,dim)-1);								// neigh
		wptemp=allocZV(intpower(3,dim)-1);
		if(!ntemp || !wptemp) return 1;
		for(b=0;b<nbox;b++) {
			bptr=blist[b];
			indx=bptr->indx;
			if(sim->accur<6)
				nneigh=neighborZV(indx,ntemp,side,dim,0,NULL,&bptr->midneigh);
			else if(sim->accur<9) {
				for(w=0;w<2*dim;w++) wptemp[w]=(sim->wlist[w]->type=='p' && sim->srfss==NULL);
				nneigh=neighborZV(indx,ntemp,side,dim,6,wptemp,&bptr->midneigh); }
			else {
				for(w=0;w<2*dim;w++) wptemp[w]=(sim->wlist[w]->type=='p' && sim->srfss==NULL);
				nneigh=neighborZV(indx,ntemp,side,dim,7,wptemp,&bptr->midneigh); }
			if(nneigh==-1) return 1;
			w=0;
			if(sim->accur>=6) for(b2=0;b2<nneigh;b2++) w+=wptemp[b2];

			if(nneigh && nneigh!=bptr->nneigh) {
				free(bptr->neigh);
				free(bptr->wpneigh);
				bptr->wpneigh=NULL;
				bptr->neigh=(boxptr*) calloc(nneigh,sizeof(boxptr));
				if(!bptr->neigh) return 1;
				if(w) {
					bptr->wpneigh=allocZV(nneigh);
					if(!bptr->wpneigh) return 1; }
				bptr->nneigh=nneigh; }
			if(nneigh)
				for(b2=0;b2<nneigh;b2++) bptr->neigh[b2]=blist[ntemp[b2]];
			if(w) {
				for(b2=0;b2<nneigh;b2++) bptr->wpneigh[b2]=wptemp[b2]; }}
		
		neighborZV(NULL,NULL,NULL,0,-1,NULL,NULL);
		freeZV(ntemp);
		freeZV(wptemp); }
	
	for(b=0;b<nbox;b++) {														// box->nwall, wlist
		bptr=blist[b];
		indx=bptr->indx;
		nwall=0;
		for(d=0;d<dim;d++) {
			if(indx[d]==0) nwall++;
			if(indx[d]==side[d]-1) nwall++; }
		if(nwall && nwall!=bptr->nwall) {
			free(bptr->wlist);
			bptr->wlist=(wallptr*) calloc(nwall,sizeof(wallptr));
			if(!bptr->wlist) return 1;
			bptr->nwall=nwall; }
		if(nwall) {
			w=0;
			for(d=0;d<dim;d++) {
				if(indx[d]==0) bptr->wlist[w++]=sim->wlist[2*d];
				if(indx[d]==side[d]-1) bptr->wlist[w++]=sim->wlist[2*d+1]; }}}

		return 0; }


/* boxesupdate */
int boxesupdate(simptr sim) {
	int er;

	if(sim->dim<=0 || !sim->wlist) return 3;

	if(!sim->boxs || sim->boxs->condition<=SClists) {
		er=boxesupdatelists(sim);
		if(er) return er;
		boxsetcondition(sim->boxs,SCparams,1); }

	if(sim->boxs->condition==SCparams) {
		er=boxesupdateparams(sim);
		if(er) return er;
		boxsetcondition(sim->boxs,SCok,1); }
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* line2nextbox */
boxptr line2nextbox(simptr sim,double *pt1,double *pt2,boxptr bptr) {
	int dim,d,d2,boxside,boxside2,adrs,z1[DIMMAX],*side,sum,flag;
	double *size,*min,crsmin,edge,crs;

	if(pos2box(sim,pt2)==bptr) return NULL;
	dim=sim->dim;
	size=sim->boxs->size;
	side=sim->boxs->side;
	min=sim->boxs->min;
	crsmin=1.01;
	boxside2=0;
	d2=0;
	flag=0;
	for(d=0;d<dim;d++) {
		z1[d]=bptr->indx[d];
		if(pt2[d]!=pt1[d]) {
			boxside=(pt2[d]>pt1[d])?1:0;		// 1 for high side, 0 for low side
			sum=bptr->indx[d]+boxside;
			if(sum>0 && sum<side[d]) {
				edge=min[d]+(double)sum*size[d];		// absolute location of potential edge crossing
				crs=(edge-pt1[d])/(pt2[d]-pt1[d]);	// relative position of potential edge crossing on line
				if(crs<crsmin) {
					crsmin=crs;
					d2=d;
					boxside2=boxside;
					flag=0; }
				else if(crs==crsmin) {
					if(boxside==0 && boxside2==0 && (flag==0 || flag==2)) flag=2;
					else flag=1; }}}}

	if(flag) {
		for(d=0;d<dim;d++) {
			if(pt2[d]!=pt1[d]) {
				boxside=(pt2[d]>pt1[d])?1:0;
				sum=bptr->indx[d]+boxside;
				if(sum>0 && sum<side[d]) {
					edge=min[d]+(double)sum*size[d];
					crs=(edge-pt1[d])/(pt2[d]-pt1[d]);
					if(crs==crsmin && (boxside==1 || flag==2))
						z1[d]+=boxside?1:-1; }}}
		adrs=indx2addZV(z1,sim->boxs->side,dim);
		return sim->boxs->blist[adrs]; }

	if(crsmin==1.01) return NULL;
	z1[d2]+=boxside2?1:-1;
	adrs=indx2addZV(z1,sim->boxs->side,dim);
	return sim->boxs->blist[adrs]; }


/* reassignmolecs */
int reassignmolecs(simptr sim,int diffusing,int reborn) {
	int m,nmol,m2,ll,b,s;
	boxptr bptr1;
	boxssptr boxss;
	surfacessptr srfss;
	moleculeptr mptr,*mlist,*mlist2;
	surfaceptr srf;

	if(!sim->mols) return 0;
	boxss=sim->boxs;
	srfss=sim->srfss;

	if(!reborn) {						// all molecules
		for(ll=0;ll<sim->mols->nlist;ll++)
			if(sim->mols->listtype[ll]==MLTsystem)
				if(diffusing==0 || sim->mols->diffuselist[ll]==1) {
					for(b=0;b<boxss->nbox;b++)				// clear out box list
						boxss->blist[b]->nmol[ll]=0;
					if(srfss)
						for(s=0;s<srfss->nsrf;s++)			// clear out surface list
							srfss->srflist[s]->nmol[ll]=0;
					nmol=sim->mols->nl[ll];
					mlist=sim->mols->live[ll];
					for(m=0;m<nmol;m++) {
						mptr=mlist[m];
						bptr1=pos2box(sim,mptr->pos);
						mptr->box=bptr1;								// add to new box
						if(bptr1->nmol[ll]==bptr1->maxmol[ll])
							if(expandbox(bptr1,1+bptr1->nmol[ll],ll)) return 1;
						bptr1->mol[ll][bptr1->nmol[ll]++]=mptr;
						if(mptr->pnl) {									// add to surface
							srf=mptr->pnl->srf;
							if(srf->nmol[ll]==srf->maxmol[ll])
								if(surfexpandmollist(srf,2*srf->nmol[ll]+1,ll)) return 1;
							srf->mol[ll][srf->nmol[ll]++]=mptr; }}}}

	else {										// reborn molecules only
		for(ll=0;ll<sim->mols->nlist;ll++)
			if(sim->mols->listtype[ll]==MLTsystem)
				if(diffusing==0 || sim->mols->diffuselist[ll]==1) {
					nmol=sim->mols->nl[ll];
					mlist=sim->mols->live[ll];
					for(m=sim->mols->topl[ll];m<nmol;m++) {
						mptr=mlist[m];
						bptr1=pos2box(sim,mptr->pos);
						if(mptr->box!=bptr1) {
							mlist2=mptr->box->mol[ll];		// remove from current box
							for(m2=0;mlist2[m2]!=mptr;m2++);
							mlist2[m2]=mlist2[--mptr->box->nmol[ll]];
							mptr->box=bptr1;								// add to new box
							if(bptr1->nmol[ll]==bptr1->maxmol[ll])
								if(expandbox(bptr1,1+bptr1->nmol[ll],ll)) return 1;
							bptr1->mol[ll][bptr1->nmol[ll]++]=mptr; }
						if(mptr->pnl) {									// add to surface
							srf=mptr->pnl->srf;						// there is no check for prior listing on a surface because that is impossible
							if(srf->nmol[ll]==srf->maxmol[ll])
								if(surfexpandmollist(srf,2*srf->nmol[ll]+1,ll)) return 1;
							srf->mol[ll][srf->nmol[ll]++]=mptr; }}}}
	return 0; }



