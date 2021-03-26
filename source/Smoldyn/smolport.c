/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "string2.h"
#include "Zn.h"
#include <string>

#include "smoldyn.h"
#include "smoldynfuncs.h"

/******************************************************************************/
/************************************* Ports **********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// memory management
portptr portalloc(void);
void portfree(portptr port);
portssptr portssalloc(portssptr portss,int maxport);

// data structure output

// structure set up
int portsupdateparams(simptr sim);
int portsupdatelists(simptr sim);

// core simulation functions


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* portalloc */
portptr portalloc(void) {
	portptr port;

	port=(portptr)malloc(sizeof(struct portstruct));
	CHECKMEM(port);
	port->portname=NULL;
	port->srf=NULL;
	port->face=PFnone;
	port->llport=-1;
	return port;
failure:
	simLog(NULL,10,"Unable to allocate memory in portalloc");
	return NULL; }


/* portfree */
void portfree(portptr port) {
	if(!port) return;
	free(port);
	return; }


/* portssalloc */
portssptr portssalloc(portssptr portss,int maxport) {
	int p;
	char **newnames;
	portptr *newportlist;

	if(maxport<1) return NULL;

	newnames=NULL;
	newportlist=NULL;

	if(!portss) {																			// new allocation
		portss=(portssptr) malloc(sizeof(struct portsuperstruct));
		CHECKMEM(portss);
		portss->condition=SCinit;
		portss->sim=NULL;
		portss->maxport=0;
		portss->nport=0;
		portss->portnames=NULL;
		portss->portlist=NULL; }
	else {																						// minor check
		if(maxport<portss->maxport) return NULL; }

	if(maxport>portss->maxport) {											// allocate new port names and ports
		CHECKMEM(newnames=(char**) calloc(maxport,sizeof(char*)));
		for(p=0;p<maxport;p++) newnames[p]=NULL;
		for(p=0;p<portss->maxport;p++) newnames[p]=portss->portnames[p];
		for(;p<maxport;p++)
			CHECKMEM(newnames[p]=EmptyString());

		CHECKMEM(newportlist=(portptr*) calloc(maxport,sizeof(portptr)));	// port list
		for(p=0;p<maxport;p++) newportlist[p]=NULL;
		for(p=0;p<portss->maxport;p++) newportlist[p]=portss->portlist[p];
		for(;p<maxport;p++) {
			CHECKMEM(newportlist[p]=portalloc());
			newportlist[p]->portss=portss;
			newportlist[p]->portname=newnames[p]; }}

	portss->maxport=maxport;
	free(portss->portnames);
	portss->portnames=newnames;
	free(portss->portlist);
	portss->portlist=newportlist;

	return portss;

 failure:
 	portssfree(portss);
	simLog(NULL,10,"Unable to allocate memory in portssalloc");
 	return NULL; }


