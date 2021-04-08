/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include "Geometry.h"
#include "math2.h"
#include "random2.h"
#include "Rn.h"
#include "RnSort.h"
#include "string2.h"
#include "Zn.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smoldynconfigure.h"

/******************************************************************************/
/*********************************** Molecules ********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated type functions
enum MolListType molstring2mlt(char *string);
char *molmlt2string(enum MolListType mlt,char *string);

// low level utilities
char *molpos2string(simptr sim,moleculeptr mptr,char *string);

// memory management
moleculeptr molalloc(int dim);
void molfree(moleculeptr mptr);
void molfreesurfdrift(double *****surfdrift,int maxspec,int maxsrf);
molssptr molssalloc(molssptr mols,int maxspecies);
int mollistalloc(molssptr mols,int maxlist,enum MolListType mlt);
int molexpandlist(molssptr mols,int dim,int ll,int nspaces,int nmolecs);
int molpatternindexalloc(int **indexptr,int n);
int molpatternalloc(simptr sim,int maxpattern);

// data structure output

// structure setup
int molsetmaxspecies(simptr sim,int max);
int molsupdateparams(molssptr mols,double dt);
int molsupdatelists(simptr sim);
int molgeneratespecies(simptr sim,const char *name,int nparents,int parent1,int parent2);

// adding and removing molecules
moleculeptr newestmol(molssptr mols);
int molgetexport(simptr sim,int ident,enum MolecState ms);
int molputimport(simptr sim,int nmol,int ident,enum MolecState ms,panelptr pnl,enum PanelFace face);
int moldummyporter(simptr sim);

// core simulation functions


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* molstring2ms */
enum MolecState molstring2ms(char *string) {
	enum MolecState ans;

	if(!strcmp(string,"solution")) ans=MSsoln;
	else if(!strcmp(string,"fsoln")) ans=MSsoln;
	else if(!strcmp(string,"soln")) ans=MSsoln;
	else if(!strcmp(string,"aq")) ans=MSsoln;
	else if(!strcmp(string,"front")) ans=MSfront;
	else if(!strcmp(string,"back")) ans=MSback;
	else if(!strcmp(string,"up")) ans=MSup;
	else if(!strcmp(string,"down")) ans=MSdown;
	else if(!strcmp(string,"bsoln")) ans=MSbsoln;
	else if(!strcmp(string,"all")) ans=MSall;
	else ans=MSnone;
	return ans; }


/* molms2string */
char *molms2string(enum MolecState ms,char *string) {
	if(ms==MSsoln) strcpy(string,"solution");
	else if(ms==MSfront) strcpy(string,"front");
	else if(ms==MSback) strcpy(string,"back");
	else if(ms==MSup) strcpy(string,"up");
	else if(ms==MSdown) strcpy(string,"down");
	else if(ms==MSbsoln) strcpy(string,"bsoln");
	else if(ms==MSall) strcpy(string,"all");
	else if(ms==MSsome) strcpy(string,"some");
	else strcpy(string,"none");
	return string; }


/* molstring2mlt */
enum MolListType molstring2mlt(char *string) {
	enum MolListType ans;

	if(!strcmp(string,"system")) ans=MLTsystem;
	else if(!strcmp(string,"port")) ans=MLTport;
	else ans=MLTnone;
	return ans; }


