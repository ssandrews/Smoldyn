/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.  See documentation
 called SmoldynUsersManual.pdf and SmoldynCodeDoc.pdf.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu General Public License (GPL). */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include "Geometry.h"
#include "List.h"
#include "math2.h"
#include "random2.h"
#include "Rn.h"
#include "rxnparam.h"
#include "SimCommand.h"
#include "Sphere.h"
#include "string2.h"
#include "Zn.h"

#include "smoldynconfigure.h"
#ifdef OPTION_NSV
  #include "nsvc.h"
#endif

#include "smoldyn.h"
#include "smoldynfuncs.h"

/******************************************************************************/
/********************************** Reactions *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types

// low level utilities
int rxnpackident(int order,int maxspecies,const int *ident);
void rxnunpackident(int order,int maxspecies,int ipack,int *ident);
enum MolecState rxnpackstate(int order,enum MolecState *mstate);
void rxnunpackstate(int order,enum MolecState mspack,enum MolecState *mstate);
int rxnreactantstate(rxnptr rxn,enum MolecState *mstate,int convertb2f);
int rxnallstates(rxnptr rxn);
int findreverserxn(simptr sim,int order,int r,int *optr,int *rptr);

// memory management
rxnptr rxnalloc(int order);
rxnssptr rxnssalloc(rxnssptr rxnss,int order,int maxspecies);

// data structure output

// parameter calculations
int rxnsetrates(simptr sim,int order);
int rxnsetproduct(simptr sim,int order,int r);
int rxnsetproducts(simptr sim,int order);
double rxncalcrate(simptr sim,int order,int r,double *pgemptr);
void rxncalctau(simptr sim,int order);

// structure set up
void RxnCopyRevparam(simptr sim,rxnptr rxn,const rxnptr templ);
int rxnsupdateparams(simptr sim);
int rxnsupdatelists(simptr sim,int order);
rxnptr RxnTestRxnExist(simptr sim,int order,const char *rname,const int *rctident,const enum MolecState *rctstate,int nprod,const int *prdident,const enum MolecState *prdstate,int exact);

// core simulation functions
int morebireact(simptr sim,rxnptr rxn,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,enum EventType et,double *vect);


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* rxnstring2rp */
enum RevParam rxnstring2rp(const char *string) {
	enum RevParam ans;

	if(!strcmp(string,"i")) ans=RPirrev;
	else if(!strcmp(string,"a")) ans=RPconfspread;
	else if(!strcmp(string,"p")) ans=RPpgem;
	else if(!strcmp(string,"x")) ans=RPpgemmax;
	else if(!strcmp(string,"r")) ans=RPratio;
	else if(!strcmp(string,"b")) ans=RPunbindrad;
	else if(!strcmp(string,"q")) ans=RPpgem2;
	else if(!strcmp(string,"y")) ans=RPpgemmax2;
	else if(!strcmp(string,"s")) ans=RPratio2;
	else if(!strcmp(string,"o")) ans=RPoffset;
	else if(!strcmp(string,"f")) ans=RPfixed;
	else if(!strcmp(string,"irrev")) ans=RPirrev;
	else if(!strcmp(string,"confspread")) ans=RPconfspread;
	else if(!strcmp(string,"bounce")) ans=RPbounce;
	else if(!strcmp(string,"pgem")) ans=RPpgem;
	else if(!strcmp(string,"pgemmax")) ans=RPpgemmax;
	else if(!strcmp(string,"ratio")) ans=RPratio;
	else if(!strcmp(string,"unbindrad")) ans=RPunbindrad;
	else if(!strcmp(string,"pgem2")) ans=RPpgem2;
	else if(!strcmp(string,"pgemmax2")) ans=RPpgemmax2;
	else if(!strcmp(string,"ratio2")) ans=RPratio2;
	else if(!strcmp(string,"offset")) ans=RPoffset;
	else if(!strcmp(string,"fixed")) ans=RPfixed;
	else ans=RPnone;
	return ans; }


/* rxnrp2string */
char *rxnrp2string(enum RevParam rp,char *string) {
	if(rp==RPirrev) strcpy(string,"irrev");
	else if(rp==RPconfspread) strcpy(string,"confspread");
	else if(rp==RPbounce) strcpy(string,"bounce");
	else if(rp==RPpgem) strcpy(string,"pgem");
	else if(rp==RPpgemmax) strcpy(string,"pgemmax");
	else if(rp==RPpgemmaxw) strcpy(string,"pgemmaxw");
	else if(rp==RPratio) strcpy(string,"ratio");
	else if(rp==RPunbindrad) strcpy(string,"unbindrad");
	else if(rp==RPpgem2) strcpy(string,"pgem2");
	else if(rp==RPpgemmax2) strcpy(string,"pgemmax2");
	else if(rp==RPratio2) strcpy(string,"ratio2");
	else if(rp==RPoffset) strcpy(string,"offset");
	else if(rp==RPfixed) strcpy(string,"fixed");
	else strcpy(string,"none");
	return string; }


/* rxnstring2sr */
enum SpeciesRepresentation rxnstring2sr(const char *string) {
	enum SpeciesRepresentation ans;

	if(strbegin(string,"particle",1)) ans=SRparticle;
	else if(strbegin(string,"lattice",1)) ans=SRlattice;
	else if(strbegin(string,"both",1)) ans=SRboth;
	else if(!strcmp(string,"all")) ans=SRboth;
	else ans=SRnone;
	return ans; }


/* rxnsr2string */
char *rxnsr2string(enum SpeciesRepresentation sr,char *string) {
	if(sr==SRparticle) strcpy(string,"particle");
	else if(sr==SRlattice) strcpy(string,"lattice");
	else if(sr==SRboth) strcpy(string,"both");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/***************************** low level utilities ****************************/
/******************************************************************************/


/* readrxnname. */
int readrxnname(simptr sim,const char *rname,int *orderptr,rxnptr *rxnpt,listptrv *vlistptr,int rxntype) {
	int r,order,*index,j,d;
	unsigned int rnamelen;
	ruleptr rule;
	rxnptr rxn;
	char *testname;
	listptrv vlist;

	if(rxntype==1) {															// defined reaction with this exact name
		r=-1;
		for(order=0;order<MAXORDER;order++)					// look for a reaction with this name
			if(sim->rxnss[order]) {
				r=stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,rname);
				if(r>=0) {															// found a reaction with this name
					if(orderptr) *orderptr=order;
					if(rxnpt) *rxnpt=sim->rxnss[order]->rxn[r];
					if(vlistptr) *vlistptr=NULL;
					return r; }}
		return -1; }																// return -1 for not found

	if(rxntype==2) {															// defined reactions with name_number
		vlist=NULL;
		rxn=NULL;
		rnamelen=strlen(rname);
		for(order=0;order<MAXORDER && vlist==NULL;order++)
			if(sim->rxnss[order])
				for(r=0;r<sim->rxnss[order]->totrxn;r++) {
					testname=sim->rxnss[order]->rname[r];
					if(strbegin(rname,testname,1) && strlen(testname)>rnamelen+1 && testname[rnamelen]=='_' && strisnumber(testname+rnamelen+1)) {
						if(!rxn) rxn=sim->rxnss[order]->rxn[r];	// rxn is the first one that's found
						vlist=ListAppendItemV(vlist,(void*) sim->rxnss[order]->rxn[r]);
						if(!vlist) return -2; }}						// return -2 for out of memory
		if(vlist) {
			order--;
			if(orderptr) *orderptr=order;
			if(rxnpt) *rxnpt=rxn;
			if(vlistptr) *vlistptr=vlist;
			else ListFreeV(vlist);
			return 0; }																// return 0 for reaction(s) found
		return -1; }																// return -1 for none found

	if(rxntype==3) {															// reaction rule with this name
		if(!sim->ruless) return -1;
		for(r=0;r<sim->ruless->nrule;r++) {
			rule=sim->ruless->rule[r];
			if(rule->ruletype==RTreaction && !strcmp(rule->rulename,rname)) {	// found a reaction rule with this name
				molpatternindex(sim,rule->rulepattern,rname,1,0,&index);
				order=index[PDmatch];
				if(!rule->rulerxn) {										// create a reaction template for the rule if none already
					rule->rulerxn=rxnalloc(order);
					if(!rule->rulerxn) return -2;
					rxn=rule->rulerxn;
					rxn->rname=StringCopy(rule->rulename);
					if(!rxn->rname) return -2;
					for(j=0;j<order;j++)
						rxn->rctstate[j]=(enum MolecState) rule->ruledetailsi[j];
					rxn->nprod=index[PDsubst];
					rxn->prdstate=(enum MolecState*) calloc(rxn->nprod,sizeof(enum MolecState));
					if(!rxn->prdstate) return -2;
					for(j=0;j<rxn->nprod;j++)
						rxn->prdstate[j]=(enum MolecState) rule->ruledetailsi[order+j];
					rxn->prdpos=(double **) calloc(rxn->nprod,sizeof(double*));
					if(!rxn->prdpos) return -2;
					for(j=0;j<rxn->nprod;j++) rxn->prdpos[j]=NULL;
					for(j=0;j<rxn->nprod;j++) {
						rxn->prdpos[j]=(double*) calloc(sim->dim,sizeof(double));
						if(!rxn->prdpos[j]) return -2;
						for(d=0;d<sim->dim;d++) rxn->prdpos[j][d]=0; }}
				if(orderptr) *orderptr=order;
				if(rxnpt) *rxnpt=rule->rulerxn;
				if(vlistptr) *vlistptr=NULL;
				return r; }}
		return -1; }

	return -1; }


/* rxnpackident */
int rxnpackident(int order,int maxspecies,const int *ident) {
	if(order==0) return 0;
	if(order==1) return ident[0];
	if(order==2) return ident[0]*maxspecies+ident[1];
	return 0; }


/* rxnunpackident */
void rxnunpackident(int order,int maxspecies,int ipack,int *ident) {
	if(order==0);
	else if(order==1) ident[0]=ipack;
	else if(order==2) {ident[0]=ipack/maxspecies;ident[1]=ipack%maxspecies;}
	return; }


/* rxnpackstate */
enum MolecState rxnpackstate(int order,enum MolecState *mstate) {
	if(order==0) return (MolecState)0;
	if(order==1) return mstate[0];
	if(order==2) return (MolecState)(mstate[0]*MSMAX1+mstate[1]);
	return (MolecState)0; }


/* rxnunpackstate */
void rxnunpackstate(int order,enum MolecState mspack,enum MolecState *mstate) {
	if(order==0);
	else if(order==1) mstate[0]=mspack;
	else if(order==2) {mstate[0]=(MolecState)(mspack/MSMAX1);mstate[1]=(MolecState)(mspack%MSMAX1);}
	return; }


/* rxnallstates */
int rxnallstates(rxnptr rxn) {
	enum MolecState ms;
	int nms2o;

	if(rxn->rxnss->order==0) return 0;
	nms2o=intpower(MSMAX1,rxn->rxnss->order);
	for(ms=(MolecState)0;ms<nms2o && rxn->permit[ms];ms=(MolecState)(ms+1));
	if(ms==nms2o) return 1;
	return 0; }


/* rxnreactantstate */
int rxnreactantstate(rxnptr rxn,enum MolecState *mstate,int convertb2f) {
	int order,permit;
	enum MolecState ms,ms1,ms2;
	int mspair;

	order=rxn->rxnss->order;
	permit=0;

	if(order==0) permit=1;

	else if(order==1) {
		if(rxn->permit[MSsoln]) {
			ms=MSsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln]) {
			ms=MSbsoln;
			permit=1; }
		else {
			for(ms=(MolecState)0;ms<MSMAX1 && !rxn->permit[ms];ms=(MolecState)(ms+1));
			if(ms<MSMAX1) permit=1; }
		if(permit && convertb2f && ms==MSbsoln) ms=MSsoln;
		if(mstate) {
			if(permit) mstate[0]=ms;
			else mstate[0]=MSnone; }}

	else if(order==2) {
		if(rxn->permit[MSsoln*MSMAX1+MSsoln]) {
			ms1=ms2=MSsoln;
			permit=1; }
		else if(rxn->permit[MSsoln*MSMAX1+MSbsoln]) {
			ms1=MSsoln;
			ms2=MSbsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln*MSMAX1+MSsoln]) {
			ms1=MSbsoln;
			ms2=MSsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln*MSMAX1+MSbsoln]) {
			ms1=MSbsoln;
			ms2=MSbsoln;
			permit=1; }
		if(!permit) {
			for(ms1=(MolecState)0;ms1<MSMAX1 && !rxn->permit[ms1*MSMAX1+MSsoln];ms1=(MolecState)(ms1+1));
			if(ms1<MSMAX1) {
				ms2=MSsoln;
				permit=1; }}
		if(!permit) {
			for(ms2=(MolecState)0;ms2<MSMAX1 && !rxn->permit[MSsoln*MSMAX1+ms2];ms2=(MolecState)(ms2+1));
			if(ms2<MSMAX1) {
				ms1=MSsoln;
				permit=1; }}
		if(!permit) {
			for(mspair=0;mspair<MSMAX1*MSMAX1 && !rxn->permit[mspair];mspair++);
			if(mspair<MSMAX1*MSMAX1) {
				ms1=(MolecState)(mspair/MSMAX1);
				ms2=(MolecState)(mspair%MSMAX1);
				permit=1; }}
		if(permit && convertb2f) {
			if(ms1==MSbsoln) ms1=MSsoln;
			if(ms2==MSbsoln) ms2=MSsoln; }
		if(mstate) {
			mstate[0]=permit?ms1:MSnone;
			mstate[1]=permit?ms2:MSnone; }}

	return permit; }


/* findreverserxn */
int findreverserxn(simptr sim,int order,int r,int *optr,int *rptr) {
	rxnssptr rxnss,rxnssr;
	rxnptr rxn,rxnr;
	int orderr,rr,rrreturn,rev,identr,identrprd,j,jr,work[MAXORDER];
	enum MolecState mstater,mstaterprd;

	if(!sim || order<0 || order>MAXORDER || r<0) return -1;
	rxnss=sim->rxnss[order];
	if(!rxnss || r>=rxnss->totrxn) return -1;
	rxn=rxnss->rxn[r];

	orderr=rrreturn=0;
	if(order==0 || rxn->nprod==0 || rxn->nprod>=MAXORDER || !sim->rxnss[rxn->nprod]) rev=0;
	else {
		orderr=rxn->nprod;
		rxnssr=sim->rxnss[orderr];
		identr=rxnpackident(orderr,rxnssr->maxspecies,rxn->prdident);
		mstater=rxnpackstate(orderr,rxn->prdstate);

		rev=0;
		for(j=0;j<rxnssr->nrxn[identr];j++) {
			rr=rxnssr->table[identr][j];
			rxnr=rxnssr->rxn[rr];
			if(rxnr->permit[mstater]) {
				if(rev!=1 && rxnr->nprod==order && Zn_sameset(rxn->rctident,rxnr->prdident,work,order)) {
					identrprd=rxnpackident(order,rxnss->maxspecies,rxnr->prdident);
					mstaterprd=rxnpackstate(order,rxnr->prdstate);
					for(jr=0;jr<rxnss->nrxn[identrprd];jr++)
						if(rxnss->table[identrprd][jr]==r && rxnss->rxn[r]->permit[mstaterprd]) {
							rev=1;
							rrreturn=rr; }}
				if(!rev) {
					rev=2;
					rrreturn=rr; }}}}

	if(optr) *optr=orderr;
	if(rptr) *rptr=rrreturn;
	return rev; }


/* rxnisprod */
int rxnisprod(simptr sim,int i,enum MolecState ms,int code) {
	int order,r,prd;
	rxnssptr rxnss;
	rxnptr rxn;

	for(order=0;order<MAXORDER;order++) {
		rxnss=sim->rxnss[order];
		if(rxnss) {
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				for(prd=0;prd<rxn->nprod;prd++)
					if(rxn->prdident[prd]==i && rxn->prdstate[prd]==ms) {
						if(code==0) return 1;
						if(rxn->rparamt==RPconfspread || rxn->unbindrad>0) return 1;
						if(dotVVD(rxn->prdpos[prd],rxn->prdpos[prd],sim->dim)>0) return 1; }}}}
	return 0; }


/* rxnstring2sernocode */
long int rxnstring2sernocode(char *pattern,int prd) {
	long int pserno,bitcode;
	char *dotptr;
	int itct;

	pserno=0;
	bitcode=0;
	if((dotptr=strchr(pattern,'.'))) {
		*dotptr='\0';																				// there is a dot
		if(!pattern[0]) return 0;														// before the dot pattern
		else if(!strcmp(pattern,"new")) bitcode=0x0100;
		else if(!strcmp(pattern,"r1") || !strcmp(pattern,"r1R")) bitcode=0x0400;
		else if(!strcmp(pattern,"r2") || !strcmp(pattern,"r2R")) bitcode=0x0500;
		else if((!strcmp(pattern,"p1") || !strcmp(pattern,"p1R")) && prd>0) bitcode=0x0800;
		else if((!strcmp(pattern,"p2") || !strcmp(pattern,"p2R")) && prd>1) bitcode=0x0900;
		else if((!strcmp(pattern,"p3") || !strcmp(pattern,"p3R")) && prd>2) bitcode=0x0A00;
		else if((!strcmp(pattern,"p4") || !strcmp(pattern,"p4R")) && prd>3) bitcode=0x0B00;
		else if(!strcmp(pattern,"r1L")) bitcode=0x1400;
		else if(!strcmp(pattern,"r2L")) bitcode=0x1500;
		else if(!strcmp(pattern,"p1L") && prd>0) bitcode=0x1800;
		else if(!strcmp(pattern,"p2L") && prd>1) bitcode=0x1900;
		else if(!strcmp(pattern,"p3L") && prd>2) bitcode=0x1A00;
		else if(!strcmp(pattern,"p4L") && prd>3) bitcode=0x1B00;
		else return 0;
		*dotptr='.';
		dotptr++;
		if(!dotptr[0]) return 0;															// after the dot pattern
		else if(!strcmp(dotptr,"new")) bitcode|=0x0001;
		else if(!strcmp(dotptr,"r1") || !strcmp(dotptr,"r1R")) bitcode|=0x0004;
		else if(!strcmp(dotptr,"r2") || !strcmp(dotptr,"r2R")) bitcode|=0x0005;
		else if((!strcmp(dotptr,"p1") || !strcmp(dotptr,"p1R")) && prd>0) bitcode|=0x0008;
		else if((!strcmp(dotptr,"p2") || !strcmp(dotptr,"p2R")) && prd>1) bitcode|=0x0009;
		else if((!strcmp(dotptr,"p3") || !strcmp(dotptr,"p3R")) && prd>2) bitcode|=0x000A;
		else if((!strcmp(dotptr,"p4") || !strcmp(dotptr,"p4R")) && prd>3) bitcode|=0x000B;
		else if(!strcmp(dotptr,"r1L")) bitcode|=0x0014;
		else if(!strcmp(dotptr,"r2L")) bitcode|=0x0015;
		else if(!strcmp(dotptr,"p1L") && prd>0) bitcode|=0x0018;
		else if(!strcmp(dotptr,"p2L") && prd>1) bitcode|=0x0019;
		else if(!strcmp(dotptr,"p3L") && prd>2) bitcode|=0x001A;
		else if(!strcmp(dotptr,"p4L") && prd>3) bitcode|=0x001B;
		else return 0; }

	else if(!strcmp(pattern,"new")) bitcode=0x0001;					// no dot
	else if(!strcmp(pattern,"r1")) bitcode=0x1404;
	else if(!strcmp(pattern,"r2")) bitcode=0x1505;
	else if(!strcmp(pattern,"p1") && prd>0) bitcode=0x1808;
	else if(!strcmp(pattern,"p2") && prd>1) bitcode=0x1909;
	else if(!strcmp(pattern,"p3") && prd>2) bitcode=0x1A0A;
	else if(!strcmp(pattern,"p4") && prd>3) bitcode=0x1B0B;
	else if(!strcmp(pattern,"r1L")) bitcode=0x0014;
	else if(!strcmp(pattern,"r2L")) bitcode=0x0015;
	else if(!strcmp(pattern,"p1L") && prd>0) bitcode=0x0018;
	else if(!strcmp(pattern,"p2L") && prd>1) bitcode=0x0019;
	else if(!strcmp(pattern,"p3L") && prd>2) bitcode=0x001A;
	else if(!strcmp(pattern,"p4L") && prd>3) bitcode=0x001B;
	else if(!strcmp(pattern,"r1R")) bitcode=0x0004;
	else if(!strcmp(pattern,"r2R")) bitcode=0x0005;
	else if(!strcmp(pattern,"p1R") && prd>0) bitcode=0x0008;
	else if(!strcmp(pattern,"p2R") && prd>1) bitcode=0x0009;
	else if(!strcmp(pattern,"p3R") && prd>2) bitcode=0x000A;
	else if(!strcmp(pattern,"p4R") && prd>3) bitcode=0x000B;
	else {
		itct=sscanf(pattern,"%li",&pserno);
		if(itct!=1 || pserno<=0) return 0; }

	if(pserno==0)
		pserno=~bitcode;

	return pserno; }


