/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "string2.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"


#ifdef WINDOWS_BUILD
	#define DEVNULL "> NUL"
#else
	#define DEVNULL "> /dev/null"
#endif


/******************************************************************************/
/*********************************** BioNetGen ********************************/
/******************************************************************************/



/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// memory management

bngptr bngalloc(bngptr bng,int maxparams,int maxmonomer,int maxbspecies,int maxbrxns);
void bngfree(bngptr bng);
bngssptr bngssalloc(bngssptr bngss,int maxbng);
void bngssfree(bngssptr bngss);

// structure setup

void bngsetcondition(bngssptr bngss,enum StructCond cond,int upgrade);
int bngenablebng(simptr sim,int maxbng);
bngptr bngaddbng(simptr sim,const char *bngname);
int bngparseparameter(bngptr bng,int index);
int bngaddparameter(bngptr bng,const char *name,const char *string);
int bngaddmonomer(bngptr bng,const char *name,enum MolecState ms);

int bngmakeshortname(bngptr bng,int index,int totalmn,int hasmods);
enum MolecState bngmakedefaultstate(bngptr bng,int index,int totalmn);
double bngmakedifc(bngptr bng,int index,int totalmn);

int bngaddspecies(bngptr bng,int bindex,const char *longname,const char *countstr);
int bngaddreaction(bngptr bng,int bindex,const char *reactants,const char *products,const char *rate);
int bngaddgroup(bngptr bng,int gindex,const char *gname,const char *specieslist);
bngptr bngreadstring(simptr sim,ParseFilePtr pfp,bngptr bng,const char *word,char *line2);
int bngupdateparams(simptr sim);
int bngupdatelists(simptr sim);
int bngupdate(simptr sim);



/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/


/* bngallocsurfacedata */
void bngallocsurfacedata(bngptr bng,int maxsurface) {
	int oldmaxsurface,s,i;
	enum SrfAction **newmonomeraction;
	surfactionptr **newmonomeractdetails;
	enum PanelFace face;

	oldmaxsurface=bng->bngmaxsurface;
	for(i=0;i<bng->maxmonomer;i++) {
		if(!bng->monomeraction[i]) {
			CHECKMEM(bng->monomeraction[i]=(enum SrfAction**) calloc(maxsurface,sizeof(enum SrfAction*)));
			CHECKMEM(bng->monomeractdetails[i]=(surfactionptr **) calloc(maxsurface,sizeof(surfactionptr*)));
			for(s=0;s<maxsurface;s++) {
				bng->monomeraction[i][s]=NULL;
				bng->monomeractdetails[i][s]=NULL; }
			for(s=0;s<maxsurface;s++) {
				CHECKMEM(bng->monomeraction[i][s]=(enum SrfAction*) calloc(3,sizeof(enum SrfAction)));
				CHECKMEM(bng->monomeractdetails[i][s]=(surfactionptr*) calloc(3,sizeof(surfactionptr)));
				for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1)) {
					bng->monomeraction[i][s][face]=SAtrans;
					bng->monomeractdetails[i][s][face]=NULL; }}}

		else if(oldmaxsurface<maxsurface) {
			CHECKMEM(newmonomeraction=(enum SrfAction**) calloc(maxsurface,sizeof(enum SrfAction*)));
			CHECKMEM(newmonomeractdetails=(surfactionptr **) calloc(maxsurface,sizeof(surfactionptr*)));
			for(s=0;s<oldmaxsurface;s++) {
				newmonomeraction=bng->monomeraction[i];
				newmonomeractdetails=bng->monomeractdetails[i]; }
			for(s=oldmaxsurface;s<maxsurface;s++) {
				newmonomeraction[s]=NULL;
				newmonomeractdetails[s]=NULL; }
			for(s=oldmaxsurface;s<maxsurface;s++) {
				CHECKMEM(newmonomeraction[s]=(enum SrfAction*) calloc(3,sizeof(enum SrfAction)));
				CHECKMEM(newmonomeractdetails[s]=(surfactionptr*) calloc(3,sizeof(surfactionptr)));
				for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1)) {
					newmonomeraction[s][face]=SAtrans;
					newmonomeractdetails[s][face]=NULL; }}
			free(bng->monomeraction[i]);
			free(bng->monomeractdetails[i]);
			bng->monomeraction[i]=newmonomeraction;
			bng->monomeractdetails[i]=newmonomeractdetails; }}

		bng->bngmaxsurface=maxsurface;
	return;
 failure:
	return; }