/* molmlt2string */
char *molmlt2string(enum MolListType mlt,char *string) {
	if(mlt==MLTsystem) strcpy(string,"system");
	else if(mlt==MLTport) strcpy(string,"port");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/******************************** molecule patterns ***************************/
/******************************************************************************/


/* molstring2pattern */
int molstring2pattern(const char *str,enum MolecState *msptr,char *pat,int mode) {
	char nm[STRCHAR],*pareno,*parenc;
	int itct;
	enum MolecState ms;

	if(!pat) return -1;
	if(!str) {
		if(mode==2) {
			strcat(pat,"\n");
			return 0; }
		return -1; }

	itct=sscanf(str,"%s",nm);
	if(itct!=1) return -1;							// cannot read name

	if(nm[strlen(nm)-1]==')') {
		parenc=nm+strlen(nm)-1;
		*parenc='\0';
		pareno=strrchr(nm,'(');
		if(!pareno) return -2;						// improper open parenthesis
		*pareno='\0';
		pareno++;
		ms=molstring2ms(pareno);
		if(ms==MSnone) return -3; }				// cannot read state
	else ms=MSsoln;

	if(mode==0) pat[0]='\0';
	else if((mode==1 && pat[0]!='\0') || (mode==2 && strchr(pat,'\n'))) strcat(pat," ");
	else if(mode==2) strcat(pat,"\n");
	if(strlen(pat)+strlen(nm)>STRCHAR) return -4;

	strcat(pat,nm);

	if(msptr) *msptr=ms;
	return 0; }


/* molreversepattern */
int molreversepattern(simptr sim,const char *pattern,char *patternrev) {
	const char *newline;

	newline=strchr(pattern,'\n');
	if(!newline) return -1;					// function is only for reactions

	strcpy(patternrev,newline+1);
	strcat(patternrev,"\n");
	strncat(patternrev,pattern,newline-pattern);

	return 0; }


/* molpatternindex */
int molpatternindex(simptr sim,const char *pattern,const char *rname,int isrule,int update,int **indexptr) {
	int npattern,pat,er,i,i1,i2,j,istart,jstart,matchwords,subwords,totalwords,nspecies,haswildcard,hasspeciesgroup;
	int iword,ismatch,keepgoing,onthefly,imatch,imatch2;
	int **patindex,*index,*patindextemp,*matchindex1,*matchindex2;
	char **patlist,*patlisttemp,matchstr[STRCHAR],*substr,*newline,**patrname,*patrnametemp;
	char teststring[STRCHAR],deststring[STRCHAR],tempstring[STRCHAR];
	const char *newline2;
	molssptr mols;

	mols=sim->mols;
	patlist=mols->patlist;
	patindex=mols->patindex;
	patrname=mols->patrname;
	npattern=mols->npattern;

	if(sim->ruless) onthefly=sim->ruless->ruleonthefly;
	else onthefly=0;
	newline2=strchr(pattern,'\n');

	if(npattern) {
		pat=locateVstr(patlist,pattern,npattern,1);			// look for pattern in patlist list
		if(rname) {																			// if a reaction, set pat to also get same reaction name
			while(pat>=0 && !strcmp(patlist[pat],pattern) && patrname[pat] && strcmp(rname,patrname[pat]))
				pat--; }}																		// if not located, pat should be at one below destination spot
	else pat=-1;

	index=(pat>=0 && !strcmp(patlist[pat],pattern))?patindex[pat]:NULL;
	if(update && index && isrule && index[PDrule]!=isrule) index[PDrule]=isrule;		// it wasn't a rule but is now
	if(update==0 || (index && index[PDnspecies]<0) || (!(onthefly && newline2) && index && index[PDnspecies]>=mols->nspecies)) {
		if(indexptr) *indexptr=index;										// no updating required, so just return results
		return 0; }

	// check to see if this is a new pattern and put it in the lists if so
	if(!index) {
		pat++;
		if(npattern==sim->mols->maxpattern) {						// expand pattern list if its already full
			er=molpatternalloc(sim,2*npattern+2);
			if(er) return -1;
			patlist=mols->patlist;
			patindex=mols->patindex;
			patrname=mols->patrname; }

		patlisttemp=patlist[npattern];									// store new spot as patlisttemp
		patindextemp=patindex[npattern];
		patrnametemp=patrname[npattern];
		for(j=npattern-1;j>=pat;j--) {									// shift the list up one so new pattern goes to correct spot
			patlist[j+1]=patlist[j];
			patindex[j+1]=patindex[j];
			patrname[j+1]=patrname[j]; }
		mols->npattern++;
		npattern=mols->npattern;

		patlist[pat]=patlisttemp;												// put the new spot in the freed up space
		patindex[pat]=patindextemp;
		patrname[pat]=patrnametemp;
		strcpy(patlist[pat],pattern);										// set initial patlist and header values
		if(rname) {
			patrname[pat]=StringCopy(rname);
			if(!patrname[pat]) return -1; }
		index=patindex[pat];
		index[PDnresults]=0;
		index[PDnspecies]=0;
		index[PDmatch]=0;
		index[PDsubst]=0;
		index[PDrule]=isrule; }

	// prepare to update index variable for current species list
	strcpy(matchstr,pattern);													// assign to matchstr and substr and determine newline
	newline=strchr(matchstr,'\n');
	if(newline) {
		*newline='\0';
		substr=newline+1; }
	else substr=NULL;
	istart=index[PDnspecies];													// istart is first species to start checking
	jstart=index[PDnresults];													// jstart is first result that will be added
	if(istart==0) {																		// setup header
		istart=1;
		matchwords=wordcount(matchstr);
		subwords=newline?wordcount(substr):0;
		index[PDmatch]=matchwords;
		index[PDsubst]=subwords; }
	else {																						// read header
		matchwords=index[PDmatch];
		subwords=index[PDsubst]; }
	haswildcard=strpbrk(pattern,"*?&|{}[]")?1:0;
	nspecies=mols->nspecies;
	totalwords=matchwords+subwords;
	hasspeciesgroup=0;
	if(newline)																				// hasspeciesgroup is only defined if there is a newline
		for(iword=0;iword<matchwords;iword++) {
			sscanf(strnword(matchstr,iword+1),"%s",tempstring);
			if(!strpbrk(tempstring,"*?&|{}[]")) {
				molpatternindex(sim,tempstring,NULL,0,0,&matchindex1);
				if(matchindex1 && matchindex1[PDnresults]>1) hasspeciesgroup=1; }}

	if(indexptr) *indexptr=index;
	if(update==1) return 0;

	// now update index variable for current species list
	if(!strcmp(pattern,"all")) {											// pattern == "all"
		if(index[PDalloc]<PDMAX+nspecies) {
			er=molpatternindexalloc(&patindex[pat],PDMAX+nspecies);
			index=patindex[pat];
			if(er) return -1; }
		for(i=istart,j=jstart;i<nspecies;i++,j++)
			index[PDMAX+j]=i;
		index[PDnresults]=j;
		index[PDnspecies]=nspecies;
		sortVii(index+PDMAX,NULL,index[PDnresults]); }	// sort list

	else if(!newline && !haswildcard && matchwords==1) {		// a single species name, no wildcards
		if(index[PDalloc]<PDMAX+1) {										// only need space for 1 item
			er=molpatternindexalloc(&patindex[pat],PDMAX+1);
			index=patindex[pat];
			if(er) return -1; }
		sscanf(matchstr,"%s",tempstring);
		i=stringfind(mols->spname,nspecies,tempstring);
		if(i<0) {																				// not a current species name
			index[PDnspecies]=nspecies;
			if(indexptr) *indexptr=index;
			return -2; }
		index[PDMAX]=i;
		index[PDnresults]=1;
		index[PDnspecies]=-1; }													// will never need updating

	else if(!newline && matchwords==1) {							// single species name with wildcards
		j=jstart;
		for(i=istart;i<nspecies;i++) {
			ismatch=strEnhWildcardMatch(matchstr,mols->spname[i]);
			if(ismatch<0) return -10+ismatch;
			if(ismatch==1) {
				if(index[PDalloc]<PDMAX+j+1) {
					er=molpatternindexalloc(&patindex[pat],PDMAX+2*(j+1));
					index=patindex[pat];
					if(er) return -1; }
				index[PDMAX+j]=i;
				j++;
				index[PDnresults]=j; }}
		index[PDnspecies]=nspecies;
		sortVii(index+PDMAX,NULL,index[PDnresults]); }	// sort list

	else if(!newline) {																// no newline but multiple matchwords
		simLog(sim,7,"BUG in molpatternindex: pattern '%s' has multiple matchwords but no newline character\n",pattern); }

	else if(!haswildcard && !hasspeciesgroup && matchwords==0) {	// pattern has no wildcards or multi-species species group, so one entry at most
		if(index[PDalloc]<PDMAX+totalwords) {
			er=molpatternindexalloc(&patindex[pat],PDMAX+totalwords);
			index=patindex[pat];
			if(er) return -1; }

		for(iword=0;iword<subwords;iword++) {					// subwords
			sscanf(strnword(substr,iword+1),"%s",tempstring);
			i=stringfind(mols->spname,nspecies,tempstring);
			if(i<0) {
				if(!isrule) return -3;										// unknown substitute species
				else {
					i=molgeneratespecies(sim,tempstring,0,0,0);
					if(i<0) return -6; }}
			index[PDMAX+iword]=i; }
		index[PDnresults]=1;
		index[PDnspecies]=-1; }											// will never need updating

	else if(!haswildcard && !hasspeciesgroup && matchwords==1) {	// pattern has no wildcards or multi-species species group, so one entry at most
		if(index[PDalloc]<PDMAX+totalwords) {
			er=molpatternindexalloc(&patindex[pat],PDMAX+totalwords);
			index=patindex[pat];
			if(er) return -1; }
		sscanf(matchstr,"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex1);
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		i1=matchindex1[PDMAX];

		keepgoing=0;
		if(isrule && onthefly) {
			if(mols->expand[i1]==1) keepgoing=1; }
		else keepgoing=1;

		if(keepgoing) {
			index[PDMAX]=i1;
			for(iword=0;iword<subwords;iword++) {					// subwords
				sscanf(strnword(substr,iword+1),"%s",tempstring);
				i=stringfind(mols->spname,nspecies,tempstring);
				if(i<0) {
					if(!isrule) return -3;										// unknown substitute species
					else {
						i=molgeneratespecies(sim,tempstring,1,i1,0);
						if(i<0) return -6; }}
				index[PDMAX+1+iword]=i; }
			index[PDnresults]=1;
			index[PDnspecies]=-1; }}											// will never need updating

	else if(!haswildcard && !hasspeciesgroup && matchwords==2) {	// pattern has no wildcards or multi-species species group, so one entry at most
		if(index[PDalloc]<PDMAX+totalwords) {
			er=molpatternindexalloc(&patindex[pat],PDMAX+totalwords);
			index=patindex[pat];
			if(er) return -1; }
		sscanf(matchstr,"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex1);
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		i1=matchindex1[PDMAX];
		sscanf(strnword(matchstr,2),"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex2);
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		i2=matchindex2[PDMAX];

		keepgoing=0;
		if(isrule && onthefly) {
			if(mols->expand[i1]==1 && mols->expand[i2]>0) keepgoing=1;
			if(mols->expand[i2]==1 && mols->expand[i1]>0) keepgoing=1; }
		else keepgoing=1;

		if(keepgoing) {
			index[PDMAX]=i1;
			index[PDMAX+1]=i2;
			for(iword=0;iword<subwords;iword++) {					// subwords
				sscanf(strnword(substr,iword+1),"%s",tempstring);
				i=stringfind(mols->spname,nspecies,tempstring);
				if(i<0) {
					if(!isrule) return -3;										// unknown substitute species
					else {
						i=molgeneratespecies(sim,tempstring,2,i1,i2);
						if(i<0) return -6; }}
				index[PDMAX+2+iword]=i; }
			index[PDnresults]=1;
			index[PDnspecies]=-1; }}											// will never need updating

	else if(!haswildcard && !hasspeciesgroup) {
		simLog(sim,7,"BUG in molpatternindex: pattern '%s' has more than two reactants\n",pattern); }

	else if(matchwords==0) {													// pattern has wildcards or species group and 0 matchwords
		j=jstart;
		while((ismatch=strEnhWildcardMatchAndSub(matchstr,"",substr,deststring))>0) {
			if(index[PDalloc]<PDMAX+totalwords*(j+1)) {
				er=molpatternindexalloc(&patindex[pat],PDMAX+2*totalwords*(j+1));
				index=patindex[pat];
				if(er) return -1; }
			for(iword=0;iword<subwords;iword++) {
				sscanf(strnword(deststring,iword+1),"%s",tempstring);
				i=stringfind(mols->spname,mols->nspecies,tempstring);
				if(i<0) {
					if(!isrule) return -3;								// unknown substitute species
					else {
						i=molgeneratespecies(sim,tempstring,1,0,0);
						if(i<0) return -6; }}
				index[PDMAX+totalwords*j+iword]=i; }
			j++;
			index[PDnresults]=j; }
		if(ismatch<0) return -10+ismatch;								// wildcard expansion failure
		index[PDnspecies]=-1; }													// never needs updating

	else if(matchwords==1) {													// pattern has wildcards or species group and 1 matchword
		j=jstart;
		sscanf(matchstr,"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex1);		// get index of matchword
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		for(imatch=0;imatch<matchindex1[PDnresults];imatch++) {	// loop over all species that match to matchword
			i1=matchindex1[PDMAX+imatch];									// species identity

			keepgoing=0;
			if(isrule && onthefly) {
				if(mols->expand[i1]==1) keepgoing=1; }
			else if(i1>=istart) keepgoing=1;

			if(keepgoing) {
				strcpy(teststring,mols->spname[i1]);
				while((ismatch=strEnhWildcardMatchAndSub(matchstr,teststring,substr,deststring))>0) {
					if(index[PDalloc]<PDMAX+totalwords*(j+1)) {
						er=molpatternindexalloc(&patindex[pat],PDMAX+2*totalwords*(j+1));
						index=patindex[pat];
						if(er) return -1; }
					index[PDMAX+totalwords*j]=i1;
					for(iword=0;iword<subwords;iword++) {
						sscanf(strnword(deststring,iword+1),"%s",tempstring);
						i=stringfind(mols->spname,mols->nspecies,tempstring);
						if(i<0) {
							if(!isrule) return -3;								// unknown substitute species
							else {
								i=molgeneratespecies(sim,tempstring,1,i1,0);
								if(i<0) return -6; }}
						index[PDMAX+totalwords*j+1+iword]=i; }
					j++;
					index[PDnresults]=j; }
				if(ismatch<0) return -10+ismatch; }}				// wildcard expansion failure
		index[PDnspecies]=nspecies; }

	else if(matchwords==2) {													// pattern has wildcards or species group and 2 matchwords
		j=jstart;
		sscanf(matchstr,"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex1);		// get index of matchword
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		sscanf(strnword(matchstr,2),"%s",tempstring);
		er=molpatternindex(sim,tempstring,NULL,0,2,&matchindex2);		// get index of matchword
		if(er) {
			if(isrule && er==-2) {if(indexptr) *indexptr=index;er=0;}			// a matchword doesn't exist
			return er; }
		patindex=mols->patindex;
		while(index!=patindex[pat]) pat++;							// prior molpatternindex call might have shifted pat
		for(imatch=0;imatch<matchindex1[PDnresults];imatch++)
			for(imatch2=0;imatch2<matchindex2[PDnresults];imatch2++) {
				i1=matchindex1[PDMAX+imatch];								// species identity
				i2=matchindex2[PDMAX+imatch2];

				keepgoing=0;
				if(isrule && onthefly) {
					if(mols->expand[i1]==1 && mols->expand[i2]>0) keepgoing=1;
					if(mols->expand[i2]==1 && mols->expand[i1]>0) keepgoing=1; }
				else if(i1>=istart || i2>=istart) keepgoing=1;
				if(keepgoing && i2<i1) {										// avoid double counting by removing reactions in which i2<i1 and both reactants are in both lists
					for(i=0;i<matchindex1[PDnresults] && matchindex1[PDMAX+i]!=i2;i++);
					if(matchindex1[PDMAX+i]==i2) {
						for(i=0;i<matchindex2[PDnresults] && matchindex2[PDMAX+i]!=i1;i++);
						if(matchindex2[PDMAX+i]==i1) keepgoing=0; }}

			if(keepgoing) {
				snprintf(teststring,STRCHAR,"%s %s",mols->spname[i1],mols->spname[i2]);
				while((ismatch=strEnhWildcardMatchAndSub(matchstr,teststring,substr,deststring))>0) {
					if(index[PDalloc]<PDMAX+totalwords*(j+1)) {
						er=molpatternindexalloc(&patindex[pat],PDMAX+2*totalwords*(j+1));
						index=patindex[pat];
						if(er) return -1; }
					index[PDMAX+totalwords*j]=i1;
					index[PDMAX+totalwords*j+1]=i2;
					for(iword=0;iword<subwords;iword++) {
						sscanf(strnword(deststring,iword+1),"%s",tempstring);
						i=stringfind(mols->spname,mols->nspecies,tempstring);
						if(i<0) {
							if(!isrule) return -3;								// unknown substitute species
							else {
								i=molgeneratespecies(sim,tempstring,2,i1,i2);
								if(i<0) return -6; }}
						index[PDMAX+totalwords*j+2+iword]=i; }
					j++;
					index[PDnresults]=j; }
				if(ismatch<0) return -10+ismatch; }}				// wildcard expansion failure
		index[PDnspecies]=nspecies; }

	else {
		simLog(sim,7,"BUG in molpatternindex: pattern '%s' has more than two reactants\n",pattern); }

	if(indexptr) *indexptr=index;
	return 0; }


/* molstring2index1 */
int molstring2index1(simptr sim,const char *str,enum MolecState *msptr,int **indexptr) {
	int er,isall,*index;
	char pattern[STRCHAR];

	index=NULL;
	er=molstring2pattern(str,msptr,pattern,0);
	if(er) return er;
	isall=strcmp(pattern,"all")?0:1;
	er=molpatternindex(sim,pattern,NULL,0,2,&index);
	if(indexptr) *indexptr=index;
	if(isall && !er) return -5;
	if(er==-1 || er==-11) return -7;
	if(er==-13 || er==-15) return -6;
	if(er==-2 || (index && index[PDnresults]==0)) return -4;
	if(index[PDnresults]==1 && index[PDmatch]==1)
		return index[PDMAX];
	return 0; }


/* moladdspeciesgroup */
int moladdspeciesgroup(simptr sim,const char *group,char *species,int imol) {
	int er,*gpindex,*spindex,isp,igp,gpnresults,pat;
	enum MolecState ms;

	if(stringfind(sim->mols->spname,sim->mols->nspecies,group)>=0) return -9;	// cannot use a species name as a group name

	er=molstring2index1(sim,group,&ms,&gpindex);	// get group index
	if(er==-1 || er==-2 || er==-3 || er==-5 || er==-6 || er==-7) return er;
	if(ms!=MSsoln && ms!=MSall) return -8;

	if(species) {
		er=molstring2index1(sim,species,&ms,&spindex);	// get species index
		if(er==-1 || er==-2 || er==-3 || er==-4 || er==-6 || er==-7) return er;
		if(ms!=MSsoln && ms!=MSall) return -8;

		if(gpindex[PDalloc]<PDMAX+gpindex[PDnresults]+spindex[PDnresults]) {		// expand group index if needed
			pat=locateVstr(sim->mols->patlist,group,sim->mols->npattern,0);			// look for pattern in patlist list
			er=molpatternindexalloc(&(sim->mols->patindex[pat]),PDMAX+2*(gpindex[PDnresults]+spindex[PDnresults]));
			gpindex=sim->mols->patindex[pat];
			if(er) return -7; }

		for(isp=0;isp<spindex[PDnresults];isp++) {	// integrate species list into group list
			imol=spindex[PDMAX+isp];
			gpnresults=gpindex[PDnresults];
			igp=locateVi(gpindex+PDMAX,imol,gpnresults,0);
			if(igp<0) {																// not in group list so add it
				for(igp=PDMAX+gpnresults;igp>PDMAX && gpindex[igp-1]>imol;igp--)
					gpindex[igp]=gpindex[igp-1];
				gpindex[igp]=imol;
				gpindex[PDnresults]++;
				gpnresults++; }}}

	if(imol) {
		if(gpindex[PDalloc]<PDMAX+gpindex[PDnresults]+1) {		// expand group index if needed
			pat=locateVstr(sim->mols->patlist,group,sim->mols->npattern,0);			// look for pattern in patlist list
			er=molpatternindexalloc(&(sim->mols->patindex[pat]),PDMAX+2*(gpindex[PDnresults]+1));
			gpindex=sim->mols->patindex[pat];
			if(er) return -7; }

		gpnresults=gpindex[PDnresults];
		igp=locateVi(gpindex+PDMAX,imol,gpnresults,0);
		if(igp<0) {																// not in group list so add it
			for(igp=PDMAX+gpnresults;igp>PDMAX && gpindex[igp-1]>imol;igp--)
				gpindex[igp]=gpindex[igp-1];
			gpindex[igp]=imol;
			gpindex[PDnresults]++;
			gpnresults++; }}

	gpindex[PDnspecies]=-1;		// never needs updating

	return 0; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* molserno2string */
char *molserno2string(unsigned long long serno,char *string) {
	if(serno<0xFFFFFFFF)
		snprintf(string,128*sizeof(string),LLUFORMAT,serno);
	else
		snprintf(string,128*sizeof(string),LLUFORMAT "." LLUFORMAT,serno>>32,serno&0xFFFFFFFF);
	return string; }


/* molstring2serno */
unsigned long long molstring2serno(char *string) {
	char *dotptr;
	unsigned long long i1,i2,answer;

	answer=0;
	if((dotptr=strchr(string,'.'))) {
		i1=i2=0;
		*dotptr='\0';
		sscanf(string,LLUFORMAT,&i1);
		sscanf(dotptr+1,LLUFORMAT,&i2);
		*dotptr='.';
		if(i1==0 || i2==0) return 0;
		answer= i1<<32 | i2; }
	else
		sscanf(string,LLUFORMAT,&answer);

	return answer; }


/* molfindserno */
unsigned long long molfindserno(simptr sim,unsigned long long def,long int pserno,unsigned long long r1serno,unsigned long long r2serno,unsigned long long *sernolist) {
	unsigned long long answer,i1,i2;
	long int bitcode;

	if(pserno>0) return (unsigned long long)pserno;
	bitcode=~pserno;
	i1=i2=0;

	if(bitcode&0xFF00) {
		if(bitcode&0x800) {																			// p
			if((bitcode&0x300)==0) i1=sernolist[0];								// p1
			else if((bitcode&0x300)==0x100) i1=sernolist[1];			// p2
			else if((bitcode&0x300)==0x200) i1=sernolist[2];			// p3
			else i1=sernolist[3]; }																// p4
		else if(bitcode&0x400) {																// r
			if((bitcode&0x300)==0) i1=r1serno;										// r1
			else i1=r2serno; }																		// r2
		else i1=def;																						// new
		if(bitcode&0x1000) i1>>=32;															// left
		else i1&=0xFFFFFFFF; }																	// right
	else
		i1=0;

	if(bitcode&0xFF) {
		if(bitcode&0x8) {																						// p
			if((bitcode&0x3)==0) i2=sernolist[0];											// p1
			else if((bitcode&0x3)==0x1) i2=sernolist[1];							// p2
			else if((bitcode&0x3)==0x2) i2=sernolist[2];							// p3
			else i2=sernolist[3]; }																		// p4
		else if(bitcode&0x4) {																			// r
			if((bitcode&0x3)==0) i2=r1serno;													// r1
			else i2=r2serno; }																				// r2
		else if((bitcode&0xFF00)==0x100) i2=sim->mols->serno++;			// new.new
		else i2=def;																								// new
		if(bitcode&0x10) i2>>=32;																		// left
		else i2&=0xFFFFFFFF; }																			// right
	else
		i2=0;

	answer=(i1<<32)|i2;
	return answer; }


/* molismobile */
int molismobile(simptr sim,int species,enum MolecState ms) {
	molssptr mols;
	int d,dim,s;
	enum PanelShape ps;

	if(ms==MSbsoln) ms=MSsoln;
	mols=sim->mols;
	dim=sim->dim;
	if(mols->difc[species][ms]>0) return 1;
	if(mols->difm && mols->difm[species] && mols->difm[species][ms])
		for(d=0;d<dim*dim;d++)
			if(mols->difm[species][ms][d]!=0) return 1;
	if(mols->drift && mols->drift[species] && mols->drift[species][ms])
		for(d=0;d<dim;d++)
			if(mols->drift[species][ms][d]!=0) return 1;
	if(mols->surfdrift && mols->surfdrift[species] && mols->surfdrift[species][ms])
		for(s=0;s<sim->srfss->nsrf;s++)
			if(mols->surfdrift[species][ms][s])
				for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
					if(mols->surfdrift[species][ms][s][ps])
						for(d=0;d<dim-1;d++)
							if(mols->surfdrift[species][ms][s][ps][d]!=0) return 1;

	return 0; }


/* molpos2string. */
char *molpos2string(simptr sim,moleculeptr mptr,char *string) {
	int d,dim,done,p,tryagain,count;
	char *line2;
	double newpos[DIMMAX],crosspt[DIMMAX],dist;
	boxptr bptr;
	panelptr pnl;

	dim=sim->dim;
	done=0;
	dist=0;
	count=0;

	line2=string;												// write position to string
	for(d=0;d<dim;d++) {
		snprintf(line2,STRCHAR*sizeof(line2)," %g",mptr->pos[d]);
		line2+=strlen(line2); }

	if(!sim->srfss) done=1;
	while(!done) {
		line2=string;											// read in written position
		for(d=0;d<dim;d++) {
			sscanf(line2,"%lg",&newpos[d]);
			line2=strnword(line2,2); }

		tryagain=0;
		bptr=pos2box(sim,newpos);
		if(bptr!=pos2box(sim,mptr->pos)) tryagain=1;		// check for same box
		for(p=0;p<bptr->npanel && tryagain==0;p++) {		// check for no panels crossed
			pnl=bptr->panel[p];
			if(mptr->pnl!=pnl && lineXpanel(mptr->pos,newpos,pnl,dim,crosspt,NULL,NULL,NULL,NULL,NULL,0)) tryagain=1; }
		if(!tryagain) done=1;

		if(!done) {
			if(++count>50) {
				simLog(sim,8,"WARNING: unable to write %s molecule position (%s) on the correct side of all surfaces\n",sim->mols->spname[mptr->ident],string);
				return string; }

			if(dist==0) {
				for(d=0;d<dim;d++) dist+=(newpos[d]-mptr->pos[d])*(newpos[d]-mptr->pos[d]);
				dist=50*sqrt(dist); }

			line2=string;												// write position to string
			for(d=0;d<dim;d++) {
				snprintf(line2,STRCHAR*sizeof(line2)," %g",mptr->pos[d]+unirandCCD(-dist,dist));
				line2+=strlen(line2); }}}
		
		return string; }


/* molchangeident */
void molchangeident(simptr sim,moleculeptr mptr,int ll,int m,int i,enum MolecState ms,panelptr pnl) {
	int dim,ll2;
	double epsilon;

	if(i==0) {
		molkill(sim,mptr,ll,m);
		return; }

	dim=sim->dim;
	epsilon=sim->srfss?sim->srfss->epsilon:0;

	mptr->ident=i;
	mptr->mstate=ms;
	if(ms==MSsoln || ms==MSbsoln) mptr->pnl=NULL;
	else mptr->pnl=pnl;

	if(ms==MSsoln && !mptr->pnlx);								// soln -> soln
	else if(ms==MSsoln) {													// surf -> front soln
		fixpt2panel(mptr->posx,mptr->pnlx,dim,PFfront,epsilon); }
	else if(ms==MSbsoln && !mptr->pnlx)						// fsoln -> bsoln
		mptr->mstate=MSsoln;
	else if(ms==MSbsoln) {												// surf -> back soln
		mptr->mstate=MSsoln;
		fixpt2panel(mptr->posx,mptr->pnlx,dim,PFback,epsilon); }
	else if(ms==MSfront)													// any -> front surf
		fixpt2panel(mptr->pos,pnl,dim,PFfront,epsilon);
	else if(ms==MSback)														// any -> back surf
		fixpt2panel(mptr->pos,pnl,dim,PFback,epsilon);
	else																					// any -> up or down
		fixpt2panel(mptr->pos,pnl,dim,PFnone,epsilon);

	ll2=sim->mols->listlookup[i][ms];
	if(ll>=0 && ll2!=ll) {
		mptr->list=ll2;
		if(m<0) sim->mols->sortl[ll]=0;
		else if(m<sim->mols->sortl[ll]) sim->mols->sortl[ll]=m; }

	sim->mols->touch++;

	return; }


/******************************************************************************/
/***************************** set structure values ***************************/
/******************************************************************************/


/* molssetgausstable */
int molssetgausstable(simptr sim,int size) {
	int er;
	molssptr mols;
	double *newtable;

	er=molenablemols(sim,-1);
	if(er) return er;
	mols=sim->mols;

	if(mols->ngausstbl>0 && (mols->ngausstbl==size || size==-1)) return 0;
	if(size<1) size=4096;
	else if(!is2ton(size)) return 3;

	newtable=(double*) calloc(size,sizeof(double));
	CHECKMEM(newtable);
	randtableD(newtable,size,1);
	randshuffletableD(newtable,size);

	if(mols->gausstbl) free(mols->gausstbl);
	mols->ngausstbl=size;
	mols->gausstbl=newtable;
	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in molssetgausstable");
	return 1; }


/* molsetdifc */
void molsetdifc(simptr sim,int ident,int *index,enum MolecState ms,double difc) {
	int j;
	enum MolecState mslo,mshi;

	if(index) {
		for(j=0;j<index[PDnresults];j++)
			molsetdifc(sim,index[PDMAX+j],NULL,ms,difc);
		return; }

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return;
	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(0);mshi=(enum MolecState)(MSMAX);}

	for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1))
		sim->mols->difc[ident][ms]=difc;

	molsetcondition(sim->mols,SCparams,0);
	rxnsetcondition(sim,-1,SCparams,0);
	surfsetcondition(sim->srfss,SCparams,0);
	return; }


/* molsetdifm */
int molsetdifm(simptr sim,int ident,int *index,enum MolecState ms,double *difm) {
	int j,d,dim,er;
	enum MolecState mslo,mshi;
	double *difmat,dm2[DIMMAX*DIMMAX];

	if(index) {
		for(j=0;j<index[PDnresults];j++) {
			er=molsetdifm(sim,index[PDMAX+j],NULL,ms,difm);
			if(er) return er; }
		return 0; }

	dim=sim->dim;
	if(!difm) return 0;

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return 0;
	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(0);mshi=(enum MolecState)(MSMAX);}

	for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1)) {
		difmat=sim->mols->difm[ident][ms];
		if(!difmat) {
			difmat=(double*) calloc(sim->dim*sim->dim,sizeof(double));
			CHECKMEM(difmat);
			sim->mols->difm[ident][ms]=difmat; }
		for(d=0;d<sim->dim*sim->dim;d++)
			difmat[d]=difm[d];
		dotMMD(difmat,difmat,dm2,dim,dim,dim);
		sim->mols->difc[ident][ms]=traceMD(dm2,dim)/dim; }

	molsetcondition(sim->mols,SCparams,0);
	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in molsetdifm");
	return 1; }


/* molsetdrift */
int molsetdrift(simptr sim,int ident,int *index,enum MolecState ms,double *drift) {
	int j,d,er;
	enum MolecState mslo,mshi;
	double *driftvect;

	if(index) {
		for(j=0;j<index[PDnresults];j++) {
			er=molsetdrift(sim,index[PDMAX+j],NULL,ms,drift);
			if(er) return er; }
		return 0; }

	if(!drift) return 0;

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return 0;
	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(0);mshi=(enum MolecState)(MSMAX);}

	for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1)) {
		driftvect=sim->mols->drift[ident][ms];
		if(!driftvect) {
			driftvect=(double*) calloc(sim->dim,sizeof(double));
			CHECKMEM(driftvect);
			sim->mols->drift[ident][ms]=driftvect; }
		for(d=0;d<sim->dim;d++)
			driftvect[d]=drift[d]; }

	molsetcondition(sim->mols,SCparams,0);
	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in molsetdrift");
	return 1; }