/* rxnsernocode2string */
char *rxnsernocode2string(long int pserno,char *pattern) {
	long int bitcode;

	if(pserno>=0) {
		snprintf(pattern,STRCHAR*sizeof(pattern),"%li",pserno);
		return pattern; }

	pattern[0]='\0';
	bitcode=~pserno;

	if(bitcode==0x0001) strcat(pattern,"new");				// 0001 = new
	else if((bitcode&0x0F00)>>8==(bitcode&0x00F) && bitcode&0x1000 && !(bitcode&0x0010)) {	// 1x0x = r1, r2, p1, p2, p3, p4
		if(bitcode&0x8) strcat(pattern,"p");
		else strcat(pattern,"r");
		if((bitcode&0x3)==0x0) strcat(pattern,"1");
		else if((bitcode&0x3)==0x1) strcat(pattern,"2");
		else if((bitcode&0x3)==0x2) strcat(pattern,"3");
		else strcat(pattern,"4"); }
	else {																						// everything else
		if(bitcode&0xFF00) {
			if((bitcode&0xFF00)==0x0100) strcat(pattern,"new");
			else if(bitcode&0x0800) strcat(pattern,"p");
			else strcat(pattern,"r");
			if((bitcode&0xFF00)==0x0100);
			else if((bitcode&0x0300)==0x0000) strcat(pattern,"1");
			else if((bitcode&0x0300)==0x0100) strcat(pattern,"2");
			else if((bitcode&0x0300)==0x0200) strcat(pattern,"3");
			else strcat(pattern,"4");
			if((bitcode&0xFF00)==0x0100);
			else if(bitcode&0x1000) strcat(pattern,"L");
			else strcat(pattern,"R"); }

		if(bitcode&0xFF00 && bitcode&0xFF)
			strcat(pattern,".");

		if(bitcode&0xFF) {
			if((bitcode&0xFF)==0x01) strcat(pattern,"new");
			else if(bitcode&0x08) strcat(pattern,"p");
			else strcat(pattern,"r");
			if((bitcode&0xFF)==0x01);
			else if((bitcode&0x03)==0x00) strcat(pattern,"1");
			else if((bitcode&0x03)==0x01) strcat(pattern,"2");
			else if((bitcode&0x03)==0x02) strcat(pattern,"3");
			else strcat(pattern,"4");
			if((bitcode&0xFF)==0x01);
			else if(bitcode&0x10) strcat(pattern,"L");
			else strcat(pattern,"R"); }}

	return pattern; }



/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/


/* rxnalloc */
rxnptr rxnalloc(int order) {
	rxnptr rxn;
	int rct,nms2o;
	enum MolecState ms;

	CHECKMEM(rxn=(rxnptr) malloc(sizeof(struct rxnstruct)));
	rxn->rxnss=NULL;
	rxn->rname=NULL;
	rxn->rctident=NULL;
	rxn->rctstate=NULL;
	rxn->permit=NULL;
	rxn->nprod=0;
	rxn->prdident=NULL;
	rxn->prdstate=NULL;
	rxn->rctrep=NULL;
	rxn->prdrep=NULL;
	rxn->prdserno=NULL;
	rxn->prdintersurf=NULL;
	rxn->logserno=NULL;
	rxn->logfile=NULL;
	rxn->rate=-1;
	rxn->multiplicity=-1;
#ifdef OPTION_VCELL
	rxn->rateValueProvider=NULL;
#endif
	rxn->bindrad2=-1;
	rxn->prob=-1;
	rxn->chi=-1;
	rxn->tau=-1;
	rxn->rparamt=RPnone;
	rxn->rparam=0;
	rxn->unbindrad=-1;
	rxn->prdpos=NULL;
	rxn->disable=0;
	rxn->cmpt=NULL;
	rxn->srf=NULL;

	if(order>0) {
		CHECKMEM(rxn->rctident=(int*)calloc(order,sizeof(int)));
		for(rct=0;rct<order;rct++) rxn->rctident[rct]=0;
		CHECKMEM(rxn->rctstate=(enum MolecState*)calloc(order,sizeof(int)));
		for(rct=0;rct<order;rct++) rxn->rctstate[rct]=MSnone;
		nms2o=intpower(MSMAX1,order);
		CHECKMEM(rxn->permit=(int*)calloc(nms2o,sizeof(int)));
		for(ms=(MolecState)0;ms<nms2o;ms=(MolecState)(ms+1)) rxn->permit[ms]=0; }
	return rxn;

 failure:
	rxnfree(rxn);
	simLog(NULL,10,"Unable to allocate memory in rxnalloc");
	return NULL; }


/* rxnfree */
void rxnfree(rxnptr rxn) {
	int prd;

	if(!rxn) return;
	if(rxn->prdpos)
		for(prd=0;prd<rxn->nprod;prd++)
			free(rxn->prdpos[prd]);
	free(rxn->prdpos);
	free(rxn->prdstate);
	free(rxn->rctrep);
	free(rxn->prdrep);
	free(rxn->prdserno);
	free(rxn->prdintersurf);
	ListFreeLI(rxn->logserno);
	free(rxn->logfile);
	free(rxn->prdident);
	free(rxn->permit);
	free(rxn->rctstate);
	free(rxn->rctident);
	free(rxn);
	return; }


/* rxnssalloc */
rxnssptr rxnssalloc(rxnssptr rxnss,int order,int maxspecies) {
	int newni2o,oldni2o,i,i2,failfree;
	int ilist[MAXORDER],*newnrxn,**newtable;

	failfree=0;
	if(!rxnss) {																				// new reaction superstructure
		rxnss=(rxnssptr) malloc(sizeof(struct rxnsuperstruct));
		CHECKMEM(rxnss);
		failfree=1;
		rxnss->condition=SCinit;
		rxnss->sim=NULL;
		rxnss->order=order;
		rxnss->maxspecies=0;
		rxnss->maxlist=0;
		rxnss->nrxn=NULL;
		rxnss->table=NULL;
		rxnss->maxrxn=0;
		rxnss->totrxn=0;
		rxnss->rname=NULL;
		rxnss->rxn=NULL;
		rxnss->rxnmollist=NULL; }

	if(maxspecies>rxnss->maxspecies) {									// initialize or expand nrxn and table
		if(order>0) {
			newni2o=intpower(maxspecies,order);							// allocate new stuff
			CHECKMEM(newnrxn=(int*) calloc(newni2o,sizeof(int)));
			for(i=0;i<newni2o;i++) newnrxn[i]=0;
			CHECKMEM(newtable=(int**) calloc(newni2o,sizeof(int*)));
			for(i=0;i<newni2o;i++) newtable[i]=NULL;

			oldni2o=intpower(rxnss->maxspecies,order);
			for(i=0;i<oldni2o;i++) {												// copy over old nrxn and table
				rxnunpackident(order,rxnss->maxspecies,i,ilist);
				i2=rxnpackident(order,maxspecies,ilist);
				newnrxn[i2]=rxnss->nrxn[i];
				newtable[i2]=rxnss->table[i]; }

			free(rxnss->nrxn);															// replace nrxn and table with new ones
			rxnss->nrxn=newnrxn;
			free(rxnss->table);
			rxnss->table=newtable; }
		rxnss->maxspecies=maxspecies; }										// set maxspecies

	return rxnss;

 failure:
	if(failfree) rxnssfree(rxnss);
	simLog(NULL,10,"Unable to allocate memory in rxnssalloc");
	return NULL; }


/* rxnssfree */
void rxnssfree(rxnssptr rxnss) {
	int r,i,ni2o;

	if(!rxnss) return;

	free(rxnss->rxnmollist);
	if(rxnss->rxn)
		for(r=0;r<rxnss->maxrxn;r++) rxnfree(rxnss->rxn[r]);
	free(rxnss->rxn);
	if(rxnss->rname)
		for(r=0;r<rxnss->maxrxn;r++) free(rxnss->rname[r]);
	free(rxnss->rname);
	if(rxnss->table) {
		ni2o=intpower(rxnss->maxspecies,rxnss->order);
		for(i=0;i<ni2o;i++) free(rxnss->table[i]);
		free(rxnss->table); }
	free(rxnss->nrxn);
	free(rxnss);
	return; }


/* rxnexpandmaxspecies */
int rxnexpandmaxspecies(simptr sim,int maxspecies) {
	rxnssptr rxnss;
	int order;

	for(order=0;order<MAXORDER;order++) {
		if(sim->rxnss[order] && sim->rxnss[order]->maxspecies<maxspecies) {
			rxnss=sim->rxnss[order];
			rxnss=rxnssalloc(rxnss,order,maxspecies);
			if(!rxnss) return order+1; }}
	return 0; }


/******************************************************************************/
/**************************** data structure output ***************************/
/******************************************************************************/


/* rxnoutput */
void rxnoutput(simptr sim,int order) {
	rxnssptr rxnss;
	int d,dim,maxlist,maxll2o,ll,ord,ni2o,i,j,r,rct,prd,rev,identlist[MAXORDER],orderr,rr,i1,i2,o2,r2;
	rxnptr rxn,revrxn;
	enum MolecState ms,ms1,ms2,nms2o,statelist[MAXORDER];
	double dsum,step,pgem,rate3,bindrad,ratio,smolmodelrate,actualrate,revunbindrad,chi,lambdap;
	double revrate,revdsum,revprob,rparam,bindradrev;
	enum RevParam rparamt;
	char string[STRCHAR];
	int vflag;

	vflag=strchr(sim->flags,'v')?1:0;

	simLog(sim,2,"ORDER %i REACTION PARAMETERS\n",order);
	if(!sim || !sim->mols || !sim->rxnss[order]) {
		simLog(sim,2," No reactions of order %i\n\n",order);
		return; }
	rxnss=sim->rxnss[order];
	if(rxnss->condition!=SCok)
		simLog(sim,2," structure condition: %s\n",simsc2string(rxnss->condition,string));
	dim=sim->dim;

	simLog(sim,1," allocated for %i species\n",rxnss->maxspecies-1);
	simLog(sim,1," allocated for %i molecule lists\n",rxnss->maxlist);

	simLog(sim,2," %i reactions defined",rxnss->totrxn);
	simLog(sim,1,", of %i allocated",rxnss->maxrxn);
	simLog(sim,2,"\n");
	
	if(order>0) {
		simLog(sim,2," Reactive molecule lists:");
		if(!rxnss->rxnmollist || !sim->mols->nlist)
			simLog(sim,2," not set up yet");
		else {
			maxlist=sim->mols->maxlist;
			maxll2o=intpower(maxlist,order);
			for(ll=0;ll<maxll2o;ll++)
				if(rxnss->rxnmollist[ll]) {
					simLog(sim,2," ");
					for(ord=0;ord<order;ord++)
						simLog(sim,2,"%s%s",sim->mols->listname[(ll/intpower(maxlist,ord))%maxlist],ord<order-1?"+":""); }}
		simLog(sim,2,"\n"); }

	if(order>0) {
		simLog(sim,2," Reactants, sorted by molecule species:\n");
		ni2o=intpower(rxnss->maxspecies,order);
		for(i=0;i<ni2o;i++)
			if(rxnss->nrxn[i]) {
				rxnunpackident(order,rxnss->maxspecies,i,identlist);
				if(Zn_issort(identlist,order)>=1) {
					simLog(sim,2,"  ");
					for(ord=0;ord<order;ord++)
						simLog(sim,2,"%s%s",sim->mols->spname[identlist[ord]],ord<order-1?"+":"");
					simLog(sim,2," :");
					for(j=0;j<rxnss->nrxn[i];j++) simLog(sim,2," %s",rxnss->rname[rxnss->table[i][j]]);
					simLog(sim,2,"\n"); }}}

	simLog(sim,2," Reaction details:\n");
	for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
		simLog(sim,2,"  Reaction %s:",rxn->rname);
		if(order==0) simLog(sim,2," 0");							// reactants
		for(rct=0;rct<order;rct++) {
			simLog(sim,2," %s",sim->mols->spname[rxn->rctident[rct]]);
			if(rxn->rctstate[rct]!=MSsoln) simLog(sim,2," (%s)",molms2string(rxn->rctstate[rct],string));
			if(rct<order-1) simLog(sim,2," +"); }
		simLog(sim,2," ->");
		if(rxn->nprod==0) simLog(sim,2," 0");					// products
		for(prd=0;prd<rxn->nprod;prd++) {
			simLog(sim,2," %s",sim->mols->spname[rxn->prdident[prd]]);
			if(rxn->prdstate[prd]!=MSsoln) simLog(sim,2," (%s)",molms2string(rxn->prdstate[prd],string));
			if(prd<rxn->nprod-1) simLog(sim,2," +"); }
		simLog(sim,2,"\n");

		for(rct=0;rct<order;rct++)							// permit
			if(rxn->rctstate[rct]==MSsome) rct=order+1;
		if(rct==order+1) {
			simLog(sim,2,"   permit:");
			nms2o=MolecState(intpower(MSMAX1,order));
			for(ms=MolecState(0);ms<nms2o;ms=MolecState(ms + 1)) {
				if(rxn->permit[ms]) {
					rxnunpackstate(order,ms,statelist);
					simLog(sim,2," ");
					for(ord=0;ord<order;ord++)
						simLog(sim,2,"%s%s",molms2string(statelist[ms],string),ord<order-1?"+":""); }}}

		if(rxn->prdserno) {											// product serial number rules
			simLog(sim,2,"   serial number rules:");
			for(prd=0;prd<rxn->nprod;prd++)
				simLog(sim,2," %s",rxnsernocode2string(rxn->prdserno[prd],string));
			simLog(sim,2,"\n"); }

		if(rxn->prdintersurf) {									// product intersurface rules
			simLog(sim,2,"   intersurface rules:");
			if(!rxn->nprod)
				simLog(sim,2," allowed");
			else {
				for(prd=0;prd<rxn->nprod;prd++) {
					i1=rxn->prdintersurf[prd];
					if(i1==1) simLog(sim,2," r1");
					else simLog(sim,2," r2"); }}
			simLog(sim,2,"\n"); }

		if(rxn->rctrep && rxn->prdrep) {				// reaction representation rules
			simLog(sim,2,"   representation rules:");
			if(order==0)
				simLog(sim,2," 0");
			for(rct=0;rct<order;rct++)
				simLog(sim,2,"%s %s",rct==0?"":" +",rxnsr2string(rxn->rctrep[rct],string));
			simLog(sim,2," ->");
			if(rxn->nprod==0)
				simLog(sim,2," 0");
			for(prd=0;prd<rxn->nprod;prd++)
				simLog(sim,2,"%s %s",prd==0?"":" +",rxnsr2string(rxn->prdrep[prd],string));
			simLog(sim,2,"\n"); }

		if(rxn->logserno) {											// reaction log
			simLog(sim,2,"   reaction logged to file %s",rxn->logfile);
			if(rxn->logserno->n==1 && rxn->logserno->xs[0]==-1)
				simLog(sim,2," for all molecules\n");
			else {
				simLog(sim,2,"\n    molecules:");
				for(i=0;i<rxn->logserno->n && i<5;i++)
					simLog(sim,2," %li",rxn->logserno->xs[i]);
				if(i<rxn->logserno->n)
					simLog(sim,2,"...%li",rxn->logserno->xs[rxn->logserno->n-1]);
				simLog(sim,2,"\n"); }}

		if(rxn->cmpt) simLog(sim,2,"   compartment: %s\n",rxn->cmpt->cname);
		if(rxn->srf) simLog(sim,2,"   surface: %s\n",rxn->srf->sname);
		if(rxn->multiplicity>1) simLog(sim,2,"   elementary rate and multiplicity: %g, %i\n",rxn->rate,rxn->multiplicity);
		actualrate=rxncalcrate(sim,order,r,&pgem);								// actual rate constant and pgem values for this reaction
		if(rxn->rate>=0) simLog(sim,2,"   requested and actual rate constants: %g, %g\n",rxn->rate*rxn->multiplicity,actualrate);
		else simLog(sim,2,"   actual rate constant: %g\n",actualrate);
		if(pgem>=0) simLog(sim,2,"   geminate recombination probability: %g\n",pgem);
		if(rxn->rparamt==RPconfspread) simLog(sim,2,"   conformational spread reaction\n");
		if(rxn->tau>=0) simLog(sim,2,"   characteristic time: %g\n",rxn->tau);
		if(order==0) simLog(sim,2,"   average reactions per time step: %g\n",rxn->prob);
		else if(order==1) simLog(sim,2,"   conditional reaction probability per time step: %g\n",rxn->prob);			// this is the conditional probability, with the condition that prior possible reactions did not happen
		else if(rxn->prob>=0 && rxn->prob!=1) simLog(sim,2,"   reaction probability after collision: %g\n",rxn->prob);
		bindrad=sqrt(rxn->bindrad2);															// binding radius for this reaction
		if(rxn->bindrad2>=0) simLog(sim,2,"   binding radius: %g\n",bindrad);

		if(rxn->nprod==2) {													// unbinding information
			if(rxn->unbindrad>=0) simLog(sim,2,"   unbinding radius: %g\n",rxn->unbindrad);
			else if(rxn->rparamt==RPbounce && rxn->unbindrad==-1) simLog(sim,2,"   unbinding radius: calculated from molecule overlap\n");
			else if(rxn->rparamt==RPbounce && rxn->unbindrad==-2) simLog(sim,2,"   unbinding radius: calculated for ballistic reflection\n");
			else simLog(sim,2,"   unbinding radius: 0\n");
			if(rxn->rparamt!=RPconfspread && rxn->rparamt!=RPbounce && findreverserxn(sim,order,r,&o2,&r2)==1) {
				revrxn=sim->rxnss[o2]->rxn[r2];
				revdsum=MolCalcDifcSum(sim,rxn->prdident[0],rxn->prdstate[0],rxn->prdident[1],rxn->prdstate[1]);
				revrate=rxncalcrate(sim,o2,r2,NULL);
				revprob=revrxn->prob;
				if(rxn->prdident[0]==rxn->prdident[1]) revrate*=2;				// same reactants
				rparamt=rxn->rparamt;
				rparam=rxn->rparam;
				if(rparamt==RPunbindrad) bindradrev=bindingradiusprob(revrate,0,revdsum,rparam,0,-1,&revprob);
				else if(rparamt==RPratio) bindradrev=bindingradiusprob(revrate,0,revdsum,rparam,1,-1,&revprob);
				else if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw) {
					bindradrev=bindingradiusprob(revrate*(1.0-rparam),0,revdsum,-1,0,-1,&revprob);
					pgem=rparam; }
				else bindradrev=bindingradiusprob(revrate,0,revdsum,-1,0,-1,&revprob);
				simLog(sim,2,"   unbinding radius if dt were 0: %g\n",unbindingradius(pgem,0,revdsum,bindradrev)); }}

		if(order==2 && rxnreactantstate(rxn,statelist,1) && (vflag || rxn->chi>0 || rxn->prob<1)) {			// compute reaction rate details
			ms1=statelist[0];
			ms2=statelist[1];
			i1=rxn->rctident[0];
			i2=rxn->rctident[1];
			dsum=MolCalcDifcSum(sim,i1,ms1,i2,ms2);
			step=sqrt(2.0*dsum*sim->dt);
			ratio=step/sqrt(rxn->bindrad2);
			rate3=actualrate;
			if(i1==i2) rate3*=2;																		// rate3 is actual rate constant, corrected here for same reactants
			rev=findreverserxn(sim,2,r,&o2,&r2);
			revrxn=rev?sim->rxnss[o2]->rxn[r2]:NULL;
			rparamt=revrxn?revrxn->rparamt:RPnone;
			chi=smolmodelrate=-1;
			if(step>0 && (!rev || rparamt!=RPconfspread || rparamt!=RPbounce)) {					// a normal bimolecular reaction
				revunbindrad=(revrxn && (rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPratio || rparamt==RPunbindrad)) ? revrxn->unbindrad:-1;
				smolmodelrate=modelrxnrate(bindrad,revunbindrad,dsum,1);									// rate for Smoluchowski model with simulation parameters
				chi=rate3/smolmodelrate;
				if(rxn->chi>0) simLog(sim,2,"   requested and actual chi values: %g, %g\n",rxn->chi,chi);
				else simLog(sim,1,"   actual chi value: %g\n",chi); }
			simLog(sim,1,"   mutual rms step length: %g\n",step);
			if(step>0) simLog(sim,1,"   step length / binding radius: %g (%s %s steps)\n",ratio,ratio>0.1 && ratio<10?"somewhat":"very",ratio>1?"large":"small");
			if(step>0 && (!rev || rparamt!=RPconfspread || rparamt!=RPbounce)) {					// a normal bimolecular reaction
				simLog(sim,1,"   activation-limited reaction rate: %g\n",1/(1/rate3-1/smolmodelrate));
				simLog(sim,1,"   dynamics are like two-radius Smoluchowski model with radius: %g\n",bindrad*chi);
				simLog(sim,1,"   dynamics are like C&K model with gamma' and gamma: %g, %g\n",(1-chi)/chi,bindrad*(1-chi)/chi);
				lambdap=(3.18243*chi-3.40864*chi*chi+0.984385*chi*chi*chi)/pow(1-chi,2.08761);
				simLog(sim,1,"   dynamics are like Doi model with lambda' and lambda: %g, %g\n",lambdap,lambdap/bindrad*lambdap/bindrad*dsum);
			}}

		if(rxn->rparamt!=RPbounce)
			if(findreverserxn(sim,order,r,&orderr,&rr)==1)
				simLog(sim,2,"   with reverse reaction %s, equilibrium constant is: %g\n",sim->rxnss[orderr]->rname[rr],rxn->rate*rxn->multiplicity/(sim->rxnss[orderr]->rxn[rr]->rate*sim->rxnss[orderr]->rxn[rr]->multiplicity));
		if(rxn->rparamt!=RPnone) simLog(sim,2,"   product placement method and parameter: %s %g\n",rxnrp2string(rxn->rparamt,string),rxn->rparam);
		for(prd=0;prd<rxn->nprod;prd++) {
			if(dotVVD(rxn->prdpos[prd],rxn->prdpos[prd],dim)>0) {
				simLog(sim,2,"   product %s displacement:",sim->mols->spname[rxn->prdident[prd]]);
				for(d=0;d<dim;d++) simLog(sim,2," %g",rxn->prdpos[prd][d]);
				simLog(sim,2,"\n"); }}

		if(rxn->nprod==2 && sim->rxnss[2] && rxn->rparamt!=RPconfspread && rxn->rparamt!=RPbounce) {
			i=rxnpackident(2,rxnss->maxspecies,rxn->prdident);
			for(j=0;j<sim->rxnss[2]->nrxn[i];j++) {
				rr=sim->rxnss[2]->table[i][j];
				revrxn=sim->rxnss[2]->rxn[rr];
				if(revrxn->bindrad2>=0 && revrxn->rparamt!=RPconfspread) {
					dsum=MolCalcDifcSum(sim,rxn->prdident[0],rxn->prdstate[0],rxn->prdident[1],rxn->prdstate[1]);
					step=sqrt(2.0*sim->dt*dsum);
					pgem=1.0-numrxnrateprob(step,sqrt(revrxn->bindrad2),-1,revrxn->prob)/numrxnrateprob(step,sqrt(revrxn->bindrad2),rxn->unbindrad,revrxn->prob);	// phi = 1-k_i/k_r
					rev=(revrxn->nprod==order);
					rev=rev && Zn_sameset(revrxn->prdident,rxn->rctident,identlist,order);
					simLog(sim,2,"   probability of geminate %s reaction '%s' is %g\n",rev?"reverse":"continuation",revrxn->rname,pgem); }}}}

	simLog(sim,2,"\n");
	return; }