/* bngalloc */
bngptr bngalloc(bngptr bng,int maxparams,int maxmonomer,int maxbspecies,int maxbrxns) {
	int i,newmax,oldmax;
	char **strlist;
	double *dbllist,**dblptrlist;
  int *intlist,**intptrlist;
  rxnptr *rxnlist;
	enum MolecState *mslist;
	enum SrfAction ***salist;
	surfactionptr ***sadlist;

	newmax=oldmax=0;
	strlist=NULL;
	dbllist=NULL;
  intlist=NULL;

	if(!bng) {
		bng=(bngptr)malloc(sizeof(struct bngstruct));
		CHECKMEM(bng);
    bng->bngss=NULL;
		bng->bngname=NULL;
    bng->bngindex=0;
		bng->unirate=1;
		bng->birate=1;
		bng->maxparams=0;
		bng->nparams=0;
		bng->paramnames=NULL;
    bng->paramstrings=NULL;
		bng->paramvalues=NULL;
    bng->maxmonomer=0;
    bng->nmonomer=0;
    bng->monomernames=NULL;
    bng->monomercount=NULL;
		bng->monomerdifc=NULL;
		bng->monomerdisplaysize=NULL;
		bng->monomercolor=NULL;
		bng->monomerstate=NULL;
		bng->bngmaxsurface=0;
		bng->monomeraction=NULL;
		bng->monomeractdetails=NULL;
		bng->maxbspecies=0;
		bng->nbspecies=0;
		bng->bsplongnames=NULL;
		bng->bspshortnames=NULL;
		bng->bspstate=NULL;
		bng->bspcountstr=NULL;
		bng->bspcount=NULL;
		bng->spindex=NULL;
		bng->maxbrxns=0;
		bng->nbrxns=0;
		bng->brxnreactstr=NULL;
		bng->brxnprodstr=NULL;
    bng->brxnratestr=NULL;
    bng->brxnreact=NULL;
    bng->brxnprod=NULL;
    bng->brxnorder=NULL;
    bng->brxnnprod=NULL;
    bng->brxn=NULL; }

	if(maxparams>bng->maxparams) {					// enlarge params
		newmax=maxparams;
		oldmax=bng->maxparams;
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->paramnames[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->paramnames);
		bng->paramnames=strlist;
    
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->paramstrings[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->paramstrings);
		bng->paramstrings=strlist;
    
		CHECKMEM(dbllist=(double*)calloc(newmax,sizeof(double)));
		for(i=0;i<oldmax;i++) dbllist[i]=bng->paramvalues[i];
		for(;i<newmax;i++) dbllist[i]=0;
		free(bng->paramvalues);
		bng->paramvalues=dbllist;

    bng->maxparams=newmax; }
  
  if(maxmonomer>bng->maxmonomer) {          // enlarge monomers
		newmax=maxmonomer;
		oldmax=bng->maxmonomer;
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->monomernames[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->monomernames);
		bng->monomernames=strlist;

    CHECKMEM(intlist=(int*)calloc(newmax,sizeof(int)));
		for(i=0;i<oldmax;i++) intlist[i]=bng->monomercount[i];
		for(;i<newmax;i++) intlist[i]=0;
		free(bng->monomercount);
		bng->monomercount=intlist;

		CHECKMEM(dbllist=(double*)calloc(newmax,sizeof(double)));
		for(i=0;i<oldmax;i++) dbllist[i]=bng->monomerdifc[i];
		for(;i<newmax;i++) dbllist[i]=0;
		free(bng->monomerdifc);
		bng->monomerdifc=dbllist;

		CHECKMEM(dbllist=(double*)calloc(newmax,sizeof(double)));
		for(i=0;i<oldmax;i++) dbllist[i]=bng->monomerdisplaysize[i];
		for(;i<newmax;i++) dbllist[i]=0;
		free(bng->monomerdisplaysize);
		bng->monomerdisplaysize=dbllist;

		CHECKMEM(dblptrlist=(double**)calloc(newmax,sizeof(double *)));
		for(i=0;i<oldmax;i++) dblptrlist[i]=bng->monomercolor[i];
		for(;i<newmax;i++) {
			CHECKMEM(dblptrlist[i]=(double*)calloc(3,sizeof(double)));
			dblptrlist[i][0]=0;
			dblptrlist[i][1]=0;
			dblptrlist[i][2]=0; }
		free(bng->monomercolor);
		bng->monomercolor=dblptrlist;

		CHECKMEM(mslist=(enum MolecState *)calloc(newmax,sizeof(enum MolecState)));
		for(i=0;i<oldmax;i++) mslist[i]=bng->monomerstate[i];
		for(;i<newmax;i++) mslist[i]=MSsoln;
		free(bng->monomerstate);
		bng->monomerstate=mslist;

		CHECKMEM(salist=(enum SrfAction ***)calloc(newmax,sizeof(enum SrfAction **)));
		for(i=0;i<oldmax;i++) salist[i]=bng->monomeraction[i];
		for(;i<newmax;i++) salist[i]=NULL;
		free(bng->monomeraction);
		bng->monomeraction=salist;

		CHECKMEM(sadlist=(surfactionptr ***)calloc(newmax,sizeof(surfactionptr **)));
		for(i=0;i<oldmax;i++) sadlist[i]=bng->monomeractdetails[i];
		for(;i<newmax;i++) sadlist[i]=NULL;
		free(bng->monomeractdetails);
		bng->monomeractdetails=sadlist;

		bng->maxmonomer=newmax; }

	if(maxbspecies>bng->maxbspecies) {				// enlarge bspecies
		newmax=maxbspecies;
		oldmax=bng->maxbspecies;
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->bsplongnames[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->bsplongnames);
		bng->bsplongnames=strlist;

    CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->bspshortnames[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->bspshortnames);
		bng->bspshortnames=strlist;

		CHECKMEM(mslist=(enum MolecState *)calloc(newmax,sizeof(enum MolecState)));
		for(i=0;i<oldmax;i++) mslist[i]=bng->bspstate[i];
		for(;i<newmax;i++) mslist[i]=MSsoln;
		free(bng->bspstate);
		bng->bspstate=mslist;

    CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->bspcountstr[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->bspcountstr);
		bng->bspcountstr=strlist;

		CHECKMEM(dbllist=(double*)calloc(newmax,sizeof(double)));
		for(i=0;i<oldmax;i++) dbllist[i]=bng->bspcount[i];
		for(;i<newmax;i++) dbllist[i]=0;
		free(bng->bspcount);
		bng->bspcount=dbllist;

    CHECKMEM(intlist=(int*)calloc(newmax,sizeof(int)));
		for(i=0;i<oldmax;i++) intlist[i]=bng->spindex[i];
		for(;i<newmax;i++) intlist[i]=0;
		free(bng->spindex);
		bng->spindex=intlist;

    bng->maxbspecies=newmax; }

	if(maxbrxns>bng->maxbrxns) {				// enlarge brxns
		newmax=maxbrxns;
		oldmax=bng->maxbrxns;
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->brxnreactstr[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->brxnreactstr);
		bng->brxnreactstr=strlist;
    
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->brxnprodstr[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->brxnprodstr);
		bng->brxnprodstr=strlist;
    
		CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bng->brxnratestr[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bng->brxnratestr);
		bng->brxnratestr=strlist;

    CHECKMEM(intptrlist=(int**)calloc(newmax,sizeof(int *)));
		for(i=0;i<oldmax;i++) intptrlist[i]=bng->brxnreact[i];
		for(;i<newmax;i++) {
      CHECKMEM(intptrlist[i]=(int*)calloc(2,sizeof(int)));
      intptrlist[i][0]=0;
      intptrlist[i][1]=0; }
		free(bng->brxnreact);
		bng->brxnreact=intptrlist;

    CHECKMEM(intptrlist=(int**)calloc(newmax,sizeof(int *)));
		for(i=0;i<oldmax;i++) intptrlist[i]=bng->brxnprod[i];
		for(;i<newmax;i++) {
      CHECKMEM(intptrlist[i]=(int*)calloc(2,sizeof(int)));
      intptrlist[i][0]=0;
      intptrlist[i][1]=0; }
		free(bng->brxnprod);
		bng->brxnprod=intptrlist;
    
    CHECKMEM(intlist=(int*)calloc(newmax,sizeof(int)));
		for(i=0;i<oldmax;i++) intlist[i]=bng->brxnorder[i];
		for(;i<newmax;i++) intlist[i]=0;
		free(bng->brxnorder);
		bng->brxnorder=intlist;
    
    CHECKMEM(intlist=(int*)calloc(newmax,sizeof(int)));
		for(i=0;i<oldmax;i++) intlist[i]=bng->brxnnprod[i];
		for(;i<newmax;i++) intlist[i]=0;
		free(bng->brxnnprod);
		bng->brxnnprod=intlist;
    
    CHECKMEM(rxnlist=(rxnptr*)calloc(newmax,sizeof(rxnptr)));
		for(i=0;i<oldmax;i++) rxnlist[i]=bng->brxn[i];
		for(;i<newmax;i++) rxnlist[i]=NULL;
		free(bng->brxn);
		bng->brxn=rxnlist;

    bng->maxbrxns=newmax; }
  
		return bng;
failure:
	bngfree(bng);
	simLog(NULL,10,"Unable to allocate memory in bngalloc");
	return NULL; }


/* bngfree */
void bngfree(bngptr bng) {
	int i,s;

	if(!bng) return;

	for(i=0;i<bng->maxbrxns;i++) {
		free(bng->brxnreactstr[i]);
		free(bng->brxnprodstr[i]);
    free(bng->brxnratestr[i]);
    free(bng->brxnreact[i]);
    free(bng->brxnprod[i]); }
	free(bng->brxnreactstr);
	free(bng->brxnprodstr);
  free(bng->brxnreact);
  free(bng->brxnprod);
  free(bng->brxnorder);
  free(bng->brxnnprod);
  free(bng->brxn);

	for(i=0;i<bng->maxbspecies;i++) {
		free(bng->bsplongnames[i]);
		free(bng->bspshortnames[i]);
		free(bng->bspcountstr[i]); }
	free(bng->bsplongnames);
	free(bng->bspshortnames);
	free(bng->bspstate);
	free(bng->bspcountstr);
	free(bng->bspcount);
	free(bng->spindex);

  for(i=0;i<bng->maxmonomer;i++) {
    free(bng->monomernames[i]); }
  free(bng->monomernames);
  free(bng->monomercount);
	free(bng->monomerdifc);
	free(bng->monomerdisplaysize);
	for(i=0;i<bng->maxmonomer;i++)
		free(bng->monomercolor[i]);
	free(bng->monomercolor);
	free(bng->monomerstate);
	for(i=0;i<bng->maxmonomer;i++) {
		if(bng->monomeraction[i])
			for(s=0;s<bng->bngmaxsurface;s++) {
				free(bng->monomeraction[i][s]);
				free(bng->monomeractdetails[i][s]); }
		free(bng->monomeraction[i]);
		free(bng->monomeractdetails[i]); }
	free(bng->monomeraction);
	free(bng->monomeractdetails);

	for(i=0;i<bng->maxparams;i++) {
		free(bng->paramnames[i]);
		free(bng->paramstrings[i]); }
	free(bng->paramnames);
	free(bng->paramstrings);
	free(bng->paramvalues);

	free(bng);
	return; }