/* molsetsurfdrift */
int molsetsurfdrift(simptr sim,int ident,int *index,enum MolecState ms,int surface,enum PanelShape ps,double *drift) {
	int d,i1,j,dim,s1,slo,shi,er;
	enum MolecState mslo,mshi,ms1;
	enum PanelShape ps1,pslo,pshi;
	molssptr mols;

	if(index) {
		for(j=0;j<index[PDnresults];j++) {
			er=molsetsurfdrift(sim,index[PDMAX+j],NULL,ms,surface,ps,drift);
			if(er) return er; }
		return 0; }

	dim=sim->dim;
	mols=sim->mols;

	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(1);mshi=(enum MolecState)(MSMAX);}

	if(surface!=-1) shi=(slo=surface)+1;
	else {slo=0;shi=sim->srfss->nsrf;}

	if(ps!=PSall) pshi=(enum PanelShape)((pslo=ps)+1);
	else {pslo=(enum PanelShape)(0);pshi=(enum PanelShape)(PSMAX);}

	if(!mols->surfdrift) {
		CHECKMEM(mols->surfdrift=(double*****) calloc(mols->maxspecies,sizeof(double****)));
		for(i1=0;i1<mols->maxspecies;i1++) mols->surfdrift[i1]=NULL; }

	if(!mols->surfdrift[ident]) {
		CHECKMEM(mols->surfdrift[ident]=(double****) calloc(MSMAX,sizeof(double***)));
		for(ms1=(enum MolecState)0;ms1<MSMAX;ms1=(enum MolecState)(ms1+1)) mols->surfdrift[ident][ms1]=NULL; }
	for(ms1=mslo;ms1<mshi;ms1=(enum MolecState)(ms1+1)) {
		if(!mols->surfdrift[ident][ms1]) {
			CHECKMEM(mols->surfdrift[ident][ms1]=(double***) calloc(sim->srfss->maxsrf,sizeof(double**)));
			for(s1=0;s1<sim->srfss->maxsrf;s1++) mols->surfdrift[ident][ms1][s1]=NULL; }
		for(s1=slo;s1<shi;s1++) {
			if(!mols->surfdrift[ident][ms1][s1]) {
				CHECKMEM(mols->surfdrift[ident][ms1][s1]=(double**) calloc(PSMAX,sizeof(double*)));
				for(ps1=(enum PanelShape)0;ps1<PSMAX;ps1=(enum PanelShape)(ps1+1)) mols->surfdrift[ident][ms1][s1][ps1]=NULL; }
			for(ps1=pslo;ps1<pshi;ps1=(PanelShape)(ps1+1)) {
				if(!mols->surfdrift[ident][ms1][s1][ps1]) {
					CHECKMEM(mols->surfdrift[ident][ms1][s1][ps1]=(double*) calloc(dim-1,sizeof(double)));
					for(d=0;d<dim-1;d++) mols->surfdrift[ident][ms1][s1][ps1][d]=0; }

				for(d=0;d<dim-1;d++)
					mols->surfdrift[ident][ms1][s1][ps1][d]=drift[d]; }}}
	
	molsetcondition(sim->mols,SCparams,0);
	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in molsetsurfdrift");
	return 1; }


/* molsetdisplaysize */
void molsetdisplaysize(simptr sim,int ident,int *index,enum MolecState ms,double dsize) {
	int j;
	enum MolecState mslo,mshi;

	if(index) {
		for(j=0;j<index[PDnresults];j++)
			molsetdisplaysize(sim,index[PDMAX+j],NULL,ms,dsize);
		return; }

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return;
	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(0);mshi=(enum MolecState)(MSMAX);}

	for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1))
		sim->mols->display[ident][ms]=dsize;

	return; }


/* molsetcolor */
void molsetcolor(simptr sim,int ident,int *index,enum MolecState ms,const double *color) {
	int col,j;
	enum MolecState mslo,mshi;

	if(index) {
		for(j=0;j<index[PDnresults];j++)
			molsetcolor(sim,index[PDMAX+j],NULL,ms,color);
		return; }

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return;
	if(ms!=MSall) mshi=(enum MolecState)((mslo=ms)+1);
	else {mslo=(enum MolecState)(0);mshi=(enum MolecState)(MSMAX);}

	for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1))
		for(col=0;col<3;col++)
			sim->mols->color[ident][ms][col]=color[col];

	return; }


/* molsetlistlookup */
void molsetlistlookup(simptr sim,int ident,int *index,enum MolecState ms,int ll) {
	int i,j,diffusing;
	enum MolecState ms1;

	if(index) {
		for(j=0;j<index[PDnresults];j++)
			molsetlistlookup(sim,index[PDMAX+j],NULL,ms,ll);
		return; }

	if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSnone) return;

	if(ident>=0) {																		// single species
		if(ms==MSsoln || ms==MSbsoln)
			sim->mols->listlookup[ident][MSsoln]=sim->mols->listlookup[ident][MSbsoln]=ll;
		else if(ms!=MSall)
			sim->mols->listlookup[ident][ms]=ll;
		else
			for(ms1=(enum MolecState)0;ms1<(enum MolecState)MSMAX1;ms1=(enum MolecState)(ms1+1))
				sim->mols->listlookup[ident][ms1]=ll; }

	else {																						// all diffusing or non-diffusing species
		diffusing=(ident==-7)?1:0;
		for(i=0;i<sim->mols->nspecies;i++) {
			if(ms==MSsoln || ms==MSbsoln) {
				if(molismobile(sim,i,MSsoln)==diffusing)
					sim->mols->listlookup[i][MSsoln]=sim->mols->listlookup[i][MSbsoln]=ll; }
			else if(ms!=MSall) {
				if(molismobile(sim,i,ms)==diffusing)
					sim->mols->listlookup[i][ms]=ll; }
			else {
				for(ms1=(enum MolecState)0;ms1<(enum MolecState)MSMAX1;ms1=(enum MolecState)(ms1+1))
					if(molismobile(sim,i,ms1)==diffusing)
						sim->mols->listlookup[i][ms1]=ll; }}}

	return; }


/* molsetexist */
void molsetexist(simptr sim,int ident,enum MolecState ms,int exist) {
	if(ident<=0) return;
	if(ms==MSnone) return;
	else if(ms==MSbsoln) ms=MSsoln;
	else if(ms==MSall) {
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) sim->mols->exist[ident][ms]=exist;
		return; }
	sim->mols->exist[ident][ms]=exist;
	return; }


/* molcount */
int molcount(simptr sim,int i,int *index,enum MolecState ms,int max) {
	int count,ll,nmol,top,m,j,nresults,uselist;
	moleculeptr *mlist;
	molssptr mols;
	enum MolecState msval;

	mols=sim->mols;
	if(!mols) return 0;
	if(mols->condition==SCinit) return 0;
	if(max<0) max=INT_MAX;
	count=0;

	if(i<0 && ms==MSall) {																	// all species, all states
		for(ll=0;ll<mols->nlist;ll++)
			count+=mols->sortl[ll];
		count+=sim->mols->nd-sim->mols->topd;									// count resurrected molecules
		for(ll=0;ll<mols->nlist;ll++) {												// count molecules that need sorting
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol && count<max;m++)
				if(mlist[m]->ident>0)
					count++; }}

	else if(i<0) {																					// all species, one state
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			top=mols->sortl[ll];
			for(m=0;m<top && count<max;m++)										// count properly sorted molecules
				if(mlist[m]->ident>0 && mlist[m]->mstate==ms)
					count++; }
		mlist=mols->dead;																			// count resurrected molecules
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol && count<max;m++)
			if(mlist[m]->ident>0 && mlist[m]->mstate==ms)
				count++;
		for(ll=0;ll<mols->nlist;ll++) {												// count molecules that need sorting
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol && count<max;m++)
				if(mlist[m]->ident>0 && mlist[m]->mstate==ms)
					count++; }}

	else if(i>0 && ms==MSall) {															// one species, all states
		for(ll=0;ll<mols->nlist;ll++) {
			uselist=0;
			for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
				if(mols->listlookup[i][msval]==ll) uselist=1;
			if(uselist) {
				mlist=mols->live[ll];
				top=mols->sortl[ll];
				for(m=0;m<top && count<max;m++)										// count properly sorted molecules
					if(mlist[m]->ident==i)
						count++; }}
		mlist=mols->dead;																				// count resurrected molecules
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol && count<max;m++)
			if(mlist[m]->ident==i)
				count++;
		for(ll=0;ll<mols->nlist;ll++) {													// count molecules that need sorting
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol && count<max;m++)
				if(mlist[m]->ident==i)
					count++; }}

	else if(i>0) {																					// single species, single state
		ll=mols->listlookup[i][ms];
		mlist=mols->live[ll];
		top=mols->sortl[ll];
		for(m=0;m<top && count<max;m++)												// count properly sorted molecules
			if(mlist[m]->ident==i && mlist[m]->mstate==ms) count++;
		mlist=mols->dead;																			// count resurrected molecules
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol && count<max;m++)
			if(mlist[m]->ident==i && mlist[m]->mstate==ms) count++;
		for(ll=0;ll<mols->nlist;ll++) {												// count molecules that need sorting
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol && count<max;m++)
				if(mlist[m]->ident==i && mlist[m]->mstate==ms) count++; }}

	else if(index) {																				// list of molecules in index
		nresults=index[PDnresults];
		for(ll=0;ll<mols->nlist;ll++) {
			uselist=0;
			if(ms==MSall) {
				for(j=0;j<nresults;j++)
					for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
						if(mols->listlookup[index[PDMAX+j]][msval]==ll) uselist=1; }
			else {
				for(j=0;j<nresults;j++)
					if(mols->listlookup[index[PDMAX+j]][ms]==ll) uselist=1; }
			if(uselist) {
				mlist=mols->live[ll];
				top=mols->sortl[ll];
				for(m=0;m<top && count<max;m++)									// count properly sorted molecules
					if(locateVi(index+PDMAX,mlist[m]->ident,nresults,0)>=0 && (ms==MSall || mlist[m]->mstate==ms))
						count++; }}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol && count<max;m++)
			if(locateVi(index+PDMAX,mlist[m]->ident,nresults,0)>=0 && (ms==MSall || mlist[m]->mstate==ms))
				count++;
		for(ll=0;ll<mols->nlist;ll++) {												// count molecules that need sorting
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol && count<max;m++)
				if(locateVi(index+PDMAX,mlist[m]->ident,nresults,0)>=0 && (ms==MSall || mlist[m]->mstate==ms))
					count++; }}

	return count; }