/* portssfree */
void portssfree(portssptr portss) {
	int prt;

	if(!portss) return;
	if(portss->maxport && portss->portlist)
		for(prt=0;prt<portss->maxport;prt++) portfree(portss->portlist[prt]);
	free(portss->portlist);
	if(portss->maxport && portss->portnames)
		for(prt=0;prt<portss->maxport;prt++) free(portss->portnames[prt]);
	free(portss->portnames);
	free(portss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/

/* portoutput */
void portoutput(simptr sim) {
	portssptr portss;
	portptr port;
	int prt;
	char string[STRCHAR];

	portss=sim->portss;
	if(!portss) return;
	simLog(sim,2,"PORT PARAMETERS\n");
	simLog(sim,2," Ports allocated: %i, ports defined: %i\n",portss->maxport,portss->nport);
	for(prt=0;prt<portss->nport;prt++) {
		port=portss->portlist[prt];
		simLog(sim,2," Port: %s\n",portss->portnames[prt]);
		if(port->srf) simLog(sim,2,"  surface: %s, %s\n",port->srf->sname,surfface2string(port->face,string));
		else simLog(sim,2,"  no surface assigned\n");
		if(port->llport>=0) simLog(sim,2,"  molecule list: %s\n",sim->mols->listname[port->llport]);
		else simLog(sim,2,"  no molecule list assigned"); }
	simLog(sim,2,"\n");
	return; }


/* writeports */
void writeports(simptr sim,FILE *fptr) {
	portssptr portss;
	portptr port;
	int prt;
	char string[STRCHAR];

	portss=sim->portss;
	if(!portss) return;
	fprintf(fptr,"# Port parameters\n");
	fprintf(fptr,"max_port %i\n",portss->maxport);
	for(prt=0;prt<portss->nport;prt++) {
		port=portss->portlist[prt];
		fprintf(fptr,"start_port %s\n",port->portname);
		if(port->srf) fprintf(fptr,"surface %s\n",port->srf->sname);
		if(port->srf) fprintf(fptr,"face %s\n",surfface2string(port->face,string));
		fprintf(fptr,"end_port\n\n"); }
	return; }


/* checkportparams */
int checkportparams(simptr sim,int *warnptr) {
	int error,warn,prt,er,i;
	portssptr portss;
	portptr port;
	char string[STRCHAR];

	error=warn=0;
	portss=sim->portss;
	if(!portss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(portss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: port structure %s\n",simsc2string(portss->condition,string)); }

	for(prt=0;prt<portss->nport;prt++) {
		port=portss->portlist[prt];					// check for porting surface
		if(!port->srf) {warn++;simLog(sim,5," WARNING: there is no porting surface assigned to port %s\n",port->portname);}
		if(!(port->face==PFfront || port->face==PFback))
			{warn++;simLog(sim,5," WARNING: no surface face has been assigned to port %s\n",port->portname);}

		if(port->srf && port->srf->port[port->face]!=port)
			{error++;simLog(sim,10," ERROR: port %s is not registered by surface %s\n",port->portname,port->srf->sname);}

		if(sim->mols && port->srf && port->srf->action) {
			er=1;																// make sure surface action is set to port
			for(i=0;i<sim->mols->nspecies && er;i++)
				if(port->srf->action[i][MSsoln][port->face]==SAport) er=0;
			if(er) {warn++;simLog(sim,5," WARNING: port %s is nonfunctional because no molecule actions at the surface %s are set to port\n",port->portname,port->srf->sname);}
			if(!port->llport) {error++;simLog(sim,10," BUG: port %s has no molecule buffer\n",port->portname);} }}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/******************************** structure set up ****************************/
/******************************************************************************/


/* portsetcondition */
void portsetcondition(portssptr portss,enum StructCond cond,int upgrade) {
	if(!portss) return;
	if(upgrade==0 && portss->condition>cond) portss->condition=cond;
	else if(upgrade==1 && portss->condition<cond) portss->condition=cond;
	else if(upgrade==2) portss->condition=cond;
	if(portss->sim && portss->condition<portss->sim->condition) {
		cond=portss->condition;
		simsetcondition(portss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* portenableports */
int portenableports(simptr sim,int maxport) {
	portssptr portss;

	if(sim->portss)									// check for redundant function call
		if(maxport==-1 || sim->portss->maxport==maxport)
			return 0;
	portss=portssalloc(sim->portss,maxport<0?5:maxport);
	if(!portss) return 1;
	sim->portss=portss;
	portss->sim=sim;
	portsetcondition(sim->portss,SClists,0);
	return 0; }


/* portaddport */
portptr portaddport(simptr sim,const char *portname,surfaceptr srf,enum PanelFace face) {
	int er,p;
	portssptr portss;
	portptr port;

	if(!sim->portss) {
	 er=portenableports(sim,-1);
	 if(er) return NULL; }
	portss=sim->portss;

	p=stringfind(portss->portnames,portss->nport,portname);
	if(p<0) {
		if(portss->nport==portss->maxport) {
			er=portenableports(sim,portss->nport*2+1);
			if(er) return NULL; }
		p=portss->nport++;
		strncpy(portss->portnames[p],portname,STRCHAR-1);
		portss->portnames[p][STRCHAR-1]='\0';
		port=portss->portlist[p]; }
	else
		port=portss->portlist[p];

	if(srf) port->srf=srf;
	if(face!=PFnone) port->face=face;
	if(port->srf && port->face!=PFnone)
		port->srf->port[port->face]=port;
	portsetcondition(portss,SClists,0);
	return port; }


/* portreadstring */
portptr portreadstring(simptr sim,ParseFilePtr pfp,portptr port,const char *word,char *line2) {
	char nm[STRCHAR];
	int itct,s;
	enum PanelFace face;


	if(!strcmp(word,"name")) {								// name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading port name");
		port=portaddport(sim,nm,NULL,PFnone);
		CHECKS(port,"failed to add port");
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"surface")) {						// surface
		CHECKS(port,"port name has to be entered before surface");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading surface name");
		s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
		CHECKS(s>=0,"surface '%s' not recognized",nm);
		port=portaddport(sim,port->portname,sim->srfss->srflist[s],PFnone);
		CHECKBUG(port,"SMOLDYN BUG adding surface to port");
		CHECKS(!strnword(line2,2),"unexpected text following surface"); }

	else if(!strcmp(word,"face")) {								// face
		CHECKS(port,"port name has to be entered before face");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading face name");
		face=surfstring2face(nm);
		CHECKS(face==PFfront || face==PFback,"face needs to be either front or back");
		port=portaddport(sim,port->portname,NULL,face);
		CHECKBUG(port,"SMOLDYN BUG adding face to port");
		CHECKS(!strnword(line2,2),"unexpected text following face"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within port block: statement not recognized"); }

	return port;

 failure:
	simParseError(sim,pfp);
	return NULL; }


/* loadport */
int loadport(simptr sim,ParseFilePtr *pfpptr,char* line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	portptr port;

	pfp=*pfpptr;
	done=0;
	port=NULL;
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
		else if(!strcmp(word,"end_port")) {						// end_port
			CHECKS(!line2,"unexpected text following end_port");
			return 0; }
		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }
		else {
			port=portreadstring(sim,pfp,port,word,line2);
			CHECK(port); }}															// failed but error has already been reported

	CHECKS(0,"end of file encountered before end_port statement");	// end of file

 failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* portsupdateparams */
int portsupdateparams(simptr sim) {
	return 0; }


/* portsupdatelists */
int portsupdatelists(simptr sim) {
	portssptr portss;
	portptr port;
	int prt,ll;

	portss=sim->portss;
	if(sim->mols && sim->mols->condition<SCparams) return 2;

	if(sim->mols) {
		for(prt=0;prt<portss->nport;prt++) {
			port=portss->portlist[prt];									// create port molecule list if none
			if(port->llport<0) {
				ll=addmollist(sim,port->portname,MLTport);
				if(ll<0) return 1;
				port->llport=ll; }}}

	return 0; }


/* portsupdate */
int portsupdate(simptr sim) {
	int er;
	portssptr portss;
	
	portss=sim->portss;
	if(portss) {
		if(portss->condition<=SClists) {
			er=portsupdatelists(sim);
			if(er) return er;
			portsetcondition(portss,SCparams,1); }
		if(portss->condition==SCparams) {
			er=portsupdateparams(sim);
			if(er) return er;
			portsetcondition(portss,SCok,1); }}
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* portgetmols */
int portgetmols(simptr sim,portptr port,int ident,enum MolecState ms,int remove) {
	int ll,nmol,count,m;
	moleculeptr *mlist;

	ll=port->llport;
	mlist=sim->mols->live[ll];
	nmol=sim->mols->nl[ll];
	if(ident<0 && ms==MSall && !remove) return nmol;
	count=0;
	for(m=0;m<nmol;m++) {
		if((ident==-1 || mlist[m]->ident==ident) && (ms==MSall || mlist[m]->mstate==ms)) {
			count++;
			if(remove) molkill(sim,mlist[m],ll,m); }}
	sim->eventcount[ETexport]+=count;
	return count; }


/* portgetmols2 */
int portgetmols2(simptr sim,portptr port,int ident,enum MolecState ms,int remove, double **positions) {
	int ll,nmol,count,m;
	moleculeptr *mlist;

	ll=port->llport;
	mlist=sim->mols->live[ll];
	nmol=sim->mols->nl[ll];
	if(ident<0 && ms==MSall && !remove && !positions) return nmol;
	count=0;
	for(m=0;m<nmol;m++) {
		if((ident==-1 || mlist[m]->ident==ident) && (ms==MSall || mlist[m]->mstate==ms)) {
			count++;
			if(positions) positions[count] = mlist[m]->pos;
			if(remove) molkill(sim,mlist[m],ll,m); }}
	sim->eventcount[ETexport]+=count;
	return count; }


/* portputmols */
int portputmols(simptr sim,portptr port,int nmol,int ident,int *species,double **positions,double **positionsx) {
	moleculeptr mptr;
	int dim,m,d;
	panelptr pnl;

	if(!nmol) return 0;
	if(!positionsx) {											// no positionsx is ok, but then the port has to have a surface
		if(!port->srf) return 2;
		if(port->face==PFnone) return 3;
		if(port->srf->totpanel==0) return 4; }
	dim=sim->dim;

	for(m=0;m<nmol;m++) {
		mptr=getnextmol(sim->mols);
		if(!mptr) return 1;
		if(species) mptr->ident=species[m];
		else mptr->ident=ident;
		mptr->mstate=MSsoln;
		mptr->list=sim->mols->listlookup[mptr->ident][MSsoln];
		sim->mols->expand[mptr->ident]|=1;
		if(positionsx) {
			for(d=0;d<dim;d++) {
				mptr->pos[d]=positions[m][d];
				mptr->posx[d]=positionsx[m][d];}}
		else if(positions) {
			closestsurfacept(port->srf,sim->dim,positions[m],mptr->posx,&pnl,NULL);
			fixpt2panel(mptr->posx,pnl,dim,port->face,sim->srfss->epsilon);
			for(d=0;d<dim;d++) mptr->pos[d]=positions[m][d]; }
		else {
			pnl=surfrandpos(port->srf,mptr->posx,dim);
			fixpt2panel(mptr->posx,pnl,dim,port->face,sim->srfss->epsilon);
			for(d=0;d<dim;d++) mptr->pos[d]=mptr->posx[d]; }
		mptr->box=pos2box(sim,mptr->pos); }
	sim->eventcount[ETimport]+=nmol;
	return 0; }


/* porttransport */
int porttransport(simptr sim1,portptr port1,simptr sim2,portptr port2) {
	int i,nmol,er;

	if(!portgetmols(sim1,port1,-1,MSall,0)) return 0;
	er=0;
	for(i=1;i<sim1->mols->nspecies && !er;i++) {
		nmol=portgetmols(sim1,port1,i,MSall,1);
		er=portputmols(sim2,port2,nmol,i,NULL,NULL,NULL); }
	return er; }