/* bngssalloc */
bngssptr bngssalloc(bngssptr bngss,int maxbng) {
	int i,newmax,oldmax;
	char **strlist;
	bngptr *newbnglist;

	if(!bngss) {
		CHECKMEM(bngss=(bngssptr) malloc(sizeof(struct bngsuperstruct)));
		bngss->condition=SCinit;
		bngss->sim=NULL;
		bngss->BNG2path=NULL;
		bngss->maxbng=0;
		bngss->nbng=0;
		bngss->bngnames=NULL;
		bngss->bnglist=NULL; }

	if(!bngss->BNG2path) {
		CHECKMEM(bngss->BNG2path=EmptyString());
		strcpy(bngss->BNG2path,BNG2_PATH); }

	if(maxbng>bngss->maxbng) {
		newmax=maxbng;
		oldmax=bngss->maxbng;
    CHECKMEM(strlist=(char**)calloc(newmax,sizeof(char *)));
		for(i=0;i<oldmax;i++) strlist[i]=bngss->bngnames[i];
		for(;i<newmax;i++) {CHECKMEM(strlist[i]=EmptyString());}
		free(bngss->bngnames);
		bngss->bngnames=strlist;

    CHECKMEM(newbnglist=(bngptr*)calloc(newmax,sizeof(bngptr)));
		for(i=0;i<oldmax;i++) newbnglist[i]=bngss->bnglist[i];
		for(;i<newmax;i++) {
			CHECKMEM(newbnglist[i]=bngalloc(NULL,1,1,1,1));
			newbnglist[i]->bngss=bngss;
			newbnglist[i]->bngname=bngss->bngnames[i];
      newbnglist[i]->bngindex=i; }
		free(bngss->bnglist);
		bngss->bnglist=newbnglist;

    bngss->maxbng=newmax; }
	return bngss;

failure:
	bngssfree(bngss);
	simLog(NULL,10,"Unable to allocate memory in bngssalloc");
	return NULL; }