/* molscancmd */
void molscancmd(simptr sim,int i,int *index,enum MolecState ms,cmdptr cmd,enum CMDcode(*fn)(simptr,cmdptr,char*)) {
	int ll,nmol,top,m,j,nresults,uselist;
	moleculeptr *mlist,mptr;
	molssptr mols;
	enum MolecState msval;
	enum CMDcode code;

	mols=sim->mols;
	if(!mols) return;
	code=CMDok;

	if(i<0 && ms==MSall) {																	// all species, all states
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident>0) {
					code=(*fn)(sim,cmd,(char*)mptr);
					if(code!=CMDok) return; }}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident>0) {
				code=(*fn)(sim,cmd,(char*)mptr);
				if(code!=CMDok) return; }}}

	else if(i<0) {																					// all species, one state
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident>0 && mptr->mstate==ms) {
					code=(*fn)(sim,cmd,(char*)mptr);
					if(code!=CMDok) return; }}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident>0 && mptr->mstate==ms) {
				code=(*fn)(sim,cmd,(char*)mptr);
				if(code!=CMDok) return; }}}

	else if(i>0 && ms==MSall) {															// one species, all states
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			uselist=0;
			for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
				if(mols->listlookup[i][msval]==ll) uselist=1;
			if(uselist) {
				for(m=0;m<nmol;m++) {
					mptr=mlist[m];
					if(mptr->ident==i) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}
			else {
				for(m=top;m<nmol;m++) {
					mptr=mlist[m];
					if(mptr->ident==i) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident==i) {
				code=(*fn)(sim,cmd,(char*)mptr);
				if(code!=CMDok) return; }}}

	else if(i>0) {																					// single species, single state
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			uselist=(mols->listlookup[i][ms]==ll)?1:0;
			if(uselist) {
				for(m=0;m<nmol;m++) {
					mptr=mlist[m];
					if(mptr->ident==i && mptr->mstate==ms) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}
			else {
				for(m=top;m<nmol;m++) {
					mptr=mlist[m];
					if(mptr->ident==i && mptr->mstate==ms) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident==i && mptr->mstate==ms) {
				code=(*fn)(sim,cmd,(char*)mptr);
				if(code!=CMDok) return; }}}

	else if(index) {																				// list of molecules in index
		nresults=index[PDnresults];
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			uselist=0;
			if(ms==MSall) {
				for(j=0;j<nresults;j++)
					for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
						if(mols->listlookup[index[PDMAX+j]][msval]==ll) uselist=1; }
			else {
				for(j=0;j<nresults;j++)
					if(mols->listlookup[index[PDMAX+j]][ms]==ll) uselist=1; }
			if(uselist) {
				for(m=0;m<nmol;m++) {
					mptr=mlist[m];
					if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}
			else {
				for(m=top;m<nmol;m++) {
					mptr=mlist[m];
					if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
						code=(*fn)(sim,cmd,(char*)mptr);
						if(code!=CMDok) return; }}}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
				code=(*fn)(sim,cmd,(char*)mptr);
				if(code!=CMDok) return; }}}

	return; }


/* molscanfn */
void molscanfn(simptr sim,int i,int *index,enum MolecState ms,char *erstr,double(*fn)(void*,char*,char*)) {
	int ll,nmol,top,m,j,nresults,uselist;
	moleculeptr *mlist,mptr;
	molssptr mols;
	enum MolecState msval;
	double code;

	mols=sim->mols;
	if(!mols) return;

	if(i<0 && ms==MSall) {																	// all species, all states
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident>0) {
					code=(*fn)(sim,erstr,(char*)mptr);
					if(code!=0) return; }}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident>0) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}}

	else if(i<0) {																					// all species, one state
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident>0 && mptr->mstate==ms) {
					code=(*fn)(sim,erstr,(char*)mptr);
					if(code!=0) return; }}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident>0 && mptr->mstate==ms) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}}

	else if(i>0 && ms==MSall) {															// one species, all states
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			top=mols->sortl[ll];
			uselist=0;
			for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
				if(mols->listlookup[i][msval]==ll) uselist=1;
			if(uselist)
				for(m=0;m<top;m++) {
					mptr=mlist[m];
					if(mptr->ident==i) {
						code=(*fn)(sim,erstr,(char*)mptr);
						if(code!=0) return; }}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident==i) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			top=mols->sortl[ll];
			nmol=mols->nl[ll];
			for(m=top;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident==i) {
					code=(*fn)(sim,erstr,(char*)mptr);
					if(code!=0) return; }}}}

	else if(i>0) {																					// single species, single state
		ll=mols->listlookup[i][ms];
		mlist=mols->live[ll];
		top=mols->sortl[ll];
		for(m=0;m<top;m++) {
			mptr=mlist[m];
			if(mptr->ident==i && mptr->mstate==ms) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(mptr->ident==i && mptr->mstate==ms) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			top=mols->sortl[ll];
			nmol=mols->nl[ll];
			for(m=top;m<nmol;m++) {
				mptr=mlist[m];
				if(mptr->ident==i && mptr->mstate==ms) {
					code=(*fn)(sim,erstr,(char*)mptr);
					if(code!=0) return; }}}}

	else if(index) {																				// list of molecules in index
		nresults=index[PDnresults];
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			top=mols->sortl[ll];
			uselist=0;
			if(ms==MSall) {
				for(j=0;j<nresults;j++)
					for(msval=(enum MolecState)0;msval<MSMAX && !uselist;msval=(enum MolecState)(msval+1))
						if(mols->listlookup[index[PDMAX+j]][msval]==ll) uselist=1; }
			else {
				for(j=0;j<nresults;j++)
					if(mols->listlookup[index[PDMAX+j]][ms]==ll) uselist=1; }
			if(uselist) {
				for(m=0;m<top;m++) {
					mptr=mlist[m];
					if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
						code=(*fn)(sim,erstr,(char*)mptr);
						if(code!=0) return; }}}}
		mlist=mols->dead;
		nmol=mols->nd;
		top=mols->topd;
		for(m=top;m<nmol;m++) {
			mptr=mlist[m];
			if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
				code=(*fn)(sim,erstr,(char*)mptr);
				if(code!=0) return; }}
		for(ll=0;ll<mols->nlist;ll++) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			top=mols->sortl[ll];
			for(m=top;m<nmol;m++) {
				mptr=mlist[m];
				if(locateVi(index+PDMAX,mptr->ident,nresults,0)>=0 && (ms==MSall || mptr->mstate==ms)) {
					code=(*fn)(sim,erstr,(char*)mptr);
					if(code!=0) return; }}}}

	return; }


/* molismatch */
int molismatch(moleculeptr mptr,int i,int *index,enum MolecState ms) {
	if(ms!=MSall && mptr->mstate!=ms) return 0;
	if(i==-5) return 1;
	if(mptr->ident==i) return 1;
	if(locateVi(index+PDMAX,mptr->ident,index[PDnresults],0)==-1) return 0;
	return 1; }


/* MolCalcDifcSum */
double MolCalcDifcSum(simptr sim,int i1,enum MolecState ms1,int i2,enum MolecState ms2) {
	double sum;

	sum=0;
	if(i1) {
		if(ms1>=MSMAX) ms1=MSsoln;
		sum+=sim->mols->difc[i1][ms1]; }
	if(i2) {
		if(ms2>=MSMAX) ms2=MSsoln;
		sum+=sim->mols->difc[i2][ms2]; }
	return sum; }


/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/

/* molalloc */
moleculeptr molalloc(int dim) {
	moleculeptr mptr;
	int d;

	mptr=NULL;
	CHECKMEM(mptr=(moleculeptr) malloc(sizeof(struct moleculestruct)));
	mptr->serno=0;
	mptr->list=-1;
	mptr->pos=NULL;
	mptr->posx=NULL;
	mptr->via=NULL;
	mptr->posoffset=NULL;
	mptr->ident=0;
	mptr->mstate=MSsoln;
	mptr->box=NULL;
	mptr->pnl=NULL;
	mptr->pnlx=NULL;

	CHECKMEM(mptr->pos=(double*) calloc(dim,sizeof(double)));
	CHECKMEM(mptr->posx=(double*) calloc(dim,sizeof(double)));
	CHECKMEM(mptr->via=(double*) calloc(dim,sizeof(double)));
	CHECKMEM(mptr->posoffset=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++)
		mptr->pos[d]=mptr->posx[d]=mptr->via[d]=mptr->posoffset[d]=0;
	return mptr;
 failure:
	molfree(mptr);
	simLog(NULL,10,"Unable to allocate memory in molalloc");
	return NULL; }


/* molfree */
void molfree(moleculeptr mptr) {
	if(!mptr) return;
	if(mptr->pos) free(mptr->pos);
	if(mptr->posx) free(mptr->posx);
	if(mptr->posoffset) free(mptr->posoffset);
	if(mptr->via) free(mptr->via);
	free(mptr);
	return; }


/* molexpandsurfdrift */
int molexpandsurfdrift(simptr sim,int oldmaxspec,int oldmaxsrf) {	//?? needs to be called whenever maxspecies or maxsrf increase
	double *****oldsurfdrift;
	int i,s;
	enum MolecState ms;
	enum PanelShape ps;
	
	if(!sim->mols->surfdrift) return 0;
	oldsurfdrift=sim->mols->surfdrift;
	sim->mols->surfdrift=NULL;
	
	for(i=0;i<oldmaxspec;i++)
		if(oldsurfdrift[i])
			for(ms=(enum MolecState)0;ms<MSMAX;ms=(enum MolecState)(ms+1))
				if(oldsurfdrift[i][ms])
					for(s=0;s<oldmaxsrf;s++)
						if(oldsurfdrift[i][ms][s])
							for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
								if(oldsurfdrift[i][ms][s][ps]) {
									CHECK(molsetsurfdrift(sim,i,NULL,ms,s,ps,oldsurfdrift[i][ms][s][ps])==0); }
	
	molfreesurfdrift(oldsurfdrift,oldmaxspec,oldmaxsrf);
	return 0;
failure:
	return 1; }


/* molfreesurfdrift */
void molfreesurfdrift(double *****surfdrift,int maxspec,int maxsrf) {
	int i,s;
	enum MolecState ms;
	enum PanelShape ps;
	
	if(surfdrift) {
		for(i=0;i<maxspec;i++)
			if(surfdrift[i]) {
				for(ms=(enum MolecState)0;ms<MSMAX;ms=(enum MolecState)(ms+1))
					if(surfdrift[i][ms]) {
						for(s=0;s<maxsrf;s++)
							if(surfdrift[i][ms][s]) {
								for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
									free(surfdrift[i][ms][s][ps]);
								free(surfdrift[i][ms][s]); }
						free(surfdrift[i][ms]); }
				free(surfdrift[i]); }
		free(surfdrift); }
	return; }


/* molpatternindexalloc */
int molpatternindexalloc(int **indexptr,int n) {
	int j;
	int *index,*newindex;

	index=*indexptr;
	if(n<PDMAX) n=(index?index[PDalloc]*2:PDMAX+1);
	newindex=(int*) calloc(n,sizeof(int));
	if(!newindex) return 1;
	j=0;
	if(index)
		for(;j<index[PDalloc] && j<n;j++)
			newindex[j]=index[j];
	for(;j<n;j++)
		newindex[j]=0;
	newindex[PDalloc]=n;
	free(index);
	*indexptr=newindex;
	return 0; }


/* molpatternalloc */
int molpatternalloc(simptr sim,int maxpattern) {
	int i,er;
	char **newpatlist;
	int **newpatindex;
	char **newpatrname;

	newpatlist=(char **) calloc(maxpattern,sizeof(char*));
	if(!newpatlist) return 1;
	newpatindex=(int **) calloc(maxpattern,sizeof(int*));
	if(!newpatindex) return 1;
	newpatrname=(char **) calloc(maxpattern,sizeof(char*));
	if(!newpatrname) return 1;

	for(i=0;i<sim->mols->maxpattern;i++) {
		newpatlist[i]=sim->mols->patlist[i];
		newpatindex[i]=sim->mols->patindex[i];
		newpatrname[i]=sim->mols->patrname[i]; }
	for(;i<maxpattern;i++) {
		newpatlist[i]=EmptyString();
		if(!newpatlist[i]) return 1;
		newpatlist[i][0]='\0';
		newpatindex[i]=NULL;
		er=molpatternindexalloc(&newpatindex[i],PDMAX+1);
		if(er) return 1;
		newpatrname[i]=NULL; }

	free(sim->mols->patlist);
	free(sim->mols->patindex);
	free(sim->mols->patrname);
	sim->mols->maxpattern=maxpattern;
	sim->mols->patlist=newpatlist;
	sim->mols->patindex=newpatindex;
	sim->mols->patrname=newpatrname;
	return 0; }