/* writereactions */
void writereactions(simptr sim,FILE *fptr) {
	int order,r,prd,d,rct,i;
	long int pserno;
	rxnptr rxn;
	rxnssptr rxnss;
	char string[STRCHAR],string2[STRCHAR];
	enum MolecState ms,ms1,ms2;
	enum RevParam rparamt;

	fprintf(fptr,"# Reaction parameters\n");
	for(order=0;order<=2;order++)
		if(sim->rxnss[order]) {
			rxnss=sim->rxnss[order];
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];

				if(rxn->cmpt) fprintf(fptr,"reaction_cmpt %s",rxn->cmpt->cname);
				else if(rxn->srf) fprintf(fptr,"reaction_surface %s",rxn->srf->sname);
				else fprintf(fptr,"reaction");
				fprintf(fptr," %s",rxn->rname);
				if(order==0) fprintf(fptr," 0");							// reactants
				for(rct=0;rct<order;rct++) {
					fprintf(fptr," %s",sim->mols->spname[rxn->rctident[rct]]);
					if(rxn->rctstate[rct]!=MSsoln) fprintf(fptr,"(%s)",molms2string(rxn->rctstate[rct],string));
					if(rct<order-1) fprintf(fptr," +"); }
				fprintf(fptr," ->");
				if(rxn->nprod==0) fprintf(fptr," 0");					// products
				for(prd=0;prd<rxn->nprod;prd++) {
					fprintf(fptr," %s",sim->mols->spname[rxn->prdident[prd]]);
					if(rxn->prdstate[prd]!=MSsoln) fprintf(fptr,"(%s)",molms2string(rxn->prdstate[prd],string));
					if(prd<rxn->nprod-1) fprintf(fptr," +"); }
				if(rxn->rate>=0) fprintf(fptr," %g",rxn->rate);
				fprintf(fptr,"\n");
				if(rxn->multiplicity!=1) fprintf(fptr,"reaction_multiplicity %s %i\n",rxn->rname,rxn->multiplicity);

				if(order==1 && rxn->rctstate[0]==MSsome) {
					for(ms=(MolecState)0;ms<MSMAX;ms=(MolecState)(ms+1))
						if(rxn->permit[ms])
							fprintf(fptr,"reaction_permit %s %s\n",rxn->rname,molms2string(ms,string)); }
				else if(order==2 && (rxn->rctstate[0]==MSsome || rxn->rctstate[1]==MSsome)) {
					for(ms1=(MolecState)0;ms1<MSMAX1;ms1=(MolecState)(ms1+1))
						for(ms2=(MolecState)0;ms2<MSMAX1;ms2=(MolecState)(ms2+1))
							if(rxn->permit[ms1*MSMAX1+ms2])
								fprintf(fptr,"reaction_permit %s %s %s\n",rxn->rname,molms2string(ms1,string),molms2string(ms2,string2)); }

				if(rxn->rparamt==RPconfspread)
					fprintf(fptr,"confspread_radius %s %g\n",rxn->rname,rxn->bindrad2<0?0:sqrt(rxn->bindrad2));
				if(rxn->rate<0) {
					if(order==0 && rxn->prob>=0) fprintf(fptr,"reaction_production %s %g\n",rxn->rname,rxn->prob);
					else if(order==1 && rxn->prob>=0) fprintf(fptr,"reaction_probability %s %g\n",rxn->rname,rxn->prob);
					else if(order==2 && rxn->bindrad2>=0) fprintf(fptr,"binding_radius %s %g\n",rxn->rname,sqrt(rxn->bindrad2)); }
				if((order==2 && rxn->prob!=1 && rxn->rparamt!=RPconfspread) || (rxn->rparamt==RPconfspread && rxn->rate<0))
					fprintf(fptr,"reaction_probability %s %g\n",rxn->rname,rxn->prob);

				rparamt=rxn->rparamt;
				if(rparamt==RPirrev)
					fprintf(fptr,"product_placement %s irrev\n",rxn->rname);
				else if(rparamt==RPbounce || rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2)
					fprintf(fptr,"product_placement %s %s %g\n",rxn->rname,rxnrp2string(rparamt,string),rxn->rparam);
				else if(rparamt==RPoffset || rparamt==RPfixed) {
					for(prd=0;prd<rxn->nprod;prd++) {
						fprintf(fptr,"product_placement %s %s %s\n",rxn->rname,rxnrp2string(rparamt,string),sim->mols->spname[rxn->prdident[prd]]);
						for(d=0;d<sim->dim;d++) fprintf(fptr," %g",rxn->prdpos[prd][d]);
						fprintf(fptr,"\n"); }}

				if(rxn->prdserno) {
					fprintf(fptr,"reaction_serialnum %s",rxn->rname);
					for(prd=0;prd<rxn->nprod;prd++)
						fprintf(fptr," %s",rxnsernocode2string(rxn->prdserno[prd],string));
					fprintf(fptr,"\n"); }

				if(rxn->prdintersurf) {
					fprintf(fptr,"reaction_intersurface %s",rxn->rname);
					if(!rxn->nprod)
						fprintf(fptr," on");
					else {
						for(prd=0;prd<rxn->nprod;prd++) {
							pserno=rxn->prdserno[prd];
							if(pserno==1) fprintf(fptr," r1");
							else fprintf(fptr," r2"); }}
					fprintf(fptr,"\n"); }

				if(rxn->logserno) {											// reaction log
					fprintf(fptr,"reaction_log %s %s",rxn->logfile,rxn->rname);
					if(rxn->logserno->n==1 && rxn->logserno->xs[0]==-1)
						fprintf(fptr," all\n");
					else {
						for(i=0;i<rxn->logserno->n;i++)
							fprintf(fptr," %li",rxn->logserno->xs[i]);
						fprintf(fptr,"\n"); }}}}

	fprintf(fptr,"\n");
	return; }