/* bngssfree */
void bngssfree(bngssptr bngss) {
	int i;

	if(!bngss) return;
	for(i=0;i<bngss->maxbng;i++) {
		bngfree(bngss->bnglist[i]);
		free(bngss->bngnames[i]); }
	free(bngss->bnglist);
	free(bngss->bngnames);
	free(bngss->BNG2path);
	free(bngss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* bngoutput */
void bngoutput(simptr sim) {
	bngssptr bngss;
	bngptr bng;
	int b,i,s;
	char string[STRCHAR];

	bngss=sim->bngss;
	if(!bngss) return;
	simLog(sim,2,"BioNetGen parameters\n");
	simLog(sim,2," BNG2.pl path: %s\n",bngss->BNG2path);
	simLog(sim,1," BNG allocated: %i,",bngss->maxbng);
	simLog(sim,2," BNG defined: %i\n",bngss->nbng);
	for(b=0;b<bngss->nbng;b++) {
		bng=bngss->bnglist[b];
		simLog(sim,2," BNG: %s\n",bng->bngname);
		if(bng->unirate!=1 || bng->birate!=1)
			simLog(sim,2,"  rate multipliers: unimolecular: %g, bimolecular: %g\n",bng->unirate,bng->birate);
		simLog(sim,1,"  parameters allocated: %i,",bng->maxparams);
		simLog(sim,2,"  parameters defined: %i\n",bng->nparams);
		for(i=0;i<bng->nparams;i++) {
			simLog(sim,2,"   %i %s %g\n",i,bng->paramnames[i],bng->paramvalues[i]); }
		simLog(sim,1,"  monomers allocated: %i,",bng->maxmonomer);
		simLog(sim,2,"  monomers defined: %i\n",bng->nmonomer);
		for(i=0;i<bng->nmonomer;i++) {
			simLog(sim,2,"   %s: default state: %s, diffusion coeff.: %g\n",bng->monomernames[i],molms2string(bng->monomerstate[i],string),bng->monomerdifc[i]);
			simLog(sim,2,"    display size: %g, color: %g %g %g\n",bng->monomerdisplaysize[i],bng->monomercolor[i][0],bng->monomercolor[i][1],bng->monomercolor[i][2]);
			for(s=0;s<bng->bngmaxsurface;s++) {
				simLog(sim,2,"    for surface %s: %s at front",sim->srfss->srflist[s]->sname,surfact2string(bng->monomeraction[i][s][PFfront],string));
				simLog(sim,2,", %s at back\n",surfact2string(bng->monomeraction[i][s][PFback],string)); }}
		simLog(sim,1,"  species allocated: %i,",bng->maxbspecies);
		simLog(sim,2,"  species defined: %i\n",bng->nbspecies-1);
		for(i=0;i<bng->nbspecies;i++) {
			if(bng->spindex[i]>0)
				simLog(sim,2,"   %i %s (%s), count: %g, longname: %s\n",i,bng->bspshortnames[i],molms2string(bng->bspstate[i],string),bng->bspcount[i],bng->bsplongnames[i]); }
		simLog(sim,1,"  reactions allocated: %i,",bng->maxbrxns);
		simLog(sim,2,"  reactions defined: %i\n",bng->nbrxns-1);
		for(i=0;i<bng->nbrxns;i++)
			if(bng->brxn[i]) {
				simLog(sim,2,"   %i",i);
				if(bng->brxnorder[i]>=1) simLog(sim,2," %s",bng->bspshortnames[bng->brxnreact[i][0]]);
				if(bng->brxnorder[i]==2) simLog(sim,2," + %s",bng->bspshortnames[bng->brxnreact[i][1]]);
				simLog(sim,2," ->");
				if(bng->brxnnprod[i]>=1) simLog(sim,2," %s",bng->bspshortnames[bng->brxnprod[i][0]]);
				if(bng->brxnnprod[i]==2) simLog(sim,2," + %s",bng->bspshortnames[bng->brxnprod[i][1]]);
				simLog(sim,2,"  rate: %g",bng->brxn[i]->rate);
				simLog(sim,2,"\n"); }}
	simLog(sim,2,"\n");
	return; }


/* checkbngparams */
int checkbngparams(simptr sim,int *warnptr) {
	int error,warn,b,i;
	bngssptr bngss;
	bngptr bng;
	char string[STRCHAR];

	error=warn=0;
	bngss=sim->bngss;
	if(!bngss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(bngss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: bng structure condition is %s\n",simsc2string(bngss->condition,string)); }

	for(b=0;b<bngss->nbng;b++) {
		bng=bngss->bnglist[b];
		for(i=0;i<bng->nbspecies;i++) {
			if(bng->bspcount[i]>0 && bng->bspcount[i]<1) simLog(sim,7," WARNING: count for %s is very low\n",bng->bspshortnames[i]); }}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/**************************** structure set up - bng **************************/
/******************************************************************************/


/* bngsetcondition */
void bngsetcondition(bngssptr bngss,enum StructCond cond,int upgrade) {
	if(!bngss) return;
	if(upgrade==0 && bngss->condition>cond) bngss->condition=cond;
	else if(upgrade==1 && bngss->condition<cond) bngss->condition=cond;
	else if(upgrade==2) bngss->condition=cond;
	if(bngss->sim && bngss->condition<bngss->sim->condition) {
		cond=bngss->condition;
		simsetcondition(bngss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* bngenablebng */
int bngenablebng(simptr sim,int maxbng) {
	bngssptr bngss;

	if(sim->bngss)									// check for redundant function call
		if(maxbng==-1 || sim->bngss->maxbng==maxbng)
			return 0;
	bngss=bngssalloc(sim->bngss,maxbng<0?1:maxbng);
	if(!bngss) return 1;
	sim->bngss=bngss;
	bngss->sim=sim;
	bngsetcondition(sim->bngss,SClists,0);
	return 0; }


/* bngaddbng */
bngptr bngaddbng(simptr sim,const char *bngname) {
	int er,i;
	bngssptr bngss;
	bngptr bng;

	if(!sim->bngss) {
	 er=bngenablebng(sim,-1);
	 if(er) return NULL; }
	bngss=sim->bngss;

	i=stringfind(bngss->bngnames,bngss->nbng,bngname);
	if(i<0) {
		if(bngss->nbng==bngss->maxbng) {
			er=bngenablebng(sim,bngss->nbng*2+1);
			if(er) return NULL; }
		i=bngss->nbng++;
		strncpy(bngss->bngnames[i],bngname,STRCHAR-1);
		bngss->bngnames[i][STRCHAR-1]='\0';
		bng=bngss->bnglist[i]; }
	else
		bng=bngss->bnglist[i];

	bngsetcondition(bngss,SClists,0);
	return bng; }


/* bngsetparam */
int bngsetparam(bngptr bng,char *parameter,double amount) {
	if(!strcmp(parameter,"unimolecular_rate")) {
		if(amount<0) return 2;
		bng->unirate=amount; }
	else if(!strcmp(parameter,"bimolecular_rate")) {
		if(amount<0) return 2;
		bng->birate=amount; }
	else
		return 1;
	return 0; }


/* bngsetBNG2path */
int bngsetBNG2path(bngptr bng,char *path) {
	strcpy(bng->bngss->BNG2path,path);
	return 0; }


/******************************************************************************/
/************************ structure set up - parameters ***********************/
/******************************************************************************/


/* bngparseparameter */
int bngparseparameter(bngptr bng,int index) {
  int er;
  double value;

  er=0;
  if(bng->paramstrings[index]) {
    value=strmatheval(bng->paramstrings[index],bng->paramnames,bng->paramvalues,bng->nparams);
    er=strmatherror(NULL,1);
    bng->paramvalues[index]=value; }
  return (er==0)?0:1; }


/* bngaddparameter */
int bngaddparameter(bngptr bng,const char *name,const char *string) {
	int i,er;

	if(bng->nparams==bng->maxparams) {		// enlarge list if needed
		bng=bngalloc(bng,bng->maxparams*2+1,0,0,0);
		if(!bng) return -1; }

  i=stringfind(bng->paramnames,bng->nparams,name);
  if(i<0) {
    i=bng->nparams;
    bng->nparams++;
    strcpy(bng->paramnames[i],name); }
  if(string) strcpy(bng->paramstrings[i],string);
  else bng->paramstrings[i][0]='\0';
  er=bngparseparameter(bng,i);
  return (er==0)?i:-2; }


/******************************************************************************/
/************************** structure set up - monomers ***********************/
/******************************************************************************/


/* bngaddmonomer */
int bngaddmonomer(bngptr bng,const char *name,enum MolecState ms) {
	simptr sim;
  int i,i2,j,n,newcount,len,s;
	char *newname,*spname;
	double newdifc,newdisplaysize,*newcolor;
	enum MolecState newstate;
	surfaceptr srf;
	enum SrfAction **newaction;
	surfactionptr **newactdetails;
	enum PanelFace face;

	sim=bng->bngss->sim;
  i=stringfind(bng->monomernames,bng->nmonomer,name);	// see if it's already in there

  if(i<0) {
		if(!strokname(name))
			return -2;
		if(ms==MSbsoln) ms=MSsoln;

		if(bng->nmonomer==bng->maxmonomer) {		// enlarge list if needed
			bng=bngalloc(bng,0,bng->maxmonomer*2+1,0,0);
			if(!bng) return -1; }

		bng->nmonomer++;
		n=bng->nmonomer;
		strcpy(bng->monomernames[n-1],name);					// put the new monomer at the end

		j=stringfind(sim->mols->spname,sim->mols->nspecies,name);	// look for monomer in Smoldyn species list
		if(j<=0) {																		// not there, so try Smoldyn species without dots
			for(j=1;j<sim->mols->nspecies;j++) {
				spname=sim->mols->spname[j];
				if(strsymbolcount(spname,'.')==2 && (len=strlen(name))==strchr(spname,'.')-spname && !strncmp(name,spname,len)) break; }
			if(j==sim->mols->nspecies) j=-1; }
		if(j>0) {																			// found Smoldyn species so add in data
			bng->monomerdifc[n-1]=sim->mols->difc[j][ms];
			bng->monomerdisplaysize[n-1]=sim->mols->display[j][ms];
			bng->monomercolor[n-1][0]=sim->mols->color[j][ms][0];
			bng->monomercolor[n-1][1]=sim->mols->color[j][ms][1];
			bng->monomercolor[n-1][2]=sim->mols->color[j][ms][2];
			if(sim->srfss) {														// assign surface interaction data
				if(bng->bngmaxsurface<sim->srfss->nsrf || !bng->monomeraction[n-1]) {
					bngallocsurfacedata(bng,sim->srfss->nsrf);
					if(bng->bngmaxsurface<sim->srfss->nsrf) return -1; }
				for(s=0;s<sim->srfss->nsrf;s++) {
					srf=sim->srfss->srflist[s];
					for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1)) {
						bng->monomeraction[n-1][s][face]=srf->action[j][ms][face];
						bng->monomeractdetails[n-1][s][face]=srf->actdetails[j][ms][face]; }}}}

		newname=bng->monomernames[n-1];								// create extra copy of the new stuff
		newcount=bng->monomercount[n-1];
		newdifc=bng->monomerdifc[n-1];
		newdisplaysize=bng->monomerdisplaysize[n-1];
		newcolor=bng->monomercolor[n-1];
		newstate=bng->monomerstate[n-1];
		newaction=bng->monomeraction[n-1];
		newactdetails=bng->monomeractdetails[n-1];

		for(i=0;i<n-1;i++)														// sort new monomer to where it belongs
			if(strcmp(newname,bng->monomernames[i])<0) {
				for(i2=n-2;i2>=i;i2--) {
					bng->monomernames[i2+1]=bng->monomernames[i2];
					bng->monomercount[i2+1]=bng->monomercount[i2];
					bng->monomerdifc[i2+1]=bng->monomerdifc[i2];
					bng->monomerdisplaysize[i2+1]=bng->monomerdisplaysize[i2];
					bng->monomercolor[i2+1]=bng->monomercolor[i2];
					bng->monomerstate[i2+1]=bng->monomerstate[i2];
					bng->monomeraction[i2+1]=bng->monomeraction[i2];
					bng->monomeractdetails[i2+1]=bng->monomeractdetails[i2]; }
				bng->monomernames[i]=newname;
				bng->monomercount[i]=newcount;
				bng->monomerdifc[i]=newdifc;
				bng->monomerdisplaysize[i]=newdisplaysize;
				bng->monomercolor[i]=newcolor;
				bng->monomerstate[i]=newstate;
				bng->monomeraction[i]=newaction;
				bng->monomeractdetails[i]=newactdetails;
				break; }}

  return i; }


/* bngsetmonomerdifc */
int bngsetmonomerdifc(bngptr bng,char *name,double difc) {
	int i;

	if(!strcmp(name,"all")) {
		for(i=0;i<bng->nmonomer;i++)
			bng->monomerdifc[i]=difc; }
	else {
		i=bngaddmonomer(bng,name,MSsoln);
		if(i<0) return i;
		bng->monomerdifc[i]=difc; }

	return 0; }


/* bngsetmonomerdisplaysize */
int bngsetmonomerdisplaysize(bngptr bng,char *name,double displaysize) {
	int i;

	if(!strcmp(name,"all")) {
		for(i=0;i<bng->nmonomer;i++)
			bng->monomerdisplaysize[i]=displaysize; }
	else {
		i=bngaddmonomer(bng,name,MSsoln);
		if(i<0) return i;
		bng->monomerdisplaysize[i]=displaysize; }

	return 0; }


/* bngsetmonomercolor */
int bngsetmonomercolor(bngptr bng,char *name,double *color) {
	int i;

	if(!strcmp(name,"all")) {
		for(i=0;i<bng->nmonomer;i++) {
			bng->monomercolor[i][0]=color[0];
			bng->monomercolor[i][1]=color[1];
			bng->monomercolor[i][2]=color[2]; }}
	else {
		i=bngaddmonomer(bng,name,MSsoln);
		if(i<0) return i;
		bng->monomercolor[i][0]=color[0];
		bng->monomercolor[i][1]=color[1];
		bng->monomercolor[i][2]=color[2]; }

	return 0; }


/* bngsetmonomerstate */
int bngsetmonomerstate(bngptr bng,char *name,enum MolecState ms) {
	int i;

	if(!strcmp(name,"all")) {
		for(i=0;i<bng->nmonomer;i++)
			bng->monomerstate[i]=ms; }
	else {
		i=bngaddmonomer(bng,name,ms);
		if(i<0) return i;
		bng->monomerstate[i]=ms; }

	return 0; }


/******************************************************************************/
/************************** structure set up - species ************************/
/******************************************************************************/


/* bngmakeshortname */
int bngmakeshortname(bngptr bng,int index,int totalmn,int hasmods) {
	char *shortname,string[STRCHAR],*lastdot,*cmpname;
	int length,mn,i1,i2,snlength,cmplength;

	shortname=bng->bspshortnames[index];				// generate the short name root, diffusion coefficient, and default state
	shortname[0]='\0';
	length=STRCHAR-20;													// length remaining in shortname string

	if(totalmn==1 && hasmods==0) {							// just 1 monomer and no modifications possible
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				strcpy(shortname,bng->monomernames[mn]);
				mn=bng->nmonomer; }}

	else {																			// multiple monomers or 1 monomer and possible modifications
		for(mn=0;mn<bng->nmonomer && length>0;mn++)
			if(bng->monomercount[mn]>0) {
				snprintf(string,STRCHAR,"%s.%i.",bng->monomernames[mn],bng->monomercount[mn]);
				string[length-1]='\0';
				strcat(shortname,string);
				length-=strlen(string); }

		i2=0;																			// append the short name isomer number
		snlength=strlen(shortname);
		for(i1=0;i1<index;i1++) {
			cmpname=bng->bspshortnames[i1];
			lastdot=strrchr(cmpname,'.');
			if(lastdot) {
				cmplength=lastdot-cmpname;
				if(!strncmp(shortname,cmpname,cmplength>snlength?cmplength:snlength)) i2++; }}
		snprintf(string,STRCHAR,"%i",i2);
		strcat(shortname,string); }

	return 0; }


/* bngmakedefaultstate */
enum MolecState bngmakedefaultstate(bngptr bng,int index,int totalmn) {
	enum MolecState ms,trialms;
	int mn;

	ms=MSsoln;

	if(totalmn==1) {      // just 1 monomer
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				ms=bng->monomerstate[mn];
				mn=bng->nmonomer; }}

	else {                // multiple monomers
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				trialms=bng->monomerstate[mn];
				if(ms==MSbsoln) {
					if(trialms!=MSsoln) ms=trialms; }
				else if(trialms==MSbsoln) {
					if(ms==MSsoln) ms=trialms; }
				else if(trialms>ms) ms=trialms; }}

	bng->bspstate[index]=ms;

	return ms; }


/* bngmakedifc */
double bngmakedifc(bngptr bng,int index,int totalmn) {
	double difc;
	int mn,j;
	simptr sim;
	enum MolecState ms;

	difc=-1;
	sim=bng->bngss->sim;
	j=stringfind(sim->mols->spname,sim->mols->nspecies,bng->bspshortnames[index]);	// look for species in Smoldyn species list

	if(j>0) {
		ms=bng->bspstate[index];
		if(ms==MSbsoln) ms=MSsoln;
		difc=sim->mols->difc[j][ms]; }
	else if(totalmn==1) {      // just 1 monomer
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				difc=bng->monomerdifc[mn];
				mn=bng->nmonomer; }}
	else {                // multiple monomers
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				if(difc==-1 && bng->monomerdifc[mn]==0) difc=0;
				else if(difc==-1) difc=bng->monomercount[mn]*pow(bng->monomerdifc[mn],-3.0);
				else if(bng->monomerdifc[mn]==0) difc=0;
				else difc+=bng->monomercount[mn]*pow(bng->monomerdifc[mn],-3.0); }
		if(difc!=0) difc=pow(difc,-1.0/3.0); }

	return difc; }


/* bngmakedisplaysize */
double bngmakedisplaysize(bngptr bng,int index,int totalmn) {
	double displaysize;
	int mn,j;
	simptr sim;
	enum MolecState ms;

	displaysize=0;
	sim=bng->bngss->sim;
	j=stringfind(sim->mols->spname,sim->mols->nspecies,bng->bspshortnames[index]);	// look for species in Smoldyn species list

	if(j>0) {
		ms=bng->bspstate[index];
		if(ms==MSbsoln) ms=MSsoln;
		displaysize=sim->mols->display[j][ms]; }
	else if(totalmn==1) {      // just 1 monomer
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				displaysize=bng->monomerdisplaysize[mn];
				mn=bng->nmonomer; }}
	else {                // multiple monomers
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				displaysize+=bng->monomercount[mn]*pow(bng->monomerdisplaysize[mn],3.0); }
		if(displaysize>0) displaysize=pow(displaysize,1.0/3.0); }

	return displaysize; }