/* molssalloc */
molssptr molssalloc(molssptr mols,int maxspecies) {
	int i,**newexist,**newlistlookup,*newexpand,oldmaxspecies;
	enum MolecState ms;
	char **newspname;
	double **newdifc,**newdifstep,***newdifm,***newdrift,**newdisplay,***newcolor;

	if(maxspecies<1) return NULL;
	maxspecies++;


	if(!mols) {
		mols=(molssptr) malloc(sizeof(struct molsuperstruct));
		CHECKMEM(mols);

		mols->condition=SCinit;
		mols->sim=NULL;
		mols->maxspecies=0;
		mols->nspecies=1;
		mols->spname=NULL;
		mols->maxpattern=0;
		mols->npattern=0;
		mols->patlist=NULL;
		mols->patindex=NULL;
		mols->patrname=NULL;
		mols->difc=NULL;
		mols->difstep=NULL;
		mols->difm=NULL;
		mols->drift=NULL;
		mols->surfdrift=NULL;
		mols->display=NULL;
		mols->color=NULL;
		mols->exist=NULL;
		mols->dead=NULL;
		mols->maxdlimit=-1;
		mols->maxd=0;
		mols->nd=0;
		mols->topd=0;
		mols->maxlist=0;
		mols->nlist=0;
		mols->listlookup=NULL;
		mols->listname=NULL;
		mols->listtype=NULL;
		mols->live=NULL;
		mols->maxl=NULL;
		mols->nl=NULL;
		mols->topl=NULL;
		mols->sortl=NULL;
		mols->diffuselist=NULL;
		mols->serno=1;
		mols->ngausstbl=0;
		mols->gausstbl=NULL;
		mols->expand=NULL;
		mols->touch=0; }

	if(maxspecies>mols->maxspecies) {
		oldmaxspecies=mols->maxspecies;

		CHECKMEM(newspname=(char**) calloc(maxspecies,sizeof(char*)));
		for(i=0;i<maxspecies;i++) newspname[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newspname[i]=mols->spname[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newspname[i]=EmptyString()); }

		strncpy(newspname[0],"empty",STRCHAR-1);

		CHECKMEM(newdifc=(double**) calloc(maxspecies,sizeof(double*)));
		for(i=0;i<maxspecies;i++) newdifc[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newdifc[i]=mols->difc[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newdifc[i]=(double*) calloc(MSMAX,sizeof(double)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newdifc[i][ms]=0; }

		CHECKMEM(newdifstep=(double**) calloc(maxspecies,sizeof(double*)));
		for(i=0;i<maxspecies;i++) newdifstep[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newdifstep[i]=mols->difstep[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newdifstep[i]=(double*) calloc(MSMAX,sizeof(double)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newdifstep[i][ms]=0; }

		CHECKMEM(newdifm=(double***) calloc(maxspecies,sizeof(double**)));
		for(i=0;i<maxspecies;i++) newdifm[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newdifm[i]=mols->difm[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newdifm[i]=(double**) calloc(MSMAX,sizeof(double*)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newdifm[i][ms]=NULL; }

		CHECKMEM(newdrift=(double***) calloc(maxspecies,sizeof(double**)));
		for(i=0;i<maxspecies;i++) newdrift[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newdrift[i]=mols->drift[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newdrift[i]=(double**) calloc(MSMAX,sizeof(double*)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newdrift[i][ms]=NULL; }

		CHECKMEM(newdisplay=(double**) calloc(maxspecies,sizeof(double*)));
		for(i=0;i<maxspecies;i++) newdisplay[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newdisplay[i]=mols->display[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newdisplay[i]=(double*) calloc(MSMAX,sizeof(double)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newdisplay[i][ms]=3; }

		CHECKMEM(newcolor=(double ***) calloc(maxspecies,sizeof(double **)));
		for(i=0;i<maxspecies;i++) newcolor[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newcolor[i]=mols->color[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newcolor[i]=(double**) calloc(MSMAX,sizeof(double*)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newcolor[i][ms]=NULL;
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
				CHECKMEM(newcolor[i][ms]=(double*) calloc(3,sizeof(double)));
				newcolor[i][ms][0]=newcolor[i][ms][1]=newcolor[i][ms][2]=0; }}

		CHECKMEM(newexist=(int**) calloc(maxspecies,sizeof(int*)));
		for(i=0;i<maxspecies;i++) newexist[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newexist[i]=mols->exist[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newexist[i]=(int*) calloc(MSMAX,sizeof(int)));
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) newexist[i][ms]=0; }

		CHECKMEM(newlistlookup=(int**) calloc(maxspecies,sizeof(int*)));
		for(i=0;i<maxspecies;i++) newlistlookup[i]=NULL;
		for(i=0;i<oldmaxspecies;i++) newlistlookup[i]=mols->listlookup[i];
		for(;i<maxspecies;i++) {
			CHECKMEM(newlistlookup[i]=(int*) calloc(MSMAX1,sizeof(int)));
			for(ms=(enum MolecState)(0);ms<MSMAX1;ms=(enum MolecState)(ms+1)) newlistlookup[i][ms]=-1; }

		CHECKMEM(newexpand=(int*) calloc(maxspecies,sizeof(int)));
		for(i=0;i<oldmaxspecies;i++) newexpand[i]=mols->expand[i];
		for(;i<maxspecies;i++) newexpand[i]=0;

		mols->maxspecies=maxspecies;
		free(mols->spname);
		mols->spname=newspname;
		free(mols->difc);
		mols->difc=newdifc;
		free(mols->difstep);
		mols->difstep=newdifstep;
		free(mols->difm);
		mols->difm=newdifm;
		free(mols->drift);
		mols->drift=newdrift;
		free(mols->display);
		mols->display=newdisplay;
		free(mols->color);
		mols->color=newcolor;
		free(mols->exist);
		mols->exist=newexist;
		free(mols->listlookup);
		mols->listlookup=newlistlookup;
		free(mols->expand);
		mols->expand=newexpand;
	
		if(mols->surfdrift && mols->sim->srfss) {
			CHECK(molexpandsurfdrift(mols->sim,oldmaxspecies,mols->sim->srfss->maxsrf)==0); }}

	return mols;

 failure:
	simLog(NULL,10,"Unable to allocate memory in molssalloc");
	return NULL; }


/* mollistalloc */
int mollistalloc(molssptr mols,int maxlist,enum MolListType mlt) {
	int *maxl,*nl,*topl,*sortl,*diffuselist,ll,m;
	moleculeptr **live,mptr;
	char **listname;
	enum MolListType *listtype;

	if(maxlist<=0) return -2;
	if(!mols) return -3;
	maxlist+=mols->maxlist;

	listname=NULL;							// allocate new arrays
	listtype=NULL;
	live=NULL;
	maxl=NULL;
	nl=NULL;
	topl=NULL;
	sortl=NULL;
	diffuselist=NULL;

	CHECKMEM(listname=(char**) calloc(maxlist,sizeof(char*)));
	for(ll=0;ll<maxlist;ll++) listname[ll]=NULL;

	CHECKMEM(listtype=(enum MolListType*) calloc(maxlist,sizeof(enum MolListType)));
	for(ll=0;ll<maxlist;ll++) listtype[ll]=MLTnone;

	CHECKMEM(live=(moleculeptr**) calloc(maxlist,sizeof(moleculeptr*)));
	for(ll=0;ll<maxlist;ll++) live[ll]=NULL;

	CHECKMEM(maxl=(int*) calloc(maxlist,sizeof(int)));
	for(ll=0;ll<maxlist;ll++) maxl[ll]=0;

	CHECKMEM(nl=(int*) calloc(maxlist,sizeof(int)));
	for(ll=0;ll<maxlist;ll++) nl[ll]=0;

	CHECKMEM(topl=(int*) calloc(maxlist,sizeof(int)));
	for(ll=0;ll<maxlist;ll++) topl[ll]=0;

	CHECKMEM(sortl=(int*) calloc(maxlist,sizeof(int)));
	for(ll=0;ll<maxlist;ll++) sortl[ll]=0;

	CHECKMEM(diffuselist=(int*) calloc(maxlist,sizeof(int)));
	for(ll=0;ll<maxlist;ll++) diffuselist[ll]=0;

	for(ll=0;ll<mols->maxlist;ll++) {			// copy over existing portions
		listname[ll]=mols->listname[ll];
		listtype[ll]=mols->listtype[ll];
		live[ll]=mols->live[ll];
		maxl[ll]=mols->maxl[ll];
		nl[ll]=mols->nl[ll];
		topl[ll]=mols->topl[ll];
		sortl[ll]=mols->sortl[ll];
		diffuselist[ll]=mols->diffuselist[ll]; }

	for(ll=mols->maxlist;ll<maxlist;ll++) {					// listnames and listtypes
		CHECKMEM(listname[ll]=EmptyString());
		listtype[ll]=mlt; }

	for(ll=mols->maxlist;ll<maxlist;ll++) maxl[ll]=1;			// calculate maxl
	for(m=mols->topd;m<mols->nd;m++) {
		mptr=mols->dead[m];
		if(mptr && mptr->list>=mols->maxlist && mptr->list<maxlist) maxl[mptr->list]++; }
	for(ll=mols->maxlist;ll<maxlist;ll++) {
		maxl[ll]*=2;
		if(maxl[ll]>mols->maxd) maxl[ll]=mols->maxd; }

	for(ll=mols->maxlist;ll<maxlist;ll++) {			// allocate live lists
		CHECKMEM(live[ll]=(moleculeptr*) calloc(maxl[ll],sizeof(moleculeptr)));
		for(m=0;m<maxl[ll];m++) live[ll][m]=NULL; }

	if(mols->maxlist) {										// free any old lists
		free(mols->listname);
		free(mols->listtype);
		free(mols->live);
		free(mols->maxl);
		free(mols->nl);
		free(mols->topl);
		free(mols->sortl);
		free(mols->diffuselist); }
	ll=mols->maxlist;
	mols->maxlist=maxlist;									// store new lists
	mols->listname=listname;
	mols->listtype=listtype;
	mols->live=live;
	mols->maxl=maxl;
	mols->nl=nl;
	mols->topl=topl;
	mols->sortl=sortl;
	mols->diffuselist=diffuselist;
	return ll;

 failure:
	if(listname)
		for(ll=0;ll<maxlist;ll++) free(listname[ll]);
	free(listname);
	free(listtype);
	if(live)
		for(ll=mols->maxlist;ll<maxlist;ll++) free(live[ll]);
	free(live);
	free(maxl);
	free(nl);
	free(topl);
	free(sortl);
	free(diffuselist);
	simLog(NULL,10,"Unable to allocate memory in mollistalloc");
	return -1; }


/* molexpandlist */
int molexpandlist(molssptr mols,int dim,int ll,int nspaces,int nmolecs) {
	moleculeptr *newlist,*oldlist;
	int m,nold,maxold,maxnew;

	if(!mols || ll>=mols->nlist) return 2;
	if(ll>=0 && nmolecs>0) return 2;							// can't add molecules to live list

	maxold=ll<0?mols->maxd:mols->maxl[ll];				// maxold is previous allocated size
	nold=ll<0?mols->nd:mols->nl[ll];							// nold is previous number of molecules
	oldlist=ll<0?mols->dead:mols->live[ll];				// oldlist is previous list

	maxnew=nspaces>0?maxold+nspaces:2*maxold+1;		// maxnew is new allocated size
	if(nold+nmolecs>maxnew) return 3;

	newlist=(moleculeptr*) calloc(maxnew,sizeof(moleculeptr));
	CHECKMEM(newlist);
	for(m=0;m<maxold;m++) newlist[m]=oldlist[m];
	for(;m<maxnew;m++) newlist[m]=NULL;
	if(ll<0) {
		free(mols->dead);
		mols->dead=newlist;
		mols->maxd=maxnew; }
	else {
		free(mols->live[ll]);
		mols->live[ll]=newlist;
		mols->maxl[ll]=maxnew; }

	if(nmolecs) {
		for(m=mols->nd-1;m>=mols->topd;m--) {					// copy resurrected molecules higher on list
			newlist[m+nmolecs]=newlist[m];
			newlist[m]=NULL; }
		for(m=mols->topd;m<mols->topd+nmolecs;m++) {		// create new empty molecules
			newlist[m]=molalloc(dim);
			if(!newlist[m]) return 4; }
		mols->topd+=nmolecs;
		mols->nd+=nmolecs; }
	return 0;
failure:
	simLog(NULL,10,"Unable to allocate memory in molexpandlist");
	return 1; }


/* molssfree */
void molssfree(molssptr mols,int maxsrf) {
	int m,ll,i,maxspecies;
	enum MolecState ms;

	if(!mols) return;
	maxspecies=mols->maxspecies;

	free(mols->expand);

	free(mols->gausstbl);

	for(ll=0;ll<mols->maxlist;ll++) {
		if(mols->listname) free(mols->listname[ll]);
		if(mols->live && mols->live[ll]) {
			for(m=0;m<mols->nl[ll];m++) molfree(mols->live[ll][m]);
			free(mols->live[ll]); }}
	free(mols->diffuselist);
	free(mols->sortl);
	free(mols->topl);
	free(mols->nl);
	free(mols->maxl);
	free(mols->live);
	free(mols->listtype);
	free(mols->listname);

	if(mols->listlookup) {
		for(i=0;i<maxspecies;i++) free(mols->listlookup[i]);
		free(mols->listlookup); }

	if(mols->exist) {
		for(i=0;i<maxspecies;i++) free(mols->exist[i]);
		free(mols->exist); }

	if(mols->dead) {
		for(m=0;m<mols->nd;m++) molfree(mols->dead[m]);
		free(mols->dead); }

	if(mols->color) {
		for(i=0;i<maxspecies;i++)
			if(mols->color[i]) {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) free(mols->color[i][ms]);
				free(mols->color[i]); }
		free(mols->color); }

	if(mols->display) {
		for(i=0;i<maxspecies;i++) free(mols->display[i]);
		free(mols->display); }

	molfreesurfdrift(mols->surfdrift,mols->maxspecies,maxsrf);
	
	if(mols->drift) {
		for(i=0;i<maxspecies;i++)
			if(mols->drift[i]) {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) free(mols->drift[i][ms]);
				free(mols->drift[i]); }
		free(mols->drift); }

	if(mols->difm) {
		for(i=0;i<maxspecies;i++)
			if(mols->difm[i]) {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) free(mols->difm[i][ms]);
				free(mols->difm[i]); }
		free(mols->difm); }

	if(mols->difstep) {
		for(i=0;i<maxspecies;i++) free(mols->difstep[i]);
		free(mols->difstep); }

	if(mols->difc) {
		for(i=0;i<maxspecies;i++) free(mols->difc[i]);
		free(mols->difc); }

	if(mols->patindex) {
		for(i=0;i<mols->maxpattern;i++) free(mols->patindex[i]);
		free(mols->patindex); }

	if(mols->patrname) {
		for(i=0;i<mols->maxpattern;i++) free(mols->patrname[i]);
		free(mols->patrname); }

	if(mols->patlist) {
		for(i=0;i<mols->maxpattern;i++) free(mols->patlist[i]);
		free(mols->patlist); }

	if(mols->spname) {
		for(i=0;i<maxspecies;i++) free(mols->spname[i]);
		free(mols->spname); }

	free(mols);
	return; }


/******************************************************************************/
/*************************** data structure output ****************************/
/******************************************************************************/

/* molssoutput */
void molssoutput(simptr sim) {
	int nspecies,i,ll,same,sum,s,pat,*index,vflag;
	molssptr mols;
	char string[STRCHAR];
	double maxstep;
	enum MolecState ms;
	enum PanelShape ps;

	vflag=strchr(sim->flags,'v')?1:0;

	simLog(sim,2,"MOLECULE PARAMETERS\n");
	if(!sim || !sim->mols) {
		simLog(sim,2," No molecule superstructure defined\n\n");
		return; }
	mols=sim->mols;
	nspecies=mols->nspecies;

	if(mols->condition!=SCok)
		simLog(sim,7," Molecule superstructure condition: %s\n",simsc2string(mols->condition,string));
	simLog(sim,1," Next molecule serial number: %lu\n",mols->serno);
	if(mols->gausstbl) simLog(sim,1," Table for Gaussian distributed random numbers has %i values\n",mols->ngausstbl);
	else simLog(sim,1," Table for Gaussian distributed random numbers has not been set up\n");

	simLog(sim,2," %i molecule lists:\n",mols->nlist);
	for(ll=0;ll<mols->nlist;ll++) {
		if(vflag) {
			if(mols->live[ll]==NULL) simLog(sim,1,"  list %i is not allocated\n",ll);
			simLog(sim,1,"  %s: type=%s, allocated size=%i, number of molecules=%i",mols->listname[ll],molmlt2string(mols->listtype[ll],string),mols->maxl[ll],mols->nl[ll]);
			if(mols->topl[ll]!=mols->nl[ll] && mols->topl!=0) simLog(sim,1,", top value=%i",mols->topl[ll]);
			if(mols->sortl[ll]!=mols->nl[ll]) simLog(sim,1,", sort value=%i",mols->sortl[ll]);
			simLog(sim,1,"\n"); }
		else
			simLog(sim,2,"%s%s%s",ll==0?"  ":" ",mols->listname[ll],ll==mols->nlist-1?"\n":","); }

	simLog(sim,1," Diffusion molecule lists:");
	for(ll=0;ll<mols->nlist;ll++)
		if(mols->diffuselist[ll]) simLog(sim,1," %s",mols->listname[ll]);
	simLog(sim,1,"\n");

	if(mols->npattern>0) {
		simLog(sim,1," Species groups:\n");
		for(pat=0;pat<mols->npattern;pat++) {
			index=mols->patindex[pat];
			if(index[PDmatch]==1 && index[PDsubst]==0) {
				simLog(sim,1,"  %s:",mols->patlist[pat]);
				for(i=0;i<index[PDnresults];i++)
					simLog(sim,1," %s",mols->spname[index[PDMAX+i]]);
				simLog(sim,1,"\n"); }}}

	simLog(sim,1," %i species allocated\n",mols->maxspecies-1);
	simLog(sim,2," %i species defined:\n",mols->nspecies-1);
	maxstep=-1;
	for(i=1;i<nspecies;i++) {
		simLog(sim,2," %s:\n",mols->spname[i]);
		simLog(sim,1,"  states used:");
		sum=0;
		if(mols->exist && mols->exist[i]) {
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
				if(mols->exist[i][ms]) {
					sum++;
					simLog(sim,1," %s",molms2string(ms,string)); }}
		if(!sum) simLog(sim,1," none");
		simLog(sim,1,"\n");

		same=1;
		if(mols->difc && mols->difc[i] && mols->difm && mols->difm[i] && mols->drift && mols->drift[i] && mols->listlookup && mols->listlookup[i] && mols->difstep && mols->difstep[i]) {
			for(ms=(enum MolecState)(0);ms<MSMAX && same;ms=(enum MolecState)(ms+1)) {
				if(mols->difc[i][ms]!=mols->difc[i][MSsoln]) same=0;
				if(mols->difm[i][ms] && !mols->difm[i][MSsoln]) same=0;
				if(!mols->difm[i][ms] && mols->difm[i][MSsoln]) same=0;
				if(mols->drift[i][ms] && !mols->drift[i][MSsoln]) same=0;
				if(!mols->drift[i][ms] && mols->drift[i][MSsoln]) same=0;
				if(mols->listlookup[i][ms]!=mols->listlookup[i][MSsoln]) same=0; }
			if(same) {
				if(mols->difstep[i][MSsoln]>maxstep) maxstep=mols->difstep[i][MSsoln];
				simLog(sim,2,"  all states: difc=%g, rms step=%g",mols->difc[i][MSsoln],mols->difstep[i][MSsoln]);
				if(mols->difm[i][MSsoln]) simLog(sim,2," (anisotropic)");
				if(mols->drift[i][MSsoln]) simLog(sim,2," (drift)");
				if(mols->listname) simLog(sim,2,", list=%s",mols->listname[mols->listlookup[i][MSsoln]]);
				simLog(sim,2,", number=%i\n",molcount(sim,i,NULL,MSall,-1)); }
			else {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
					if(mols->exist && mols->exist[i] && mols->exist[i][ms]) {
						if(mols->difstep[i][ms]>maxstep) maxstep=mols->difstep[i][ms];
						simLog(sim,2,"  %s: difc=%g, rms step=%g",molms2string(ms,string),mols->difc[i][ms],mols->difstep[i][ms]);
						if(mols->difm[i][ms]) simLog(sim,2," (anisotropic)");
						if(mols->drift[i][ms]) simLog(sim,2," (drift)");
						if(mols->listname) simLog(sim,2,", list=%s",mols->listname[mols->listlookup[i][ms]]);
						simLog(sim,2,", number=%i\n",molcount(sim,i,NULL,ms,-1)); }}}

		if(mols->surfdrift && mols->surfdrift[i]) {
			simLog(sim,2,"  surface drift:\n");
			for(ms=(enum MolecState)1;ms<MSMAX;ms=(enum MolecState)(ms+1))
				if(mols->surfdrift[i][ms] && sim->srfss) {
					simLog(sim,2,"   %s:",molms2string(ms,string));
					for(s=0;s<sim->srfss->nsrf;s++)
						if(mols->surfdrift[i][ms][s])
							for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
								if(mols->surfdrift[i][ms][s][ps]) {
									simLog(sim,2," %s,%s",sim->srfss->snames[s],surfps2string(ps,string));
									if(sim->dim==2) simLog(sim,2,"=%g",mols->surfdrift[i][ms][s][ps][0]);
									else simLog(sim,2,"=(%g,%g)",mols->surfdrift[i][ms][s][ps][0],mols->surfdrift[i][ms][s][ps][1]); }
					simLog(sim,2,"\n"); }}

		if(sim->graphss && mols->color && mols->color[i] && mols->display && mols->display[i]) {
			same=1;
			for(ms=(enum MolecState)(0);ms<MSMAX && same;ms=(enum MolecState)(ms+1)) {
				if(mols->display[i][ms]!=mols->display[i][MSsoln]) same=0;
				if(mols->color[i][ms][0]!=mols->color[i][MSsoln][0]) same=0;
				if(mols->color[i][ms][1]!=mols->color[i][MSsoln][1]) same=0;
				if(mols->color[i][ms][2]!=mols->color[i][MSsoln][2]) same=0; }
			if(same) {
				simLog(sim,2,"  all states:");
				if(mols->display[i][MSsoln])
					simLog(sim,2," color= %g,%g,%g, size=%g\n",mols->color[i][MSsoln][0],mols->color[i][MSsoln][1],mols->color[i][MSsoln][2],mols->display[i][MSsoln]);
				else simLog(sim,2," not displayed to graphics\n"); }
			else {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
					if(mols->exist && mols->exist[i] && mols->exist[i][ms]) {
						simLog(sim,2,"  %s:",molms2string(ms,string));
						if(mols->display[i][ms])
							simLog(sim,2," color= %g,%g,%g, display size= %g\n",mols->color[i][ms][0],mols->color[i][ms][1],mols->color[i][ms][2],mols->display[i][ms]);
						else simLog(sim,2," not displayed to graphics\n"); }}}}

	if(mols->dead==NULL) simLog(sim,1," No dead list allocated\n");
	simLog(sim,1," Dead list: allocated size=%i, number of molecules=%i",mols->maxd,mols->nd);
	if(mols->topd!=mols->nd) simLog(sim,1,", top value=%i",mols->topd);
	simLog(sim,1,"\n");
	if(mols->maxdlimit>=0) simLog(sim,1,"  limited to %i molecules\n",mols->maxdlimit);

	simLog(sim,2," Overall spatial resolution:");
	if(maxstep==-1 || mols->condition<SCok) simLog(sim,2," not computed\n");
	else simLog(sim,2," %g\n",maxstep);
	simLog(sim,2,"\n");
	return; }


/* writemols */
void writemols(simptr sim,FILE *fptr) {
	int i,d,ll,dim;
	char **spname,string[STRCHAR];
	enum MolecState ms;
	molssptr mols;
	double val0,val1,val2;

	mols=sim->mols;
	if(!mols) return;
	dim=sim->dim;
	spname=mols->spname;
	fprintf(fptr,"# Molecule parameters\n");

	fprintf(fptr,"max_species %i\n",mols->maxspecies-1);
	for(i=1;i<mols->nspecies;i++) fprintf(fptr,"species %s\n",spname[i]);
	fprintf(fptr,"\n");
	if(sim->mols->maxdlimit>=0)
		fprintf(fptr,"max_mol %i\n",sim->mols->maxdlimit);
	fprintf(fptr,"gauss_table_size %i\n\n",mols->ngausstbl);

	for(ll=0;ll<mols->nlist;ll++)
		if(mols->listtype[ll]==MLTsystem)
			fprintf(fptr,"molecule_lists %s\n",mols->listname[ll]);
	fprintf(fptr,"\n");
	
	for(i=1;i<mols->nspecies;i++) {
		val0=mols->difc[i][0];
		for(ms=(enum MolecState)(1);ms<MSMAX && mols->difc[i][ms]==val0;ms=(enum MolecState)(ms+1));
		if(ms==MSMAX) fprintf(fptr,"difc %s(all) %g\n",spname[i],val0);
		else {
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
				if(mols->difc[i][ms]>0)
					fprintf(fptr,"difc %s(%s) %g\n",spname[i],molms2string(ms,string),mols->difc[i][ms]); }
		
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			if(mols->difm[i][ms]) {
				fprintf(fptr,"difm %s(%s)",spname[i],molms2string(ms,string));
				for(d=0;d<dim*dim;d++)
					fprintf(fptr," %g",mols->difm[i][ms][d]);
				fprintf(fptr,"\n"); }}
		
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			if(mols->drift[i][ms]) {
				fprintf(fptr,"drift %s(%s)",spname[i],molms2string(ms,string));
				for(d=0;d<dim;d++)
					fprintf(fptr," %g",mols->drift[i][ms][d]);
				fprintf(fptr,"\n"); }}
		
		if(mols->nlist) {
			ll=mols->listlookup[i][0];
			for(ms=(enum MolecState)(1);ms<MSMAX && mols->listlookup[i][ms]==ll;ms=(enum MolecState)(ms+1));
			if(ms==MSMAX) fprintf(fptr,"mol_list %s(all) %s\n",spname[i],mols->listname[ll]);
			else {
				for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
					fprintf(fptr,"mol_list %s(%s) %s\n",spname[i],molms2string(ms,string),mols->listname[mols->listlookup[i][ms]]); }}
		
		val0=mols->display[i][0];
		for(ms=(enum MolecState)(1);ms<MSMAX && mols->display[i][ms]==val0;ms=(enum MolecState)(ms+1));
		if(ms==MSMAX) fprintf(fptr,"display_size %s(all) %g\n",spname[i],val0);
		else {
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
				fprintf(fptr,"display_size %s(%s) %g\n",spname[i],molms2string(ms,string),mols->display[i][ms]); }
		
		val0=mols->color[i][0][0];
		val1=mols->color[i][0][1];
		val2=mols->color[i][0][2];
		for(ms=(enum MolecState)(1);ms<MSMAX && mols->color[i][ms][0]==val0 && mols->color[i][ms][1]==val1 && mols->color[i][ms][2]==val2;ms=(enum MolecState)(ms+1));
		if(ms==MSMAX) fprintf(fptr,"color %s(all) %g %g %g\n",spname[i],val0,val1,val2);
		else {
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
				fprintf(fptr,"color %s(%s) %g %g %g\n",spname[i],molms2string(ms,string),mols->color[i][ms][0],mols->color[i][ms][1],mols->color[i][ms][2]); }
		fprintf(fptr,"\n"); }
	return; }


/* writemolecules */
void writemolecules(simptr sim,FILE *fptr) {
	int m,ll;
	char **spname,string[STRCHAR];
	molssptr mols;
	moleculeptr mptr;

	mols=sim->mols;
	if(!mols) return;
	spname=mols->spname;
	fprintf(fptr,"# Individual molecules\n");
	
	for(ll=0;ll<mols->nlist;ll++)
		if(mols->listtype[ll]==MLTsystem)
			for(m=0;m<mols->nl[ll];m++) {
				mptr=mols->live[ll][m];
				if(mptr->ident>0) {
					if(mptr->mstate==MSsoln)
						fprintf(fptr,"mol 1 %s",spname[mptr->ident]);
					else {
						fprintf(fptr,"surface_mol 1 %s(%s) %s",spname[mptr->ident],molms2string(mptr->mstate,string),mptr->pnl->srf->sname);
						fprintf(fptr," %s %s",surfps2string(mptr->pnl->ps,string),mptr->pnl->pname); }
					fprintf(fptr,"%s\n",molpos2string(sim,mptr,string)); }}

	return; }


/* checkmolparams */
int checkmolparams(simptr sim,int *warnptr) {
	int dim,i,nspecies,m,ll,warn,error,sum,same;
	molssptr mols;
	moleculeptr mptr;
	char **spname,string[STRCHAR];
	double m2[DIMMAX*DIMMAX],diag;
	enum MolecState ms;

	error=warn=0;
	mols=sim->mols;
	if(!mols) {
		if(warnptr) *warnptr=warn;
		return 0; }
	dim=sim->dim;
	nspecies=mols->nspecies;
	spname=mols->spname;

	if(mols->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: molecule structure %s\n",simsc2string(mols->condition,string)); }

	if(mols->condition>SClists) {				// check molecule list sorting
		for(ll=0;ll<mols->nlist;ll++) {
			for(m=0;m<mols->nl[ll];m++) {
				mptr=mols->live[ll][m];
				if(!mptr) {error++;simLog(sim,10," SMOLDYN BUG: NULL molecule in live list %i at %i\n",ll,m);}
				else if(mptr->list!=mols->listlookup[mptr->ident][mptr->mstate]) {error++;simLog(sim,10," SMOLDYN BUG: molecule list value for species %i (%s) is %i but should be %i\n",mptr->ident,molms2string(mptr->mstate,string),mptr->list,mols->listlookup[mptr->ident][mptr->mstate]);}
				else if(mptr->list!=ll) {warn++;simLog(sim,9," WARNING: mis-sorted molecule in live list %i at %i\n",ll,m);}
				else if(!mptr->ident) {warn++;simLog(sim,5," WARNING: empty molecule in live list %i at %i\n",ll,m);} }
			for(;m<mols->maxl[ll];m++) {
				mptr=mols->live[ll][m];
				if(mptr) {error++;simLog(sim,10," SMOLDYN BUG: misplaced molecule in live list %i at %i\n",ll,m);} }}

		for(m=0;m<mols->topd;m++) {
			mptr=mols->dead[m];
			if(!mptr) {error++;simLog(sim,10," SMOLDYN BUG: NULL molecule in dead list at %i\n",m);}
			else if(mptr->list!=-1) {error++;simLog(sim,10," SMOLDYN BUG: mis-sorted molecule in dead list at %i (species %i, serno %s)\n",m,mptr->ident,molserno2string(mptr->serno,string));}
			else if(mptr->ident) {error++;simLog(sim,10," SMOLDYN BUG: live molecule in dead list at %i\n",m);} }
		for(;m<mols->nd;m++) {
			mptr=mols->dead[m];
			if(!mptr) {error++;simLog(sim,10," SMOLDYN BUG: NULL molecule in resurrected list at %i\n",m);}
			else if(mptr->list==-1) {error++;simLog(sim,10," SMOLDYN BUG: mis-sorted molecule in resurrected list at %i\n",m);}
			else if(!mptr->ident) {error++;simLog(sim,10," BUG: dead molecule in resurrected list at %i\n",m);} }
		for(;m<mols->maxd;m++) {
			mptr=mols->dead[m];
			if(mptr) {error++;simLog(sim,10," SMOLDYN BUG: misplaced molecule in dead list at %i\n",m);} }}

	for(ll=0;ll<mols->nlist;ll++)
		for(m=0;m<mols->nl[ll];m++)	{									// check for molecules outside system
			mptr=mols->live[ll][m];
			if(!posinsystem(sim,mptr->pos)) {
				simLog(sim,5," WARNING: molecule #%s, of type '%s', is outside system volume\n",molserno2string(mptr->serno,string),spname[mptr->ident]);
				warn++; }}

	for(i=1;i<nspecies;i++)														// check for asymmetric diffusion matrices
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
			if(mols->difm[i][ms]) {
				dotMMD(mols->difm[i][ms],mols->difm[i][ms],m2,dim,dim,dim);
				if(!issymmetricMD(m2,dim)) {
					simLog(sim,5," WARNING: diffusion matrix for molecule %s (%s) is asymmetric\n",spname[i],molms2string(ms,string));
					warn++; }}

	for(i=1;i<nspecies;i++) {													// check for unused molecules
		sum=0;
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) sum+=mols->exist[i][ms];
		if(!sum) {
			simLog(sim,5," WARNING: molecule %s is never used\n",spname[i]);
			warn++; }}

	if(sim->graphss && sim->graphss->graphics>1) {		// check for molecules that may not display
		diag=systemdiagonal(sim);
		for(i=1;i<nspecies;i++) {
			same=1;
			for(ms=(enum MolecState)(0);ms<MSMAX && same;ms=(enum MolecState)(ms+1))
				if(mols->display[i][ms]!=mols->display[i][MSsoln]) same=0;
			for(ms=(enum MolecState)(0);ms<MSMAX && (same==0 || ms==MSsoln);ms=(enum MolecState)(ms+1)) {
				if(mols->display[i][ms]>0.1*diag) {
					simLog(sim,5," WARNING: very large display size for molecule %s (%s)\n",spname[i],same?"all":molms2string(ms,string));
					warn++; }
				if(mols->display[i][ms]<0.001*diag) {
					simLog(sim,5," WARNING: very small display size for molecule %s (%s)\n",spname[i],same?"all":molms2string(ms,string));
					warn++; }}}}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/********************************* structure set up ***************************/
/******************************************************************************/


/* molenablemols */
int molenablemols(simptr sim,int maxspecies) {
	molssptr mols;
	int er;

	if(sim->mols) {									// check for redundant function call
		if(maxspecies==-1) {
			if(sim->mols->nspecies<sim->mols->maxspecies) return 0; }
		else {
			if(maxspecies==sim->mols->maxspecies) return 0;
			if(maxspecies<sim->mols->maxspecies) return 2; }}

	if(maxspecies<0) maxspecies=sim->mols?sim->mols->maxspecies*2+1:5;	// need to initialize or increase maxspecies
	mols=molssalloc(sim->mols,maxspecies);
	if(!mols) return 1;
	sim->mols=mols;
	mols->sim=sim;
	molsetcondition(sim->mols,SClists,0);
	boxsetcondition(sim->boxs,SClists,0);
	er=rxnexpandmaxspecies(sim,maxspecies+1);
	if(er) return 1;
	er=surfexpandmaxspecies(sim,maxspecies+1);
	if(er) return 1;
	rxnsetcondition(sim,-1,SClists,0);
	surfsetcondition(sim->srfss,SClists,0);
	portsetcondition(sim->portss,SClists,0);
	return 0; }


/* molsetcondition */
void molsetcondition(molssptr mols,enum StructCond cond,int upgrade) {
	if(!mols) return;
	if(upgrade==0 && mols->condition>cond) mols->condition=cond;
	else if(upgrade==1 && mols->condition<cond) mols->condition=cond;
	else if(upgrade==2) mols->condition=cond;
	if(mols->sim && mols->condition<mols->sim->condition) {
		cond=mols->condition;
		simsetcondition(mols->sim,cond==SCinit?SClists:cond,0); }
	return; }



/* addmollist */
int addmollist(simptr sim,const char *nm,enum MolListType mlt) {
	int ll,er;
	molssptr mols;

	if(!sim->mols) {
		er=molenablemols(sim,-1);
		if(er) return -1; }
	mols=sim->mols;
	if(!mols || !nm) return -3;
	if(stringfind(mols->listname,mols->nlist,nm)!=-1) return -2;
	if(mols->nlist==mols->maxlist) {
		er=mollistalloc(mols,mols->maxlist+1,mlt);
		if(er<0) return -1; }
	ll=mols->nlist++;
	mols->listtype[ll] = mlt;
	strcpy(mols->listname[ll],nm);
	boxsetcondition(sim->boxs,SClists,0);
	rxnsetcondition(sim,-1,SClists,0);
	surfsetcondition(sim->srfss,SClists,0);
	portsetcondition(sim->portss,SClists,0);
	return ll; }


/* molsetmaxspecies */
int molsetmaxspecies(simptr sim,int max) {
	return molenablemols(sim,max); }


/* molsetmaxmol */
int molsetmaxmol(simptr sim,int max) {
	int er;

	if(!sim->mols) {
		er=molenablemols(sim,-1);
		if(er) return er; }
	if(max>=0 && max<sim->mols->maxd) return 5;
	sim->mols->maxdlimit=max;
	return 0; }


/* moladdspecies */
int moladdspecies(simptr sim,const char *nm) {
	molssptr mols;
	int found,er;

	er=molenablemols(sim,-1);
	if(er) return -1;
	mols=sim->mols;
	if(!strcmp(nm,"empty")) return -4;
	if(strchr(nm,'?') || strchr(nm,'*')) return -6;

	found=stringfind(mols->spname,mols->nspecies,nm);
	if(found>=0) return -5;

	strncpy(mols->spname[mols->nspecies++],nm,STRCHAR);
	molsetcondition(mols,SClists,0);
	rxnsetcondition(sim,-1,SClists,0);
	surfsetcondition(sim->srfss,SClists,0);
	return mols->nspecies-1; }


/* molgeneratespecies */
int molgeneratespecies(simptr sim,const char *name,int nparents,int parent1,int parent2) {
	int i,c,s;
	enum MolecState ms,ms0,ms1,ms2,ms4;
	molssptr mols;
	double d1,d2,r1,r2,color[3];
	surfaceptr srf;
	enum PanelFace face;
	enum SrfAction act1,act2;
	surfactionptr det1,det2;
	const double blackcolor[3]={0,0,0};

	mols=sim->mols;

	i=moladdspecies(sim,name);
	if(i<0) return i;

	if(nparents==0) {
		molsetdifc(sim,i,NULL,MSall,0);
		molsetdisplaysize(sim,i,NULL,MSall,3);
		molsetcolor(sim,i,NULL,MSall,blackcolor);	}

	if(nparents==1) {
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			molsetdifc(sim,i,NULL,ms,mols->difc[parent1][ms]);
			molsetdisplaysize(sim,i,NULL,ms,mols->display[parent1][ms]);
			molsetcolor(sim,i,NULL,ms,mols->color[parent1][ms]);
			if(sim->srfss) {
				for(s=0;s<sim->srfss->nsrf;s++) {
					srf=sim->srfss->srflist[s];
					for(face=(enum PanelFace)0;face<=PFnone;face=(PanelFace)(face+1)) {
						surfsetaction(srf,i,NULL,ms,face,srf->action[parent1][ms][face],-1);
						if(srf->action[parent1][ms][face]==SAmult) {
							for(ms4=(enum MolecState)0;ms4<(enum MolecState)MSMAX1;ms4=(enum MolecState)(ms4+1)) {
								srfindex2tristate(ms,face,ms4,&ms0,&ms1,&ms2);
								surfsetrate(srf,i,NULL,ms0,ms1,ms2,i,srf->actdetails[parent1][ms][face]->srfrate[ms4],1); }}}}}}}

	else if(nparents==2) {
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			d1=mols->difc[parent1][ms];
			d2=mols->difc[parent2][ms];
			r1=mols->display[parent1][ms];
			r2=mols->display[parent2][ms];
			if(d1==0 || d2==0)
				molsetdifc(sim,i,NULL,ms,0);
			else
				molsetdifc(sim,i,NULL,ms,pow(pow(d1,-3)+pow(d2,-3),-1.0/3));
			if(r2==0) {
				molsetdisplaysize(sim,i,NULL,ms,r1);
				molsetcolor(sim,i,NULL,ms,mols->color[parent1][ms]); }
			else if(r1==0) {
				molsetdisplaysize(sim,i,NULL,ms,r2);
				molsetcolor(sim,i,NULL,ms,mols->color[parent2][ms]); }
			else {
				molsetdisplaysize(sim,i,NULL,ms,pow(pow(r1,3)+pow(r2,3),1.0/3));
				for(c=0;c<3;c++)
					color[c]=(r1*mols->color[parent1][ms][c]+r2*mols->color[parent2][ms][c])/(r1+r2);
				molsetcolor(sim,i,NULL,ms,color); }
			if(sim->srfss) {
				for(s=0;s<sim->srfss->nsrf;s++) {
					srf=sim->srfss->srflist[s];
					for(face=(enum PanelFace)0;face<=PFnone;face=(PanelFace)(face+1)) {
						act1=srf->action[parent1][ms][face];
						act2=srf->action[parent2][ms][face];
						det1=srf->actdetails[parent1][ms][face];
						det2=srf->actdetails[parent2][ms][face];
						if(srfcompareaction(act1,det1,act2,det2)>0) {
							act1=act2;
							det1=det2; }
						surfsetaction(srf,i,NULL,ms,face,act1,-1);
						if(act1==SAmult) {
							for(ms4=(enum MolecState)0;ms4<(enum MolecState)MSMAX1;ms4=(enum MolecState)(ms4+1)) {
								srfindex2tristate(ms,face,ms4,&ms0,&ms1,&ms2);
								surfsetrate(srf,i,NULL,ms0,ms1,ms2,i,det1->srfrate[ms4],1); }}}}}}}

	return i; }


/* molsupdateparams */
int molsupdateparams(molssptr mols,double dt) {
	int i,ll;
	enum MolecState ms;

	for(ll=0;ll<mols->nlist;ll++) mols->diffuselist[ll]=0;		// set diffuselist
	for(i=0;i<mols->nspecies;i++)
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			if(molismobile(mols->sim,i,ms))
				mols->diffuselist[mols->listlookup[i][ms]]=1; }

	for(i=0;i<mols->nspecies;i++)					// calculate difstep
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
			mols->difstep[i][ms]=sqrt(2.0*mols->difc[i][ms]*dt);

	return 0; }


/* molsupdatelists */
int molsupdatelists(simptr sim) {
	int i,ll,m,ndif,nfix,ok,er;
	enum MolecState ms;
	molssptr mols;
	moleculeptr mptr;
	
	mols=sim->mols;

	er=molssetgausstable(sim,-1);				// gaussian lookup table
	if(er) return 1;

	for(i=1;i<mols->nspecies;i++)					// set exist values
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
			mols->exist[i][ms]=0;
	for(m=mols->topd;m<mols->nd;m++) {
		mptr=mols->dead[m];
		mols->exist[mptr->ident][mptr->mstate]=1; }
	for(ll=0;ll<mols->nlist;ll++)
		for(m=0;m<mols->nl[ll];m++) {
			mptr=mols->live[ll][m];
			mols->exist[mptr->ident][mptr->mstate]=1; }
	for(i=1;i<mols->nspecies;i++) {
		for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
			if(mols->exist[i][ms]==0 && rxnisprod(sim,i,ms,0)) mols->exist[i][ms]=1;
			if(mols->exist[i][ms]==0 && issurfprod(sim,i,ms)) mols->exist[i][ms]=1; }
		if(mols->exist[i][MSsoln]==0 && rxnisprod(sim,i,MSbsoln,0)) mols->exist[i][MSsoln]=1;
		if(mols->exist[i][MSsoln]==0 && issurfprod(sim,i,MSbsoln)) mols->exist[i][MSsoln]=1; }

	for(ll=0;ll<mols->nlist;ll++)					// create system molecule lists if none yet
		if(mols->listtype[ll]==MLTsystem) ll=mols->nlist+1;
	if(ll==mols->nlist && mols->maxd>0 && mols->nspecies>1) {
		ndif=nfix=0;
		for(i=1;i<mols->nspecies;i++)
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1)) {
				if(molismobile(sim,i,ms)) ndif=1;
				else nfix=1; }
		if(ndif) {
			ll=addmollist(sim,"diffuselist",MLTsystem);
			if(ll<0) return 1;
			molsetlistlookup(sim,-7,NULL,MSall,ll); }
		if(nfix) {
			ll=addmollist(sim,"fixedlist",MLTsystem);
			if(ll<0) return 1;
			molsetlistlookup(sim,-8,NULL,MSall,ll); }}

	ok=1;															// set any list lookup values that weren't done yet
	for(i=0;i<mols->nspecies && ok;i++)
		for(ms=(enum MolecState)(0);ms<MSMAX && ok;ms=(enum MolecState)(ms+1))
			if(mols->listlookup[i][ms]<0)
				ok=0;
	if(!ok) {
		ll=stringfind(mols->listname,mols->nlist,"unassignedlist");
		if(ll<0) {
			ll=addmollist(sim,"unassignedlist",MLTsystem);
			if(ll<0) return 1; }
		for(i=0;i<mols->nspecies;i++)
			for(ms=(enum MolecState)(0);ms<MSMAX;ms=(enum MolecState)(ms+1))
				if(mols->listlookup[i][ms]<0)
					molsetlistlookup(sim,i,NULL,ms,ll); }

	for(m=mols->topd;m<mols->nd;m++) {		// set molecule list values for molecules in dead list
		mptr=mols->dead[m];
		mptr->list=mols->listlookup[mptr->ident][mptr->mstate]; }

	return 0; }