/* checkrxnparams */
int checkrxnparams(simptr sim,int *warnptr) {
	int d,dim,warn,error,i1,i2,j,nspecies,r,i,ct,j1,j2,order,prd,vflag,prdparamflag;
	int rev,o2,r2;
	long int pserno,bitcode;
	molssptr mols;
	double minboxsize,vol,amax,vol2,vol3,actualrate,smolmodelrate,revunbindrad,chi,dsum;
	rxnptr rxn,rxn1,rxn2,revrxn;
	rxnssptr rxnss;
	char **spname,string[STRCHAR],string2[STRCHAR];
	enum MolecState ms1,ms2,ms,statelist[2];

	error=warn=0;
	dim=sim->dim;
	nspecies=sim->mols->nspecies;
	mols=sim->mols;
	spname=sim->mols->spname;
	vflag=strchr(sim->flags,'v')?1:0;

	for(order=0;order<=2;order++) {										// condition
		rxnss=sim->rxnss[order];
		if(rxnss) {
			if(rxnss->condition!=SCok) {
				warn++;
				simLog(sim,5," WARNING: order %i reaction structure %s is not up to date\n",order,simsc2string(rxnss->condition,string)); }}}

	for(order=0;order<=2;order++) {										// maxspecies
		rxnss=sim->rxnss[order];
		if(rxnss) {
			if(!sim->mols) {error++;simLog(sim,10," SMOLDYN BUG: Reactions are declared, but not molecules\n");}
			else if(sim->mols->maxspecies!=rxnss->maxspecies) {error++;simLog(sim,10," SMOLDYN BUG: number of molecule species differ between mols and rxnss\n");} }}

	for(order=0;order<=2;order++) {										// look for bugs in reactions
		rxnss=sim->rxnss[order];
		if(rxnss)
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->rxnss!=rxnss) {error++;simLog(sim,10," SMOLDYN BUG: Reaction %s does not list its superstructure correctly.\n",rxn->rname);} }}

	prdparamflag=0;
	for(order=1;order<=2 && !prdparamflag;order++) {			// reversible parameters
		rxnss=sim->rxnss[order];
		if(rxnss)
			for(r=0;r<rxnss->totrxn && !prdparamflag;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->rparamt==RPpgemmaxw) {
					if(!vflag) {
						prdparamflag=1;
						simLog(sim,5," WARNING: unspecified product parameter for at least one reversible reaction.  Defaults are used.\n");
						warn++; }
					else {
						simLog(sim,5," WARNING: unspecified product parameter for reversible reaction %s.  Defaults are used.\n",rxn->rname);
						warn++; }}}}

	rxnss=sim->rxnss[2];															// check for multiple bimolecular reactions with same reactants
	if(rxnss) {
		for(i1=1;i1<nspecies;i1++)
			for(i2=1;i2<=i1;i2++)	{
				i=i1*rxnss->maxspecies+i2;
				for(j1=0;j1<rxnss->nrxn[i];j1++) {
					rxn1=rxnss->rxn[rxnss->table[i][j1]];
					for(j2=0;j2<j1;j2++) {
						rxn2=rxnss->rxn[rxnss->table[i][j2]];
						if(rxnallstates(rxn1) && rxnallstates(rxn2)) {
							simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(all) + %s(all)\n",spname[i1],spname[i2]);
							warn++; }
						else if(rxnallstates(rxn1)) {
							for(ms2=(MolecState)0;ms2<=MSMAX1;ms2=(MolecState)(ms2+1)) {
								ms=(MolecState)(MSsoln*MSMAX1+ms2);
								if(rxn2->permit[ms]) {
									simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(all) + %s(%s)\n",spname[i1],spname[i2],molms2string(ms2,string2));
									warn++; }}}
						else if(rxnallstates(rxn2)) {
							for(ms1=(MolecState)0;ms1<=MSMAX1;ms1=(MolecState)(ms1+1)) {
								ms=(MolecState)(ms1*MSMAX1+MSsoln);
								if(rxn1->permit[ms]) {
									simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(%s) + %s(all)\n",spname[i1],molms2string(ms1,string),spname[i2]);
									warn++; }}}
						else {
							for(ms1=(MolecState)0;ms1<MSMAX1;ms1=(MolecState)(ms1+1))
								for(ms2=(MolecState)0;ms2<=ms1;ms2=(MolecState)(ms2+1)) {
									ms=(MolecState)(ms1*MSMAX1+ms2);
									if(rxn1->permit[ms] && rxn2->permit[ms]) {
										simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(%s) + %s(%s)\n",spname[i1],molms2string(ms1,string),spname[i2],molms2string(ms2,string2));
										warn++; }}}}}}}

	rxnss=sim->rxnss[2];														// warn that difm ignored for reaction rates
	if(rxnss)
		for(i1=1;i1<nspecies;i1++)
			if(mols->difm[i1][MSsoln])
				for(i2=1;i2<i1;i2++)
					for(j=0;j<rxnss->nrxn[i=i1*rxnss->maxspecies+i2];j++) {
						rxn=rxnss->rxn[rxnss->table[i][j]];
						if(rxn->rate) {
							simLog(sim,5," WARNING: diffusion matrix for %s was ignored for calculating rate for reaction %s\n",spname[i1],rxn->rname);
							warn++; }}

	rxnss=sim->rxnss[2];														// warn that drift ignored for reaction rates
	if(rxnss)
		for(i1=1;i1<nspecies;i1++)
			if(mols->drift[i1][MSsoln])
				for(i2=1;i2<i1;i2++)
					for(j=0;j<rxnss->nrxn[i=i1*rxnss->maxspecies+i2];j++) {
						rxn=rxnss->rxn[rxnss->table[i][j]];
						if(rxn->rate) {
							simLog(sim,5," WARNING: drift vector for %s was ignored for calculating rate for reaction %s\n",spname[i1],rxn->rname);
							warn++; }}

	for(order=1;order<=2;order++) {									// product surface-bound states imply reactant surface-bound
		rxnss=sim->rxnss[order];
		if(rxnss) {
			for(i=1;i<intpower(rxnss->maxspecies,order);i++)
				for(j=0;j<rxnss->nrxn[i];j++) {
					rxn=rxnss->rxn[rxnss->table[i][j]];
					if(rxn->permit[order==1?MSsoln:MSsoln*MSMAX1+MSsoln]) {
						for(prd=0;prd<rxn->nprod;prd++)
							if(rxn->prdstate[prd]!=MSsoln) {
								simLog(sim,10," ERROR: a product of reaction %s is surface-bound, but no reactant is\n",rxn->rname);
								error++; }}}}}

	for(order=0;order<=2;order++) {									// reaction compartment
		rxnss=sim->rxnss[order];
		if(rxnss)
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->cmpt) {
					if(order==0 && rxn->cmpt->volume<=0) {
						simLog(sim,10," ERROR: reaction %s cannot work in compartment %s because the compartment has no volume\n",rxn->rname,rxn->cmpt->cname);
						error++; }}}}

	for(order=0;order<=2;order++) {									// reaction surface
		rxnss=sim->rxnss[order];
		if(rxnss)
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->srf) {
					if(order==0 && surfacearea(rxn->srf,sim->dim,NULL)<=0) {
						simLog(sim,10," ERROR: reaction %s cannot work on surface %s because the surface has no area\n",rxn->rname,rxn->srf->sname);
						error++; }}}}

	for(order=0;order<=2;order++) {									// reaction serial numbers
		rxnss=sim->rxnss[order];
		if(rxnss)
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->prdserno) {
					j=0;
					for(prd=0;prd<rxn->nprod;prd++) {
						pserno=rxn->prdserno[prd];
						bitcode=~pserno;
						if(pserno>0) j=1;
						else if((bitcode&0x8) || (bitcode&0x800)) j=1; }
					if(j) {
						simLog(sim,5," WARNING: multiple molecules might have the same serial number due to reaction %s\n",rxn->rname);
						warn++; }}}}

	rxnss=sim->rxnss[0];														// order 0 reactions
	if(rxnss)
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(!rxn->srf) {
				for(prd=0;prd<rxn->nprod;prd++)
					if(rxn->prdstate[prd]!=MSsoln) {
						simLog(sim,10," ERROR: order 0 reaction %s has surface-bound products but no surface listed\n",rxn->rname);
						error++; }}
			if(rxn->prob<0) {
				simLog(sim,5," WARNING: reaction rate not set for reaction order 0, name %s\n",rxn->rname);
				rxn->prob=0;
				warn++; }}

	rxnss=sim->rxnss[1];														// order 1 reactions
	if(rxnss)
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(rxn->prob<0) {
				simLog(sim,5," WARNING: reaction rate not set for reaction order 1, name %s\n",rxn->rname);
				rxn->prob=0;
				warn++; }
			else if(rxn->prob>0 && rxn->prob<10.0/(double)RAND2_MAX) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is at lower end of random number generator resolution\n",rxn->rname);
				warn++; }
			else if(rxn->prob>((double)RAND2_MAX-10.0)/(double)RAND2_MAX && rxn->prob<1.0) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is at upper end of random number generator resolution\n",rxn->rname);
				warn++; }
			else if(rxn->prob>0.2) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is quite high\n",rxn->rname);
				warn++; }
			if(rxn->tau<5*sim->dt) {
				simLog(sim,5," WARNING: order 1 reaction %s time constant is only %g times longer than the simulation time step\n",rxn->rname,rxn->tau/sim->dt);
				warn++; }}

	minboxsize=sim->boxs->size[0];
	for(d=1;d<dim;d++)
		if(sim->boxs->size[d]<minboxsize) minboxsize=sim->boxs->size[d];

	rxnss=sim->rxnss[2];															// order 2 reactions
	if(rxnss) {
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(rxn->bindrad2<0) {
				if(rxn->rparamt==RPconfspread) simLog(sim,5," WARNING: confspread radius not set for order 2 reaction %s\n",rxn->rname);
				else simLog(sim,5," WARNING: reaction rate not set for reaction order 2, name %s\n",rxn->rname);
				rxn->bindrad2=0;
				warn++; }
			else if(sqrt(rxn->bindrad2)>minboxsize) {
				if(rxn->rparamt==RPconfspread) simLog(sim,5," WARNING: confspread radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				else if(rxn->rparamt==RPbounce) simLog(sim,5," WARNING: bounce radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				else simLog(sim,5," WARNING: binding radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				warn++; }
			if(rxn->prob<0 || rxn->prob>1) {
				simLog(sim,10," ERROR: reaction %s probability is not between 0 and 1\n",rxn->rname);
				error++; }
			if(rxn->tau<5*sim->dt) {
				simLog(sim,5," WARNING: order 2 reaction %s time constant is only %g times longer than the simulation time step\n",rxn->rname,rxn->tau/sim->dt);
				warn++; }
			if(rxn->chi>0) {
				rxnreactantstate(rxn,statelist,1);
				dsum=MolCalcDifcSum(sim,rxn->rctident[0],statelist[0],rxn->rctident[1],statelist[1]);
				actualrate=rxncalcrate(sim,2,r,NULL);
				if(rxn->rctident[0]==rxn->rctident[1]) actualrate*=2;
				rev=findreverserxn(sim,2,r,&o2,&r2);
				revrxn=rev?sim->rxnss[o2]->rxn[r2]:NULL;
				revunbindrad=revrxn?revrxn->unbindrad:-1;
				smolmodelrate=modelrxnrate(sqrt(rxn->bindrad2),revunbindrad,dsum,1);
				chi=actualrate/smolmodelrate;
				if(chi<0.98*rxn->chi) {
					warn++;
					simLog(sim,5," WARNING: order 2 reaction %s cannot achieve requested chi of %g but is only %g\n",rxn->rname,rxn->chi,chi); }}}}


	rxnss=sim->rxnss[2];															// more order 2 reactions
	if(rxnss) {
		vol=systemvolume(sim);
		vol2=0;
		for(i=1;i<nspecies;i++) {
			amax=0;
			for(i1=1;i1<nspecies;i1++)
				for(j=0;j<rxnss->nrxn[i*rxnss->maxspecies+i1];j++) {
					r=rxnss->table[i*rxnss->maxspecies+i1][j];
					rxn=rxnss->rxn[r];
					if(amax<sqrt(rxn->bindrad2)) amax=sqrt(rxn->bindrad2); }
			ct=molcount(sim,i,NULL,MSsoln,-1);
			vol3=ct*4.0/3.0*PI*amax*amax*amax;
			vol2+=vol3;
			if(vol3>vol/10.0) {
				simLog(sim,5," WARNING: reactive volume of %s is %g %% of total volume\n",spname[i],vol3/vol*100);
				warn++; }}
		if(vol2>vol/10.0) {
			simLog(sim,5," WARNING: total reactive volume is a large fraction of total volume\n");
			warn++; }}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/*************************** parameter calculations ***************************/
/******************************************************************************/


/* rxnsetrate */
int rxnsetrate(simptr sim,int order,int r,char *erstr) {
	rxnssptr rxnss;
	int i,j,i1,i2,rev,o2,r2,permit;
	rxnptr rxn,rxn2,revrxn;
	double vol,rate3,dsum,rparam,unbindrad,prob,product,sum;
	enum MolecState ms,ms1,ms2,statelist[MAXORDER];
	enum RevParam rparamt;

	rxnss=sim->rxnss[order];
	rxn=rxnss->rxn[r];

	if(rxn->disable) {															// disabled reaction
		rxn->prob=0; }

	else if(rxn->rparamt==RPconfspread) {						// confspread
		if(rxn->rate<0) {snprintf(erstr,STRCHAR*sizeof(erstr),"reaction %s rate is undefined",rxn->rname);return 1;}
		if(rxn->rate>=0) rxn->prob=1.0-exp(-sim->dt*rxn->rate*rxn->multiplicity); }

	else if(order==0) {															// order 0
		if(rxn->rate<0) {snprintf(erstr,STRCHAR*sizeof(erstr),"reaction %s rate is undefined",rxn->rname);return 1;}
		if(rxn->cmpt) vol=rxn->cmpt->volume;
		else if(rxn->srf) vol=surfacearea(rxn->srf,sim->dim,NULL);
		else vol=systemvolume(sim);
		rxn->prob=rxn->rate*rxn->multiplicity*sim->dt*vol; }

	else if(order==1) {															// order 1
		for(ms=(MolecState)0;ms<MSMAX && !rxn->permit[ms];ms=(MolecState)(ms+1));		// determine first state that this is permitted for (usually only state)
		if(rxn->rate<0) {snprintf(erstr,STRCHAR*sizeof(erstr),"reaction %s rate is undefined",rxn->rname);return 1;}
		else if(rxn->rate==0) rxn->prob=0;
		else if(ms==MSMAX) rxn->prob=0;
		else {
			i=rxn->rctident[0];
			sum=0;
			for(j=0;j<rxnss->nrxn[i];j++) {
				rxn2=rxnss->rxn[rxnss->table[i][j]];
				if(rxn2->permit[ms] && rxn2->rate>0) {
					for(ms1=(MolecState)0;ms1<MSMAX;ms1=(MolecState)(ms1+1))
						if(rxn->permit[ms1]!=rxn2->permit[ms1]) {snprintf(erstr, STRCHAR*sizeof(erstr),"reactions %s and %s have the same reactant but different state permissions, which is not allowed",rxn->rname,rxn2->rname);return 2;}
					sum+=rxn2->rate*rxn2->multiplicity; }}
			rxn->prob=rxn->rate*rxn->multiplicity/sum*(1.0-exp(-sum*sim->dt));				// desired reaction probability

			product=1.0;
			for(j=0;j<rxnss->nrxn[i];j++) {		// increase reaction probability to account for prior reactions in the list
				rxn2=rxnss->rxn[rxnss->table[i][j]];
				if(rxn2==rxn) break;
				if(rxn2->permit[ms]) {
					prob=rxn2->prob*product;
					product*=1.0-prob; }}
			rxn->prob/=product;								// probability, accounting for prior reactions, see documentation
			if(!(rxn->prob>=0 && rxn->prob<=1)) {snprintf(erstr,STRCHAR*sizeof(erstr),"reaction %s probability is %g, which is out of range",rxn->rname,rxn->prob);return 5;}}

			rev=findreverserxn(sim,1,r,&o2,&r2);	// set own reversible parameter if needed
			if(rev>0 && o2==2) {
				if(rxn->rparamt==RPnone) {
					rxn->rparamt=RPpgemmaxw;
					rxn->rparam=0.2; }}}

	else if(order==2) {															// order 2
		if(rxn->rate<0) {
			if(rxn->prob<0) rxn->prob=1;
			snprintf(erstr,STRCHAR*sizeof(erstr),"reaction rate %s is undefined",rxn->rname);
			return 1; }
		i1=rxn->rctident[0];
		i2=rxn->rctident[1];
		permit=rxnreactantstate(rxn,statelist,1);
		ms1=statelist[0];
		ms2=statelist[1];

		rate3=rxn->rate*rxn->multiplicity;
		if(i1==i2) rate3*=2;				// same reactants
		if((ms1==MSsoln && ms2!=MSsoln) || (ms1!=MSsoln && ms2==MSsoln)) rate3*=2;	// one surface, one solution
		dsum=MolCalcDifcSum(sim,i1,ms1,i2,ms2);
		rev=findreverserxn(sim,2,r,&o2,&r2);		// rev is 0 for irreversible, 1 for direct reverse reaction, 2 for continuation reaction

		if(rev>0 && o2==2) {						// set own reversible parameter if needed
			if(rxn->rparamt==RPnone) {
				rxn->rparamt=RPpgemmaxw;
				rxn->rparam=0.2; }}

		if(rev==1) {										// set incoming reversible parameter if needed, and then use it
			revrxn=sim->rxnss[o2]->rxn[r2];
			if(revrxn->rparamt==RPnone) {
				revrxn->rparamt=RPpgemmaxw;
				revrxn->rparam=0.2; }
			rparamt=revrxn->rparamt;
			rparam=revrxn->rparam; }
		else {
			rparamt=RPnone;
			rparam=0; }

		if(rxn->prob<0) rxn->prob=1;
		if(!permit) rxn->bindrad2=0;
		else if(rate3<=0) rxn->bindrad2=0;
		else if(dsum<=0) {snprintf(erstr,STRCHAR*sizeof(erstr),"Both diffusion coefficients are 0");return 4;}
		else if(rparamt==RPunbindrad) rxn->bindrad2=bindingradiusprob(rate3,sim->dt,dsum,rparam,0,rxn->chi,&rxn->prob);
		else if(rparamt==RPratio) rxn->bindrad2=bindingradiusprob(rate3,sim->dt,dsum,rparam,1,rxn->chi,&rxn->prob);
		else if(rparamt==RPpgem) rxn->bindrad2=bindingradiusprob(rate3*(1.0-rparam),sim->dt,dsum,-1,0,rxn->chi,&rxn->prob);
		else if(rparamt==RPpgemmax || rparamt==RPpgemmaxw) {
			rxn->bindrad2=bindingradiusprob(rate3,sim->dt,dsum,0,0,rxn->chi,&rxn->prob);
			unbindrad=unbindingradius(rparam,sim->dt,dsum,rxn->bindrad2);
			if(unbindrad>0) rxn->bindrad2=bindingradiusprob(rate3*(1.0-rparam),sim->dt,dsum,-1,0,rxn->chi,&rxn->prob); }
		else rxn->bindrad2=bindingradiusprob(rate3,sim->dt,dsum,-1,0,rxn->chi,&rxn->prob);					// irreversible or assumed irreversible

		rxn->bindrad2*=rxn->bindrad2; }

	return 0; }


/* rxnsetrates */
int rxnsetrates(simptr sim,int order,char *erstr) {
	rxnssptr rxnss;
	int r,er;

	rxnss=sim->rxnss[order];
	if(!rxnss) return -1;

	for(r=0;r<rxnss->totrxn;r++) {
		er=rxnsetrate(sim,order,r,erstr);
		if(er>1) return r; }		// error code of 1 is just a warning, not an error

	return -1; }


/* rxnsetproduct */
int rxnsetproduct(simptr sim,int order,int r,char *erstr) {
	rxnssptr rxnss;
	rxnptr rxn,rxnr;
	int er,dim,nprod,orderr,rr,rev,d,prd;
	double rpar,dc1,dc2,dsum,bindradr,dist;
	enum RevParam rparamt;
	enum MolecState ms1,ms2;

	rxnss=sim->rxnss[order];
	rxn=rxnss->rxn[r];
	nprod=rxn->nprod;
	rpar=rxn->rparam;
	rparamt=rxn->rparamt;
	er=0;
	dim=sim->dim;

    if(nprod==0) {  // nprod==0
		if(rparamt==RPnone || rparamt==RPirrev || rparamt==RPconfspread)
			rxn->unbindrad=0;
		else {
			snprintf(erstr,STRCHAR*sizeof(erstr),"Illegal product parameter because reaction has no products");
			er=1; }}

	else if(rparamt==RPoffset || rparamt==RPfixed) {	// nprod>0, offset and fixed
		dist=0;
		if(nprod==1)
			for(d=0;d<dim;d++) dist=rxn->prdpos[0][d];
		else
			for(d=0;d<dim;d++) dist+=rxn->prdpos[0][d]-rxn->prdpos[1][d];
		rxn->unbindrad=sqrt(dist); }

	else if(nprod==1) {    // nprod==1, all others (i.e. not offset or fixed)
		if(rparamt==RPnone || rparamt==RPirrev || rparamt==RPconfspread) {
			rxn->unbindrad=0;						// only 1 product so no unbinding radius
			for(d=0;d<dim;d++) rxn->prdpos[0][d]=0; }
		else {
			snprintf(erstr,STRCHAR*sizeof(erstr),"Illegal product parameter because reaction only has one product");
			er=2; }}

	else if(rparamt==RPbounce) {										// nprod>=2, bounce
		ms1=rxn->rctstate[0];
		ms2=rxn->rctstate[1];
		if(ms1==MSbsoln || ms1==MSnone || ms1==MSall || ms1==MSsome) ms1=MSsoln;
		if(ms2==MSbsoln || ms2==MSnone || ms2==MSall || ms2==MSsome) ms2=MSsoln;
		dc1=MolCalcDifcSum(sim,rxn->rctident[0],ms1,0,MSnone);
		dc2=MolCalcDifcSum(sim,rxn->rctident[1],ms2,0,MSnone);
		dsum=dc1+dc2;
		if(dsum==0)
			dc1=dc2=0.5;
		else {
			dc1/=dsum;				// dc1 and dc2 are diffusion coefficients relative to dsum
			dc2/=dsum; }
		if(rpar>=0) {
			rxn->unbindrad=rpar;
			rxn->prdpos[0][0]=rpar*dc1;
			rxn->prdpos[1][0]=rpar*dc2;
			for(prd=2;prd<nprod;prd++)
				rxn->prdpos[prd][0]=0; }
		else if(rpar<0) {
			rxn->unbindrad=rpar;
			rxn->prdpos[0][0]=sqrt(rxn->bindrad2)*dc1;
			rxn->prdpos[1][0]=sqrt(rxn->bindrad2)*dc2;
			for(prd=2;prd<nprod;prd++)
				rxn->prdpos[prd][0]=0; }}

	else if(nprod>=2) {																// nprod>=2, not offset or fixed
		ms1=rxn->prdstate[0];
		ms2=rxn->prdstate[1];
		if(ms1==MSbsoln) ms1=MSsoln;
		if(ms2==MSbsoln) ms2=MSsoln;
		dc1=MolCalcDifcSum(sim,rxn->prdident[0],ms1,0,MSnone);
		dc2=MolCalcDifcSum(sim,rxn->prdident[1],ms2,0,MSnone);
		dsum=dc1+dc2;
		if(dsum==0)
			dc1=dc2=0.5;
		else {
			dc1/=dsum;				// dc1 and dc2 are diffusion coefficients relative to dsum
			dc2/=dsum; }
		rev=findreverserxn(sim,order,r,&orderr,&rr);

		if(rparamt==RPirrev || rparamt==RPconfspread) {	// nprod>=2, irrev or confspread
			rxn->unbindrad=0;
			for(prd=0;prd<nprod;prd++)
				for(d=0;d<dim;d++)
					rxn->prdpos[prd][d]=0; }

		else if(rev==0) {																// nprod>=2, irreversible, not offset, fixed, irrev, confspread, or bounce
			if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPratio || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2) {
				snprintf(erstr,STRCHAR*sizeof(erstr),"Illegal product parameter because products don't react");er=3; }
			else if(rparamt==RPunbindrad) {								// nprod>=2, irrev., unbindrad
				rxn->unbindrad=rpar;
				rxn->prdpos[0][0]=rpar*dc1;
				rxn->prdpos[1][0]=-rpar*dc2; }
			else {																				// nprod>=2, irreversible, none
				rxn->unbindrad=0;
				rxn->prdpos[0][0]=0;
				rxn->prdpos[1][0]=0; }}

		else {																					// nprod>=2, reversible, not offset, fixed, irrev, confspread, or bounce
			rxnr=sim->rxnss[orderr]->rxn[rr];
			if(rxnr->bindrad2>=0) bindradr=sqrt(rxnr->bindrad2);
			else bindradr=-1;

			if(rparamt==RPnone) {
				snprintf(erstr,STRCHAR*sizeof(erstr),"BUG: Undefined product placement for reversible reaction");er=5; }
			else if(rparamt==RPunbindrad) {
				rxn->unbindrad=rpar;
				rxn->prdpos[0][0]=rpar*dc1;
				rxn->prdpos[1][0]=-rpar*dc2; }
			else if(rxnr->bindrad2<0) {			// all below options require bindrad2 >= 0
				snprintf(erstr,STRCHAR*sizeof(erstr),"Binding radius of reaction products is undefined");er=6; }
      else if(rxnr->bindrad2==0) {    // set to 0 if bindrad2 == 0
        rxn->unbindrad=0;
        rxn->prdpos[0][0]=0;
        rxn->prdpos[1][0]=0; }
			else if(rparamt==RPratio || rparamt==RPratio2) {
				rxn->unbindrad=rpar*bindradr;
				rxn->prdpos[0][0]=rpar*bindradr*dc1;
				rxn->prdpos[1][0]=-rpar*bindradr*dc2; }
			else if(dsum<=0) {						// all below options require dsum > 0
				snprintf(erstr,STRCHAR*sizeof(erstr),"Cannot set unbinding distance because sum of product diffusion constants is 0");er=4; }
			else if(rparamt==RPpgem || rparamt==RPpgem2) {
				rpar=unbindingradius(rpar,sim->dt,dsum,bindradr);
				if(rpar==-2) {
					snprintf(erstr,STRCHAR*sizeof(erstr),"Cannot create an unbinding radius due to illegal input values");er=7; }
				else if(rpar<0) {
					snprintf(erstr,STRCHAR*sizeof(erstr),"Maximum possible geminate binding probability is %g",-rpar);er=8; }
				else {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1;
					rxn->prdpos[1][0]=-rpar*dc2; }}
			else if(rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPpgemmax2) {
				rpar=unbindingradius(rpar,sim->dt,dsum,bindradr);
				if(rpar==-2) {
					snprintf(erstr,STRCHAR*sizeof(erstr),"Illegal input values");er=9; }
				else if(rpar<=0) {
					rxn->unbindrad=0; }
				else if(rpar>0) {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1;
					rxn->prdpos[1][0]=-rpar*dc2; }}
			else {
				simLog(sim,10,"BUG in rxnsetproduct");er=10; }}}		

	return er; }


/* rxnsetproducts */
int rxnsetproducts(simptr sim,int order,char *erstr) {
	rxnssptr rxnss;
	int r,er;

	rxnss=sim->rxnss[order];
	if(!rxnss) return -1;
	for(r=0;r<rxnss->totrxn;r++) {
		er=rxnsetproduct(sim,order,r,erstr);
		if(er) return r; }
	return -1; }


/* rxncalcrate */
double rxncalcrate(simptr sim,int order,int r,double *pgemptr) {
	rxnssptr rxnss;
	double ans,vol;
	int i1,i2,i,j,r2,rev,o2,permit;
	double step,a,bval,product,probthisrxn,prob,sum,ratesum;
	rxnptr rxn,rxnr,rxn2;
	enum MolecState ms1,ms2,statelist[MAXORDER];
	enum RevParam rparamt;

	if(!sim) return -1;
	rxnss=sim->rxnss[order];
	if(!rxnss || r<0 || r>=rxnss->totrxn) return -1;
	rxn=rxnss->rxn[r];

	if(order==0) {																	// order 0
		if(rxn->cmpt) vol=rxn->cmpt->volume;
		else if(rxn->srf) vol=surfacearea(rxn->srf,sim->dim,NULL);
		else vol=systemvolume(sim);
		if(rxn->prob<0) ans=0;
		else ans=rxn->prob/sim->dt/vol; }

	else if(order==1) {															// order 1
		ans=0;
		for(ms1=(MolecState)0;ms1<MSMAX && !rxn->permit[ms1];ms1=(MolecState)(ms1+1));
		if(rxn->prob>0 && ms1!=MSMAX) {
			i1=rxn->rctident[0];

			probthisrxn=0;
			product=1.0;
			sum=0;
			for(j=0;j<rxnss->nrxn[i1];j++) {		// increase reaction probability to account for prior reactions in the list
				rxn2=rxnss->rxn[rxnss->table[i1][j]];
				if(rxn2->permit[ms1] && rxn2->prob>0) {
					prob=rxn2->prob*product;				// prob is probability that rxn2 is chosen
					if(rxn2==rxn) probthisrxn=prob;
					sum+=prob;											// sum is the probability that some reaction is chosen
					product*=1.0-prob; }}						// product is the probability that no reaction is chosen

			ratesum=-log(1.0-sum)/sim->dt;
			ans=ratesum*probthisrxn/sum; }}

	else if(order==2) {															// order 2
		if(rxn->bindrad2<0 || rxn->prob<0) ans=0;
		else {
			i1=rxn->rctident[0];
			i2=rxn->rctident[1];
			i=rxnpackident(order,rxnss->maxspecies,rxn->rctident);
			for(j=0;j<rxnss->nrxn[i] && rxnss->table[i][j]!=r;j++);
			if(rxnss->table[i][j]!=r) return -1;
			permit=rxnreactantstate(rxn,statelist,1);
			ms1=statelist[0];
			ms2=statelist[1];
			if(!permit) return 0;
			if(rxn->rparamt==RPconfspread) return -log(1.0-rxn->prob)/sim->dt;
			step=sqrt(2.0*MolCalcDifcSum(sim,i1,ms1,i2,ms2)*sim->dt);
			a=sqrt(rxn->bindrad2);
			rev=findreverserxn(sim,order,r,&o2,&r2);
			prob=rxn->prob;
			if(rev==1) {
				rparamt=sim->rxnss[o2]->rxn[r2]->rparamt; }
//				rpar=sim->rxnss[o2]->rxn[r2]->rparam; }				// rpar isn't used in this function
			else {
				rparamt=RPnone; }
//				rpar=0; }
			if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPratio || rparamt==RPunbindrad) {	// list was changed in version 2.56 to agree with Smoldyn User's manual
				rxnr=sim->rxnss[o2]->rxn[r2];
				bval=distanceVVD(rxnr->prdpos[0],rxnr->prdpos[1],sim->dim);
				ans=numrxnrateprob(step,a,bval,prob)/sim->dt; }
			else
				ans=numrxnrateprob(step,a,-1,prob)/sim->dt;
			if(i1==i2) ans/=2.0;
			if(!rxn->permit[MSsoln*MSMAX1+MSsoln]) ans/=2.0; }}

	else ans=0;

	if(pgemptr) {
		if(rxn->nprod!=2 || findreverserxn(sim,order,r,&o2,&r2)==0) *pgemptr=-1;
		else {
			step=sqrt(2.0*MolCalcDifcSum(sim,rxn->prdident[0],rxn->prdstate[0],rxn->prdident[1],rxn->prdstate[1])*sim->dt);
			bval=distanceVVD(rxn->prdpos[0],rxn->prdpos[1],sim->dim);
			rxnr=sim->rxnss[o2]->rxn[r2];
			a=sqrt(rxnr->bindrad2);
			prob=rxnr->prob;
			*pgemptr=1.0-numrxnrateprob(step,a,-1,prob)/numrxnrateprob(step,a,bval,prob); }}	// phi = 1-ki/kr

	return ans; }