/* bngmakecolor */
int bngmakecolor(bngptr bng,int index,int totalmn,double *color) {
	double weight,totalweight;
	int mn,j;
	simptr sim;
	enum MolecState ms;

	color[0]=color[1]=color[2]=0;
	sim=bng->bngss->sim;
	j=stringfind(sim->mols->spname,sim->mols->nspecies,bng->bspshortnames[index]);	// look for species in Smoldyn species list

	if(j>0) {
		ms=bng->bspstate[index];
		if(ms==MSbsoln) ms=MSsoln;
		color[0]=sim->mols->color[j][ms][0];
		color[1]=sim->mols->color[j][ms][1];
		color[2]=sim->mols->color[j][ms][2]; }
	else if(totalmn==1) {      // just 1 monomer
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				color[0]=bng->monomercolor[mn][0];
				color[1]=bng->monomercolor[mn][1];
				color[2]=bng->monomercolor[mn][2];
				mn=bng->nmonomer; }}
	else {                // multiple monomers
		weight=0;
		totalweight=0;
		for(mn=0;mn<bng->nmonomer;mn++)
			if(bng->monomercount[mn]>0) {
				weight=bng->monomercount[mn]*bng->monomerdisplaysize[mn];
				totalweight+=weight;
				color[0]+=weight*bng->monomercolor[mn][0];
				color[1]+=weight*bng->monomercolor[mn][1];
				color[2]+=weight*bng->monomercolor[mn][2]; }
		color[0]/=totalweight;
		color[1]/=totalweight;
		color[2]/=totalweight; }
	return 0; }