/* molsupdate */
int molsupdate(simptr sim) {
	int er;
	molssptr mols;

	mols=sim->mols;
	if(mols) {
		if(mols->condition<=SClists) {
			er=molsupdatelists(sim);
			if(er) return er;
			molsetcondition(mols,SCparams,1); }
		if(mols->condition==SCparams) {
			er=molsupdateparams(mols,sim->dt);
			if(er) return er;
			molsetcondition(mols,SCok,1); }}
	return 0; }

/******************************************************************************/
/*********************** adding and removing molecules ************************/
/******************************************************************************/


/* molkill */
void molkill(simptr sim,moleculeptr mptr,int ll,int m) {
	int d;

	mptr->ident=0;
	mptr->mstate=MSsoln;
	mptr->list=-1;
	for(d=0;d<sim->dim;d++) mptr->posoffset[d]=0;
	mptr->pnl=NULL;
	mptr->pnlx=NULL;
  if(ll<0);
	else if(m<0) sim->mols->sortl[ll]=0;
	else if(m<sim->mols->sortl[ll]) sim->mols->sortl[ll]=m;
	sim->mols->touch++;
	return; }


/* getnextmol */
moleculeptr getnextmol(molssptr mols) {
	moleculeptr mptr;
	int er,nmol;

	if(mols->topd==0) {
		if(mols->maxdlimit>=0 && mols->maxd>=mols->maxdlimit) return NULL;
		nmol=mols->maxd+1;
		if(mols->maxdlimit>=0 && mols->maxd+nmol>mols->maxdlimit)
			nmol=mols->maxdlimit-mols->maxd;
		er=molexpandlist(mols,mols->sim->dim,-1,nmol,nmol);
		if(er) return NULL; }
	mptr=mols->dead[--mols->topd];
	mptr->serno=(unsigned long long)(mols->serno++);

	mols->touch++;
	return mptr; }