/* rxncalctau */
void rxncalctau(simptr sim,int order) {
	rxnssptr rxnss;
	rxnptr rxn;
	int r;
	double rate,vol,conc1,conc2;

	rxnss=sim->rxnss[order];
	if(!rxnss) return;

	if(order==1) {
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			rate=rxncalcrate(sim,1,r,NULL);
			rxn->tau=1.0/rate; }}

	else if(order==2) {
		vol=systemvolume(sim);
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			conc1=(double)molcount(sim,rxn->rctident[0],NULL,MSall,-1)/vol;
			conc2=(double)molcount(sim,rxn->rctident[1],NULL,MSall,-1)/vol;
			rate=rxncalcrate(sim,2,r,NULL);
			if(rxn->rparamt==RPconfspread) rxn->tau=1.0/rate;
			else rxn->tau=(conc1+conc2)/(rate*conc1*conc2); }}

	return; }


/******************************************************************************/
/****************************** structure set up ******************************/
/******************************************************************************/


/* rxnsetcondition */
void rxnsetcondition(simptr sim,int order,enum StructCond cond,int upgrade) {
	int o1,o2;

	if(!sim) return;
	if(order<0) {
		o1=0;
		o2=2; }
	else if(order<=2)
		o1=o2=order;
	else
		return;

	for(order=o1;order<=o2;order++) {
		if(sim->rxnss[order]) {
			if(upgrade==0 && sim->rxnss[order]->condition>cond) sim->rxnss[order]->condition=cond;
			else if(upgrade==1 && sim->rxnss[order]->condition<cond) sim->rxnss[order]->condition=cond;
			else if(upgrade==2) sim->rxnss[order]->condition=cond;
			if(sim->rxnss[order]->condition<sim->condition) {
				cond=sim->rxnss[order]->condition;
				simsetcondition(sim,cond==SCinit?SClists:cond,0); }}}
	
	return; }


/* RxnSetValue */
int RxnSetValue(simptr sim,const char *option,rxnptr rxn,double value) {
	int er;

	er=0;

	if(!strcmp(option,"rate")) {
		if(value<0) er=4;
		rxn->rate=value;
		if(rxn->multiplicity<0)
			rxn->multiplicity=1; }

	else if(!strcmp(option,"multiplicity")) {
		if(value<0) er=4;
		rxn->multiplicity=(int) value; }

	else if(!strcmp(option,"multiplicity++")) {
		rxn->multiplicity++; }

	else if(!strcmp(option,"rateadd")) {
		rxn->rate+=value; }

	else if(!strcmp(option,"confspreadrad")) {
		rxn->rparamt=RPconfspread;
		if(value<0) er=4;
		rxn->bindrad2=value*value; }

	else if(!strcmp(option,"bindrad")) {
		if(value<0) er=4;
		rxn->bindrad2=value*value; }

	else if(!strcmp(option,"prob")) {
		if(value<0) er=4;
		if(rxn->rxnss->order>0 && value>1) er=4;
		rxn->prob=value; }

	else if(!strcmp(option,"chi")) {
		if(value==0 || value>=1) er=4;
		if(rxn->rxnss->order!=2) er=4;
		rxn->chi=value; }

	else if(!strcmp(option,"disable")) {
		rxn->disable=(int) value; }

	else er=2;

	if(sim)
		rxnsetcondition(sim,-1,SClists,0);

	return er; }


/* RxnSetValuePattern */
int RxnSetValuePattern(simptr sim,const char *option,const char *pattern,const char *rname,const enum MolecState *rctstate,const enum MolecState *prdstate,double value,int oldnresults,const rxnptr templ) {
	int er,nresults,matchwords,subwords,totalwords,j,iword,*index;
	int rctident[MAXORDER],prdident[MAXPRODUCT];
	rxnptr rxn;

	er=0;

	er=molpatternindex(sim,pattern,rname,0,0,&index);
	if(er) return er;
	if(!index) return 5;
	nresults=index[PDnresults];
	matchwords=index[PDmatch];
	subwords=index[PDsubst];
	totalwords=matchwords+subwords;

	for(j=oldnresults;j<nresults;j++) {
		for(iword=0;iword<matchwords;iword++)
			rctident[iword]=index[PDMAX+j*totalwords+iword];
		for(iword=0;iword<subwords;iword++)
			prdident[iword]=index[PDMAX+j*totalwords+matchwords+iword];
		rxn=RxnTestRxnExist(sim,matchwords,rname,rctident,rctstate,subwords,prdident,prdstate,0);
		if(!rxn) return 5;
		if(!strcmp(option,"rate")) {
			if(rxn->rate<0)
				RxnSetValue(sim,"rate",rxn,value);
			else if(rxn->rate==value)
				RxnSetValue(sim,"multiplicity++",rxn,0);
			else {
				simLog(sim,7,"SMOLDYN BUG: rates don't agree for reaction %s in RxnSetValuePattern",rxn->rname);
				RxnSetValue(sim,"rateadd",rxn,value); }}
		else
			RxnSetValue(sim,option,rxn,value);

		if(templ) {
			if(templ->rate>=0)
				RxnSetValue(sim,"rate",rxn,templ->rate);
			if(templ->multiplicity>=0)
				RxnSetValue(sim,"multiplicity",rxn,templ->multiplicity);
			if(templ->rparamt==RPconfspread)
				RxnSetValue(sim,"confspreadrad",rxn,sqrt(templ->bindrad2));
			if(templ->bindrad2>=0)
				RxnSetValue(sim,"bindrad",rxn,sqrt(templ->bindrad2));
			if(templ->prob>=0)
				RxnSetValue(sim,"prob",rxn,templ->prob);
			if(templ->prdserno)
				RxnSetPrdSerno(rxn,templ->prdserno);
			if(templ->prdintersurf)
				RxnSetIntersurfaceRules(rxn,templ->prdintersurf);
			if(templ->rctrep)
				RxnSetRepresentationRules(rxn,matchwords,templ->rctrep,NULL);
			if(templ->prdrep)
				RxnSetRepresentationRules(rxn,matchwords,NULL,templ->prdrep);
			if(templ->logserno && templ->logfile)
				RxnSetLog(sim,templ->logfile,rxn,templ->logserno,1);
			if(templ->rparamt!=RPnone)
				RxnCopyRevparam(sim,rxn,templ); }}

	return er; }


/* RxnSetRevparam */
int RxnSetRevparam(simptr sim,rxnptr rxn,enum RevParam rparamt,double rparam,int prd,double *pos,int dim) {
	int d,er;

	er=0;
	if(rxn->rparamt!=RPnone) er=1;
	rxn->rparamt=rparamt;

	if(rparamt==RPnone || rparamt==RPirrev || rparamt==RPconfspread);
	else if(rparamt==RPbounce)
		rxn->rparam=rparam;
	else if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPpgem2 || rparamt==RPpgemmax2) {
		if(!(rparam>0 && rparam<=1)) er=2;
		rxn->rparam=rparam; }
	else if(rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPratio2) {
		if(rparam<0) er=2;
		rxn->rparam=rparam; }
	else if(rparamt==RPoffset || rparamt==RPfixed) {
		er=0;
		if(prd<0 || prd>=rxn->nprod) er=4;
		else if(!pos) er=5;
		else for(d=0;d<dim;d++) rxn->prdpos[prd][d]=pos[d]; }
	else
		er=3;

	if(sim) rxnsetcondition(sim,-1,SCparams,0);

	return er; }


/* RxnCopyRevparam */
void RxnCopyRevparam(simptr sim,rxnptr rxn,const rxnptr templ) {
	int prd,d;

	rxn->rparamt=templ->rparamt;
	rxn->rparam=templ->rparam;
	for(prd=0;prd<rxn->nprod;prd++)
		for(d=0;d<sim->dim;d++)
			rxn->prdpos[prd][d]=templ->prdpos[prd][d];

	rxnsetcondition(sim,-1,SCparams,0);

	return; }


/* RxnSetPermit */
void RxnSetPermit(simptr sim,rxnptr rxn,int order,enum MolecState *rctstate,int value) {
	enum MolecState ms,nms2o,mslist[MSMAX1];
	int set,ord;
	static int recurse;

	if(order==0) return;
	nms2o=(MolecState)intpower(MSMAX1,order);
	for(ms=(MolecState)0;ms<nms2o;ms=(MolecState)(ms+1)) {
		rxnunpackstate(order,ms,mslist);
		set=1;
		for(ord=0;ord<order && set;ord++)
			if(!(rctstate[ord]==MSall || rctstate[ord]==mslist[ord])) set=0;
		if(set) rxn->permit[ms]=value; }

	if(order==2 && rxn->rctident[0]==rxn->rctident[1] && recurse==0) {
		recurse=1;
		mslist[0]=rctstate[1];
		mslist[1]=rctstate[0];
		RxnSetPermit(sim,rxn,order,mslist,value);
		recurse=0; }

	if(sim) {
		rxnsetcondition(sim,-1,SCparams,0);
		surfsetcondition(sim->srfss,SClists,0); }
	return; }


/* RxnSetCmpt */
void RxnSetCmpt(rxnptr rxn,compartptr cmpt) {
	rxn->cmpt=cmpt;
	return; }


/* RxnSetSurface */
void RxnSetSurface(rxnptr rxn,surfaceptr srf) {
	rxn->srf=srf;
	return; }


/* RxnSetPrdSerno */
int RxnSetPrdSerno(rxnptr rxn,long int *prdserno) {
	int prd;

	if(!rxn->prdserno) {
		rxn->prdserno=(long int*) calloc(rxn->nprod,sizeof(long int));
		if(!rxn->prdserno) return 1;
		for(prd=0;prd<rxn->nprod;prd++)
			rxn->prdserno[prd]=0; }

	for(prd=0;prd<rxn->nprod;prd++)
		rxn->prdserno[prd]=prdserno[prd];

	return 0; }


/* RxnSetIntersurfaceRules */
int RxnSetIntersurfaceRules(rxnptr rxn,int *rules) {
	int prd;

	if(!rxn->prdintersurf) {
		rxn->prdintersurf=(int *) calloc(rxn->nprod>0?rxn->nprod:1,sizeof(int));
		if(!rxn->prdintersurf) return 1;
		for(prd=0;prd<rxn->nprod;prd++)
			rxn->prdintersurf[prd]=0; }

	if(rules[0]==-1) {
		free(rxn->prdintersurf);
		rxn->prdintersurf=NULL; }

	else if(rxn->nprod==0)
		rxn->prdintersurf[0]=0;

	else
		for(prd=0;prd<rxn->nprod;prd++)
			rxn->prdintersurf[prd]=rules[prd];

	return 0; }


/* RxnSetRepresentationRules */
int RxnSetRepresentationRules(rxnptr rxn,int order,const enum SpeciesRepresentation *rctrep,const enum SpeciesRepresentation *prdrep) {		// used for lattice/particle hybrid
	int rct,prd,nprod;

	nprod=rxn->nprod;

	if(!rxn->rctrep) {
		rxn->rctrep=(enum SpeciesRepresentation*) calloc(order>0?order:1,sizeof(enum SpeciesRepresentation));
		if(!rxn->rctrep) return 1;
		rxn->rctrep[0]=SRparticle;
		for(rct=1;rct<order;rct++)
			rxn->rctrep[rct]=SRparticle; }
	if(!rxn->prdrep) {
		rxn->prdrep=(enum SpeciesRepresentation*) calloc(nprod>0?nprod:1,sizeof(enum SpeciesRepresentation));
		if(!rxn->prdrep) return 1;
		rxn->prdrep[0]=SRparticle;
		for(prd=1;prd<nprod;prd++)
			rxn->prdrep[prd]=SRparticle; }

	if(rctrep[0]==SRfree) {
		free(rxn->rctrep);
		free(rxn->prdrep);
		rxn->rctrep=NULL;
		rxn->prdrep=NULL;
		return 0; }

	for(rct=0;rct<order;rct++)
		rxn->rctrep[rct]=rctrep[rct];
	for(prd=0;prd<nprod;prd++)
		rxn->prdrep[prd]=prdrep[prd];

	return 0; }


/* RxnSetLog */
int RxnSetLog(simptr sim,char *filename,rxnptr rxn,listptrli list,int turnon) {
	int order,r,er;
	rxnssptr rxnss;

	er=0;
	if(!rxn) {												// all reactions
		for(order=0;order<=2;order++) {
			rxnss=sim->rxnss[order];
			if(rxnss)
				for(r=0;r<rxnss->totrxn;r++) {
					rxn=rxnss->rxn[r];
					er=RxnSetLog(sim,filename,rxn,list,turnon);
					if(er) return er; }}
		return 0; }

	if(turnon) {												// turn on logging
		if(list->n==1 && list->xs[0]==-1) {
			ListFreeLI(rxn->logserno);
			CHECKMEM(rxn->logserno=ListAppendItemLI(NULL,-1)); }
		else {
			CHECKMEM(rxn->logserno=ListAppendListLI(rxn->logserno,list)); }

		if(rxn->logfile && !strcmp(rxn->logfile,filename));
		else if(rxn->logfile) {
			er=2;											// warning that prior logfile name is overwritten
			free(rxn->logfile);
			CHECKMEM(rxn->logfile=StringCopy(filename)); }
		else
			CHECKMEM(rxn->logfile=StringCopy(filename)); }

	else {															// turn off logging
		if(list->n==1 && list->xs[0]==-1) {
			ListFreeLI(rxn->logserno);
			rxn->logserno=NULL;
			free(rxn->logfile);
			rxn->logfile=NULL; }
		else {
			ListRemoveListLI(rxn->logserno,list);
			if(rxn->logserno->n==0) {
				ListFreeLI(rxn->logserno);
				rxn->logserno=NULL;
				free(rxn->logfile);
				rxn->logfile=NULL; }}}

	return er;

 failure:
	return 1; }