/* bngmakesurfaction */
void bngmakesurfaction(bngptr bng,int index,int totalmn,enum SrfAction **srfaction,surfactionptr **actdetails) {
	int j,s,mn;
	simptr sim;
	enum MolecState ms;
	enum SrfAction trialact,currentact;
	enum PanelFace face;
	surfaceptr srf;
	surfactionptr trialdet,currentdet;

	sim=bng->bngss->sim;
	j=stringfind(sim->mols->spname,sim->mols->nspecies,bng->bspshortnames[index]);	// look for species in Smoldyn species list
	for(s=0;s<bng->bngmaxsurface;s++)
		srfaction[s][PFfront]=srfaction[s][PFback]=SAtrans;

	ms=bng->bspstate[index];
	if(ms==MSbsoln) ms=MSsoln;

	if(j>0) {
		for(s=0;s<bng->bngmaxsurface;s++) {
			srf=sim->srfss->srflist[s];
			for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1)) {
				srfaction[s][face]=srf->action[j][ms][face];
				actdetails[s][face]=srf->actdetails[j][ms][face]; }}}

	else if(totalmn==1) {
		for(mn=0;mn<bng->nmonomer;mn++) {
			if(bng->monomercount[mn]>0) {
				for(s=0;s<bng->bngmaxsurface;s++) {
					for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1)) {
						srfaction[s][face]=bng->monomeraction[mn][s][face];
						actdetails[s][face]=bng->monomeractdetails[mn][s][face]; }}
				mn=bng->nmonomer; }}}

	else {
		for(s=0;s<bng->bngmaxsurface;s++)
			for(face=PFfront;face<=PFnone;face=(enum PanelFace)(face+1))
				for(mn=0;mn<bng->nmonomer;mn++)
					if(bng->monomercount[mn]>0) {	// priority: SAtrans < SAmult < SAreflect < SAjump < SAabsorb < SAport
						trialact=bng->monomeraction[mn][s][face];
						trialdet=bng->monomeractdetails[mn][s][face];
						currentact=srfaction[s][face];
						currentdet=actdetails[s][face];
						if(srfcompareaction(currentact,currentdet,trialact,trialdet)>0) {
							srfaction[s][face]=trialact;
							actdetails[s][face]=trialdet; }}}

	return; }


/* bngparsespecies */
int bngparsespecies(bngptr bng,int index) {
  int i1,i2,i3,mn,totalmn,er,hasmods,s;
  char *longname,ch,ch3,monomername[STRCHAR];
  simptr sim;
  double value,poslo[DIMMAX],poshi[DIMMAX],difc,displaysize,color[3];
	enum MolecState ms,ms0,ms1,ms2,ms4;
	enum SrfAction **action;
	surfactionptr **actdetails;
	surfaceptr srf;
	enum PanelFace face;

  sim=bng->bngss->sim;

  for(mn=0;mn<bng->nmonomer;mn++)           // clear monomer counts
    bng->monomercount[mn]=0;

  longname=bng->bsplongnames[index];        // count numbers of each monomer in the longname
  totalmn=0;
	hasmods=0;
  i1=i2=i3=0;
  while(longname[i2]!='\0') {			// i1 and i2 walk along string, i2 is ahead, they bracket monomer names
    while((ch=longname[i2])!='(' && ch!='.' && ch!='\0') i2++;

		if(ch=='(') {									// i3 and ch3 look for a ~ character to test for modifications
			i3=i2;
			while((ch3=longname[i3])!='\0' && ch3!=')' && ch3!='~') i3++;
			if(ch3=='~') hasmods=1; }

		strncpy(monomername,longname+i1,i2-i1);
		monomername[i2-i1]='\0';
    mn=stringfind(bng->monomernames,bng->nmonomer,monomername);
    if(mn<0) {
      mn=bngaddmonomer(bng,monomername,MSsoln);
      if(mn<0) return -1; }
    bng->monomercount[mn]++;
    totalmn++;
    if(ch=='.') i1=i2=i2+1;
    else if(ch=='(') {
      i1=i2=strparenmatch(longname,i2)+1;
      if(i1<=0) return -2;		// cannot parse name
			while(longname[i1]=='.') i1=i2=i1+1; }}

	bngmakeshortname(bng,index,totalmn,hasmods);
	ms=bngmakedefaultstate(bng,index,totalmn);
	difc=bngmakedifc(bng,index,totalmn);
	displaysize=bngmakedisplaysize(bng,index,totalmn);
	bngmakecolor(bng,index,totalmn,color);
	action=NULL;
	actdetails=NULL;
	if(bng->bngmaxsurface) {
		action=(enum SrfAction**) calloc(bng->bngmaxsurface,sizeof(enum SrfAction*));
		actdetails=(surfactionptr **) calloc(bng->bngmaxsurface,sizeof(surfactionptr*));
		if(!action || !actdetails) return -1;
		for(s=0;s<bng->bngmaxsurface;s++) {
			action[s]=(enum SrfAction*) calloc(3,sizeof(enum SrfAction));
			actdetails[s]=(surfactionptr*) calloc(3,sizeof(surfactionptr));
			if(!action[s] || !actdetails[s]) return -1;
			action[s][PFfront]=action[s][PFback]=action[s][PFnone]=SAtrans;
			actdetails[s][PFfront]=actdetails[s][PFback]=actdetails[s][PFnone]=NULL; }
		bngmakesurfaction(bng,index,totalmn,action,actdetails); }

  i1=moladdspecies(sim,bng->bspshortnames[index]);            // add to Smoldyn simulation
  if(i1==-1) return -1;         // out of memory
  else if(i1==-4) return -3;    // illegal name
  else if(i1==-5)               // already exists
    i1=stringfind(sim->mols->spname,sim->mols->nspecies,bng->bspshortnames[index]);
  else if(i1==-6) return -3;    // illegal name
    
  bng->spindex[index]=i1;												// set spindex element

	if(ms==MSbsoln) ms=MSsoln;
	molsetdifc(sim,i1,NULL,ms,difc);							// set diffusion coefficient
	molsetdisplaysize(sim,i1,NULL,MSall,displaysize);
	molsetcolor(sim,i1,NULL,MSall,color);
	if(bng->bngmaxsurface) {
		for(s=0;s<bng->bngmaxsurface;s++) {
			srf=sim->srfss->srflist[s];
			for(face=(enum PanelFace)0;face<=PFnone;face=(enum PanelFace)(face+1)) {
				surfsetaction(srf,i1,NULL,ms,face,action[s][face],-1);
				if(action[s][face]==SAmult) {
					for(ms4=(enum MolecState)0;ms4<(enum MolecState)MSMAX1;ms4=(enum MolecState)(ms4+1)) {
						srfindex2tristate(ms,face,ms4,&ms0,&ms1,&ms2);
						surfsetrate(srf,i1,NULL,ms0,ms1,ms2,i1,actdetails[s][face]->srfrate[ms4],1); }}}}
		for(s=0;s<bng->bngmaxsurface;s++) {
			free(action[s]);
			free(actdetails[s]); }
		free(action);
		free(actdetails); }

  if(bng->bspcountstr[index]) {                 // parse count information
    value=strmatheval(bng->bspcountstr[index],bng->paramnames,bng->paramvalues,bng->nparams);
    if(strmatherror(NULL,1)) return -4;   // cannot parse count string
    bng->bspcount[index]=value;
    if(value>0.5) {                             // add molecule to Smoldyn simulation
      systemcorners(sim,poslo,poshi);
			if(ms==MSsoln)
				er=addmol(sim,(int)(value+0.5),bng->spindex[index],poslo,poshi,0);
			else {
				er=addsurfmol(sim,(int)(value+0.5),bng->spindex[index],bng->bspstate[index],NULL,NULL,-1,PSall,NULL); }
      if(er==1) return -1;
			else if(er==2) return -6;			// no surface panels
      else if(er==3) return -5; }}  // too many molecules
    
  return 0; }


