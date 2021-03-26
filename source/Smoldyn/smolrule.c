/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.  See documentation
 called SmoldynManual.pdf and SmoldynCodeDoc.pdf.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu General Public License (GPL). */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string2.h"

#include "smoldynconfigure.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"

/******************************************************************************/
/************************************ Rules ***********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types
char *rulert2string(enum RuleType rt,char *string);

// memory management
void rulefree(ruleptr rule);

// data structure output

// structure set up

// core simulation functions


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* rulestring2rt */
enum RuleType rulestring2rt(const char *string) {
	enum RuleType ans;

	if(!strcmp(string,"reaction")) ans=RTreaction;
	else if(!strcmp(string,"difc")) ans=RTdifc;
	else if(!strcmp(string,"difm")) ans=RTdifm;
	else if(!strcmp(string,"drift")) ans=RTdrift;
	else if(!strcmp(string,"surfdrift")) ans=RTsurfdrift;
	else if(!strcmp(string,"mollist")) ans=RTmollist;
	else if(!strcmp(string,"dispsize")) ans=RTdispsize;
	else if(!strcmp(string,"color")) ans=RTcolor;
	else if(!strcmp(string,"surfaction")) ans=RTsurfaction;
	else if(!strcmp(string,"surfrate")) ans=RTsurfrate;
	else if(!strcmp(string,"surfrateint")) ans=RTsurfrateint;
	else ans=RTnone;
	return ans; }