/* RxnAddReaction */
rxnptr RxnAddReaction(simptr sim,const char *rname,int order,int *rctident,enum MolecState *rctstate,int nprod,int *prdident,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf) {
	char **newrname,string[STRCHAR];
	rxnptr *newrxn;
	int *newtable,identlist[MAXORDER],vflag;
	rxnssptr rxnss;
	rxnptr rxn;
	int maxrxn,maxspecies,i,j,r,rct,prd,d,k,done,freerxn;

	rxnss=NULL;
	rxn=NULL;
	newrname=NULL;
	newrxn=NULL;
	newtable=NULL;
	maxrxn=0;
	freerxn=1;

	if(!sim->rxnss[order]) {													// allocate reaction superstructure, if needed
		CHECKS(sim->mols,"Cannot add reaction because no molecules defined");
		CHECKMEM(sim->rxnss[order]=rxnssalloc(NULL,order,sim->mols->maxspecies));
		sim->rxnss[order]->sim=sim;
		rxnsetcondition(sim,order,SCinit,0);
		rxnsetcondition(sim,-1,SClists,0); }
	rxnss=sim->rxnss[order];
	maxspecies=rxnss->maxspecies;
	r=stringfind(rxnss->rname,rxnss->totrxn,rname);		// r is reaction index

	if(r>=0) {
		CHECKBUG(rxnss->rxn[r]->nprod==0,"RxnAddReaction cannot be called for a reaction that already has products");
		rxn=rxnss->rxn[r]; }
	else {
		if(rxnss->totrxn==rxnss->maxrxn) {							// make more reaction space in superstructure, if needed
			maxrxn=(rxnss->maxrxn>0)?2*rxnss->maxrxn:2;
			CHECKMEM(newrname=(char**)calloc(maxrxn,sizeof(char*)));
			for(r=0;r<rxnss->maxrxn;r++) newrname[r]=rxnss->rname[r];
			for(r=rxnss->maxrxn;r<maxrxn;r++) newrname[r]=NULL;
			for(r=rxnss->maxrxn;r<maxrxn;r++) CHECK(newrname[r]=EmptyString());
			CHECKMEM(newrxn=(rxnptr*)calloc(maxrxn,sizeof(rxnptr)));
			for(r=0;r<rxnss->maxrxn;r++) newrxn[r]=rxnss->rxn[r];
			for(r=rxnss->maxrxn;r<maxrxn;r++) newrxn[r]=NULL;
			rxnss->maxrxn=maxrxn;
			free(rxnss->rname);
			rxnss->rname=newrname;
			newrname=NULL;
			free(rxnss->rxn);
			rxnss->rxn=newrxn;
			newrxn=NULL; }

		CHECKMEM(rxn=rxnalloc(order));									// create reaction and set up reactants
		rxn->rxnss=rxnss;
		rxn->rname=rxnss->rname[rxnss->totrxn];
		if(order>0) {
			for(rct=0;rct<order;rct++) rxn->rctident[rct]=rctident[rct];
			for(rct=0;rct<order;rct++) rxn->rctstate[rct]=rctstate[rct];
			RxnSetPermit(sim,rxn,order,rctstate,1); }

		if(order>0) {																		// set up nrxn and table; table is automatically enlarged
			k=0;
			done=0;
			while(!done) {
				k=Zn_permute(rctident,identlist,order,k);
				CHECKBUG(k!=-1,"SMOLDYN BUG: Zn_permute.\n");
				if(k==0) done=1;
				i=rxnpackident(order,maxspecies,identlist);
				CHECKMEM(newtable=(int*)calloc(rxnss->nrxn[i]+1,sizeof(int)));
				for(j=0;j<rxnss->nrxn[i];j++) newtable[j]=rxnss->table[i][j];
				newtable[j]=rxnss->totrxn;
				free(rxnss->table[i]);
				rxnss->table[i]=newtable;
				newtable=NULL;
				rxnss->nrxn[i]++; }}

		strncpy(rxnss->rname[rxnss->totrxn],rname,STRCHAR-1);		// plug in reaction
		rxnss->rname[rxnss->totrxn][STRCHAR-1]='\0';
		rxnss->totrxn++;
		rxnss->rxn[rxnss->totrxn-1]=rxn; }
	freerxn=0;

	rxn->nprod=nprod;																					// set up products
	if(nprod) {
		CHECKMEM(rxn->prdident=(int*)calloc(nprod,sizeof(int)));
		for(prd=0;prd<nprod;prd++) rxn->prdident[prd]=prdident[prd];
		CHECKMEM(rxn->prdstate=(enum MolecState*)calloc(nprod,sizeof(enum MolecState)));
		for(prd=0;prd<nprod;prd++) rxn->prdstate[prd]=prdstate[prd];
		CHECKMEM(rxn->prdpos=(double**)calloc(nprod,sizeof(double*)));
		for(prd=0;prd<nprod;prd++) rxn->prdpos[prd]=NULL;
		for(prd=0;prd<nprod;prd++) {
			CHECKMEM(rxn->prdpos[prd]=(double*)calloc(sim->dim,sizeof(double)));
			for(d=0;d<sim->dim;d++) rxn->prdpos[prd][d]=0; }}

	RxnSetCmpt(rxn,cmpt);																						// add reaction compartment
	RxnSetSurface(rxn,srf);																							// add reaction surface
	rxnsetcondition(sim,-1,SClists,0);
	surfsetcondition(sim->srfss,SClists,0);

	vflag=strchr(sim->flags,'v')?1:0;
	if(vflag) {
		simLog(sim,1,"Added reaction %s:",rxn->rname);
		for(rct=0;rct<order;rct++)
			simLog(sim,1," %s(%s) %s",sim->mols->spname[rctident[rct]],molms2string(rctstate[rct],string),rct<order-1?"+":"->");
		for(prd=0;prd<nprod;prd++)
			simLog(sim,1," %s(%s)%s",sim->mols->spname[prdident[prd]],molms2string(prdstate[prd],string),prd<nprod-1?" +":"");
		simLog(sim,1,"\n"); }

	return rxn;

 failure:
	if(!rxnss) return NULL;
	if(newrname) {
		for(r=rxnss->maxrxn;r<maxrxn;r++) free(newrname[r]);
		free(newrname); }
	free(newrxn);
	if(freerxn) rxnfree(rxn);
	if(ErrorType==2) simLog(sim,8,"%s",ErrorString);
	else simLog(sim,10,"%s",ErrorString);
	return NULL; }


/* RxnTestRxnExist */
rxnptr RxnTestRxnExist(simptr sim,int order,const char *rname,const int *rctident,const enum MolecState *rctstate,int nprod,const int *prdident,const enum MolecState *prdstate,int exact) {
	int i,j,maxspecies,work[MAXPRODUCT],r;
	const char *chptr;
	rxnssptr rxnss;
	rxnptr rxn;

	if(!sim->rxnss[order]) return NULL;
	rxnss=sim->rxnss[order];

	if(exact) {																		// exact match so just check name
		r=stringfind(rxnss->rname,rxnss->totrxn,rname);
		if(r>=0) return rxnss->rxn[r];
		else return NULL; }

	maxspecies=rxnss->maxspecies;									// during expansion
	if(order==0) {
		for(j=0;j<rxnss->totrxn;j++) {
			rxn=rxnss->rxn[j];												// rxn is same if same name or name portion and same reactants and same products
			if((chptr=strstr(rxn->rname,rname))) {
				chptr+=strlen(rname);
				if(chptr[0]=='\0' || (chptr[0]=='_' && strisnumber(chptr+1)))
					if(nprod==rxn->nprod)
						if(Zn_sameset(prdident,rxn->prdident,work,nprod) && Zn_sameset((int*)prdstate,(int*)(rxn->prdstate),work,nprod))
							return rxn; }}}
	else {
		i=rxnpackident(order,maxspecies,rctident);
		if(!rxnss->nrxn[i]) return NULL;
		for(j=0;j<rxnss->nrxn[i];j++) {
			rxn=rxnss->rxn[rxnss->table[i][j]];
			if((chptr=strstr(rxn->rname,rname))) {
				chptr+=strlen(rname);
				if(chptr[0]=='\0' || (chptr[0]=='_' && strisnumber(chptr+1)))
					if(nprod==rxn->nprod)
						if(Zn_sameset((int*)rctstate,(int*)(rxn->rctstate),work,order))
							if(Zn_sameset(prdident,rxn->prdident,work,nprod) && Zn_sameset((int*)prdstate,(int*)(rxn->prdstate),work,nprod))
								return rxn; }}}
	return NULL; }


/* RxnAddReactionPattern */
int RxnAddReactionPattern(simptr sim,const char *rname,const char *pattern,int oldnresults,enum MolecState *rctstate,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf,int isrule,rxnptr *rxnpt) {
	int er,*index,iword,nresults,matchwords,subwords,totalwords,j;
	int rctident[MAXORDER],prdident[MAXPRODUCT];
	rxnptr rxn;
	char rnamelocal[STRCHAR];

	if(oldnresults<0) {
		molpatternindex(sim,pattern,rname,isrule,0,&index);			// get oldnresults from index
		if(index) oldnresults=index[PDnresults];
		else oldnresults=0; }

	rxn=NULL;
	er=molpatternindex(sim,pattern,rname,isrule,2,&index);	// update pattern, registering and expanding as needed
	if(er) return er;
	nresults=index[PDnresults];
	matchwords=index[PDmatch];
	subwords=index[PDsubst];
	totalwords=matchwords+subwords;

	for(j=oldnresults;j<nresults;j++) {								// loop over all reactions that needed to be added
		strcpy(rnamelocal,rname);
		if(nresults>1)
			snprintf(rnamelocal+strlen(rnamelocal),STRCHAR-strlen(rnamelocal),"_%i",j);
		for(iword=0;iword<matchwords;iword++)
			rctident[iword]=index[PDMAX+j*totalwords+iword];
		for(iword=0;iword<subwords;iword++)
			prdident[iword]=index[PDMAX+j*totalwords+matchwords+iword];
		rxn=RxnTestRxnExist(sim,matchwords,rname,rctident,rctstate,subwords,prdident,prdstate,0);
		if(!rxn) {
			rxn=RxnAddReaction(sim,rnamelocal,matchwords,rctident,rctstate,subwords,prdident,prdstate,cmpt,srf);
			if(!rxn) return -30; }}

	if(rxnpt && rxn && nresults==1) *rxnpt=rxn;
	else if(rxnpt) *rxnpt=NULL;

	return 0; }


/* RxnAddReactionCheck */
rxnptr RxnAddReactionCheck(simptr sim,char *rname,int order,int *rctident,enum MolecState *rctstate,int nprod,int *prdident,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf) {
	rxnptr rxn;
	int i;

	CHECKBUG(sim,"sim undefined");
	CHECKBUG(sim->mols,"sim is missing molecule superstructure");
	CHECKBUG(rname,"rname is missing");
	CHECKBUG(strlen(rname)<STRCHAR,"rname is too long");
	CHECKBUG(order>=0 && order<=2,"order is out of bounds");
	if(order>0) {
		CHECKBUG(rctident,"rctident is missing"); }
	for(i=0;i<order;i++) {
		CHECKBUG(rctident[i]>0 && rctident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKBUG(rctstate[i]>=0 && rctstate[i]<MSMAX1,"reactant state out of bounds"); }
	CHECKBUG(nprod>=0,"nprod out of bounds");
	for(i=0;i<nprod;i++) {
		CHECKBUG(prdident[i]>0 && prdident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKBUG(prdstate[i]>=0 && prdstate[i]<MSMAX1,"reactant state out of bounds"); }
	if(cmpt) {
		CHECKBUG(sim->cmptss,"sim is missing compartment superstructure"); }
	if(srf)	{
		CHECKBUG(sim->srfss,"sim is missing surface superstructure"); }
	rxn=RxnAddReaction(sim,rname,order,rctident,rctstate,nprod,prdident,prdstate,cmpt,srf);
	return rxn;
failure:
	simLog(sim,10,"%s",ErrorString);
	return NULL; }


/* rxnsupdateparams */
int rxnsupdateparams(simptr sim) {
	int er,order,wflag;
	char errorstr[STRCHAR];
	
	wflag=strchr(sim->flags,'w')?1:0;
	for(order=0;order<MAXORDER;order++)
		if(sim->rxnss[order] && sim->rxnss[order]->condition<=SCparams) {
			er=rxnsetrates(sim,order,errorstr);							// set rates
			if(er>=0) {
				simLog(sim,8,"Error setting rate for reaction order %i, reaction %s\n%s\n",order,sim->rxnss[order]->rname[er],errorstr);
				return 3; }}
	
	for(order=0;order<MAXORDER;order++)
		if(sim->rxnss[order] && sim->rxnss[order]->condition<=SCparams) {
			errorstr[0]='\0';
			er=rxnsetproducts(sim,order,errorstr);						// set products
			if(er>=0) {
				simLog(sim,8,"Error setting products for reaction order %i, reaction %s\n%s\n",order,sim->rxnss[order]->rname[er],errorstr);
				return 3; }
			if(!wflag && strlen(errorstr)) simLog(sim,5,"%s\n",errorstr); }

	for(order=0;order<MAXORDER;order++)									// calculate tau values
		if(sim->rxnss[order] && sim->rxnss[order]->condition<=SCparams)
			rxncalctau(sim,order);
	
	return 0; }


/* rxnsupdatelists */
int rxnsupdatelists(simptr sim,int order) {
	rxnssptr rxnss;
	int maxlist,ll,nl2o,r,i1,i2,ll1,ll2;
	rxnptr rxn;
	enum MolecState ms1,ms2;

	rxnss=sim->rxnss[order];

	if(order==0) return 0;

	if(!sim->mols || sim->mols->condition<SCparams) return 2;

	maxlist=rxnss->maxlist;								// set reaction molecule lists
	if(maxlist!=sim->mols->maxlist) {
		free(rxnss->rxnmollist);
		rxnss->rxnmollist=NULL;
		maxlist=sim->mols->maxlist;
		if(maxlist>0) {
			nl2o=intpower(maxlist,order);
			rxnss->rxnmollist=(int*) calloc(nl2o,sizeof(int));
			CHECKMEM(rxnss->rxnmollist); }
		rxnss->maxlist=maxlist; }

	if(maxlist>0) {
		nl2o=intpower(maxlist,order);
		for(ll=0;ll<nl2o;ll++) rxnss->rxnmollist[ll]=0;

		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			i1=rxn->rctident[0];
			if(order==1) {
				for(ms1=(MolecState)0;ms1<MSMAX1;ms1=(MolecState)(ms1+1)) {
#ifdef OPTION_VCELL
					if(rxn->permit[ms1] && (rxn->prob>0 || rxn->rate>0 || rxn->rateValueProvider != NULL))
#else
					if(rxn->permit[ms1] && (rxn->prob>0 || rxn->rate>0))
#endif
						{
							ll1=sim->mols->listlookup[i1][ms1];
							rxnss->rxnmollist[ll1]=1; }}}

			else if(order==2) {
				i2=rxn->rctident[1];
				for(ms1=(MolecState)0;ms1<MSMAX1;ms1=(MolecState)(ms1+1))
					for(ms2=(MolecState)0;ms2<MSMAX1;ms2=(MolecState)(ms2+1)) {

#ifdef OPTION_VCELL
						if(rxn->permit[ms1*MSMAX1+ms2] && rxn->prob!=0 && (rxn->rate>0 || rxn->bindrad2>0 || rxn->rateValueProvider != NULL))
#else
						if(rxn->permit[ms1*MSMAX1+ms2] && rxn->prob!=0 && (rxn->rate>0 || rxn->bindrad2>0))
#endif
							{
								ll1=sim->mols->listlookup[i1][ms1==MSbsoln?MSsoln:ms1];
								ll2=sim->mols->listlookup[i2][ms2==MSbsoln?MSsoln:ms2];
								rxnss->rxnmollist[ll1*maxlist+ll2]=1;
								rxnss->rxnmollist[ll2*maxlist+ll1]=1; }}}}}

	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in rxnsupdatelists");
	return 1; }


/* rxnsupdate */
int rxnsupdate(simptr sim) {
	int er,order,doparams;

	for(order=0;order<MAXORDER;order++) {
		if(sim->rxnss[order] && sim->rxnss[order]->condition<=SClists) {
			er=rxnsupdatelists(sim,order);
			if(er) return er;
			rxnsetcondition(sim,order,SCparams,1); }}

	doparams=0;
	for(order=0;order<MAXORDER;order++)
		if(sim->rxnss[order] && sim->rxnss[order]->condition<SCok) doparams=1;
	if(doparams) {
		er=rxnsupdateparams(sim);
		if(er) return er;
		rxnsetcondition(sim,-1,SCok,1); }

	return 0; }



/******************************************************************************/
/************************** reaction parsing function *************************/
/******************************************************************************/


/* rxnparsereaction */
int rxnparsereaction(simptr sim,const char *word,char *line2,char *errstr) {
	compartptr cmpt;
	surfaceptr srf;
	int isrule,itct,c,s,order,more,er,nprod,bidirect,detailsi[2];
	char nm[STRCHAR],nm1[STRCHAR],rname[STRCHAR],rnamerev[STRCHAR],*chptr,pattern[STRCHAR],patternrev[STRCHAR];
	enum MolecState ms,rctstate[MAXORDER],prdstate[MAXPRODUCT];
	double flt1,flt2;
	rxnptr rxn,rxnrev;

	int nvar;
	char **varnames;
	double *varvalues;

	nvar=sim->nvar;
	varnames=sim->varnames;
	varvalues=sim->varvalues;

	cmpt=NULL;
	srf=NULL;
	c=s=-1;
	isrule=0;
	bidirect=0;
	if(!strcmp(word,"reaction_rule")) {						// reaction_rule
		isrule=1; }
	else if(!strcmp(word,"reaction_cmpt")) {			// reaction_cmpt: deprecated but still supported
		simLog(sim,2,"Warning: reaction_cmpt is a deprecated statement\n");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"failed to read reaction compartment");
		CHECKS(sim->cmptss,"no compartments defined");
		c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
		CHECKS(c>=0,"compartment name not recognized");
		cmpt=sim->cmptss->cmptlist[c];
		line2=strnword(line2,2);
		CHECKS(line2,"missing reaction name"); }
	else if(!strcmp(word,"reaction_surface")) {		// reaction_surface: deprecated but still supported
		simLog(sim,2,"Warning: reaction_surface is a deprecated statement\n");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"failed to read reaction surface");
		CHECKS(sim->srfss,"no surfaces defined");
		s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
		CHECKS(s>=0,"surface %s not recognized",nm);
		srf=sim->srfss->srflist[s];
		line2=strnword(line2,2);
		CHECKS(line2,"missing reaction name"); }

	itct=sscanf(line2,"%s",rname);				// reaction name, or maybe compartment or surface restriction
	CHECKS(itct==1,"failed to read first parameter");
	while((chptr=strchr(rname,'='))) {
		if(!strncmp(rname,"cmpt",chptr-rname) || !strncmp(rname,"compartment",chptr-rname)) {
			itct=sscanf(chptr+1,"%s",nm);
			CHECKS(itct==1,"failed to react compartment name");
			CHECKS(sim->cmptss,"no compartments defined");
			c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
			CHECKS(c>=0,"compartment name not recognized");
			cmpt=sim->cmptss->cmptlist[c];
			line2=strnword(line2,2);
			CHECKS(line2,"missing reaction name");
			itct=sscanf(line2,"%s",rname);				// reaction name, or maybe compartment or surface restriction
			CHECKS(itct==1,"failed to read reaction parameters"); }
		else if(!strncmp(rname,"srf",chptr-rname) || !strncmp(rname,"surface",chptr-rname)) {
			itct=sscanf(chptr+1,"%s",nm);
			CHECKS(itct==1,"failed to react surface name");
			CHECKS(sim->srfss,"no surfaces defined");
			s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
			CHECKS(s>=0,"surface %s not recognized",nm);
			srf=sim->srfss->srflist[s];
			line2=strnword(line2,2);
			CHECKS(line2,"missing reaction name");
			itct=sscanf(line2,"%s",rname);				// reaction name, or maybe compartment or surface restriction
			CHECKS(itct==1,"failed to read reaction parameters"); }}

	for(order=0;order<MAXORDER;order++)				// check for whether reaction name is new
		if(sim->rxnss[order]) {
			CHECKS(stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,rname)<0,"reaction name has already been used"); }
	CHECKS(line2=strnword(line2,2),"missing first reactant");

	order=0;
	more=1;
	pattern[0]='\0';
	while(more) {			// reactant list
		CHECKS((itct=sscanf(line2,"%s %s",nm,nm1))==2,"failed to read reactant");
		CHECKS(strcmp(nm,"->"),"missing reaction name");
		CHECKS(strcmp(nm,"<->"),"missing reaction name");
		CHECKS(strcmp(nm,"+"),"missing reaction name");
		if(strcmp(nm,"0")) {
			CHECKS(order+1<MAXORDER,"exceeded allowed reaction order");
			er=molstring2pattern(nm,&ms,pattern,1);
			CHECKS(er!=-1,"BUG: reading reaction");
			CHECKS(er!=-2,"mismatched or improper parentheses around reactant state");
			CHECKS(er!=-3,"molecule state could not be read");
			CHECKS(er!=-4,"reactant list exceeds maximum string length");
			CHECKS(ms<MSMAX1 || ms==MSall,"invalid state");
			rctstate[order]=ms;
			order++; }
		if(!strcmp(nm1,"->")) {bidirect=0;more=0;}
		else if(!strcmp(nm1,"<->")) {bidirect=1;more=0;}
		else CHECKS(!strcmp(nm1,"+"),"illegal symbol in reactant list");
		CHECKS(line2=strnword(line2,3),"missing information in reaction"); }

	nprod=0;
	more=1;
	while(more) {			// product list
		CHECKS((itct=sscanf(line2,"%s %s",nm,nm1))>=1,"failed to read product");
		if(strcmp(nm,"0")) {
			CHECKS(size_t(nprod+1)<MAXPRODUCT,"exceeded allowed number of reaction products");
			er=molstring2pattern(nm,&ms,pattern,2);
			CHECKS(er!=-1,"BUG: reading reaction");
			CHECKS(er!=-2,"mismatched or improper parentheses around reactant state");
			CHECKS(er!=-3,"molecule state could not be read");
			CHECKS(er!=-4,"reactant and product list together exceed maximum string length");
			CHECKS(ms<MSMAX1,"invalid product state");
			CHECKS(!(order==0 && srf==NULL && ms!=MSsoln),"enter a surface for order 0 reactions with surface-bound products");
			prdstate[nprod]=ms;
			nprod++; }
		else if(nprod==0) {
			er=molstring2pattern(NULL,&ms,pattern,2);
			CHECKS(!er,"BUG in simreadstring"); }
		if(itct==1) {
			more=0;
			line2=NULL; }
		else if(strcmp(nm1,"+")) {
			more=0;
			CHECKS(line2=strnword(line2,2),"SMOLDYN BUG: error in reading product list"); }
		else
			CHECKS(line2=strnword(line2,3),"incomplete product list"); }
	CHECKS(!(order==0 && nprod==0),"reaction has neither reactants nor products");

	if(bidirect) {
		strcpy(rnamerev,rname);
		strcat(rname,"fwd");
		strcat(rnamerev,"rev"); }

	if(!isrule) {
		er=RxnAddReactionPattern(sim,rname,pattern,-1,rctstate,prdstate,cmpt,srf,isrule,&rxn);
		CHECKS(er!=-1,"out of memory adding reaction");
		if(!isrule) {CHECKS(er!=-2,"one of the reactant names is unknown");}
		if(!isrule) {CHECKS(er!=-3,"one of the product names is unknown");}
		CHECKS(er!=-4,"more reactants entered than allowed");
		CHECKS(er!=-5,"internal string overflow");
		CHECKS(er!=-6,"BUG: species generation failed");
		CHECKS(er!=-11,"out of memory in wildcard expansion");
		CHECKS(er!=-12,"BUG: missing reactant in wildcard expression");
		CHECKS(er!=-13,"missing AND operand in wildcard expression");
		CHECKS(er!=-15,"mismatched braces in wildcard expression");
		CHECKS(er!=-20,"reactant and product wildcard logic don't correspond");
		CHECKS(er!=-30,"BUG: failed to add reaction"); }

	if(bidirect) {
		er=molreversepattern(sim,pattern,patternrev);
		CHECKS(er!=-4,"pattern length exceeds maximum length");
		CHECKS(er!=-5,"the reverse reaction would have more than two reactants, which is not allowed");
		CHECKS(!er,"BUG: error in reverse reaction code");
		if(!isrule) {
			er=RxnAddReactionPattern(sim,rnamerev,patternrev,-1,prdstate,rctstate,cmpt,srf,isrule,&rxnrev);
			CHECKS(er!=-1,"out of memory adding reverse reaction");
			if(!isrule) {CHECKS(er!=-2,"one of the reverse reaction reactant names is unknown");}
			if(!isrule) {CHECKS(er!=-3,"one of the reverse reaction product names is unknown");}
			CHECKS(er!=-4,"more reverse reaction reactants entered than allowed");
			CHECKS(er!=-5,"internal string overflow for reverse reaction");
			CHECKS(er!=-6,"BUG: species generation failed for reverse reaction");
			CHECKS(er!=-11,"out of memory in wildcard expansion for reverse reaction");
			CHECKS(er!=-12,"BUG: missing reactant in wildcard expression for reverse reaction");
			CHECKS(er!=-13,"missing AND operand in wildcard expression for reverse reaction");
			CHECKS(er!=-15,"mismatched braces in wildcard expression for reverse reaction");
			CHECKS(er!=-20,"reactant and product wildcard logic don't correspond for reverse reaction");
			CHECKS(er!=-30,"BUG: failed to add reaction for reverse reaction"); }}

	flt1=flt2=-1;
	if(line2) {
#ifdef OPTION_VCELL
		CHECKS(rxn,"wildcards and species groups are not supported in reactions in VCell");
		using std::stringstream;
		string rawStr, expStr;
		stringstream ss(line2);
		getline(ss,rawStr);
		size_t found = rawStr.find(";");
		if(found!=string::npos) {
			expStr = rawStr.substr(0, found);
			ValueProvider* valueProvider = sim->valueProviderFactory->createValueProvider(expStr);
			double constRate = 1;
			try {
				constRate = valueProvider->getConstantValue();
			} catch (...) {
				rxn->rateValueProvider = valueProvider;
			}

			//if rate is a exp, a "fake" set value has to be set to allow RxnSetRate to do proper job, especially when there
			//is reversible reactions, the rxn->rparamt, and rxn->bindrad2 have to be set.
			er = RxnSetValue(sim, "rate", rxn, constRate);
			line2=NULL; }
		else
#endif
		{
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
			CHECKS(itct==1,"failed to read reaction rate");
			if(!isrule) {
				if(rxn) er=RxnSetValue(sim,"rate",rxn,flt1);
				else er=RxnSetValuePattern(sim,"rate",pattern,rname,rctstate,prdstate,flt1,0,NULL);
				CHECKS(er!=4,"reaction rate value must be non-negative");
				CHECKS(er!=5,"BUG: cannot find reaction that was just created"); }
			line2=strnword(line2,2);

			if(bidirect && line2) {
				itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt2);
				CHECKS(itct==1,"failed to read reverse reaction rate");
				if(!isrule) {
					if(rxnrev) er=RxnSetValue(sim,"rate",rxnrev,flt2);
					else er=RxnSetValuePattern(sim,"rate",patternrev,rnamerev,prdstate,rctstate,flt2,0,NULL);
					CHECKS(er!=4,"reverse reaction rate value must be non-negative");
					CHECKS(er!=5,"cannot find reaction that was just created"); }
				line2=strnword(line2,2); }}}

	if(isrule) {
		detailsi[0]=c;
		detailsi[1]=s;
		er=RuleAddRule(sim,RTreaction,rname,pattern,rctstate,prdstate,flt1,detailsi,NULL);
		CHECKS(!er,"out of memory or other error adding rule");
		if(bidirect) {
			er=RuleAddRule(sim,RTreaction,rnamerev,patternrev,prdstate,rctstate,flt2,detailsi,NULL);
			CHECKS(!er,"out of memory or other error adding reverse rule"); }}

	CHECKS(!line2,"unexpected text following reaction");

	return 0;