/* bngaddspecies */
int bngaddspecies(bngptr bng,int bindex,const char *longname,const char *countstr) {
  int er;

	if(bindex>=bng->maxbspecies) {		// enlarge list if needed
		bng=bngalloc(bng,0,0,2*bindex+1,0);
		if(!bng) return -1; }

	if(longname) strncpy(bng->bsplongnames[bindex],longname,STRCHAR-1);
  else bng->bsplongnames[bindex][0]='\0';
	if(countstr) strncpy(bng->bspcountstr[bindex],countstr,STRCHAR-1);
  else bng->bspcountstr[bindex][0]='\0';
	if(bng->nbspecies<=bindex)
		bng->nbspecies=bindex+1;
  er=bngparsespecies(bng,bindex);
  return er; }


/******************************************************************************/
/************************* structure set up - reactions ***********************/
/******************************************************************************/


/* bngparsereaction */
int bngparsereaction(bngptr bng,int index) {
  int i1,i2,order,nprod,react[2],prod[2],er,allsoln;
  char string[STRCHAR];
  enum MolecState rctstate[2],prdstate[2];
	double rate;
  simptr sim;
  rxnptr rxn;

  sim=bng->bngss->sim;
  order=sscanf(bng->brxnreactstr[index],"%i,%i",&i1,&i2);   // reactant list
  bng->brxnorder[index]=order;
  if(order>=1) {
    bng->brxnreact[index][0]=i1;
    react[0]=bng->spindex[i1];
		rctstate[0]=bng->bspstate[i1]; }
  else {
    bng->brxnreact[index][0]=0;
    react[0]=0;
		rctstate[0]=MSsoln; }
  if(order==2) {
    bng->brxnreact[index][1]=i2;
    react[1]=bng->spindex[i2];
		rctstate[1]=bng->bspstate[i2]; }
  else {
    bng->brxnreact[index][1]=0;
    react[1]=0;
		rctstate[1]=MSsoln; }

  nprod=sscanf(bng->brxnprodstr[index],"%i,%i",&i1,&i2);    // product list
  bng->brxnnprod[index]=nprod;
  if(nprod>=1) {
    bng->brxnprod[index][0]=i1;
    prod[0]=bng->spindex[i1];
		prdstate[0]=bng->bspstate[i1]; }
  else {
    bng->brxnprod[index][0]=0;
    prod[0]=0;
		prdstate[0]=MSsoln; }
  if(nprod==2) {
    bng->brxnprod[index][1]=i2;
    prod[1]=bng->spindex[i2];
		prdstate[1]=bng->bspstate[i2]; }
  else {
    bng->brxnprod[index][1]=0;
    prod[1]=0;
		prdstate[1]=MSsoln; }

	allsoln=1;
	if(order>=1 && (rctstate[0]==MSup || rctstate[0]==MSdown || rctstate[0]==MSfront || rctstate[0]==MSback)) allsoln=0;
	if(order==2 && (rctstate[1]==MSup || rctstate[1]==MSdown || rctstate[1]==MSfront || rctstate[1]==MSback)) allsoln=0;
	if(allsoln) {
		rctstate[0]=rctstate[1]=MSsoln;
		prdstate[0]=prdstate[1]=MSsoln; }

  snprintf(string,STRCHAR,"%s_%i",bng->bngname,index);     // reaction name

  rxn=RxnAddReaction(sim,string,order,react,rctstate,nprod,prod,prdstate,NULL,NULL);
  if(!rxn) return 1;
  bng->brxn[index]=rxn;

	rate=strmatheval(bng->brxnratestr[index],bng->paramnames,bng->paramvalues,bng->nparams);
	if(strmatherror(NULL,1)) return 2;   // cannot parse rate string
	rate*=(order==1)?bng->unirate:bng->birate;
	if(order==2 && react[0]==react[1]) rate*=2;	// BioNetGen divides rate by 2 so undo it here
	er=RxnSetValue(sim,"rate",rxn,rate);
	if(er==4) return 2;
  return 0; }


/* bngaddreaction */
int bngaddreaction(bngptr bng,int bindex,const char *reactants,const char *products,const char *rate) {
  int er;

	if(bindex>=bng->maxbrxns) {		// enlarge list if needed
		bng=bngalloc(bng,0,0,0,2*bindex+1);
		if(!bng) return 1; }

	if(reactants) strcpy(bng->brxnreactstr[bindex],reactants);
  else bng->brxnreactstr[bindex][0]='\0';
	if(products) strcpy(bng->brxnprodstr[bindex],products);
  else bng->brxnprodstr[bindex][0]='\0';
	if(rate) strcpy(bng->brxnratestr[bindex],rate);
  else bng->brxnratestr[bindex][0]='\0';
	if(bng->nbrxns<=bindex)
		bng->nbrxns=bindex+1;
  er=bngparsereaction(bng,bindex);
  return er; }


/******************************************************************************/
/************************** structure set up - groups *************************/
/******************************************************************************/


/* bngaddgroup */
int bngaddgroup(bngptr bng,int gindex,const char *gname,const char *specieslist) {
	int er,itct,imol,i1;
	const char *splistptr;

	er=moladdspeciesgroup(bng->bngss->sim,gname,NULL,0);
	if(er) return 1;
	splistptr=specieslist;
	itct=sscanf(splistptr,"%i",&i1);
	while(itct) {
		imol=bng->spindex[i1];
		er=moladdspeciesgroup(bng->bngss->sim,gname,NULL,imol);
		if(er) return 1;
		splistptr=strchr(splistptr,',');
		if(splistptr) {
			splistptr++;
			itct=sscanf(splistptr,"%i",&i1); }
		else itct=0; }

	return 0; }


/******************************************************************************/
/****************** structure set up - high level functions *******************/
/******************************************************************************/


/* bngrunBNGL2 */
int bngrunBNGL2(bngptr bng,char *filename,char *outname) {
	char string[STRCHAR],*dot;
	FILE *fptr;
	int vflag;

	vflag=strchr(bng->bngss->sim->flags,'v')?1:0;

	fptr=fopen(bng->bngss->BNG2path,"r");	// check for software
	if(!fptr) return 1;
	fclose(fptr);

	fptr=fopen(filename,"r");		// check for input file
	if(!fptr) return 2;
	fclose(fptr);

	strcpy(outname,filename);		// create outname
	dot=strrchr(outname,'.');
	if(!dot) dot=outname+strlen(outname);
	strcpy(dot,".net");
	remove(outname);						// delete output file

	snprintf(string,STRCHAR,"perl %s %s %s",bng->bngss->BNG2path,filename,vflag?"":DEVNULL);
	simLog(bng->bngss->sim,2," Running BNG2.pl on %s\n",filename);
	system(string);							// generate network

	fptr=fopen(outname,"r");		// check for output file
	if(!fptr) return 3;					// output file was not written
	fclose(fptr);

	simLog(bng->bngss->sim,2," BNG2.pl ran successfully\n");
	return 0; }