/* newestmol */
moleculeptr newestmol(molssptr mols) {
	return mols->dead[mols->topd-1]; }


/* addmol */
int addmol(simptr sim,int nmol,int ident,double *poslo,double *poshi,int sort) {
	int m,d;
	moleculeptr mptr;

	for(m=0;m<nmol;m++) {
		mptr=getnextmol(sim->mols);
		if(!mptr) return 3;
		mptr->ident=ident;
		mptr->mstate=MSsoln;
		mptr->list=sim->mols->listlookup[ident][MSsoln];
		if(poslo==poshi)
			for(d=0;d<sim->dim;d++)
				mptr->posx[d]=mptr->pos[d]=poslo[d];
		else
			for(d=0;d<sim->dim;d++)
				mptr->posx[d]=mptr->pos[d]=unirandOOD(poslo[d],poshi[d]);
		if(sim->boxs && sim->boxs->nbox)
			mptr->box=pos2box(sim,mptr->pos);
		else mptr->box=NULL; }
	molsetexist(sim,ident,MSsoln,1);
	sim->mols->expand[ident]|=1;
	if(sort)
		if(molsort(sim,1)) return 1;
	return 0; }


/* addsurfmol */
int addsurfmol(simptr sim,int nmol,int ident,enum MolecState ms,double *pos,panelptr pnl,int surface,enum PanelShape ps,char *pname) {
	int dim,m,d,totpanel,panel;
	moleculeptr mptr;
	int s,slo,shi,pslo,pshi,p,plo,phi,pindex;
	double *areatable,area,mpos[DIMMAX],mpos2[DIMMAX];
	panelptr *paneltable;
	surfaceptr srf;

	dim=sim->dim;

	if(pnl || (surface>=0 && ps!=PSall && pname && strcmp(pname,"all"))) {			// add to a specific panel
		if(!pnl) {
			srf=sim->srfss->srflist[surface];
			panel=stringfind(srf->pname[ps],srf->npanel[ps],pname);
			if(panel<0) return 2;
			pnl=srf->panels[ps][panel]; }
		for(m=0;m<nmol;m++) {
			mptr=getnextmol(sim->mols);
			if(!mptr) return 3;
			mptr->ident=ident;
			mptr->mstate=ms;
			mptr->list=sim->mols->listlookup[ident][ms];
			mptr->pnl=mptr->pnlx=pnl;
			if(pos) {
				for(d=0;d<dim;d++) mpos[d]=pos[d];
				if(!ptinpanel(mpos,pnl,dim)) {
					for(d=0;d<dim;d++) mpos2[d]=mpos[d];
					closestpanelpt(pnl,dim,mpos2,mpos,0); }}
			else
				panelrandpos(pnl,mpos,dim);
			if(ms==MSfront) fixpt2panel(mpos,pnl,dim,PFfront,0);
			else if(ms==MSback) fixpt2panel(mpos,pnl,dim,PFback,0);
			else fixpt2panel(mpos,pnl,dim,PFnone,0);
			for(d=0;d<dim;d++) mptr->pos[d]=mptr->posx[d]=mpos[d];
			if(sim->boxs && sim->boxs->nbox) mptr->box=pos2box(sim,mpos);
			else mptr->box=NULL; }}

	else {																						// create area lookup tables
		surfacearea2(sim,surface,ps,pname,&totpanel);
		if(totpanel<1) return 2;
		areatable=(double*) calloc(totpanel,sizeof(double));
		if(!areatable) return 1;
		paneltable=(panelptr*) calloc(totpanel,sizeof(panelptr));
		if(!paneltable) {free(areatable);return 1; }

		slo=(surface>=0)?surface:0;
		shi=(surface>=0)?surface+1:sim->srfss->nsrf;
		pslo=(ps!=PSall)?ps:0;
		pshi=(ps!=PSall)?ps+1:PSMAX;

		pindex=0;																						// fill in area lookup tables
		area=0;
		for(s=slo;s<shi;s++)
			for(ps=PanelShape(pslo);ps<pshi;ps=PanelShape(ps + 1)) {
				srf=sim->srfss->srflist[s];
				if(!pname || !strcmp(pname,"all")) {plo=0;phi=srf->npanel[ps];}
				else if((panel=stringfind(srf->pname[ps],srf->npanel[ps],pname))<0) plo=phi=0;
				else {plo=panel;phi=panel+1;}
				for(p=plo;p<phi;p++) {
					area+=surfacearea2(sim,s,ps,srf->pname[ps][p],NULL);
					areatable[pindex]=area;
					paneltable[pindex]=srf->panels[ps][p];
					pindex++; }}

		for(m=0;m<nmol;m++) {															// place molecules
			mptr=getnextmol(sim->mols);
			if(!mptr) {free(paneltable);free(areatable);return 3;}
			mptr->ident=ident;
			mptr->mstate=ms;
			mptr->list=sim->mols->listlookup[ident][ms];
			pindex=intrandpD(totpanel,areatable);
			pnl=paneltable[pindex];
			mptr->pnl=mptr->pnlx=pnl;
			panelrandpos(pnl,mpos,dim);
			if(ms==MSfront) fixpt2panel(mpos,pnl,dim,PFfront,0);
			else if(ms==MSback) fixpt2panel(mpos,pnl,dim,PFback,0);
			for(d=0;d<dim;d++) mptr->pos[d]=mptr->posx[d]=mpos[d];
			if(sim->boxs && sim->boxs->nbox) mptr->box=pos2box(sim,mpos);
			else mptr->box=NULL; }

		free(paneltable);
		free(areatable); }

	molsetexist(sim,ident,ms,1);
	sim->mols->expand[ident]|=1;
	return 0; }