failure:
	strcpy(errstr,ErrorString);
	return 1; }


/******************************************************************************/
/************************** core simulation functions *************************/
/******************************************************************************/


/* doreact */
int doreact(simptr sim,rxnptr rxn,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,int m2,double *pos,panelptr rxnpnl) {
	int order,prd,d,nprod,dim,calc,dorxnlog,prd2,er;
	long int pserno;
	unsigned long long sernolist[MAXPRODUCT];
	double dc1,dc2,x,dist,pvalue;
	molssptr mols;
	moleculeptr mptr,mptrfrom;
	boxptr rxnbptr;
	double v1[DIMMAX],v2[DIMMAX],rxnpos[DIMMAX],m3[DIMMAX*DIMMAX];
	enum MolecState ms;
	FILE *fptr;
	char string[STRCHAR];

	mols=sim->mols;
	dim=sim->dim;
	order=(mptr1?1:0)+(mptr2?1:0);
	dorxnlog=0;
	fptr=NULL;

// get reaction position in rxnpos, rxnpnl, rxnbptr
	if(order==0) {																// order 0
		for(d=0;d<dim;d++) rxnpos[d]=pos[d];
		rxnbptr=pos2box(sim,rxnpos); }

	else if(order==1) {														// order 1
		for(d=0;d<dim;d++) rxnpos[d]=mptr1->pos[d];
		rxnpnl=mptr1->pnl;
		rxnbptr=mptr1->box; }

	else {																				// order 2
		dc1=mols->difc[mptr1->ident][mptr1->mstate];
		dc2=mols->difc[mptr2->ident][mptr2->mstate];
		if(dc1==0 && dc2==0) x=0.5;
		else x=dc2/(dc1+dc2);
		for(d=0;d<dim;d++) rxnpos[d]=x*mptr1->pos[d]+(1.0-x)*mptr2->pos[d];
		if(mptr1->pnl || mptr2->pnl) {
			if(mptr1->pnl && ptinpanel(rxnpos,mptr1->pnl,dim)) rxnpnl=mptr1->pnl;
			else if(mptr2->pnl && ptinpanel(rxnpos,mptr2->pnl,dim)) rxnpnl=mptr2->pnl;
			else if(mptr1->pnl)
				closestsurfacept(mptr1->pnl->srf,dim,rxnpos,NULL,&rxnpnl,mptr1->box);
			else
				closestsurfacept(mptr2->pnl->srf,dim,rxnpos,NULL,&rxnpnl,mptr2->box); }
		else rxnpnl=NULL;
		rxnbptr=(dc1<=dc2)?mptr1->box:mptr2->box; }

// determine if reaction needs to be logged
	if(rxn->logserno) {
		if(rxn->logserno->n==1 && rxn->logserno->xs[0]==-1) dorxnlog=1;
		else if(mptr1 && ListMemberLI(rxn->logserno,mptr1->serno&0xFFFFFFFF)) dorxnlog=1;
		else if(mptr1 && ListMemberLI(rxn->logserno,(mptr1->serno>>32))) dorxnlog=1;
		else if(mptr2 && ListMemberLI(rxn->logserno,mptr2->serno&0xFFFFFFFF)) dorxnlog=1;
		else if(mptr2 && ListMemberLI(rxn->logserno,(mptr2->serno>>32))) dorxnlog=1;
		else dorxnlog=0; }

// place products
	nprod=rxn->nprod;
	calc=0;
	dist=0;
	x=0;
	pvalue=-1;
	for(prd=0;prd<nprod;prd++) {
		mptr=getnextmol(sim->mols);								// mptr is current product
		if(!mptr) return 1;
		mptr->ident=rxn->prdident[prd];
		mptr->mstate=rxn->prdstate[prd];
		ms=mptr->mstate;
		if(rxn->prdrep && rxn->prdrep[prd]==SRlattice) {
			mptr->list=sim->latticess->latticelist[0]->port->llport;
			mptr->box=NULL; }
		else {
			mptr->list=sim->mols->listlookup[mptr->ident][ms];
			mptr->box=rxnbptr; }
		sim->mols->expand[mptr->ident]|=1;

		if(rxn->rparamt==RPconfspread) {					// confspread reaction
			if(prd<2) {
				mptrfrom=(prd==0)?mptr1:mptr2;
				if(mptr->box)
					mptr->box=mptrfrom->box;
				mptr->serno=mptrfrom->serno;
				for(d=0;d<dim;d++) {
					mptr->posx[d]=mptr->pos[d]=mptrfrom->pos[d];
					mptr->posoffset[d]=mptrfrom->posoffset[d]; }
				mptr->pnl=mptr->pnlx=(ms==MSsoln)?NULL:mptrfrom->pnl; }
			else {
				mptrfrom=mptr1;
				for(d=0;d<dim;d++)
					mptr->posx[d]=mptr->pos[d]=rxnpos[d];
				if(ms==MSsoln) mptr->pnl=mptr->pnlx=NULL;
				else if(mptrfrom->pnl) mptr->pnl=mptr->pnlx=mptrfrom->pnl;
				else mptr->pnl=mptr->pnlx=rxnpnl; }}

		else if(rxn->rparamt==RPbounce) {					// bounce reaction
			if(prd==0) {
				pvalue=-1;
				if(rxn->rparam==-2) {									// reflection method
					pvalue=Geo_SphereReflectSphere(mptr1->posx,mptr1->pos,mptr2->posx,mptr2->pos,dim,rxn->bindrad2,v1,v2);
					if(pvalue>0 && pvalue<1)
						for(d=0;d<dim;d++) mptr->pos[d]=v1[d];
					else
						pvalue=-1; }											// failed, so revert to other methods
				if(pvalue==-1) {
					dist=0;
					for(d=0;d<dim;d++) {
						v1[d]=mptr2->pos[d]-mptr1->pos[d];	// v1 is from mptr1 to mptr2
						dist+=v1[d]*v1[d]; }								// dist is length of v1
					if(dist==0) {
						dist=sqrt(rxn->bindrad2);
						v1[0]=dist; }
					else
						dist=sqrt(dist);
					if(rxn->rparam>=0) x=1.0/dist-(1.0/rxn->rparam);	// x is scaling factor
					else x=2.0/dist-2.0/(rxn->prdpos[0][0]+rxn->prdpos[1][0]);
					for(d=0;d<dim;d++)
						mptr->pos[d]=mptr1->pos[d]-x*rxn->prdpos[0][0]*v1[d]; }
				mptr->box=mptr1->box;
				mptr->pnl=mptr1->pnl;
				mptr->pnlx=mptr1->pnlx;
				mptr->serno=mptr1->serno;
				for(d=0;d<dim;d++) {
					mptr->posx[d]=mptr1->pos[d];
					mptr->posoffset[d]=mptr1->posoffset[d]; }
				if(mptr->pnl) movemol2closepanel(sim,mptr); }
			else if(prd==1) {
				mptr->box=mptr2->box;
				mptr->pnl=mptr2->pnl;
				mptr->pnlx=mptr2->pnlx;
				mptr->serno=mptr2->serno;
				if(pvalue!=-1) {											// reflection method
					for(d=0;d<dim;d++)
						mptr->pos[d]=v2[d]; }
				else {																// other methods
					for(d=0;d<dim;d++)
						mptr->pos[d]=mptr2->pos[d]+x*rxn->prdpos[1][0]*v1[d]; }
				for(d=0;d<dim;d++) {
					mptr->posx[d]=mptr2->pos[d];
					mptr->posoffset[d]=mptr2->posoffset[d]; }
				if(mptr->pnl) movemol2closepanel(sim,mptr); }
			else {
				for(d=0;d<dim;d++)
					mptr->posx[d]=mptr->pos[d]=rxnpos[d];
				if(ms==MSsoln) mptr->pnl=mptr->pnlx=NULL;
				else mptr->pnl=mptr->pnlx=rxnpnl; }}

		else {
			for(d=0;d<dim;d++) mptr->posx[d]=rxnpos[d];

			mptr->mstate=ms=rxn->prdstate[prd];

			if(rxn->prdintersurf) {
				if(rxn->prdintersurf[prd]==1) {
					mptr->pnl=mptr1->pnl;
					mptr->pnlx=mptr1->pnlx; }
				else {
					mptr->pnl=mptr2->pnl;
					mptr->pnlx=mptr2->pnlx; }}
			else
				mptr->pnl=mptr->pnlx=rxnpnl;

			if(!rxnpnl);														// soln -> soln
			else if(ms==MSsoln) {										// surf -> front soln
				fixpt2panelnocross(sim,mptr->posx,mptr->pnlx,dim,PFfront,sim->srfss->epsilon);
				mptr->pnl=NULL; }
			else if(ms==MSbsoln) {									// surf -> back soln
				fixpt2panelnocross(sim,mptr->posx,mptr->pnlx,dim,PFback,sim->srfss->epsilon);
				mptr->mstate=MSsoln;
				mptr->pnl=NULL; }
			else if(ms==MSfront) {									// surf -> front surf
				fixpt2panelnocross(sim,mptr->posx,mptr->pnlx,dim,PFfront,sim->srfss->epsilon); }
			else if(ms==MSback) {										// surf -> back surf
				fixpt2panelnocross(sim,mptr->posx,mptr->pnlx,dim,PFback,sim->srfss->epsilon); }
			else {																	// surf -> surf: up, down
				fixpt2panelnocross(sim,mptr->posx,mptr->pnlx,dim,PFnone,sim->srfss->epsilon); }

			for(d=0;d<dim && rxn->prdpos[prd][d]==0;d++);		// compute pos displacement away from posx
			if(d!=dim) {
				if(rxn->rparamt==RPfixed) {
					for(d=0;d<dim;d++) v1[d]=rxn->prdpos[prd][d]; }
				else if(dim==1) {
					if(!calc) {m3[0]=signrand();calc=1;}
					v1[0]=m3[0]*rxn->prdpos[prd][0]; }
				else if(dim==2) {
					if(!calc) {DirCosM2D(m3,unirandCOD(0,2*PI));calc=1;}
					dotMVD(m3,rxn->prdpos[prd],v1,2,2); }
				else if(dim==3) {
					if(!calc) {DirCosMD(m3,thetarandCCD(),unirandCOD(0,2*PI),unirandCOD(0,2*PI));calc=1;}
					dotMVD(m3,rxn->prdpos[prd],v1,3,3); }
				else {
					if(!calc) {DirCosMD(m3,thetarandCCD(),unirandCOD(0,2*PI),unirandCOD(0,2*PI));calc=1;}
					dotMVD(m3,rxn->prdpos[prd],v1,3,3); }
				for(d=0;d<dim;d++)
					mptr->pos[d]=mptr->posx[d]+v1[d]; }
			else {
				for(d=0;d<dim;d++)
					mptr->pos[d]=mptr->posx[d]; }

			if(mptr->mstate!=MSsoln)
				movemol2closepanel(sim,mptr);

			if(rxn->prdserno) {												// set product serial numbers
				pserno=rxn->prdserno[prd];
				mptr->serno=molfindserno(sim,mptr->serno,pserno,mptr1?mptr1->serno:0,mptr2?mptr2->serno:0,sernolist);
				sernolist[prd]=mptr->serno; }}

		if(rxn->logserno) {													// log reaction if needed
			if(dorxnlog==0 && (ListMemberLI(rxn->logserno,mptr->serno&0xFFFFFFFF) || (mptr->serno>0xFFFFFFF && ListMemberLI(rxn->logserno,mptr->serno>>32))))
				dorxnlog=1;
			if(dorxnlog==1) {
				er=scmdgetfptr(sim->cmds,rxn->logfile,1,&fptr,NULL);
				if(er==-1) {
					simLog(sim,8,"reaction log filename '%s' is undeclared\n",rxn->logfile);
					dorxnlog=-1; }
				else {
					scmdfprintf(sim->cmds,fptr,"%g %s",sim->time,rxn->rname);
					for(d=0;d<dim;d++) scmdfprintf(sim->cmds,fptr," %g",rxnpos[d]);
					if(mptr1) scmdfprintf(sim->cmds,fptr," %s",molserno2string(mptr1->serno,string));
					if(mptr2) scmdfprintf(sim->cmds,fptr," %s",molserno2string(mptr2->serno,string));
					for(prd2=0;prd2<prd;prd2++) scmdfprintf(sim->cmds,fptr," ?");
					dorxnlog=2; }}
			if(dorxnlog==2) {
				scmdfprintf(sim->cmds,fptr," %s",molserno2string(mptr->serno,string));
				if(prd==nprod-1) scmdfprintf(sim->cmds,fptr,"\n"); }}}

	if(mptr1) molkill(sim,mptr1,ll1,m1);					// kill reactants
	if(mptr2) molkill(sim,mptr2,ll2,m2);

	return 0; }