/* bngreadstring */
bngptr bngreadstring(simptr sim,ParseFilePtr pfp,bngptr bng,const char *word,char *line2) {
	char str1[STRCHAR],str2[STRCHAR],str3[STRCHAR];
	bngssptr bngss;
	int itct,index,er;
	enum MolecState ms;
	static int inparams=0,inspecies=0,inrxns=0,ingroups=0;
	double f1,v2[4];

	int nvar;
	char **varnames;
	double *varvalues;

	nvar=sim->nvar;
	varnames=sim->varnames;
	varvalues=sim->varvalues;

	bngss=sim->bngss;

	if(inparams) {																// parameters
		if(!strcmp(word,"end")) inparams=0;
		else {
			itct=sscanf(line2,"%s %s",str1,str2);
			CHECKS(itct==2,"failed to read parameter line");
			CHECKS(bngaddparameter(bng,str1,str2)>=0,"failed to add parameter"); }}

	else if(inspecies) {													// species
		if(!strcmp(word,"end")) inspecies=0;
		else {
			itct=sscanf(word,"%i",&index);
			CHECKS(itct==1,"failed to read species index value");
			itct=sscanf(line2,"%s %s",str1,str2);
			CHECKS(itct>=1,"failed to read species on species line");
			if(itct==1) strcpy(str2,"0");
			CHECKS(bngaddspecies(bng,index,str1,str2)==0,"failed to add species"); }}

	else if(inrxns) {															// reactions
		if(!strcmp(word,"end")) inrxns=0;
		else {
			itct=sscanf(word,"%i",&index);
			CHECKS(itct==1,"failed to read reaction index value");
			itct=sscanf(line2,"%s %s %s",str1,str2,str3);
			CHECKS(itct==3,"failed to read all 3 values of reaction line");
			CHECKS(bngaddreaction(bng,index,str1,str2,str3)==0,"failed to add reaction"); }}

	else if(ingroups) {														// groups
		if(!strcmp(word,"end")) ingroups=0;
		else {
			itct=sscanf(word,"%i",&index);
			CHECKS(itct==1,"failed to read group index value");
			itct=sscanf(line2,"%s %s",str1,str2);
			CHECKS(itct==2,"failed to read all 2 values of groups line");
			CHECKS(bngaddgroup(bng,index,str1,str2)==0,"failed to add group"); }}

	else if(!strncmp(word,"setOption",9));				// setOption... (intentionally ignored)

	else if(!strcmp(word,"begin")) {							// begin
    CHECKS(bng,"need to enter a name for this set of BNG complexes before entering its data");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"error reading because 'begin' needs to followed by 'parameters', 'species', or 'reactions'");
		if(!strcmp(str1,"parameters")) inparams=1;
		else if(!strcmp(str1,"species")) inspecies=1;
		else if(!strcmp(str1,"reactions")) inrxns=1;
		else if(!strcmp(str1,"groups")) ingroups=1;
		CHECKS(!strnword(line2,2),"unexpected text at end of begin statement"); }

	else if(!strcmp(word,"name")) {								// name
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"error reading bng name");
		bng=bngaddbng(sim,str1);
		CHECKS(bng,"failed to add bng structure");
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"BNG2_path")) {					// BNG2_path
		bngsetBNG2path(bng,line2); }

	else if(!strcmp(word,"multiply")) {						// multiply
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,str1,&f1);
		CHECKS(itct==2,"multiply format: parameter amount");
		er=bngsetparam(bng,str1,f1);
		CHECKS(er!=1,"unrecognized parameter");
		CHECKS(er!=2,"multiply amount needs to be greater than 0");
		CHECKS(!strnword(line2,3),"unrecognized text following multiply statement"); }

	else if(!strcmp(word,"monomer") || !strcmp(word,"monomers")) {	// monomer or monomers
		while((itct=sscanf(line2,"%s",str1))==1) {
			er=bngaddmonomer(bng,str1,MSsoln);
			CHECKS(er!=-1,"out of memory adding monomer %s",str1);
			CHECKS(er!=-2,"monomer name '%s' is not permitted",str1);
			CHECKS(!er,"error adding monomer %s",str1); }}

	else if(!strcmp(word,"monomer_difc")) {				// monomer_difc
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,str1,&f1);
		CHECKS(itct==2,"monomer_difc format: monomer difc");
		CHECKS(f1>=0,"monomer diffusion coefficient has to be >=0");
		er=bngsetmonomerdifc(bng,str1,f1);
		CHECKS(er!=-1,"out of memory adding monomer %s",str1);
		CHECKS(er!=-2,"invalid '%s' monomer name",str1);
		CHECKS(!er,"failed to set monomer diffusion coefficient");
		CHECKS(!strnword(line2,3),"unexpected text following monomer_difc"); }

	else if(!strcmp(word,"monomer_display_size")) {	// monomer_display_size
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,str1,&f1);
		CHECKS(itct==2,"monomer_display_size format: monomer display_size");
		er=bngsetmonomerdisplaysize(bng,str1,f1);
		CHECKS(er!=-1,"out of memory adding monomer %s",str1);
		CHECKS(er!=-2,"invalid '%s' monomer name",str1);
		CHECKS(!er,"failed to set monomer display size");
		CHECKS(!strnword(line2,3),"unexpected text following monomer_display_size"); }

	else if(!strcmp(word,"monomer_color") || !strcmp(word,"monomer_colour")) {				// monomer_color
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"monomer_color format: monomer color");
		line2=strnword(line2,2);
		CHECKS(line2,"monomer_color format: monomer color");
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name");
		er=bngsetmonomercolor(bng,str1,v2);
		CHECKS(er!=-1,"out of memory adding monomer %s",str1);
		CHECKS(er!=-2,"invalid '%s' monomer name",str1);
		CHECKS(!er,"failed to set monomer display size");
		CHECKS(!strnword(line2,3),"unexpected text following monomer_display_size"); }

	else if(!strcmp(word,"monomer_state")) {			// monomer_state
		itct=sscanf(line2,"%s %s",str1,str2);
		CHECKS(itct==2,"monomer_state format: monomer state");
		ms=molstring2ms(str2);
		CHECKS(ms<MSMAX1,"monomer state name not allowed or not recognized");
		er=bngsetmonomerstate(bng,str1,ms);
		CHECKS(er!=-1,"out of memory adding monomer %s",str1);
		CHECKS(er!=-2,"invalid '%s' monomer name",str1);
		CHECKS(!er,"failed to set monomer default state");
		CHECKS(!strnword(line2,3),"unexpected text following monomer_state"); }

	else if(!strcmp(word,"expand_rules")) {				// expand_rules
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"error reading file name");
		er=bngrunBNGL2(bng,str1,str2);
		CHECKS(er!=1,"BNG2.pl software is not at '%s'",bngss->BNG2path);
		CHECKS(er!=2,"BioNetGen file '%s' not found",str1);
		CHECKS(er!=3,"Error reading BioNetGen file '%s'. Try again with '-v' flag for more information.",str1);
		CHECKS(!strnword(line2,2),"unexpected text following file name"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within bngnet block: statement not recognized"); }

	return bng;

failure:
	simParseError(sim,pfp);
	return NULL; }


/* loadbng */
int loadbng(simptr sim,ParseFilePtr *pfpptr,char* line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	bngptr bng;

	pfp=*pfpptr;
	done=0;
	bng=NULL;
	firstline2=line2?1:0;

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
		else if(!strcmp(word,"end_bng")) {					// end_bng
			CHECKS(!line2,"unexpected text following end_bng");
			return 0; }
		else {
			bng=bngreadstring(sim,pfp,bng,word,line2);
			CHECK(bng); }}															// failed but error has already been reported

	CHECKS(0,"end of file encountered before end_bng statement");	// end of file

failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* bngupdateparams */
int bngupdateparams(simptr sim) {
  return 0; }


/* bngupdatelists */
int bngupdatelists(simptr sim) {
  return 0; }


/* bngupdate */
int bngupdate(simptr sim) {
	int er;
	bngssptr bngss;
	
	bngss=sim->bngss;
	if(bngss) {
		if(bngss->condition<=SClists) {
			er=bngupdatelists(sim);
			if(er) return er;
			bngsetcondition(bngss,SCparams,1); }
		if(bngss->condition==SCparams) {
			er=bngupdateparams(sim);
			if(er) return er;
			bngsetcondition(bngss,SCok,1); }}
  return 0; }



/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/