/* rulert2string */
char *rulert2string(enum RuleType rt,char *string) {
	if(rt==RTreaction) strcpy(string,"reaction");
	else if(rt==RTdifc) strcpy(string,"difc");
	else if(rt==RTdifm) strcpy(string,"difm");
	else if(rt==RTdrift) strcpy(string,"drift");
	else if(rt==RTsurfdrift) strcpy(string,"surfdrift");
	else if(rt==RTmollist) strcpy(string,"mollist");
	else if(rt==RTdispsize) strcpy(string,"dispsize");
	else if(rt==RTcolor) strcpy(string,"color");
	else if(rt==RTsurfaction) strcpy(string,"surfaction");
	else if(rt==RTsurfrate) strcpy(string,"surfrate");
	else if(rt==RTsurfrateint) strcpy(string,"surfrateint");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/


/* rulealloc */
ruleptr rulealloc() {
	ruleptr rule;

	CHECKMEM(rule=(ruleptr) malloc(sizeof(struct rulestruct)));
	rule->ruless=NULL;
	rule->ruletype=RTnone;
	rule->rulename=NULL;
	rule->rulepattern=NULL;
	rule->rulenresults=0;
	rule->ruledetailsi=NULL;
	rule->ruledetailsf=NULL;
	rule->rulerate=-1;
	rule->rulerxn=NULL;
	return rule;

 failure:
	rulefree(rule);
	simLog(NULL,10,"Unable to allocate memory in rulealloc");
	return NULL; }


/* rulefree */
void rulefree(ruleptr rule) {
	if(!rule) return;
	free(rule->rulepattern);
	free(rule->ruledetailsi);
	free(rule->ruledetailsf);
	rxnfree(rule->rulerxn);
	free(rule);
	return; }


/* rulessalloc */
rulessptr rulessalloc(rulessptr ruless,int maxrule) {
	int failfree,i;
	char **newrulename;
	ruleptr *newrule;

	failfree=0;
	if(!ruless) {																				// new rule superstructure
		ruless=(rulessptr) malloc(sizeof(struct rulesuperstruct));
		CHECKMEM(ruless);
		failfree=1;
		ruless->sim=NULL;
		ruless->maxrule=0;
		ruless->nrule=0;
		ruless->rulename=NULL;
		ruless->rule=NULL;
		ruless->ruleonthefly=-1; }

	if(maxrule>ruless->maxrule) {												// initialize or expand rules
		CHECKMEM(newrulename=(char **) calloc(maxrule,sizeof(char *)));
		for(i=0;i<maxrule;i++) newrulename[i]=NULL;
		for(i=0;i<ruless->maxrule;i++)
			newrulename[i]=ruless->rulename[i];
		for(;i<maxrule;i++)
			CHECKMEM(newrulename[i]=EmptyString());

		CHECKMEM(newrule=(ruleptr*) calloc(maxrule,sizeof(ruleptr)));
		for(i=0;i<maxrule;i++) newrule[i]=NULL;
		for(i=0;i<ruless->maxrule;i++)
			newrule[i]=ruless->rule[i];
		for(;i<maxrule;i++) {
			CHECKMEM(newrule[i]=rulealloc());
			newrule[i]->ruless=ruless;
			newrule[i]->rulename=newrulename[i]; }

		free(ruless->rulename);
		ruless->rulename=newrulename;

		free(ruless->rule);
		ruless->rule=newrule;

		ruless->maxrule=maxrule; }

	return ruless;

 failure:
	if(failfree) rulessfree(ruless);
	simLog(NULL,10,"Unable to allocate memory in rulessalloc");
	return NULL; }


/* rulessfree */
void rulessfree(rulessptr ruless) {
	int i;

	if(!ruless) return;

	for(i=0;i<ruless->maxrule;i++) {
		rulefree(ruless->rule[i]);
		free(ruless->rulename[i]); }
	free(ruless->rulename);
	free(ruless->rule);
	free(ruless);
	return; }


/******************************************************************************/
/**************************** data structure output ***************************/
/******************************************************************************/


/* ruleoutput */
void ruleoutput(simptr sim) {
	rulessptr ruless;
	int r,*index,*detailsi,i,d;
	double *detailsf,rulerate;
	ruleptr rule;
	char *rname,*pattern,*chptr;
	char rulestring[STRCHAR],string[STRCHAR];
	enum RuleType ruletype;
	rxnptr rxn;

	ruless=sim->ruless;
	simLog(sim,2,"RULES:\n");
	if(!ruless) {
		simLog(sim,2," none\n\n");
		return; }

	simLog(sim,2," generation: ");
	if(ruless->ruleonthefly==-1) simLog(sim,2,"never\n");
	else if(ruless->ruleonthefly==0) simLog(sim,2,"generate-first\n");
	else simLog(sim,2,"on-the-fly\n");
	simLog(sim,1," %i rules defined, of %i allocated\n",ruless->nrule,ruless->maxrule);

	for(r=0;r<ruless->nrule;r++) {
		rule=ruless->rule[r];
		ruletype=rule->ruletype;
		rname=rule->rulename;
		pattern=rule->rulepattern;
		rulerate=rule->rulerate;
		detailsi=rule->ruledetailsi;
		detailsf=rule->ruledetailsf;

		molpatternindex(sim,pattern,rname,1,0,&index);
		if(!index[PDrule])
			simLog(sim,7,"BUG: rule pattern is not registered as a rule\n");
		strcpy(rulestring,pattern);
		chptr=rulestring;					// next, add in molecule states
		for(i=0;i<index[PDmatch]+index[PDsubst] && strlen(rulestring)+20<STRCHAR;i++) {		// i scans over reactant and product words that will be in output
			chptr=strnwordend(chptr,1);
			strPreCat(chptr,"(",0,1);
			chptr++;
			molms2string((enum MolecState)detailsi[i],string);
			strPreCat(chptr,string,0,strlen(string));
			chptr=strnwordend(chptr,1);
			strPreCat(chptr,")",0,1);
			chptr++; }
		strstrreplace(rulestring," "," + ",STRCHAR);
		strstrreplace(rulestring,"\n"," -> ",STRCHAR);

		if(ruletype==RTreaction)
			simLog(sim,2," reaction rule %s: %s, rate %g",rname,rulestring,rulerate);
		else if(ruletype==RTdifc)
			simLog(sim,2," difc rule: %s, value %g",rulestring,rulerate);
		else if(ruletype==RTdifm) {
			simLog(sim,2," difm rule: %s, matrix:",rulestring);
			for(d=0;d<sim->dim*sim->dim;d++)
				simLog(sim,2," %g",detailsf[d]); }
		else if(ruletype==RTdrift) {
			simLog(sim,2," drift rule: %s, vector:",rulestring);
			for(d=0;d<sim->dim;d++)
				simLog(sim,2," %g",detailsf[d]); }
		else if(ruletype==RTsurfdrift) {
			simLog(sim,2," surface drift rule: %s,",rulestring);
			simLog(sim,2," surface %s,",sim->srfss->snames[detailsi[1]]);
			simLog(sim,2," panel shape %s, vector:",surfps2string((enum PanelShape)detailsi[2],string));
			for(d=0;d<sim->dim-1;d++)
				simLog(sim,2," %g",detailsf[d]); }
		else if(ruletype==RTmollist)
			simLog(sim,2," molecule list rule: %s, list:",rulestring,sim->mols->listname[detailsi[1]]);
		else if(ruletype==RTdispsize)
			simLog(sim,2," display size rule: %s, size: %g",rulestring,rulerate);
		else if(ruletype==RTcolor)
			simLog(sim,2," color rule: %s, color: %g %g %g",rulestring,detailsf[0],detailsf[1],detailsf[2]);
		else if(ruletype==RTsurfaction) {
			simLog(sim,2," surface action rule: %s, at %s",rulestring,surfface2string((enum PanelFace)detailsi[2],string));
			simLog(sim,2," face of %s,",sim->srfss->snames[detailsi[1]]);
			simLog(sim,2," action %s",surfact2string((enum SrfAction)detailsi[3],string)); }
		else if(ruletype==RTsurfrate) {
			simLog(sim,2," surface rate rule: %s at %s,",rulestring,sim->srfss->snames[detailsi[1]]);
			simLog(sim,2," from %s to",molms2string((enum MolecState)detailsi[2],string));
			if(detailsi[4]>0) simLog(sim,2," %s(",sim->mols->spname[detailsi[4]]);
			simLog(sim,2,"%s",molms2string((enum MolecState)detailsi[3],string));
			if(detailsi[4]>0) simLog(sim,2,")");
			simLog(sim,2,", rate: %g",rulerate); }
		else if(ruletype==RTsurfrateint) {
			simLog(sim,2," surface rate internal rule: %s at %s,",rulestring,sim->srfss->snames[detailsi[1]]);
			simLog(sim,2," from %s to",molms2string((enum MolecState)detailsi[2],string));
			if(detailsi[4]>0) simLog(sim,2," %s(",sim->mols->spname[detailsi[4]]);
			simLog(sim,2,"%s",molms2string((enum MolecState)detailsi[3],string));
			if(detailsi[4]>0) simLog(sim,2,")");
			simLog(sim,2,", rate: %g",rulerate); }
		else
			simLog(sim,2," BUG: unknown rule type");

		if(index[PDnspecies]==-1)
			simLog(sim,1,", updating not required");
		else if(index[PDnspecies]==sim->mols->nspecies && index[PDnresults]==rule->rulenresults)
			simLog(sim,1,", up-to-date");
		else
			simLog(sim,1,", not fully updated");
		simLog(sim,2,"\n");

		if(rule->rulerxn) {
			simLog(sim,2,"  reaction template:");
			rxn=rule->rulerxn;
			if(rxn->rate>=0) simLog(sim,2," rate=%g",rxn->rate);
			if(rxn->multiplicity>=0) simLog(sim,2," multiplicity=%i",rxn->multiplicity);
			if(rxn->bindrad2>=0) simLog(sim,2," binding radius=%g",sqrt(rxn->bindrad2));
			if(rxn->prob>=0) simLog(sim,2," probability=%g",rxn->prob);
			if(rxn->prdserno) simLog(sim,2," serial number rules");
			if(rxn->prdintersurf) simLog(sim,2," intersurface");
			if(rxn->rctrep || rxn->prdrep) simLog(sim,2," representation rules");
			if(rxn->logserno && rxn->logfile) simLog(sim,2," log to %s",rxn->logfile);
			if(rxn->rparamt!=RPnone) simLog(sim,2," product placement=%s",rxnrp2string(rxn->rparamt,string));
			simLog(sim,2,"\n"); }}

	simLog(sim,2,"\n");
	return; }


/* writerules */
void writerules(simptr sim,FILE *fptr) {

	fprintf(fptr,"# Rules\n");

	//?? not written yet

	fprintf(fptr,"\n");
	return; }


/* checkruleparams */
int checkruleparams(simptr sim,int *warnptr) {
	int warn,error;

	error=warn=0;

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/****************************** structure set up ******************************/
/******************************************************************************/


/* RuleAddRule */
int RuleAddRule(simptr sim,enum RuleType type,const char *rname,const char *pattern,const enum MolecState *rctstate,const enum MolecState *prdstate,double rate,const int *detailsi,const double *detailsf) {
	rulessptr ruless;
	int r,*index,matchwords,subwords,i,er,d;
	ruleptr rule;

	ruless=sim->ruless;
	rule=NULL;

	if(ruless) {																	// test for a repeat of a prior rule
		for(r=0;r<ruless->nrule;r++) {
			rule=ruless->rule[r];
			if(rule->ruletype==type && (rname==NULL || !strcmp(rule->rulename,rname)) && !strcmp(rule->rulepattern,pattern) && rule->ruledetailsi[0]==(int)rctstate[0])
				break; }
		if(r==ruless->nrule) rule=NULL; }

	if(!rule) {
		if(!ruless) {																// create rule superstructure if needed
			ruless=rulessalloc(ruless,2);
			if(!ruless) return 1;
			sim->ruless=ruless;
			ruless->sim=sim; }
		else if(ruless->nrule==ruless->maxrule) {		// expand rule superstructure if needed
			ruless=rulessalloc(ruless,ruless->maxrule*2);
			if(!ruless) return 1; }

		r=ruless->nrule++;
		rule=ruless->rule[r];
		rule->ruletype=type;
		if(rname) strcpy(rule->rulename,rname);
		rule->rulepattern=(char*) calloc(strlen(pattern)+1,sizeof(char));
		if(!rule->rulepattern) return 1;
		strcpy(rule->rulepattern,pattern); }

	er=molpatternindex(sim,pattern,rname,1,1,&index);		// get or create an index for this pattern but don't populate it
	if(er) return 1;
	matchwords=index[PDmatch];
	subwords=index[PDsubst];

	rule->rulerate=rate;

	if(type==RTreaction) {
		rule->ruledetailsi=(int*) calloc(matchwords+subwords+2,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		for(i=0;i<matchwords;i++)
			rule->ruledetailsi[i]=(int)rctstate[i];
		for(i=0;i<subwords;i++)
			rule->ruledetailsi[matchwords+i]=(int)prdstate[i];
		rule->ruledetailsi[matchwords+subwords]=detailsi[0];
		rule->ruledetailsi[matchwords+subwords+1]=detailsi[1]; }

	else if(type==RTdifc) {
		rule->ruledetailsi=(int*) calloc(1,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0]; }

	else if(type==RTdifm) {
		rule->ruledetailsi=(int*) calloc(1,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsf=(double*) calloc(sim->dim*sim->dim,sizeof(double));
		if(!rule->ruledetailsf) return 1;
		for(d=0;d<sim->dim*sim->dim;d++)
			rule->ruledetailsf[d]=detailsf[d]; }

	else if(type==RTdrift) {
		rule->ruledetailsi=(int*) calloc(1,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsf=(double*) calloc(sim->dim,sizeof(double));
		if(!rule->ruledetailsf) return 1;
		for(d=0;d<sim->dim;d++)
			rule->ruledetailsf[d]=detailsf[d]; }

	else if(type==RTsurfdrift) {
		rule->ruledetailsi=(int*) calloc(3,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsi[1]=detailsi[0];
		rule->ruledetailsi[2]=detailsi[1];
		rule->ruledetailsf=(double*) calloc(sim->dim,sizeof(double));
		if(!rule->ruledetailsf) return 1;
		for(d=0;d<sim->dim;d++)
			rule->ruledetailsf[d]=detailsf[d]; }

	else if(type==RTmollist) {
		rule->ruledetailsi=(int*) calloc(2,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsi[1]=detailsi[0]; }

	else if(type==RTdispsize) {
		rule->ruledetailsi=(int*) calloc(1,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0]; }

	else if(type==RTcolor) {
		rule->ruledetailsi=(int*) calloc(1,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsf=(double*) calloc(4,sizeof(double));
		if(!rule->ruledetailsf) return 1;
		for(d=0;d<4;d++)
			rule->ruledetailsf[d]=detailsf[d]; }

	else if(type==RTsurfaction) {
		rule->ruledetailsi=(int*) calloc(4,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsi[1]=detailsi[0];
		rule->ruledetailsi[2]=detailsi[1];
		rule->ruledetailsi[3]=detailsi[2]; }

	else if(type==RTsurfrate) {
		rule->ruledetailsi=(int*) calloc(5,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsi[1]=detailsi[0];
		rule->ruledetailsi[2]=detailsi[1];
		rule->ruledetailsi[3]=detailsi[2];
		rule->ruledetailsi[4]=detailsi[3]; }

	else if(type==RTsurfrateint) {
		rule->ruledetailsi=(int*) calloc(5,sizeof(int));
		if(!rule->ruledetailsi) return 1;
		rule->ruledetailsi[0]=(int)rctstate[0];
		rule->ruledetailsi[1]=detailsi[0];
		rule->ruledetailsi[2]=detailsi[1];
		rule->ruledetailsi[3]=detailsi[2];
		rule->ruledetailsi[4]=detailsi[3]; }

	return 0; }


/******************************************************************************/
/************************** core simulation functions *************************/
/******************************************************************************/


/* RuleExpandRules */
int RuleExpandRules(simptr sim,int iterations) {
	rulessptr ruless;
	ruleptr rule;
	const char *pattern,*rname;
	int nspecies,*index,iter,r,c,s,done,matchwords,subwords,oldnresults,er,i,*detailsi,i3;
	double *detailsf;
	enum MolecState *rctstate,*prdstate,ms;
	enum RuleType ruletype;
	compartptr cmpt;
	surfaceptr srf;
	double rate;
//	int totrxn1,totrxn2; //?? DEBUG CODE

	ruless=sim->ruless;
	if(!ruless) return -41;					// no rules to expand, not an error

	if(iterations==-3 && ruless->ruleonthefly!=1) return -41;		// no rules to expand, not an error

	if(iterations==-2) {					// turns on on-the-fly but doesn't actually do it
		ruless->ruleonthefly=1;
		return 0; }

	if(iterations>=-1 && ruless->ruleonthefly<0)
		ruless->ruleonthefly=0;			// generate-first expansion has been requested

	nspecies=sim->mols->nspecies;

	if(iterations==-3) {					// on-the-fly expansion is requested; see if it is needed
		iterations=0;
		for(i=0;i<nspecies;i++)
			if(sim->mols->expand[i]==1) break;
		if(i==nspecies) return 0;
		iterations=1; }

	done=0;												// perform network expansion
	for(iter=0;!done && (iter<iterations || iterations==-1);iter++) {
		for(r=0;r<ruless->nrule;r++) {
			rule=ruless->rule[r];
			ruletype=rule->ruletype;
			rname=rule->rulename;
			pattern=rule->rulepattern;
			rate=rule->rulerate;
			detailsi=rule->ruledetailsi;
			detailsf=rule->ruledetailsf;
			oldnresults=rule->rulenresults;

			molpatternindex(sim,pattern,rname,1,0,&index);
			matchwords=index[PDmatch];
			subwords=index[PDsubst];

			if(ruletype==RTreaction) {
				rctstate=(enum MolecState*) detailsi;
				prdstate=(enum MolecState*) (detailsi+matchwords);
				if((c=detailsi[matchwords+subwords])>=0)
					cmpt=sim->cmptss->cmptlist[c];
				else cmpt=NULL;
				if((s=detailsi[matchwords+subwords+1])>=0)
					srf=sim->srfss->srflist[s];
				else srf=NULL;

//				printf("%g ExpandRules for reaction:\n species:",sim->time);//??	DEBUG CODE
//				for(i=1;i<sim->mols->nspecies;i++) printf("  %s=%i",sim->mols->spname[i],sim->mols->expand[i]); //?? DEBUG CODE
//				totrxn1=sim->rxnss[matchwords]?sim->rxnss[matchwords]->totrxn:0;	//?? DEBUG CODE
				er=RxnAddReactionPattern(sim,rname,pattern,oldnresults,rctstate,prdstate,cmpt,srf,1,NULL);
//				totrxn2=sim->rxnss[matchwords]?sim->rxnss[matchwords]->totrxn:0;	//?? DEBUG CODE
//				printf("\n ran RxnAddReactionPattern(%s) and order %i reactions went from %i to %i\n",rname,matchwords,totrxn1,totrxn2);//?? DEBUG CODE
				if(er) {
					simLog(sim,10,"Error during expansion of reaction rule %s\n",rname);
					if(er==-1 || er==-11) simLog(sim,10,"Out of memory\n");
					else if(er==-2) simLog(sim,10,"Unknown reactant\n");
					else if(er==-3) simLog(sim,10,"Unknown substitution species\n");
					else if(er==-4) simLog(sim,10,"BUG: too many reactants\n");
					else if(er==-5) simLog(sim,10,"string is overlength\n");
					else if(er==-6) simLog(sim,10,"species generation failed\n");
					else if(er==-12) simLog(sim,10,"missing OR operand\n");
					else if(er==-13) simLog(sim,10,"missing & operand\n");
					else if(er==-15) simLog(sim,10,"mismatched braces\n");
					else if(er==-20) simLog(sim,10,"product pattern is incompatible with reactant pattern\n");
					else if(er==-30) simLog(sim,10,"failed to add reaction\n");
					else simLog(sim,10,"BUG: unknown error\n");
					return er; }
				er=RxnSetValuePattern(sim,"rate",pattern,rname,rctstate,prdstate,rate,oldnresults,rule->rulerxn);
				if(er) return -40;
				molpatternindex(sim,pattern,rname,1,0,&index); }

			else if(ruletype==RTdifc) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetdifc(sim,index[PDMAX+i],NULL,ms,rate); }

			else if(ruletype==RTdifm) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetdifm(sim,index[PDMAX+i],NULL,ms,detailsf); }

			else if(ruletype==RTdrift) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetdrift(sim,index[PDMAX+i],NULL,ms,detailsf); }

			else if(ruletype==RTsurfdrift) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetsurfdrift(sim,index[PDMAX+i],NULL,ms,detailsi[1],(enum PanelShape)detailsi[2],detailsf); }

			else if(ruletype==RTmollist) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetlistlookup(sim,index[PDMAX+i],NULL,ms,detailsi[1]); }

			else if(ruletype==RTdispsize) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetdisplaysize(sim,index[PDMAX+i],NULL,ms,rate); }

			else if(ruletype==RTcolor) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				for(i=oldnresults;i<index[PDnresults];i++)
					molsetcolor(sim,index[PDMAX+i],NULL,ms,detailsf); }

			else if(ruletype==RTsurfaction) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				srf=sim->srfss->srflist[detailsi[1]];
				for(i=oldnresults;i<index[PDnresults];i++)
					surfsetaction(srf,index[PDMAX+i],NULL,ms,(enum PanelFace)detailsi[2],(enum SrfAction)detailsi[3],-1); }

			else if(ruletype==RTsurfrate) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				srf=sim->srfss->srflist[detailsi[1]];
				for(i=oldnresults;i<index[PDnresults];i++) {
					i3=detailsi[4]>0?detailsi[4]:index[PDMAX+i];
					surfsetrate(srf,index[PDMAX+i],NULL,ms,(enum MolecState)detailsi[2],(enum MolecState)detailsi[3],i3,rate,1); }}

			else if(ruletype==RTsurfrateint) {
				molpatternindex(sim,pattern,rname,1,2,&index);
				ms=(enum MolecState) detailsi[0];
				srf=sim->srfss->srflist[detailsi[1]];
				for(i=oldnresults;i<index[PDnresults];i++) {
					i3=detailsi[4]>0?detailsi[4]:index[PDMAX+i];
					surfsetrate(srf,index[PDMAX+i],NULL,ms,(enum MolecState)detailsi[2],(enum MolecState)detailsi[3],i3,rate,1); }}

			rule->rulenresults=index[PDnresults]; }

		if(iter+1<iterations || iterations==-1) {
			done=1;															// determine if done or not, for generate-first
			for(r=0;r<ruless->nrule;r++) {
				rule=ruless->rule[r];
				rname=rule->rulename;
				pattern=rule->rulepattern;
				molpatternindex(sim,pattern,rname,1,0,&index);
				if(index[PDnspecies]!=-1 && index[PDnspecies]<sim->mols->nspecies) done=0; }}}

	if(ruless->ruleonthefly==1)
		for(i=0;i<nspecies;i++)					// update expand value to indicate expansion done
			if(sim->mols->expand[i]==1)
				sim->mols->expand[i]|=2;

	return 0; }