/* zeroreact */
int zeroreact(simptr sim) {
	int i,r,nmol;
	rxnptr rxn;
	rxnssptr rxnss;
	double pos[DIMMAX];
	panelptr pnl;

	pnl=NULL;
	rxnss=sim->rxnss[0];
	if(!rxnss) return 0;
	for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
				
#ifdef OPTION_VCELL
		if(rxn->rateValueProvider != NULL) {
			if(rxn->cmpt) {
				AbstractMesh* mesh = sim->mesh;
				double delta[3];
				mesh->getDeltaXYZ(delta);
				int n[3];
				mesh->getNumXYZ(n);
				double meshv = delta[0]*delta[1]*delta[2];
				int totalNumMesh = n[0]*n[1]*n[2];			
			
				for(i=0; i<totalNumMesh; i++) {
					int volIndex = i;
					double centerPos[3];
					mesh->getCenterCoordinates(volIndex, centerPos);
					if (posincompart(sim, centerPos, rxn->cmpt,0)) {
					   // go through each mesh elements to see if it is in the compartments that the reaction happens
						double rate =  evaluateVolRnxRate(sim, rxn, centerPos);
						double prob = rate * sim->dt * meshv;	
						nmol=poisrandD(prob);
						int count = 0;
						//put generated molecules in the same mesh
						double pos[3];
						while(count < nmol) {
							count++;
							randomPosInMesh(sim,centerPos, pos); //the generated position saved in pos
							//since we are using vcell mesh, we double check the pos in compartment
							if(posincompart(sim,pos,rxn->cmpt,0)) {
								//count++;
								if(doreact(sim,rxn,NULL,NULL,-1,-1,-1,-1,pos,pnl)) return 1; }}
						sim->eventcount[ETrxn0]+=nmol; }}}
			else if(rxn->srf) {
				int numOfSuf = sim->srfss->nsrf;
				surfaceptr * surfaces = sim->srfss->srflist;
				for(i=0; i<numOfSuf; i++) {
					if(surfaces[i] == rxn->srf) { //find the surface where the reaction happens
						//get all the panels on the surface
						int numOfTriPanels = surfaces[i]->npanel[PStri];
						panelptr* panels = surfaces[i]->panels[PStri];
						for(long j=0; j<numOfTriPanels; j++) {
							//evaluate rate 
							double ** points = panels[j]->point; //point[number][dim]
							double triCenterPos[3]; 
							Geo_TriCenter(points, triCenterPos, sim->dim);
							double rate = evaluateMemRnxRate(sim, rxn, triCenterPos, panels[j]->pname);
							//get probability
							double triPanelArea = Geo_TriArea3D(points[0], points[1], points[2]);
							double prob = rate * sim->dt * triPanelArea;
							//num of molecules generated
							nmol=poisrandD(prob);
							//uniformly and randomly place the molecule 
							int count = 0;
							//put generated molecules in the same mesh
							while(count < nmol) {
								count++;
								panelrandpos(panels[j],pos,sim->dim);//randomly generate a position for surface molecule
								////since we are using vcell mesh, we double check the pos in compartment
								//if(posincompart(sim,pos,rxn->cmpt,0))
								//{
									//count++; 
									if(doreact(sim,rxn,NULL,NULL,-1,-1,-1,-1,pos,panels[j])) return 1; 
								//}
							}
							sim->eventcount[ETrxn0]+=nmol; }}}}}
		else
#endif

		{
			nmol=poisrandD(rxn->prob);
			for(i=0;i<nmol;i++) {
				if(rxn->cmpt) compartrandpos(sim,pos,rxn->cmpt);
				else if(rxn->srf) pnl=surfrandpos(rxn->srf,pos,sim->dim);
				else systemrandpos(sim,pos);
				if(doreact(sim,rxn,NULL,NULL,-1,-1,-1,-1,pos,pnl)) return 1; }
			sim->eventcount[ETrxn0]+=nmol; }}
	return 0; }


/* unireact */
int unireact(simptr sim) {
	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	moleculeptr *mlist,mptr;
	int *nrxn,**table;
	int i,j,m,nmol,ll;
	enum MolecState ms;

	rxnss=sim->rxnss[1];
	if(!rxnss) return 0;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	for(ll=0;ll<sim->mols->nlist;ll++)
		if(rxnss->rxnmollist[ll]) {
			mlist=sim->mols->live[ll];
			nmol=sim->mols->nl[ll];
			for(m=0;m<nmol;m++) {
				mptr=mlist[m];
				i=mptr->ident;
				ms=mptr->mstate;
				for(j=0;j<nrxn[i];j++) {
					rxn=rxnlist[table[i][j]];
#ifdef OPTION_VCELL
					if(rxn->rateValueProvider != NULL)
					{
						if(rxn->srf)//surface reaction
						{
							if(mptr->pnl)
							{
								rxn -> rate = evaluateMemRnxRate(sim, rxn, mptr->pos, mptr->pnl->pname);
							}
							else
							{
								simLog(sim,10,"Unimolecular membrance reaction should have a membrane reactant.");
							}
						}
						else
						{
							rxn -> rate = evaluateVolRnxRate(sim, rxn, mptr->pos);
						}
						char erstr[STRCHAR];
						int er, r;
						r = table[i][j];
						er=rxnsetrate(sim,1,r,erstr);
					}
#endif

					if(!rxn->permit[ms]);																						// failed permit test
					else if(!coinrandD(rxn->prob));																	// failed probability test
					else if(rxn->cmpt && !posincompart(sim,mptr->pos,rxn->cmpt,0));		// failed compartment test
					else if(rxn->srf && (!mptr->pnl || mptr->pnl->srf!=rxn->srf));	// failed surface test
					else if(mptr->ident==0);																				// failed existance test
					else {																													// react
						if(doreact(sim,rxn,mptr,NULL,ll,m,-1,-1,NULL,NULL)) return 1;
						sim->eventcount[ETrxn1]++;
						j=nrxn[i]; }}}}
	return 0; }


/* RxnHybridReact */
int RxnHybridReact(simptr sim) {
#ifdef OPTION_NSV
	rxnssptr rxnss;
	molssptr mols;
	rxnptr rxn;
	int partrct,parti,ll,m,i,r,lati,dim;
	enum MolecState ms,partms;
	moleculeptr mptr;
	double latconc,prob;
	NextSubvolumeMethod *nsv;

	rxnss=sim->rxnss[2];
	if(!rxnss || !sim->latticess) return 0;
	mols=sim->mols;
	nsv=sim->latticess->latticelist[0]->nsv;
	dim=sim->dim;

	for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
		if(rxn->rctrep && rxn->rctrep[0]!=rxn->rctrep[1]) {
			if(rxn->rctrep[0]==SRparticle) partrct=0;
			else partrct=1;
			parti=rxn->rctident[partrct];														// species of particle reactant
			partms=rxn->rctstate[partrct];													// state of particle reactant (must be actual state, not some, none, all)
			ll=mols->listlookup[parti][partms];											// molecule list of particle
			lati=rxn->rctident[!partrct];														// species of lattice reactant
			for(m=0;m<mols->nl[ll];m++) {
				mptr=mols->live[ll][m];
				i=mptr->ident;
				ms=mptr->mstate;
				if(i==parti && ms==partms) {
					latconc=NSV_CALL(nsv_concentration_point(nsv,lati,mptr->pos,dim));
					prob=1.0-exp(-rxn->rate*rxn->multiplicity*latconc*sim->dt);
					if(!coinrandD(prob));																						// failed probability test
					else if(rxn->cmpt && !posincompart(sim,mptr->pos,rxn->cmpt,0));		// failed compartment test
					else if(rxn->srf && (!mptr->pnl || mptr->pnl->srf!=rxn->srf));	// failed surface test
					else if(mptr->ident==0);																				// failed existance test
					else {																													// react
						if(doreact(sim,rxn,mptr,NULL,ll,m,-1,-1,NULL,NULL)) return 1;
						NSV_CALL(nsv_kill_molecule(nsv,lati,mptr->pos,dim));
						sim->eventcount[ETrxn2hybrid]++; }}}}}
#endif
	return 0; }

//?? I'm not sure what this function is supposed to do.  No function calls it at present.
//?? to do: disable order 1 reactions in Smoldyn if the reactant representation is lattice
//?? disable order 2 reactions in Smoldyn if both reactant representations are lattice
//?? check to make sure that lattice reactant states are solution
//?? check to make sure that particle reactant states are specific states, not none, all, or some

/* morebireact */
int morebireact(simptr sim,rxnptr rxn,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,enum EventType et,double *vect) {
	moleculeptr mptrA,mptrB;
	int d,swap;
	enum MolecState ms,msA,msB;
	double shift;

	if(rxn->cmpt && !(posincompart(sim,mptr1->pos,rxn->cmpt,0) && posincompart(sim,mptr2->pos,rxn->cmpt,0))) return 0;
	if(rxn->srf && !((mptr1->pnl && mptr1->pnl->srf==rxn->srf ) || (mptr2->pnl && mptr2->pnl->srf==rxn->srf))) return 0;
	if(rxn->rctrep && (rxn->rctrep[0]==SRlattice || rxn->rctrep[1]==SRlattice)) return 0;

	if(mptr1->pnl && mptr2->pnl && mptr1->pnl!=mptr2->pnl) {					// intersurface reaction
		if(rxn->rparamt==RPconfspread || rxn->rparamt==RPbounce || rxn->prdintersurf);
		else return 0; }

	if(mptr1->ident==rxn->rctident[0]) {
		mptrA=mptr1;
		mptrB=mptr2;
		swap=0; }
	else {
		mptrA=mptr2;
		mptrB=mptr1;
		swap=1; }

	msA=mptrA->mstate;
	msB=mptrB->mstate;
	if(msA==MSsoln && msB!=MSsoln)
		msA=(panelside(mptrA->pos,mptrB->pnl,sim->dim,NULL,0,0)==PFfront)?MSsoln:MSbsoln;
	else if(msB==MSsoln && msA!=MSsoln)
		msB=(panelside(mptrB->pos,mptrA->pnl,sim->dim,NULL,0,0)==PFfront)?MSsoln:MSbsoln;
	ms=(MolecState)(msA*MSMAX1+msB);

	if(rxn->permit[ms]) {
		if(et==ETrxn2wrap && rxn->rparamt!=RPconfspread) {		// if wrapping, then move faster diffusing molecule
			if(sim->mols->difc[mptr1->ident][mptr1->mstate]<sim->mols->difc[mptr2->ident][mptr2->mstate])
				for(d=0;d<sim->dim;d++) {
					shift=mptr1->pos[d]+vect[d]-mptr2->pos[d];
					mptr2->posoffset[d]-=shift;
					mptr2->pos[d]+=shift;
					mptr2->posx[d]+=shift; }
			else
				for(d=0;d<sim->dim;d++) {
					shift=mptr2->pos[d]-vect[d]-mptr1->pos[d];
					mptr1->posoffset[d]-=shift;
					mptr1->pos[d]+=shift;
					mptr1->posx[d]+=shift; }}
		sim->eventcount[et]++;
		if(!swap) return doreact(sim,rxn,mptrA,mptrB,ll1,m1,ll2,-1,NULL,NULL);
		else return doreact(sim,rxn,mptrA,mptrB,ll2,-1,ll1,m1,NULL,NULL); }

	return 0; }


#ifdef OPTION_VCELL
void setBiReactRateForHybrid(simptr sim,rxnptr rxn,moleculeptr mptr1,moleculeptr mptr2,int r)
{
	if(rxn->rateValueProvider != NULL)
	{
		int dimension = sim->dim;
		double pos[3]; 
		for(int dimIdx =0; dimIdx < dimension; dimIdx++)
		{
			pos[dimIdx] = (mptr1->pos[dimIdx] + mptr2->pos[dimIdx])/2;
		}
		if(rxn->srf)//surface reaction
		{
			if(mptr1->pnl)
			{
				rxn -> rate = evaluateMemRnxRate(sim, rxn, pos, mptr1->pnl->pname);
			}
			else if(mptr2->pnl)
			{
				rxn -> rate = evaluateMemRnxRate(sim, rxn, pos, mptr2->pnl->pname);
			}
			else
			{
				simLog(sim,10,"Bimolecular membrance reaction should have at least one membrane reactant.");
			}
		}
		else
		{
			rxn -> rate = evaluateVolRnxRate(sim, rxn, pos);
		}

		char erstr[STRCHAR];
		int er;
		er=rxnsetrate(sim,2,r,erstr);
	}
}
#endif


/* bireact */
int bireact(simptr sim,int neigh) {
	int dim,maxspecies,ll1,ll2,i,j,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,nlist,maxlist;
	int *nrxn,**table;
	double dist2,vect[DIMMAX];
	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	boxptr bptr;
	moleculeptr **live,*mlist2,mptr1,mptr2;

	rxnss=sim->rxnss[2];
	if(!rxnss) return 0;
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nlist=sim->mols->nlist;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;

	if(!neigh) {																		// same box
		for(ll1=0;ll1<nlist;ll1++)
			for(ll2=ll1;ll2<nlist;ll2++)
				if(rxnss->rxnmollist[ll1*maxlist+ll2])
					for(m1=0;m1<nl[ll1];m1++) {
						mptr1=live[ll1][m1];
						bptr=mptr1->box;
						mlist2=bptr->mol[ll2];
						nmol2=bptr->nmol[ll2];
						for(m2=0;m2<nmol2 && mlist2[m2]!=mptr1;m2++) {
							mptr2=mlist2[m2];
							i=mptr1->ident*maxspecies+mptr2->ident;
							for(j=0;j<nrxn[i];j++) {
								rxn=rxnlist[table[i][j]];
#ifdef OPTION_VCELL
								setBiReactRateForHybrid(sim, rxn, mptr1, mptr2, table[i][j]);
#endif
								dist2=0;
								for(d=0;d<dim;d++)
									dist2+=(mptr1->pos[d]-mptr2->pos[d])*(mptr1->pos[d]-mptr2->pos[d]);
								if(dist2<=rxn->bindrad2 && (rxn->prob==1 || randCOD()<rxn->prob) && (mptr1->mstate!=MSsoln || mptr2->mstate!=MSsoln || !rxnXsurface(sim,mptr1,mptr2)) && mptr1->ident!=0 && mptr2->ident!=0) {
									if(morebireact(sim,rxn,mptr1,mptr2,ll1,m1,ll2,ETrxn2intra,NULL)) return 1;
									if(mptr1->ident==0) {
										j=nrxn[i];
										m2=nmol2; }}}}}}

	else {																					// neighbor box
		for(ll1=0;ll1<nlist;ll1++)
			for(ll2=ll1;ll2<nlist;ll2++)
				if(rxnss->rxnmollist[ll1*maxlist+ll2])
					for(m1=0;m1<nl[ll1];m1++) {
						mptr1=live[ll1][m1];
						bptr=mptr1->box;
						bmax=(ll1!=ll2)?bptr->nneigh:bptr->midneigh;
						for(b2=0;b2<bmax;b2++) {
							mlist2=bptr->neigh[b2]->mol[ll2];
							nmol2=bptr->neigh[b2]->nmol[ll2];
							if(bptr->wpneigh && bptr->wpneigh[b2]) {	// neighbor box with wrapping
								wpcode=bptr->wpneigh[b2];
								for(m2=0;m2<nmol2;m2++) {
									mptr2=mlist2[m2];
									i=mptr1->ident*maxspecies+mptr2->ident;
									for(j=0;j<nrxn[i];j++) {
										rxn=rxnlist[table[i][j]];
#ifdef OPTION_VCELL
										setBiReactRateForHybrid(sim, rxn, mptr1, mptr2, table[i][j]);
#endif
										dist2=wallcalcdist2(sim,mptr1->pos,mptr2->pos,wpcode,vect);
										if(dist2<=rxn->bindrad2 && (rxn->prob==1 || randCOD()<rxn->prob) && mptr1->ident!=0 && mptr2->ident!=0) {
											if(morebireact(sim,rxn,mptr1,mptr2,ll1,m1,ll2,ETrxn2wrap,vect)) return 1;
											if(mptr1->ident==0) {
												j=nrxn[i];
												m2=nmol2;
												b2=bmax; }}}}}

							else													// neighbor box, no wrapping
								for(m2=0;m2<nmol2;m2++) {
									mptr2=mlist2[m2];
									i=mptr1->ident*maxspecies+mptr2->ident;
									for(j=0;j<nrxn[i];j++) {
										rxn=rxnlist[table[i][j]];
#ifdef OPTION_VCELL
										setBiReactRateForHybrid(sim, rxn, mptr1, mptr2, table[i][j]);
#endif
										dist2=0;
										for(d=0;d<dim;d++)
											dist2+=(mptr1->pos[d]-mptr2->pos[d])*(mptr1->pos[d]-mptr2->pos[d]);
										if(dist2<=rxn->bindrad2 && (rxn->prob==1 || randCOD()<rxn->prob) &&  (mptr1->mstate!=MSsoln || mptr2->mstate!=MSsoln || !rxnXsurface(sim,mptr1,mptr2)) && mptr1->ident!=0 && mptr2->ident!=0) {
											if(morebireact(sim,rxn,mptr1,mptr2,ll1,m1,ll2,ETrxn2inter,NULL)) return 1;
											if(mptr1->ident==0) {
												j=nrxn[i];
												m2=nmol2;
												b2=bmax; }}}}}}}

	return 0; }