/* addcompartmol */
int addcompartmol(simptr sim,int nmol,int ident,compartptr cmpt) {
	int d,dim,m,er;
	moleculeptr mptr;

	if(cmpt->npts==0 && cmpt->ncmptl==0) return 2;
	dim=sim->dim;

	for(m=0;m<nmol;m++) {
		mptr=getnextmol(sim->mols);
		if(!mptr) return 3;
		mptr->ident=ident;
		mptr->mstate=MSsoln;
		mptr->list=sim->mols->listlookup[ident][MSsoln];
		er=compartrandpos(sim,mptr->pos,cmpt);
		if(er) return 2;
		for(d=0;d<dim;d++) mptr->posx[d]=mptr->pos[d];
		if(sim->boxs && sim->boxs->nbox) mptr->box=pos2box(sim,mptr->pos);
		else mptr->box=NULL; }
	molsetexist(sim,ident,MSsoln,1);
	sim->mols->expand[ident]|=1;
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* molsort */
int molsort(simptr sim,int onlydead2live) {
	molssptr mols;
	int nlist,*maxl,*nl,*topl,*sortl,m,ll,ll2;
	moleculeptr *dead,**live,*mlist,mptr;
	enum MolListType *listtype;
	boxptr bptr;
	panelptr pnl;

	if(!sim->mols) return 0;
	mols=sim->mols;
	dead=mols->dead;
	nlist=mols->nlist;
	listtype=mols->listtype;
	live=mols->live;
	maxl=mols->maxl;
	nl=mols->nl;
	topl=mols->topl;
	sortl=mols->sortl;

  if(!onlydead2live) {
    for(ll=0;ll<nlist;ll++)								// reset topl indicies
      topl[ll]=nl[ll];

    for(ll=0;ll<nlist;ll++) {							// sort live lists
      mlist=live[ll];
      for(m=sortl[ll];m<topl[ll];m++) {
        if(mlist[m]->list!=ll) {
          mptr=mlist[m];
          if(mptr->list==-1) {						// move to dead list
            if(mptr->box) boxremovemol(mptr,ll);
						if(mptr->pnl) surfremovemol(mptr,ll);
            dead[mols->nd++]=dead[mols->topd];
            dead[mols->topd++]=mptr;
            mlist[m]=NULL; }
          else {													// move to another live list
            ll2=mptr->list;
            bptr=mptr->box;
						pnl=mptr->pnl;
            if(bptr) boxremovemol(mptr,ll);
						if(pnl) surfremovemol(mptr,ll);
            if(nl[ll2]==maxl[ll2])
              if(molexpandlist(mols,sim->dim,ll2,-1,0)) {
                simLog(sim,10,"out of memory in molsort\n");return 1;}
            live[ll2][nl[ll2]++]=mptr;
            mlist[m]=NULL;
            if(listtype[ll2]==MLTsystem) {
              if(bptr) mptr->box=bptr;
              else mptr->box=pos2box(sim,mptr->pos);
              if(boxaddmol(mptr,ll2)) {
                simLog(sim,10,"out of memory in molsort\n");return 1;}
							if(pnl) {
								mptr->pnl=pnl;
								if(surfaddmol(mptr,ll2)) {
									simLog(sim,10,"out of memory in molsort");return 1;} }}}

          mlist[m]=mlist[--topl[ll]];				// compact original live list
          mlist[topl[ll]]=mlist[--nl[ll]];
          mlist[nl[ll]]=NULL;
          m--; }}}}

	for(m=mols->topd;m<mols->nd;m++) {		// move molecules from resurrected to reborn
		mptr=dead[m];
		ll2=mptr->list;
		if(nl[ll2]==maxl[ll2])
			if(molexpandlist(mols,sim->dim,ll2,-1,0)) {
				simLog(sim,10,"out of memory in molsort\n");return 1;}
		live[ll2][nl[ll2]++]=mptr;
		dead[m]=NULL;
		if(listtype[ll2]==MLTsystem) {
			if(boxaddmol(mptr,ll2)) {
				simLog(sim,10,"out of memory in molsort\n");return 1;} }}
	mols->nd=mols->topd;

  if(!onlydead2live) {
    for(ll=0;ll<nlist;ll++)								// reset sortl indicies
      sortl[ll]=nl[ll]; }

	return 0; }


/* moldosurfdrift */
void moldosurfdrift(simptr sim,moleculeptr mptr,double dt) {
	int i,s,axis;
	enum MolecState ms;
	enum PanelShape ps;
	double *****surfdrift,vect[3],drift1,drift2,*pt1,*pt2,dist,unit0[3],unit1[3],unit2[3],top[3];
	panelptr pnl;

	i=mptr->ident;
	ms=mptr->mstate;
	pnl=mptr->pnl;
	s=pnl->srf->selfindex;
	ps=pnl->ps;
	surfdrift=sim->mols->surfdrift;
	vect[0]=vect[1]=vect[2]=0;

	if(surfdrift[i][ms][s] && surfdrift[i][ms][s][ps]) {
		if(sim->dim==2) {
			drift1=surfdrift[i][ms][s][ps][0]*dt;
			if(ps==PSrect)
				vect[(int)(pnl->front[2])]=drift1;
			else if(ps==PStri || ps==PScyl) {
				vect[0]=-drift1*pnl->front[1];
				vect[1]=drift1*pnl->front[0]; }
			else if(ps==PSsph || ps==PShemi) {
				vect[0]=-drift1*(mptr->pos[1]-pnl->point[0][1])/pnl->point[1][0];
				vect[1]=drift1*(mptr->pos[0]-pnl->point[0][0])/pnl->point[1][0]; }
			else if(ps==PSdisk) {
				pt1=mptr->pos;
				pt2=pnl->point[0];
				dist=sqrt((pt2[0]-pt1[0])*(pt2[0]-pt1[0])+(pt2[1]-pt1[1])*(pt2[1]-pt1[1]));
				if(dist>VERYCLOSE) {
					vect[0]=drift1*(pt2[0]-pt1[0])/dist;
					vect[1]=drift1*(pt2[1]-pt1[1])/dist; }
				else {
					vect[0]=-drift1*pnl->front[1];
					vect[1]=drift1*pnl->front[0]; }}
			mptr->pos[0]+=vect[0];
			mptr->pos[1]+=vect[1]; }

		else {
			drift1=surfdrift[i][ms][s][ps][0]*dt;
			drift2=surfdrift[i][ms][s][ps][1]*dt;
			if(ps==PSrect) {
				vect[(int)(pnl->front[2])]=drift1;
				axis=0;
				if(axis==(int)(pnl->front[1]) || axis==(int)(pnl->front[2])) axis++;
				if(axis==(int)(pnl->front[1]) || axis==(int)(pnl->front[2])) axis++;
				vect[axis]=drift2; }
			else if(ps==PStri) {
				Geo_TriUnitVects(pnl->point[0],pnl->point[1],pnl->point[2],unit0,unit1,unit2);
				vect[0]=drift1*unit1[0]+drift2*unit2[0];
				vect[1]=drift1*unit1[1]+drift2*unit2[1];
				vect[2]=drift1*unit1[2]+drift2*unit2[2]; }
			else if(ps==PSsph) {
				top[0]=pnl->point[0][0];
				top[1]=pnl->point[0][1];
				top[2]=pnl->point[0][2]+pnl->point[1][0];
				Geo_SphereUnitVects(pnl->point[0],top,mptr->pos,(int)(pnl->front[0]),unit0,unit1,unit2);
				vect[0]=drift1*unit1[0]+drift2*unit2[0];
				vect[1]=drift1*unit1[1]+drift2*unit2[1];
				vect[2]=drift1*unit1[2]+drift2*unit2[2]; }
			else if(ps==PScyl) {
				Geo_CylUnitVects(pnl->point[0],pnl->point[1],mptr->pos,(int)(pnl->front[2]),unit0,unit1,unit2);
				vect[0]=drift1*unit1[0]+drift2*unit2[0];
				vect[1]=drift1*unit1[1]+drift2*unit2[1];
				vect[2]=drift1*unit1[2]+drift2*unit2[2]; }
			else if(ps==PShemi) {
				top[0]=pnl->point[0][0]-pnl->point[2][0];
				top[1]=pnl->point[0][1]-pnl->point[2][1];
				top[2]=pnl->point[0][2]-pnl->point[2][2];
				Geo_SphereUnitVects(pnl->point[0],top,mptr->pos,(int)(pnl->front[0]),unit0,unit1,unit2);
				vect[0]=drift1*unit1[0]+drift2*unit2[0];
				vect[1]=drift1*unit1[1]+drift2*unit2[1];
				vect[2]=drift1*unit1[2]+drift2*unit2[2]; }
			else if(ps==PSdisk) {
				Geo_DiskUnitVects(pnl->point[0],pnl->front,mptr->pos,unit0,unit1,unit2);
				vect[0]=drift1*unit1[0]+drift2*unit2[0];
				vect[1]=drift1*unit1[1]+drift2*unit2[1];
				vect[2]=drift1*unit1[2]+drift2*unit2[2]; }
			mptr->pos[0]+=vect[0];
			mptr->pos[1]+=vect[1];
			mptr->pos[2]+=vect[2]; }
		sim->mols->touch++; }

	return; }


/* diffuse */
int diffuse(simptr sim) {
	molssptr mols;
	int ll,m,d,nmol,dim,i,ngtablem1;
	enum MolecState ms;
	double flt1;
	double v1[DIMMAX],v2[DIMMAX],**difstep,***difm,***drift,*gtable,dt;
	moleculeptr *mlist;
	moleculeptr mptr;

	if(!sim->mols) return 0;
	dim=sim->dim;
	mols=sim->mols;
	ngtablem1=mols->ngausstbl-1;
	gtable=mols->gausstbl;
	difstep=mols->difstep;
	difm=mols->difm;
	drift=mols->drift;
	dt=sim->dt;
	flt1=sqrt(2.0*dt);

	for(ll=0;ll<mols->nlist;ll++)
		if(mols->diffuselist[ll]) {
			mlist=mols->live[ll];
			nmol=mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				i=mptr->ident;
				ms=mptr->mstate;
				for(d=0;d<dim;d++)
					mptr->posx[d]=mptr->pos[d];
				mptr->pnlx=mptr->pnl;

				if(mptr->pnl && mols->surfdrift && mols->surfdrift[i] && mols->surfdrift[i][ms])
					moldosurfdrift(sim,mptr,dt);											// surface drift
				if(drift[i][ms])																		// drift
					for(d=0;d<dim;d++) mptr->pos[d]+=drift[i][ms][d]*dt;

				if(!difm[i][ms])																		// isotropic diffusion
					for(d=0;d<dim;d++)
						mptr->pos[d]+=difstep[i][ms]*gtable[randULI()&ngtablem1];
				else {																							// anisotropic diffusion
					for(d=0;d<dim;d++)
						v1[d]=flt1*gtable[randULI()&ngtablem1];
					dotMVD(difm[i][ms],v1,v2,dim,dim);
					for(d=0;d<dim;d++) mptr->pos[d]+=v2[d]; }

				if(mptr->mstate!=MSsoln) {													// surface-bound molecules
					if(dim>1)
						movemol2closepanel(sim,mptr);
					else
						mptr->pos[0]=mptr->posx[0]; }}									// 1D surface-bound molecules aren't allowed to move
			sim->mols->touch++; }

	return 0; }
