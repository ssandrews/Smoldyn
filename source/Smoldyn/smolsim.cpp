/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <cfloat>
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "List.h"
#include "math2.h"
#include "opengl2.h"
#include "random2.h"
#include "Rn.h"
#include "SimCommand.h"
#include "string2.h"
#include "Zn.h"
#include <string>
#include <sstream>

#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smoldynconfigure.h"

#ifdef OPTION_VCELL
	using std::stringstream;
#endif

#ifdef ENABLE_PYTHON_CALLBACK
#include "../python/CallbackFunc.h"
#endif

/******************************************************************************/
/************************* Global variable definitions ************************/
/******************************************************************************/

void (*LoggingCallback)(simptr,int,const char*,...)=NULL;
int ThrowThreshold=11;
FILE *LogFile=NULL;

//
// ErrorLineAndString contains ErrorString (a few places sprinf is used to put
// ErrorLine inside ErrorLineAndString therefore the size of ErrorLineAndString
// has to be bigger than ErrorString else compiler emits warning (for a good
// reason).
//
char ErrorLineAndString[STRCHARLONG+STRCHAR+100]="";
char ErrorString[STRCHARLONG]="";

int ErrorType=0;
char SimFlags[STRCHAR]="";
int VCellDefined=0;


/******************************************************************************/
/***************************** Simulation structure ***************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// error handling

// enumerated types
char *simss2string(enum SmolStruct ss,char *string);

// low level utilities

// memory management

// data structure output

// structure set up

// core simulation functions


/******************************************************************************/
/********************************* Error handling *****************************/
/******************************************************************************/


/* simSetLogging */
void simSetLogging(FILE *logfile,void (*logFunction)(simptr,int,const char*,...)) {
	LogFile=logfile;
	LoggingCallback=logFunction;
	return; }


/* simSetThrowing */
void simSetThrowing(int corethreshold) {
	ThrowThreshold=corethreshold;
	return; }


/* simLog */
void simLog(simptr sim,int importance,const char* format, ...) {
	char message[STRCHARLONG],*flags;
	va_list arguments;
	int qflag,vflag,wflag,sflag;
	FILE *fptr;

	va_start(arguments, format);
	vsprintf(message, format, arguments);
	va_end(arguments);

	if(LoggingCallback)
		(*LoggingCallback)(sim,importance,message);

	if(sim && sim->logfile) fptr=sim->logfile;
	else if(LogFile) fptr=LogFile;
	else fptr=stdout;
	if(sim) flags=sim->flags;
	else flags=SimFlags;
	qflag=strchr(flags,'q')?1:0;
	sflag=strchr(flags,'s')?1:0;
	vflag=strchr(flags,'v')?1:0;
	wflag=strchr(flags,'w')?1:0;
	if(!sflag)
		if(vflag || importance>=2)
			if(!qflag || importance>=5)
				if(!wflag || importance<=4 || importance>=7)
					fprintf(fptr,"%s", message);

//??	if(importance>=ThrowThreshold) throw message;	//?? This is removed for now because I can't statically link Libsmoldyn if it's enabled

	return; }


/* simParseError */
void simParseError(simptr sim,ParseFilePtr pfp) {
	char parseerrstr[STRCHAR],matherrstr[STRCHAR];

	if(pfp) {
		Parse_ReadFailure(pfp,parseerrstr);
		sprintf(ErrorLineAndString,"%s\nMessage: %s",parseerrstr,ErrorString);
		simLog(sim,8,"%s\nMessage: %s\n",parseerrstr,ErrorString);
		if(strmatherror(matherrstr,1))
			simLog(sim,8,"math error: %s\n",matherrstr); }
	else {
		sprintf(ErrorLineAndString,"%s",ErrorString);
		simLog(sim,8,"%s",ErrorString); }
	return; }


/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* simstring2ss */
enum SmolStruct simstring2ss(char *string) {
	enum SmolStruct ans;

	if(!strcmp(string,"molecule")) ans=SSmolec;
	else if(!strcmp(string,"wall")) ans=SSwall;
	else if(!strcmp(string,"reaction")) ans=SSrxn;
	else if(!strcmp(string,"surface")) ans=SSsurf;
	else if(!strcmp(string,"box")) ans=SSbox;
	else if(!strcmp(string,"compartment")) ans=SScmpt;
	else if(!strcmp(string,"port")) ans=SSport;
	else if(!strcmp(string,"filament")) ans=SSfilament;
	else if(!strcmp(string,"command")) ans=SScmd;
	else if(!strcmp(string,"simulation")) ans=SSsim;
	else if(!strcmp(string,"check")) ans=SScheck;
	else if(!strcmp(string,"all")) ans=SSall;
	else ans=SSnone;
	return ans; }


/* simss2string */
char *simss2string(enum SmolStruct ss,char *string) {
	if(ss==SSmolec) strcpy(string,"molecule");
	else if(ss==SSwall) strcpy(string,"wall");
	else if(ss==SSrxn) strcpy(string,"reaction");
	else if(ss==SSsurf) strcpy(string,"surface");
	else if(ss==SSbox) strcpy(string,"box");
	else if(ss==SScmpt) strcpy(string,"compartment");
	else if(ss==SSport) strcpy(string,"port");
	else if(ss==SSfilament) strcpy(string,"filament");
	else if(ss==SScmd) strcpy(string,"command");
	else if(ss==SSsim) strcpy(string,"simulation");
	else if(ss==SScheck) strcpy(string,"check");
	else if(ss==SSall) strcpy(string,"all");
	else strcpy(string,"none");
	return string; }


/* simsc2string */
char *simsc2string(enum StructCond sc,char *string) {
	if(sc==SCinit) strcpy(string,"not initialized");
	else if(sc==SClists) strcpy(string,"lists need updating");
	else if(sc==SCparams) strcpy(string,"parameters need updating");
	else if(sc==SCok) strcpy(string,"fully updated");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/


/* simversionnumber */
double simversionnumber(void) {
	double v;

	v=0;
	sscanf(VERSION,"%lg",&v);
	return v; }


/* Simsetrandseed */
void Simsetrandseed(simptr sim,long int randseed) {
	if(!sim) return;
	sim->randseed=randomize(randseed);
	return; }


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* simalloc */
simptr simalloc(const char *fileroot) {
  simptr sim;
  int order;
	enum EventType et;

	sim=NULL;
	CHECKMEM(sim=(simptr) malloc(sizeof(struct simstruct)));
	sim->logfile=NULL;
	sim->condition=SCinit;
	sim->filepath=NULL;
	sim->filename=NULL;
	sim->flags=NULL;
	sim->clockstt=time(NULL);
	sim->elapsedtime=0;
	Simsetrandseed(sim,-1);
	for(et=(EventType)0;et<ETMAX;et=(EventType)(et+1)) sim->eventcount[et]=0;
	sim->maxvar=0;
	sim->nvar=0;
	sim->varnames=NULL;
	sim->varvalues=NULL;
	sim->dim=0;
	sim->accur=10;
	sim->time=0;
	sim->tmin=0;
	sim->tmax=DBL_MAX;
	sim->tbreak=DBL_MAX;
	sim->dt=0;
	sim->quitatend=0;
	for(order=0;order<MAXORDER;order++) sim->rxnss[order]=NULL;
	sim->ruless=NULL;
	sim->mols=NULL;
	sim->wlist=NULL;
	sim->srfss=NULL;
	sim->boxs=NULL;
	sim->cmptss=NULL;
	sim->portss=NULL;
	sim->latticess=NULL;
	sim->bngss=NULL;
	sim->filss=NULL;
	sim->cmds=NULL;
	sim->graphss=NULL;

	sim->diffusefn=&diffuse;
	sim->surfaceboundfn=&checksurfacebound;
	sim->surfacecollisionsfn=&checksurfaces;
	sim->assignmols2boxesfn=&reassignmolecs;
	sim->zeroreactfn=&zeroreact;
	sim->unimolreactfn=&unireact;
	sim->bimolreactfn=&bireact;
	sim->checkwallsfn=&checkwalls;

	CHECKMEM(sim->filepath=EmptyString());
	CHECKMEM(sim->filename=EmptyString());
	CHECKMEM(sim->flags=EmptyString());
	CHECKMEM(sim->cmds=scmdssalloc(&docommand,(void*)sim,fileroot));

	simsetvariable(sim,"time",sim->time);
	simsetvariable(sim,"x",dblnan());
	simsetvariable(sim,"y",dblnan());
	simsetvariable(sim,"z",dblnan());
	simsetvariable(sim,"r",dblnan());

#if ENABLE_PYTHON_CALLBACK
    sim->ncallbacks = 0;
    sim->simstep = 0;
#endif

	return sim;

 failure:
	simfree(sim);
	simLog(NULL,10,"Unable to allocate memory in simalloc");
	return NULL; }


/* simfree */
void simfree(simptr sim) {
	int dim,order,maxsrf;

	if(!sim) return;
	dim=sim->dim;
	maxsrf=sim->srfss?sim->srfss->maxsrf:0;

	graphssfree(sim->graphss);
	scmdssfree(sim->cmds);
	filssfree(sim->filss);
	latticessfree(sim->latticess);
	portssfree(sim->portss);
	compartssfree(sim->cmptss);
	boxssfree(sim->boxs);
	surfacessfree(sim->srfss);
	wallsfree(sim->wlist,dim);
	molssfree(sim->mols,maxsrf);
	rulessfree(sim->ruless);
	for(order=0;order<MAXORDER;order++) rxnssfree(sim->rxnss[order]);

	for(size_t v=0;v<(size_t)sim->maxvar;v++)
		free(sim->varnames[v]);
	free(sim->varnames);

#ifdef ENABLE_PYTHON_CALLBACK
    for(size_t v = 0; v < sim->ncallbacks; v++)
        if(sim->callbacks[v])
            free(sim->callbacks[v]);
#endif

	free(sim->varvalues);
	free(sim->flags);
	free(sim->filename);
	free(sim->filepath);
	free(sim);
	return; }


/* simfuncfree */
void simfuncfree(void) {
	strEnhWildcardMatch(NULL,NULL);
	strEnhWildcardMatchAndSub(NULL,NULL,NULL,NULL);
	strevalfunction(NULL,NULL,NULL,NULL,NULL,NULL,0);
	return; }


/* sim_expandvariables */
int simexpandvariables(simptr sim,int spaces) {
	char **newvarnames;
	double *newvarvalues;
	int newmaxvar,i,newnvar;

	newmaxvar=sim->maxvar+spaces;
	newvarnames=(char**) calloc(newmaxvar,sizeof (char*));
	if(!newvarnames) return 1;
	newvarvalues=(double*) calloc(newmaxvar,sizeof (double));
	if(!newvarvalues) return 1;
	for(i=0;i<sim->nvar && i<newmaxvar;i++) newvarnames[i]=sim->varnames[i];
	for(i=0;i<sim->nvar && i<newmaxvar;i++) newvarvalues[i]=sim->varvalues[i];
	newnvar=i;
	for(i=newnvar;i<newmaxvar;i++) {
		newvarnames[i]=EmptyString();
		if(!newvarnames[i]) return 1; }
	for(i=newnvar;i<newmaxvar;i++) newvarvalues[i]=0;
	free(sim->varnames);
	free(sim->varvalues);
	sim->varnames=newvarnames;
	sim->varvalues=newvarvalues;
	sim->maxvar=newmaxvar;
	sim->nvar=newnvar;
	return 0; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* simoutput */
void simoutput(simptr sim) {
	int v;

	simLog(sim,2,"SIMULATION PARAMETERS\n");
	if(!sim) {
		simLog(sim,2," No simulation parameters\n\n");
		return; }
	if(sim->filename[0]!='\0')
		simLog(sim,2," file: %s%s\n",sim->filepath,sim->filename);
	simLog(sim,2," starting clock time: %s",ctime(&sim->clockstt));
	simLog(sim,2," %i dimensions\n",sim->dim);
	if(sim->accur<10) simLog(sim,2," Accuracy level: %g\n",sim->accur);
	else simLog(sim,1," Accuracy level: %g\n",sim->accur);
	simLog(sim,2," Random number seed: %li\n",sim->randseed);
	simLog(sim,sim->nvar>5?2:1," %i variable%s defined:\n",sim->nvar,sim->nvar>1?"s":"");
	for(v=0;v<sim->nvar && v<5;v++)
		simLog(sim,1,"  %s = %g\n",sim->varnames[v],sim->varvalues[v]);
	for(;v<sim->nvar;v++)
		simLog(sim,2,"  %s = %g\n",sim->varnames[v],sim->varvalues[v]);

	simLog(sim,2," Time from %g to %g step %g\n",sim->tmin,sim->tmax,sim->dt);
	if(sim->time!=sim->tmin) simLog(sim,2," Current time: %g\n",sim->time);
	simLog(sim,2,"\n");
	return; }


/* simsystemoutput */
void simsystemoutput(simptr sim) {
	int order,vflag;

	if(!sim) {
		simLog(sim,2," No simulation parameters\n\n");
		return; }
	vflag=strchr(sim->flags,'v')?1:0;
	simoutput(sim);
	graphssoutput(sim);
	walloutput(sim);
	molssoutput(sim);
	surfaceoutput(sim);
	scmdoutput(sim->cmds);
	boxssoutput(sim);
	if(vflag)
//		boxoutput(sim->boxs,0,sim->boxs->nbox,sim->dim);
		boxoutput(sim->boxs,0,10,sim->dim);
	for(order=0;order<MAXORDER;order++)
		rxnoutput(sim,order);
	ruleoutput(sim);
	compartoutput(sim);
	portoutput(sim);
	bngoutput(sim);
	latticeoutput(sim);
	filssoutput(sim);
	return; }


/* writesim */
void writesim(simptr sim,FILE *fptr) {
	fprintf(fptr,"# General simulation parameters\n");
	fprintf(fptr,"# Configuration file: %s%s\n",sim->filepath,sim->filename);
	fprintf(fptr,"dim %i\n",sim->dim);
	fprintf(fptr,"# random_seed for prior simulation was %li\n",sim->randseed);
	fprintf(fptr,"random_seed %li  # this is a new random number\n",(long int)randULI());
	fprintf(fptr,"time_start %g\n",sim->tmin);
	fprintf(fptr,"time_stop %g\n",sim->tmax);
	fprintf(fptr,"time_step %g\n",sim->dt);
	fprintf(fptr,"time_now %g\n",sim->time);
	fprintf(fptr,"accuracy %g\n",sim->accur);
	if(sim->boxs->mpbox) fprintf(fptr,"molperbox %g\n",sim->boxs->mpbox);
	else if(sim->boxs->boxsize) fprintf(fptr,"boxsize %g\n",sim->boxs->boxsize);
	fprintf(fptr,"\n");
	return; }


/* checksimparams */
int checksimparams(simptr sim) {
	int warn,error,warndiff;
	char string[STRCHAR];

	simLog(sim,2,"PARAMETER CHECK\n");
	warn=error=0;

	error+=checkmolparams(sim,&warndiff);warn+=warndiff;
	error+=checkboxparams(sim,&warndiff);warn+=warndiff;
	error+=checkwallparams(sim,&warndiff);warn+=warndiff;
	error+=checkrxnparams(sim,&warndiff);warn+=warndiff;
	error+=checkruleparams(sim,&warndiff);warn+=warndiff;
	error+=checksurfaceparams(sim,&warndiff);warn+=warndiff;
	error+=checkcompartparams(sim,&warndiff);warn+=warndiff;
	error+=checkportparams(sim,&warndiff);warn+=warndiff;
	error+=checklatticeparams(sim,&warndiff);warn+=warndiff;
	error+=filcheckparams(sim,&warndiff);warn+=warndiff;
	error+=checkgraphicsparams(sim,&warndiff);warn+=warndiff;
	error+=checkbngparams(sim,&warndiff);warn+=warndiff;

	if(sim->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: simulation structure %s\n",simsc2string(sim->condition,string)); }

	if(error>0) simLog(sim,2," %i total errors\n",error);
	else simLog(sim,2," No errors\n");
	if(warn>0) simLog(sim,2," %i total warnings\n",warn);
	else simLog(sim,2," No warnings\n");
	simLog(sim,2,"\n");
	return error; }



/******************************************************************************/
/******************************* structure set up *****************************/
/******************************************************************************/


/* simsetcondition */
void simsetcondition(simptr sim,enum StructCond cond,int upgrade) {
	if(!sim) return;
	if(upgrade==0 && sim->condition>cond) sim->condition=cond;
	else if(upgrade==1 && sim->condition<cond) sim->condition=cond;
	else if(upgrade==2) sim->condition=cond;
	return; }


/* simsetvariable */
int simsetvariable(simptr sim,const char *name,double value) {
	int v,er;

	v=stringfind(sim->varnames,sim->nvar,name);
	if(v<0) {
		if(sim->nvar==sim->maxvar) {
			er=simexpandvariables(sim,2+2*sim->nvar);
			if(er) return er; }
		v=sim->nvar++;
		strcpy(sim->varnames[v],name); }
	sim->varvalues[v]=value;
	return 0; }


/* simsetdim */
int simsetdim(simptr sim,int dim) {
	if(sim->dim!=0) return 2;
	if(dim<1 || dim>DIMMAX) return 3;
	sim->dim=dim;
	return 0; }


/* simsettime */
int simsettime(simptr sim,double time,int code) {
	int er;
	static int timedefined=0;

	if(code==-1) return timedefined;
	else if(code==0) timedefined|=1;
	else if(code==1) timedefined|=2;
	else if(code==2) timedefined|=4;
	else if(code==3) timedefined|=8;
	else if(code==4) timedefined|=16;

	er=0;
	if(code==0) {
		sim->time=time;
		simsetvariable(sim,"time",time); }
	else if(code==1) sim->tmin=time;
	else if(code==2) sim->tmax=time;
	else if(code==3) {
		if(time>0) {
			sim->dt=time;
			molsetcondition(sim->mols,SCparams,0);
			rxnsetcondition(sim,-1,SCparams,0);
			surfsetcondition(sim->srfss,SCparams,0);
			scmdsetcondition(sim->cmds,1,0); }
		else er=2; }
	else if(code==4) sim->tbreak=time;
	else er=1;
	return er; }


/* simreadstring */
int simreadstring(simptr sim,ParseFilePtr pfp,const char *word,char *line2) {
	char nm[STRCHAR],nm1[STRCHAR],shapenm[STRCHAR],ch,rname[STRCHAR],fname[STRCHAR],pattern[STRCHAR];
	char str1[STRCHAR],str2[STRCHAR],str3[STRCHAR];
	char errstr[STRCHARLONG];
	int er,i,nmol,d,i1,s,c,ll,order,*index,ft;
	int rulelist[MAXORDER+MAXPRODUCT],r,ord,rct,prd,itct,prt,lt,detailsi[8];
	long int pserno,sernolist[MAXPRODUCT];
	double flt1,flt2,v1[DIMMAX*DIMMAX],v2[4],poslo[DIMMAX],poshi[DIMMAX],thick;
	enum MolecState ms,rctstate[MAXORDER];
	enum PanelShape ps;
	enum RevParam rpart;
	enum LightParam ltparam;
	enum SpeciesRepresentation replist[MAXORDER+MAXPRODUCT];
	rxnptr rxn;
	compartptr cmpt;
	surfaceptr srf;
	portptr port;
	filamentptr fil;
	filamenttypeptr filtype;
	filamentssptr filss;
	long int li1;
	listptrli lilist;
	listptrv vlist;

	int dim,nvar;
	char **varnames;
	double *varvalues;

	dim=sim->dim;
	nvar=sim->nvar;
	varnames=sim->varnames;
	varvalues=sim->varvalues;

	// variables

	if(!strcmp(word,"variable")) {								// variable
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"variable format: name = value");
		CHECKS(strcmp(nm,"time"),"'time' cannot be used as a variable name; it is pre-defined as the simulation time");
		CHECKS(strcmp(nm,"x") && strcmp(nm,"y") && strcmp(nm,"z") && strcmp(nm,"r"),"x, y, z, and r are reserved variable names");
		CHECKS(strokname(nm),"variable name has to start with a letter and then be alphanumeric with optional underscores");
		line2=strnword(line2,2);
		CHECKS(line2,"variable format: name = value");
		if(line2[0]=='=') {
			line2=strnword(line2,2);
			CHECKS(line2,"variable format: name = value"); }
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"cannot read variable value");
		er=simsetvariable(sim,nm,flt1);
		CHECKS(!er,"out of memory allocating variable space");
		CHECKS(!strnword(line2,2),"unexpected text following variable"); }

	// space and time

	else if(!strcmp(word,"dim")) {								// dim
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&d);
		CHECKS(itct==1,"dim needs to be an integer");
		er=simsetdim(sim,d);
		CHECKS(er!=2,"dim can only be entered once");
		CHECKS(er!=3,"dim has to be between 1 and 3");
		dim=sim->dim;
		CHECKS(!strnword(line2,2),"unexpected text following dim"); }

	else if(!strcmp(word,"boundaries")) {					// boundaries
		CHECKS(dim>0,"need to enter dim before boundaries");
		itct=strmathsscanf(line2,"%s %mlg %mlg",varnames,varvalues,nvar,nm,&flt1,&flt2);
		CHECKS(itct==3,"boundaries format: dimension position position [type]");
		if(!strcmp(nm,"0") || !strcmp(nm,"x")) d=0;
		else if(!strcmp(nm,"1") || !strcmp(nm,"y")) d=1;
		else if(!strcmp(nm,"2") || !strcmp(nm,"z")) d=2;
		else d=3;
		CHECKS(d>=0 && d<dim,"illegal boundaries dimension value");
		CHECKS(flt1<flt2,"the first boundary position needs to be smaller than the second one");
		line2=strnword(line2,4);
		ch='t';
		if(line2) {
			itct=sscanf(line2,"%c",&ch);
			CHECKS(itct==1,"boundary type character could not be read");
			CHECKS(ch=='r' || ch=='p' || ch=='a' || ch=='t',"boundaries type needs to be r, p, a, or t");
			line2=strnword(line2,2); }
		er=walladd(sim,d,0,flt1,ch);
		CHECKS(er!=1,"out of memory adding wall");
		CHECKS(er!=2,"SMOLDYN BUG: adding wall");
		er=walladd(sim,d,1,flt2,ch);
		CHECKS(er!=1,"out of memory adding wall");
		CHECKS(er!=2,"SMOLDYN BUG: adding wall");
		CHECKS(!line2,"unexpected text following boundaries"); }

	else if(!strcmp(word,"low_wall")) {						// low_wall
		CHECKS(dim>0,"need to enter dim before low_wall");
		itct=strmathsscanf(line2,"%s %mlg %c",varnames,varvalues,nvar,nm,&flt1,&ch);
		CHECKS(itct==3,"low_wall format: dimension position type");
		if(!strcmp(nm,"0") || !strcmp(nm,"x")) d=0;
		else if(!strcmp(nm,"1") || !strcmp(nm,"y")) d=1;
		else if(!strcmp(nm,"2") || !strcmp(nm,"z")) d=2;
		else d=3;
		CHECKS(d>=0 && d<dim,"low_wall dimension needs to be between 0 and dim-1");
		CHECKS(ch=='r' || ch=='p' || ch=='a' || ch=='t',"low_wall type needs to be r, p, a, or t");
		er=walladd(sim,d,0,flt1,ch);
		CHECKS(er!=1,"out of memory adding wall");
		CHECKS(er!=2,"SMOLDYN BUG: adding wall");
		CHECKS(!strnword(line2,4),"unexpected text following low_wall"); }

	else if(!strcmp(word,"high_wall")) {					// high_wall
		CHECKS(dim>0,"need to enter dim before high_wall");
		itct=strmathsscanf(line2,"%s %mlg %c",varnames,varvalues,nvar,nm,&flt1,&ch);
		CHECKS(itct==3,"high_wall format: dimension position type");
		if(!strcmp(nm,"0") || !strcmp(nm,"x")) d=0;
		else if(!strcmp(nm,"1") || !strcmp(nm,"y")) d=1;
		else if(!strcmp(nm,"2") || !strcmp(nm,"z")) d=2;
		else d=3;
		CHECKS(d>=0 && d<dim,"high_wall dimension needs to be between 0 and dim-1");
		CHECKS(ch=='r' || ch=='p' || ch=='a' || ch=='t',"high_wall type needs to be r, p, a, or t");
		er=walladd(sim,d,1,flt1,ch);
		CHECKS(er!=1,"out of memory adding wall");
		CHECKS(er!=2,"SMOLDYN BUG: adding wall");
		CHECKS(!strnword(line2,4),"unexpected text following high_wall"); }

	else if(!strcmp(word,"time_start")) {					// time_start
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"time_start needs to be a number");
		simsettime(sim,flt1,0);
		simsettime(sim,flt1,1);
		CHECKS(!strnword(line2,2),"unexpected text following time_start"); }

	else if(!strcmp(word,"time_stop")) {					// time_stop
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"time_stop needs to be a number");
		simsettime(sim,flt1,2);
		CHECKS(!strnword(line2,2),"unexpected text following time_stop"); }

	else if(!strcmp(word,"time_step")) {					// time_step
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"time_step needs to be a number");
		er=simsettime(sim,flt1,3);
		CHECKS(!er,"time step must be >0");
		CHECKS(!strnword(line2,2),"unexpected text following time_step"); }

	else if(!strcmp(word,"time_now")) {						// time_now
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"time_now needs to be a number");
		simsettime(sim,flt1,0);
		CHECKS(!strnword(line2,2),"unexpected text following time_now"); }

	// molecules

	else if(!strcmp(word,"max_species") || !strcmp(word,"max_names")) {		// max_species, max_names
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"max_species needs to be an integer");
		er=molenablemols(sim,i1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"cannot decrease the number of allocated species");
		CHECKS(!strnword(line2,2),"unexpected text following max_species"); }

	else if(!strcmp(word,"species") || !strcmp(word,"names") || !strcmp(word,"name")) {// species, names, name
		while(line2) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"failed to read species name");
			er=moladdspecies(sim,nm);
			CHECKS(er!=-1,"failed to allocate memory");
			CHECKS(er!=-4,"'empty' is not a permitted species name");
			CHECKS(er!=-5,"this species has already been declared");
			CHECKS(er!=-6,"'?' and '*' are not permitted in species names");
			line2=strnword(line2,2); }}

	else if(!strcmp(word,"species_group")) {			// species_group
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"species_group format: group species species ...");
		line2=strnword(line2,2);
		if(!line2) {
			er=moladdspeciesgroup(sim,nm,NULL,0);
			CHECKS(er!=-4,"species name not recognized");
			CHECKS(er!=-6,"cannot read wildcard logic");
			CHECKS(er!=-7,"error allocating memory");
			CHECKS(er!=-8,"molecule states are not permitted");
			CHECKS(er!=-9,"species group name cannot be a species name");
			CHECKS(!er,"BUG: failed to add species group"); }
		else
			while(line2) {
				itct=sscanf(line2,"%s",nm1);
				CHECKS(itct==1,"species_group format: group species species ...");
				er=moladdspeciesgroup(sim,nm,nm1,0);
				CHECKS(er!=-4,"species name not recognized");
				CHECKS(er!=-6,"cannot read wildcard logic");
				CHECKS(er!=-7,"error allocating memory");
				CHECKS(er!=-8,"molecule states are not permitted");
				CHECKS(!er,"BUG: failed to add species group");
				line2=strnword(line2,2); }}

	else if(!strcmp(word,"max_mol")) {						// max_mol
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"max_mol needs to be an integer");
		er=molsetmaxmol(sim,i1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=5,"more molecule already exist than are requested with max_mol");
		CHECKS(!strnword(line2,2),"unexpected text following max_mol"); }

	else if(!strcmp(word,"difc")) {								// difc
		CHECKS(sim->mols,"need to enter species before difc");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"difc format: species[(state)] value");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing data for difc");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"diffusion coefficient value cannot be read");
		CHECKS(flt1>=0,"diffusion coefficient needs to be >=0");
		molsetdifc(sim,0,index,ms,flt1);
		CHECKS(!strnword(line2,2),"unexpected text following difc"); }

	else if(!strcmp(word,"difc_rule")) {					// difc_rule
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"difc_rule format: species[(state)] value");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing data for difc_rule");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"diffusion coefficient value cannot be read");
		CHECKS(flt1>=0,"diffusion coefficient needs to be >=0");
		er=RuleAddRule(sim,RTdifc,NULL,pattern,&ms,NULL,flt1,NULL,NULL);
		CHECKS(!er,"out of memory in difc_rule");
		CHECKS(!strnword(line2,2),"unexpected text following difc_rule"); }

	else if(!strcmp(word,"difm")) {								// difm
		CHECKS(sim->mols,"need to enter species before difm");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"difm format: name[(state)] values");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing matrix in difm");
		for(d=0;d<dim*dim;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete matrix in difm");
			line2=strnword(line2,2); }
		CHECKS(molsetdifm(sim,0,index,ms,v1)==0,"out of memory in difm");
		CHECKS(!line2,"unexpected text following difm"); }

	else if(!strcmp(word,"difm_rule")) {					// difm_rule
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"difm_rule format: name[(state)] values");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing matrix in difm");
		for(d=0;d<dim*dim;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete matrix in difm");
			line2=strnword(line2,2); }
		er=RuleAddRule(sim,RTdifm,NULL,pattern,&ms,NULL,0,NULL,v1);
		CHECKS(!er,"out of memory in difm_rule");
		CHECKS(!line2,"unexpected text following difm_rule"); }

	else if(!strcmp(word,"drift")) {							// drift
		CHECKS(sim->mols,"need to enter species before drift");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"drift format: species[(state)] vector");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing vector in drift");
		for(d=0;d<dim;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete vector in drift");
			line2=strnword(line2,2); }
		CHECKS(molsetdrift(sim,0,index,ms,v1)==0,"out of memory in drift");
		CHECKS(!line2,"unexpected text following drift"); }

	else if(!strcmp(word,"drift_rule")) {					// drift_rule
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"drift_rule format: species[(state)] vector");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing vector in drift");
		for(d=0;d<dim;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete vector in drift");
			line2=strnword(line2,2); }
		er=RuleAddRule(sim,RTdrift,NULL,pattern,&ms,NULL,0,NULL,v1);
		CHECKS(!er,"out of memory in drift_rule");
		CHECKS(!line2,"unexpected text following drift_rule"); }

	else if(!strcmp(word,"surface_drift")) {				// surface_drift
		CHECKS(sim->dim>1,"system dimensionality needs to be 2 or 3");
		CHECKS(sim->mols,"need to enter species before surface_drift");
		CHECKS(sim->srfss,"need to enter surfaces before surface_drift");
		CHECKS(sim->srfss->nsrf>0,"at least one surface needs to be defined before surface_drift");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"surface_drift format: species(state) surface panel_shape values");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(ms!=MSsoln,"state needs to be surface-bound");
		CHECKS(line2=strnword(line2,2),"surface_drift format: species(state) surface panel_shape values");
		itct=sscanf(line2,"%s %s",nm,shapenm);
		CHECKS(itct==2,"surface_drift format: species(state) surface panel_shape values");
		if(!strcmp(nm,"all")) s=-1;
		else {
			s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
			CHECKS(s>=0,"surface name in surface_drift is not recognized"); }
		ps=surfstring2ps(shapenm);
		CHECKS(ps!=PSnone,"in surface_drift, panel shape name not recognized");
		CHECKS(line2=strnword(line2,3),"missing vector in surface_drift");
		for(d=0;d<dim-1;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete vector in surface_drift");
			line2=strnword(line2,2); }
		CHECKS(molsetsurfdrift(sim,0,index,ms,s,ps,v1)==0,"out of memory in surface_drift");
		CHECKS(!line2,"unexpected text following surface_drift"); }

	else if(!strcmp(word,"surface_drift_rule")) {				// surface_drift_rule
		CHECKS(sim->dim>1,"system dimensionality needs to be 2 or 3");
		CHECKS(sim->srfss,"need to enter surfaces before surface_drift_rule");
		CHECKS(sim->srfss->nsrf>0,"at least one surface needs to be defined before surface_drift_rule");
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"surface_drift_rule format: species(state) surface panel_shape values");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(ms!=MSsoln,"state needs to be surface-bound");
		CHECKS(line2=strnword(line2,2),"surface_drift_rule format: species(state) surface panel_shape values");
		itct=sscanf(line2,"%s %s",nm,shapenm);
		CHECKS(itct==2,"surface_drift_rule format: species(state) surface panel_shape values");
		if(!strcmp(nm,"all")) s=-1;
		else {
			s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
			CHECKS(s>=0,"surface name in surface_drift_rule is not recognized"); }
		ps=surfstring2ps(shapenm);
		CHECKS(ps!=PSnone,"in surface_drift_rule, panel shape name not recognized");
		CHECKS(line2=strnword(line2,3),"missing vector in surface_drift");
		for(d=0;d<dim-1;d++) {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
			CHECKS(itct==1,"incomplete vector in surface_drift_rule");
			line2=strnword(line2,2); }
		detailsi[0]=s;
		detailsi[1]=(int)ps;
		er=RuleAddRule(sim,RTsurfdrift,NULL,pattern,&ms,NULL,0,detailsi,v1);
		CHECKS(!er,"out of memory in surface_drift_rule");
		CHECKS(!line2,"unexpected text following surface_drift_rule"); }

	else if(!strcmp(word,"mol")) {								// mol
		CHECKS(sim->mols,"need to enter species before mol");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&nmol);
		CHECKS(itct==1,"mol format: number name position_vector");
		CHECKS(nmol>=0,"number of molecules added needs to be >=0");
		CHECKS(line2=strnword(line2,2),"mol format: number name position_vector");
		i=molstring2index1(sim,line2,&ms,NULL);
		CHECKS(i!=0,"wildcards and species groups are not permitted");
		CHECKS(i!=-1,"error reading molecule name");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"'all' species is not permitted");
		CHECKS(i!=-6,"cannot read wildcard logic");
		CHECKS(i!=-7,"out of memory");
		CHECKS(ms==MSsoln,"state needs to be solution");
		if(sim->wlist)
			systemcorners(sim,poslo,poshi);
		else
			for(d=0;d<dim;d++) {poslo[d]=0;poshi[d]=1;}
		for(d=0;d<dim;d++) {
			CHECKS(line2=strnword(line2,2),"insufficient position data for mol");
			if(line2[0]=='u') {
				CHECKS(sim->wlist,"need to enter boundaries before using 'u' in a mol statement"); }
			else {
				itct=sscanf(line2,"%lg-%lg",&flt1,&flt2);
				if(itct==2) {
					poslo[d]=flt1;
					poshi[d]=flt2; }
				else {
					itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
					CHECKS(itct==1,"cannot read position value for mol");
					poslo[d]=poshi[d]=flt1; }}}
		er=addmol(sim,nmol,i,poslo,poshi,0);
		CHECKS(!er,"more molecules assigned than permitted with max_mol");
		CHECKS(!strnword(line2,2),"unexpected text following mol"); }

	else if(!strcmp(word,"surface_mol")) {				// surface_mol
		CHECKS(sim->mols,"need to enter species before surface_mol");
		CHECKS(sim->srfss,"surfaces need to be defined before surface_mol statement");
		CHECKS(sim->srfss->nsrf>0,"at least one surface needs to be defined before surface_mol");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&nmol);
		CHECKS(itct==1,"surface_mol format: nmol species(state) surface panel_shape panel_name [position]");
		CHECKS(nmol>=0,"in surface_mol, the number of molecules needs to be at least 0");
		CHECKS(line2=strnword(line2,2),"surface_mol format: nmol species(state) surface panel_shape panel_name [position]");
		i=molstring2index1(sim,line2,&ms,NULL);
		CHECKS(i!=0,"wildcards and species groups are not permitted");
		CHECKS(i!=-1,"error reading molecule name");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"'all' species is not permitted");
		CHECKS(i!=-6,"cannot read wildcard logic");
		CHECKS(i!=-7,"out of memory");
		CHECKS(ms<MSMAX && ms!=MSsoln,"state needs to be front, back, up, or down");
		CHECKS(line2=strnword(line2,2),"surface_mol format: nmol species(state) surface panel_shape panel_name [position]");
		itct=sscanf(line2,"%s %s %s",nm,shapenm,nm1);
		CHECKS(itct==3,"surface_mol format: nmol species(state) surface panel_shape panel_name [position]");
		if(!strcmp(nm,"all")) s=-1;
		else {
			s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
			CHECKS(s>=0,"surface name in surface_mol is not recognized"); }
		ps=surfstring2ps(shapenm);
		CHECKS(ps!=PSnone,"in surface_mol, panel shape name not recognized");
		line2=strnword(line2,4);
		if(line2) {
			for(d=0;d<dim;d++) {
				itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
				CHECKS(itct==1,"incomplete vector in drift");
				line2=strnword(line2,2); }
			CHECKS(s>=0 && ps!=PSall && strcmp(nm1,"all"),"in surface_mol, use of coordinates requires that a specific panel be specified");
			er=addsurfmol(sim,nmol,i,ms,v1,NULL,s,ps,nm1);
			CHECKS(er!=1,"in surface_mol, unable to allocate temporary storage space");
			CHECKS(er!=2,"in surface_mol, panel name not recognized");
			CHECKS(er!=3,"not enough molecules permitted by max_mol"); }
		else {
			er=addsurfmol(sim,nmol,i,ms,NULL,NULL,s,ps,nm1);
			CHECKS(er!=1,"in surface_mol, unable to allocate temporary storage space");
			CHECKS(er!=2,"in surface_mol, no panels match the given description");
			CHECKS(er!=3,"not enough molecules permitted by max_mol"); }
		CHECKS(!line2,"unexpected text following surface_mol"); }

	else if(!strcmp(word,"compartment_mol")) {		// compartment_mol
		CHECKS(sim->mols,"need to enter species before compartment_mol");
		CHECKS(sim->cmptss,"compartments need to be defined before compartment_mol statement");
		CHECKS(sim->cmptss->ncmpt>0,"at least one compartment needs to be defined before compartment_mol");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&nmol);
		CHECKS(itct==1,"compartment_mol format: nmol species compartment");
		CHECKS(nmol>=0,"the number of molecules needs to be at least 0");
		CHECKS(line2=strnword(line2,2),"compartment_mol format: nmol species compartment");
		i=molstring2index1(sim,line2,&ms,NULL);
		CHECKS(i!=0,"wildcards and species groups are not permitted");
		CHECKS(i!=-1,"error reading molecule name");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"'all' species is not permitted");
		CHECKS(i!=-6,"cannot read wildcard logic");
		CHECKS(i!=-7,"out of memory");
		CHECKS(ms==MSsoln,"state needs to be solution");
		CHECKS(line2=strnword(line2,2),"compartment_mol format: nmol species compartment");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"compartment_mol format: nmol species compartment");
		c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
		CHECKS(c>=0,"compartment name is not recognized");
		er=addcompartmol(sim,nmol,i,sim->cmptss->cmptlist[c]);
		CHECKS(er!=2,"compartment volume is zero or nearly zero");
		CHECKS(er!=3,"not enough molecules permitted by max_mol");
		CHECKS(!strnword(line2,2),"unexpected text following compartment_mol"); }

	else if(!strcmp(word,"molecule_lists")) {			// molecule_lists
		while(line2) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"unable to read molecule list name");
			er=addmollist(sim,nm,MLTsystem);
			CHECKS(er!=-1,"SMOLDYN BUG: out of memory");
			CHECKS(er!=-2,"molecule list name has already been used");
			CHECKS(er!=-3,"SMOLDYN BUG: illegal addmollist inputs");
			line2=strnword(line2,2); }
		CHECKS(!line2,"unexpected text following molecule_lists"); }

	else if(!strcmp(word,"mol_list")) {						// mol_list
		CHECKS(sim->mols && sim->mols->nlist>0,"need to enter molecule_lists before mol_list");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"mol_list format: species[(state)] list_name");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing list name for mol_list");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"mol_list format: name[(state)] list_name");
		ll=stringfind(sim->mols->listname,sim->mols->nlist,nm);
		CHECKS(ll>=0,"molecule list name is not recognized");
		molsetlistlookup(sim,0,index,ms,ll);
		CHECKS(!strnword(line2,2),"unexpected text following mol_list"); }

	else if(!strcmp(word,"mol_list_rule")) {				// mol_list_rule
		CHECKS(sim->mols && sim->mols->nlist>0,"need to enter molecule_lists before mol_list");
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"mol_list_pattern format: species[(state)] list_name");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing list name for mol_list");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"mol_list_pattern format: name[(state)] list_name");
		ll=stringfind(sim->mols->listname,sim->mols->nlist,nm);
		CHECKS(ll>=0,"molecule list name is not recognized");
		detailsi[0]=ll;
		er=RuleAddRule(sim,RTmollist,NULL,pattern,&ms,NULL,0,detailsi,NULL);
		CHECKS(!er,"out of memory in mol_list_rule");
		CHECKS(!strnword(line2,2),"unexpected text following mol_list_rule"); }

	// graphics

	else if(!strcmp(word,"graphics")) {						// graphics
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"missing graphics parameter");
		er=graphicsenablegraphics(sim,nm);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"graphics method not recognized");
		CHECKS(!strnword(line2,2),"unexpected text following graphics"); }

	else if(!strcmp(word,"graphic_iter")) {				// graphic_iter
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"graphic_iter need to be a number");
		er=graphicssetiter(sim,i1);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"graphic_iter needs to be >=1");
		CHECKS(!strnword(line2,2),"unexpected text following graphic_iter"); }

	else if(!strcmp(word,"graphic_delay")) {			// graphic_delay
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"graphic_delay need to be a number");
		er=graphicssetdelay(sim,i1);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"graphic_delay needs to be >=0");
		CHECKS(!strnword(line2,2),"unexpected text following graphic_delay"); }

	else if(!strcmp(word,"quit_at_end")) {				// quit_at_end
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"quit_at_end format: yes or no");
		if(strbegin(nm,"yes",0) || !strcmp(nm,"1")) sim->quitatend=1;
		else if(strbegin(nm,"no",0) || !strcmp(nm,"0")) sim->quitatend=0;
		else CHECKS(0,"cannot read quit_at_end value");
		CHECKS(!strnword(line2,2),"unexpected text following quit_at_end"); }

	else if(!strcmp(word,"frame_thickness")) {		// frame_thickness
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"frame_thickness needs to be a number");
		er=graphicssetframethickness(sim,flt1);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"frame_thickness needs to be >=0");
		CHECKS(!strnword(line2,2),"unexpected text following frame_thickness"); }

	else if(!strcmp(word,"frame_color") || !strcmp(word,"frame_colour")) {				// frame_color
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=graphicssetframecolor(sim,v2);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"frame_color values need to be between 0 and 1");
		CHECKS(!line2,"unexpected text following frame_color"); }

	else if(!strcmp(word,"grid_thickness")) {		// grid_thickness
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"grid_thickness needs to be a number");
		er=graphicssetgridthickness(sim,flt1);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"grid_thickness needs to be >=0");
		CHECKS(!strnword(line2,2),"unexpected text following grid_thickness"); }

	else if(!strcmp(word,"grid_color") || !strcmp(word,"grid_colour")) {					// grid_color
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=graphicssetgridcolor(sim,v2);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"grid_color values need to be between 0 and 1");
		CHECKS(!line2,"unexpected text following grid_color"); }

	else if(!strcmp(word,"background_color") || !strcmp(word,"background_colour")) {		// background_color
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=graphicssetbackcolor(sim,v2);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"background_color values need to be between 0 and 1");
		CHECKS(!line2,"unexpected text following background_color"); }

	else if(!strcmp(word,"text_color") || !strcmp(word,"text_colour")) {		// text_color
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=graphicssetbackcolor(sim,v2);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"text_color values need to be between 0 and 1");
		CHECKS(!line2,"unexpected text following text_color"); }

	else if(!strcmp(word,"text_display")) {				// text_display
		while(line2) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"error reading text_display item");
			er=graphicssettextitem(sim,nm);
			CHECKS(er!=1,"out of memory");
			CHECKS(er!=2,"unrecognized text display item (check that species have been defined)");
			line2=strnword(line2,2); }
		CHECKS(!line2,"unexpected text following text_display"); }

	else if(!strcmp(word,"light")) {							// light
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"light format: light_number parameter values");
		if(!strcmp(nm,"global") || !strcmp(nm,"room")) lt=-1;
		else {
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&lt);
			CHECKS(lt>=0 && lt<MAXLIGHTS,"light number out of bounds"); }
		line2=strnword(line2,2);
		CHECKS(line2,"light format: light_number parameter values");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"unable to read lighting parameter");
		ltparam=graphicsstring2lp(nm);
		CHECKS(ltparam!=LPnone,"unrecognized light parameter");
		line2=strnword(line2,2);
		if(ltparam==LPon || ltparam==LPoff || ltparam==LPauto) v2[0]=0;
		else {
			CHECKS(line2,"light format: light_number parameter values");
			itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&v2[0],&v2[1],&v2[2]);
			CHECKS(itct==3,"light is missing one or more values");
			v2[3]=1;
			line2=strnword(line2,4);
			if(line2) {
				itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v2[3]);
				CHECKS(itct==1,"failed to read alpha light value");
				line2=strnword(line2,2); }
			if(ltparam!=LPposition) {
				for(i1=0;i1<4;i1++)
					CHECKS(v2[i1]>=0 && v2[i1]<=1,"light color values need to be between 0 and 1"); }}
		er=graphicssetlight(sim,NULL,lt,ltparam,v2);
		CHECKS(er!=1,"out of memory");
		CHECKS(!er,"BUG: error in light statement");
		CHECKS(!line2,"unexpected text following light"); }

	else if(!strcmp(word,"display_size")) {				// display_size
		CHECKS(sim->mols,"need to enter species before display_size");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"display_size format: species[(state)] value");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing data for display_size");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"display_size format: name[(state)] size");
		CHECKS(flt1>=0,"display_size value needs to be >=0");
		molsetdisplaysize(sim,0,index,ms,flt1);
		CHECKS(!strnword(line2,2),"unexpected text following display_size"); }

	else if(!strcmp(word,"display_size_rule")) {	// display_size_rule
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"display_size_rule format: species[(state)] value");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		CHECKS(line2=strnword(line2,2),"missing data for display_size");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"display_size_rule format: name[(state)] size");
		CHECKS(flt1>=0,"display_size_rule value needs to be >=0");
		er=RuleAddRule(sim,RTdispsize,NULL,pattern,&ms,NULL,flt1,NULL,NULL);
		CHECKS(!er,"out of memory in display_size_rule");
		CHECKS(!strnword(line2,2),"unexpected text following display_size_rule"); }

	else if(!strcmp(word,"color") || !strcmp(word,"colour")) {							// color
		CHECKS(sim->mols,"need to enter species before color");
		er=molstring2index1(sim,line2,&ms,&index);
		CHECKS(er!=-1,"color format: species[(state)] color");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(er!=-4,"molecule name not recognized");
		CHECKS(er!=-6,"cannot read wildcard logic");
		CHECKS(er!=-7,"error allocating memory");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		line2=strnword(line2,2);
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		molsetcolor(sim,0,index,ms,v2);
		CHECKS(!line2,"unexpected text following color"); }

	else if(!strcmp(word,"color_rule") || !strcmp(word,"colour_rule")) {		// color_rule
		er=molstring2pattern(line2,&ms,pattern,0);
		CHECKS(er!=-1,"color format: species[(state)] color");
		CHECKS(er!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(er!=-3,"cannot read molecule state value");
		CHECKS(ms<MSMAX || ms==MSall,"invalid state");
		line2=strnword(line2,2);
		er=graphicsreadcolor(&line2,v2);
		CHECKS(er!=3,"color values need to be between 0 and 1");
		CHECKS(er!=4,"color name not recognized");
		CHECKS(er!=6,"alpha values need to be between 0 and 1");
		CHECKS(er==0,"format is either 3 numbers or color name, and then optional alpha value");
		er=RuleAddRule(sim,RTcolor,NULL,pattern,&ms,NULL,0,NULL,v2);
		CHECKS(!er,"out of memory in color_rule");
		CHECKS(!line2,"unexpected text following color_rule"); }

	else if(!strcmp(word,"tiff_iter")) {				// tiff_iter
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"tiff_iter needs to be a number");
		er=graphicssettiffiter(sim,i1);
		CHECKS(er!=1,"out of memory enabling graphics");
		CHECKS(er!=2,"BUG: missing parameter");
		CHECKS(er!=3,"tiff_iter needs to be >=1");
		CHECKS(!strnword(line2,2),"unexpected text following tiff_iter"); }

	else if(!strcmp(word,"tiff_name")) {					// tiff_name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"tiff_name needs to be a string");
		strcpy(nm1,sim->filepath);
		CHECKS(strlen(nm)<STRCHAR-strlen(nm1),"tiff name is too long");
		strcat(nm1,nm);
		gl2SetOptionStr("TiffName",nm1);
		CHECKS(!strnword(line2,2),"unexpected text following tiff_name"); }

	else if(!strcmp(word,"tiff_min")) {					// tiff_min
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"tiff_min needs to be a number");
		CHECKS(i1>=0,"tiff_min has to be at least 0");
		gl2SetOptionInt("TiffNumber",i1);
		CHECKS(!strnword(line2,2),"unexpected text following tiff_min"); }

	else if(!strcmp(word,"tiff_max")) {					// tiff_max
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"tiff_max needs to be a number");
		CHECKS(i1>=0,"tiff_max has to be at least 0");
		gl2SetOptionInt("TiffNumMax",i1);
		CHECKS(!strnword(line2,2),"unexpected text following tiff_max"); }

	// about runtime commands

	else if(!strcmp(word,"output_root")) {				// output_root
		er=scmdsetfroot(sim->cmds,line2);
		CHECKS(er!=-1,"SMOLDYN BUG: scmdsetfroot"); }

	else if(!strcmp(word,"output_files") || !strcmp(word,"output_file")) {				// output_files
		er=scmdsetfnames(sim->cmds,line2,0);
		CHECKS(er!=1,"out of memory in output_files");
		CHECKS(er!=2,"error reading file name");
		CHECKS(er!=4,"BUG: variable cmds became NULL"); }

	else if(!strcmp(word,"output_data")) {				// output_data
		er=scmdsetdnames(sim->cmds,line2);
		CHECKS(er!=1,"out of memory in output_data");
		CHECKS(er!=2,"error reading data variable name");
		CHECKS(er!=4,"BUG: variable cmds became NULL"); }

	else if(!strcmp(word,"output_precision")) {		// output_precision
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"format for output_precision: value");
		scmdsetprecision(sim->cmds,i1);
		CHECKS(!strnword(line2,2),"unexpected text following output_precision"); }

	else if(!strcmp(word,"output_format")) {			// output_format
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"format for output_format: csv or ssv");
		er=scmdsetoutputformat(sim->cmds,nm);
		CHECKS(!er,"output_format value not recognized");
		CHECKS(!strnword(line2,2),"unexpected text following output_format"); }

	else if(!strcmp(word,"append_files")) {				// append_files
		er=scmdsetfnames(sim->cmds,line2,1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"error reading file name");
		CHECKS(er!=4,"BUG: variable cmds became NULL"); }

	else if(!strcmp(word,"output_file_number")) {	// output_file_number
		itct=strmathsscanf(line2,"%s %mi",varnames,varvalues,nvar,nm,&i1);
		CHECKS(itct==2,"format for output_file_number: filename number");
		CHECKS(i1>=0,"output_file_number needs to be >= 0");
		er=scmdsetfsuffix(sim->cmds,nm,i1);
		CHECKS(!er,"error setting output_file_number");
		CHECKS(!strnword(line2,3),"unexpected text following output_file_number"); }

	else if(!strcmp(word,"cmd")) {								// cmd
		er=scmdstr2cmd(sim->cmds,line2,varnames,varvalues,nvar);
		CHECKS(er!=1,"out of memory in cmd");
		CHECKS(er!=2,"BUG: no command superstructure for cmd");
		CHECKS(er!=3,"cmd format: type [on off dt] string");
		CHECKS(er!=6,"command timing type character not recognized"); }

	else if(!strcmp(word,"max_cmd")) {						// max_cmd
		CHECKS(0,"max_cmd is an obsolete statement. It simply needs to be removed."); }

	// surfaces

	else if(!strcmp(word,"max_surface")) {				// max_surface
		CHECKS(dim>0,"need to enter dim before max_surface");
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"max_surface needs to be a number");
		CHECKS(i1>0,"max_surface must be greater than 0");
		CHECKS(surfenablesurfaces(sim,i1)==0,"failed to enable surfaces");
		CHECKS(!strnword(line2,2),"unexpected text following max_surface"); }

	else if(!strcmp(word,"new_surface")) {				// new_surface
		CHECKS(dim>0,"need to enter dim before new_surface");
		srf=surfreadstring(sim,pfp,NULL,"name",line2);
		CHECK(srf!=NULL); }

	else if(!strcmp(word,"surface")) {						// surface
		CHECKS(dim>0,"need to enter dim before surface");
		CHECKS(sim->srfss,"individual surfaces need to be defined before using surface");
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"surface format: surface_name statement_name statement_text");
		line2=strnword(line2,3);
		CHECKS(line2,"surface format: surface_name statement_name statement_text");
		s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
		CHECKS(s>=0,"surface is unrecognized");
		srf=sim->srfss->srflist[s];
		srf=surfreadstring(sim,pfp,srf,nm1,line2);
		CHECK(srf!=NULL); }

	// compartments

	else if(!strcmp(word,"max_compartment")) {		// max_compartment
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"max_compartment needs to be a number");
		CHECKS(i1>=0,"max_compartment must be at least 0");
		CHECKS(!compartenablecomparts(sim,i1),"failed to enable compartments");
		CHECKS(!strnword(line2,2),"unexpected text following max_compartment"); }

	else if(!strcmp(word,"new_compartment")) {		// new_compartment
		cmpt=compartreadstring(sim,pfp,NULL,"name",line2);
		CHECK(cmpt!=NULL); }

	else if(!strcmp(word,"compartment")) {				// compartment
		CHECKS(sim->cmptss,"individual compartments need to be defined before using compartment");
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"compartment format: compart_name statement_name statement_text");
		line2=strnword(line2,3);
		CHECKS(line2,"compartment format: compart_name statement_name statement_text");
		c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
		CHECKS(c>=0,"compartment is unrecognized");
		cmpt=sim->cmptss->cmptlist[c];
		cmpt=compartreadstring(sim,pfp,cmpt,nm1,line2);
		CHECK(cmpt!=NULL); }

	// ports

	else if(!strcmp(word,"max_port")) {						// max_port
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"max_port needs to be a number");
		CHECKS(i1>=0,"max_port must be at least 0");
		CHECKS(portenableports(sim,i1),"failed to enable ports");
		CHECKS(!strnword(line2,2),"unexpected text following max_port"); }

	else if(!strcmp(word,"new_port")) {					// new_port
		port=portreadstring(sim,pfp,NULL,"name",line2);
		CHECK(port!=NULL); }

	else if(!strcmp(word,"port")) {							// port
		CHECKS(sim->portss,"individual ports need to be defined before using port");
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"port format: port_name statement_name statement_text");
		line2=strnword(line2,3);
		CHECKS(line2,"port format: port_name statement_name statement_text");
		prt=stringfind(sim->portss->portnames,sim->portss->nport,nm);
		CHECKS(prt>=0,"port is unrecognized");
		port=sim->portss->portlist[prt];
		port=portreadstring(sim,pfp,port,nm1,line2);
		CHECK(port!=NULL); }

	// filaments

	else if(!strcmp(word,"max_filament")) {				// max_filament
		CHECKS(0,"max_filament has been deprecated"); }

/*
	else if(!strcmp(word,"new_filament")) {				// new_filament
		fil=filreadstring(sim,pfp,NULL,"name",line2);
		CHECK(fil!=NULL); }

	else if(!strcmp(word,"filament")) {						// filament
		CHECKS(sim->filss,"individual filaments need to be defined before using filament");
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"filament format: filament_name statement_name statement_text");
		line2=strnword(line2,3);
		CHECKS(line2,"filament format: filament_name statement_name statement_text");
		f=stringfind(sim->filss->fnames,sim->filss->nfil,nm);
		CHECKS(f>=0,"filament is unrecognized");
		fil=sim->filss->fillist[f];
		fil=filreadstring(sim,pfp,fil,nm1,line2);
		CHECK(fil!=NULL); }
*/

	else if(!strcmp(word,"random_filament")) {		// random_filament
		CHECKS(sim->filss,"need to enter a filament type before random_filament");
		filss=sim->filss;
		itct=sscanf(line2,"%s %s",nm,nm1);
		CHECKS(itct==2,"random_filament format: name type segments [x y z]");
		ft=stringfind(filss->ftnames,filss->ntype,nm1);
		CHECKS(ft>=0,"filament type is unknown");
		filtype=filss->filtypes[ft];
		fil=filAddFilament(filtype,NULL,nm);
		CHECKS(fil,"unable to add filament to simulation");
		line2=strnword(line2,3);

		CHECKS(line2,"random_filament format: name type segments [x y z]");
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
		if(filtype->isbead)
			er=filAddRandomBeads(fil,i1,str1,str2,str3);
		else
			er=filAddRandomSegments(fil,i1,str1,str2,str3,thick);
		CHECKS(er!=2,"random_segments positions need to be 'u' or value");
		CHECKS(er==0,"BUG: error in filAddRandomsegments");
		CHECKS(!line2,"unexpected text following random_segments"); }


	// reactions

	else if(!strcmp(word,"reaction") || !strcmp(word,"reaction_rule") || !strcmp(word,"reaction_cmpt") || !strcmp(word,"reaction_surface")) {	// reaction, reaction_rule, reaction_cmpt, reaction_surface
		CHECKS(sim->mols,"need to enter species before reaction");
		CHECKS(sim->dim>0,"need to enter system dimensionality before reaction");
		er=rxnparsereaction(sim,word,line2,errstr);
		CHECKS(!er,"%s",errstr); }

#ifdef OPTION_VCELL
	else if(!strcmp(word,"reaction_rate")) {				// reaction_rate
		if(line2){
			stringstream ss(line2);
			ss >> rname;
			string rawStr, expStr;
			getline(ss, rawStr);
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
			else {
				itct=sscanf(line2,"%s %lg",rname,&flt1);
				CHECKS(itct==2,"reaction_rate format: rname rate");
				r=readrxnname(sim,rname,&order,&rxn,NULL,1);
				CHECKS(r>=0,"unrecognized reaction name");
				er=RxnSetValue(sim,"rate",rxn,flt1);
				CHECKS(er!=4,"reaction rate value must be non-negative");
				CHECKS(!strnword(line2,3),"unexpected text following reaction"); }}}
#else

	else if(!strcmp(word,"reaction_rate")) {				// reaction_rate
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"reaction_rate format: rname rate");
		CHECKS(flt1>=0,"reaction rate value must be non-negative");
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0)
			RxnSetValue(sim,"rate",rxn,flt1);
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"rate",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0)
			RxnSetValue(NULL,"rate",rxn,flt1);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following reaction_rate"); }
#endif

	else if(!strcmp(word,"reaction_multiplicity")) {	// reaction_multiplicity
		itct=strmathsscanf(line2,"%s %mi",varnames,varvalues,nvar,rname,&i1);
		CHECKS(itct==2,"reaction_multiplicity format: rname multiplicity");
		CHECKS(i1>=0,"reaction multiplicity value must be non-negative");
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0)
			RxnSetValue(sim,"multiplicity",rxn,(double)i1);
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"multiplicity",(rxnptr) vlist->xs[i],(double)i1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0)
			RxnSetValue(NULL,"multiplicity",rxn,(double)i1);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following reaction_multiplicity"); }

	else if(!strcmp(word,"confspread_radius")) {		// confspread_radius
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"confspread_radius format: rname radius");
		CHECKS(flt1>=0,"confspread radius value must be non-negative");
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0)
			RxnSetValue(sim,"confspreadrad",rxn,flt1);
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"confspreadrad",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0)
			RxnSetValue(NULL,"confspreadrad",rxn,flt1);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following confspread_radius"); }

	else if(!strcmp(word,"binding_radius")) {		// binding_radius
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"binding_radius format: rname radius");
		CHECKS(flt1>=0,"binding radius value must be non-negative");
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0)
			RxnSetValue(sim,"bindrad",rxn,flt1);
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"bindrad",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0)
			RxnSetValue(NULL,"bindrad",rxn,flt1);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following binding_radius"); }

	else if(!strcmp(word,"reaction_probability")) {		// reaction_probability
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"reaction_probability format: rname value");
		CHECKS(flt1>=0 && flt1<=1,"probability value must be between 0 and 1");
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0)
			RxnSetValue(sim,"prob",rxn,flt1);
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"prob",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0)
			RxnSetValue(NULL,"prob",rxn,flt1);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following reaction_probability"); }

	else if(!strcmp(word,"reaction_chi")) {			// reaction_chi
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"reaction_chi format: rname value");
		CHECKS(flt1!=0 && flt1<1,"reaction chi value must be between 0 and 1 (or -1 to disable)");
		rxn=NULL;
		r=readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(r>=0) {
			CHECKS(order==2,"reaction chi value is only allowed for order 2 reactions");
			RxnSetValue(sim,"chi",rxn,flt1); }
		r=readrxnname(sim,rname,&order,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			CHECKS(order==2,"reaction chi value is only allowed for order 2 reactions");
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"chi",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			CHECKS(order==2,"reaction chi value is only allowed for order 2 reactions");
			RxnSetValue(NULL,"chi",rxn,flt1); }
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following reaction_chi"); }

	else if(!strcmp(word,"reaction_production")) {		// reaction_production
		itct=strmathsscanf(line2,"%s %mlg",varnames,varvalues,nvar,rname,&flt1);
		CHECKS(itct==2,"reaction_production format: rname value");
		CHECKS(flt1>=0 && flt1<=1,"production value must be between 0 and 1");
		rxn=NULL;
		r=readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(r>=0) {
			CHECKS(order==0,"production is only allowed for order 0 reactions");
			RxnSetValue(sim,"prob",rxn,flt1); }
		r=readrxnname(sim,rname,&order,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			CHECKS(order==0,"production is only allowed for order 0 reactions");
			for(i=0;i<vlist->n;i++)
				RxnSetValue(sim,"prob",(rxnptr) vlist->xs[i],flt1);
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			CHECKS(order==0,"production is only allowed for order 0 reactions");
			RxnSetValue(NULL,"prob",rxn,flt1); }
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(!strnword(line2,3),"unexpected text following reaction_production"); }

	else if(!strcmp(word,"reaction_permit")) {			// reaction_permit
		itct=sscanf(line2,"%s",rname);
		CHECKS(itct==1,"missing reaction name");
		r=readrxnname(sim,rname,&order,&rxn,NULL,1);
		CHECKS(r>=0,"unrecognized reaction name");
		CHECKS(order>0,"reaction_permit is not allowed for order 0 reactions");
		for(ord=0;ord<order;ord++) {
			CHECKS(line2=strnword(line2,2),"missing state term in reaction_permit");
			itct=sscanf(line2,"%s",nm1);
			rctstate[ord]=molstring2ms(nm1); }
		RxnSetPermit(sim,rxn,order,rctstate,1);
		CHECKS(!strnword(line2,3),"unexpected text following reaction_permit"); }

	else if(!strcmp(word,"reaction_forbid")) {			// reaction_forbid
		itct=sscanf(line2,"%s",rname);
		CHECKS(itct==1,"missing reaction name");
		r=readrxnname(sim,rname,&order,&rxn,&vlist,1);
		CHECKS(r>=0,"unrecognized reaction name");
		CHECKS(order>0,"reaction_forbid is not allowed for order 0 reactions");
		for(ord=0;ord<order;ord++) {
			CHECKS(line2=strnword(line2,2),"missing state term in reaction_forbid");
			itct=sscanf(line2,"%s",nm1);
			rctstate[ord]=molstring2ms(nm1); }
		RxnSetPermit(sim,rxn,order,rctstate,0);
		CHECKS(!strnword(line2,3),"unexpected text following reaction_forbid"); }

	else if(!strcmp(word,"reaction_serialnum")) {				// reaction_serialnum
		itct=sscanf(line2,"%s",rname);
		CHECKS(itct==1,"reaction_serialnum format: rname rules_list");
		rxn=NULL;
		readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,2);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(rxn,"unrecognized reaction name");
		for(prd=0;prd<rxn->nprod;prd++) {
			sernolist[prd]=0;
			line2=strnword(line2,2);
			CHECKS(line2,"not enough product codes entered");
			itct=sscanf(line2,"%s",pattern);
			CHECKS(itct==1,"error reading a product code");
			if(!strcmp(pattern,"+")) prd--;
			else {
				pserno=rxnstring2sernocode(pattern,prd);
				CHECKS(pserno!=0,"error reading a product code");
				sernolist[prd]=pserno; }}
		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0) {
			er=RxnSetPrdSerno(rxn,sernolist);
			CHECKS(!er,"out of memory allocating product serial number list"); }
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++) {
				er=RxnSetPrdSerno((rxnptr) vlist->xs[i],sernolist);
				CHECKS(!er,"out of memory allocating product serial number list"); }
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			er=RxnSetPrdSerno(rxn,sernolist);
			CHECKS(!er,"out of memory allocating product serial number list"); }
		CHECKS(!strnword(line2,2),"unexpected text following reaction_serialnum"); }

	else if(!strcmp(word,"reaction_intersurface")) {												// reaction_intersurface
		itct=sscanf(line2,"%s",rname);
		CHECKS(itct==1,"reaction_intersurface format: rname rules_list");
		rxn=NULL;
		readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,2);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(rxn,"unrecognized reaction name");
		CHECKS(order==2,"reaction_intersurface is only for bimolecular reactions");
		if(rxn->nprod==0) {
			line2=strnword(line2,2);
			CHECKS(line2,"no value entered");
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"unable to read value");
			if(!strcmp(nm,"on")) rulelist[0]=0;
			else if(!strcmp(nm,"off")) rulelist[0]=-1;
			else CHECKS(0,"value not recognized"); }
		else {
			for(prd=0;prd<rxn->nprod;prd++) {
				line2=strnword(line2,2);
				CHECKS(line2,"not enough product codes entered");
				itct=sscanf(line2,"%s",nm);
				CHECKS(itct==1,"unable to read product code");
				if(prd==0 && !strcmp(nm,"off")) {
					rulelist[0]=-1;
					break; }
				if(!strcmp(nm,"+")) prd--;
				else if(nm[0]=='r') {
					if(nm[1]=='1') rulelist[prd]=1;
					else if(nm[1]=='2') rulelist[prd]=2;
					else CHECKS(0,"error reading a reactant code"); }
				else
					CHECKS(0,"error reading product rule"); }}

		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0) {
			er=RxnSetIntersurfaceRules(rxn,rulelist);
			CHECKS(er!=1,"out of memory allocating reaction intersurface rules"); }
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++) {
				er=RxnSetIntersurfaceRules((rxnptr) vlist->xs[i],rulelist);
				CHECKS(er!=1,"out of memory allocating reaction intersurface rules"); }
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			er=RxnSetIntersurfaceRules(rxn,rulelist);
			CHECKS(er!=1,"out of memory allocating reaction intersurface rules"); }
		CHECKS(!strnword(line2,2),"unexpected text following reaction_intersurface"); }

	else if(!strcmp(word,"reaction_representation")) {									// reaction_representation
		itct=sscanf(line2,"%s",rname);
		CHECKS(itct==1,"reaction_representation format: rname rules_list");
		rxn=NULL;
		readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,2);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(rxn,"unrecognized reaction name");
		for(rct=0;rct<order;rct++) {
			line2=strnword(line2,2);
			CHECKS(line2,"not enough reactant codes entered");
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"unable to read reactant code");
			if(!strcmp(nm,"+")) rct--;
			else {
				replist[rct]=rxnstring2sr(nm);
				CHECKS(replist[rct]!=SRnone,"error reading a reactant code"); }}
		line2=strnword(line2,2);
		CHECKS(line2,"missing reaction arrow");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"unable to find reaction arrow");
		CHECKS(!strcmp(nm,"->"),"unable to find reaction arrow");
		for(prd=0;prd<rxn->nprod;prd++) {
			line2=strnword(line2,2);
			CHECKS(line2,"not enough product codes entered");
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"unable to read product code");
			if(!strcmp(nm,"+")) prd--;
			else {
				replist[order+prd]=rxnstring2sr(nm);
				CHECKS(replist[order+prd]!=SRboth,"product representation cannot be 'both'");
				CHECKS(replist[order+prd]!=SRnone,"error reading a product code"); }}

		rxn=NULL;
		r=readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(r>=0) {
			er=RxnSetRepresentationRules(rxn,order,replist,replist+order);
			CHECKS(er!=1,"out of memory allocating reaction representation rules"); }
		r=readrxnname(sim,rname,&order,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++) {
				er=RxnSetRepresentationRules((rxnptr) vlist->xs[i],order,replist,replist+order);
				CHECKS(er!=1,"out of memory allocating reaction representation rules"); }
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			er=RxnSetRepresentationRules(rxn,order,replist,replist+order);
			CHECKS(er!=1,"out of memory allocating reaction representation rules"); }
		CHECKS(!strnword(line2,2),"unexpected text following reaction_representation"); }

	else if(!strcmp(word,"reaction_log")) {						// reaction_log
		itct=sscanf(line2,"%s %s %s",fname,rname,nm);
		CHECKS(itct==3,"reaction_log format: filename rxnname serial_numbers");
		lilist=NULL;
		if(!strcmp(nm,"all")) {
			lilist=ListAppendItemLI(NULL,-1);
			CHECKS(lilist,"out of memory in reaction_log");
			CHECKS(!strnword(line2,4),"unexpected text following reaction_log"); }
		else {
			line2=strnword(line2,3);
			lilist=ListReadStringLI(line2);
			CHECKS(lilist,"unable to read serial numbers"); }

		if(!strcmp(rname,"all")) {		// all current reactions, no rules
			er=RxnSetLog(sim,fname,NULL,lilist,1);
			CHECKS(er!=2,"cannot log a reaction to multiple output files");
			CHECKS(!er,"out or memory in reaction_log"); }
		else {
			rxn=NULL;
			r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
			if(r>=0) {
				er=RxnSetLog(sim,fname,rxn,lilist,1);
				CHECKS(er!=2,"cannot log a reaction to multiple output files");
				CHECKS(!er,"out or memory in reaction_log"); }
			r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
			CHECKS(r!=-2,"out of memory reading reaction name");
			if(r>=0) {
				for(i=0;i<vlist->n;i++) {
					er=RxnSetLog(sim,fname,(rxnptr) vlist->xs[i],lilist,1);
					CHECKS(er!=2,"cannot log a reaction to multiple output files");
					CHECKS(!er,"out or memory in reaction_log"); }
				ListFreeV(vlist); }
			r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
			CHECKS(r!=-2,"out of memory reading reaction name");
			if(r>=0) {
				er=RxnSetLog(NULL,fname,rxn,lilist,1);
				CHECKS(er!=2,"cannot log a reaction to multiple output files");
				CHECKS(!er,"out or memory in reaction_log"); }}
		ListFreeLI(lilist);
		lilist=NULL; }

	else if(!strcmp(word,"reaction_log_off")) {			// reaction_log_off
		itct=sscanf(line2,"%s %s",rname,nm);
		CHECKS(itct==2,"reaction_log_off format: rxnname serial_numbers");
		lilist=NULL;
		if(!strcmp(nm,"all")) {
			lilist=ListAppendItemLI(NULL,-1);
			CHECKS(lilist,"out of memory in reaction_log");
			CHECKS(!strnword(line2,3),"unexpected text following reaction_log_off"); }
		else {
			line2=strnword(line2,2);
			lilist=ListReadStringLI(line2);
			CHECKS(lilist,"unable to read serial numbers"); }

		if(!strcmp(rname,"all")) {
			er=RxnSetLog(sim,fname,NULL,lilist,0);
			CHECKS(!er,"out or memory in reaction_log"); }
		else {
			rxn=NULL;
			r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
			if(r>=0) {
				er=RxnSetLog(sim,NULL,rxn,lilist,0);
				CHECKS(!er,"out or memory in reaction_log_off"); }
			r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
			CHECKS(r!=-2,"out of memory reading reaction name");
			if(r>=0) {
				for(i=0;i<vlist->n;i++) {
					er=RxnSetLog(sim,NULL,(rxnptr) vlist->xs[i],lilist,0);
					CHECKS(!er,"out or memory in reaction_log_off"); }
				ListFreeV(vlist); }
			r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
			CHECKS(r!=-2,"out of memory reading reaction name");
			if(r>=0) {
				er=RxnSetLog(NULL,NULL,rxn,lilist,0);
				CHECKS(!er,"out or memory in reaction_log_off"); }}
		ListFreeLI(lilist);
		lilist=NULL; }

	else if(!strcmp(word,"product_placement")) {				// product_placement
		itct=sscanf(line2,"%s %s",rname,nm1);
		CHECKS(itct==2,"product_placement format: rname type parameters");
		rxn=NULL;
		readrxnname(sim,rname,&order,&rxn,NULL,1);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,2);
		if(!rxn) readrxnname(sim,rname,&order,&rxn,NULL,3);
		CHECKS(rxn,"unrecognized reaction name");
		rpart=rxnstring2rp(nm1);
		CHECKS(!(rpart==RPbounce && (order!=2 || rxn->nprod<2)),"bounce can only be used with two reactants and at least two products");
		flt1=0;
		prd=0;
		for(d=0;d<sim->dim;d++) v1[prd]=0;
		line2=strnword(line2,3);
		if(rpart==RPirrev);
		else if(rpart==RPbounce && !line2)
			flt1=-2;
		else if(rpart==RPpgem || rpart==RPbounce || rpart==RPpgemmax || rpart==RPpgemmaxw || rpart==RPratio || rpart==RPunbindrad || rpart==RPpgem2 || rpart==RPpgemmax2 || rpart==RPratio2) {
			CHECKS(line2,"missing parameter in product_placement");
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
			CHECKS(itct==1,"error reading parameter in product_placement");
			line2=strnword(line2,2); }
		else if(rpart==RPoffset || rpart==RPfixed) {
			CHECKS(line2,"missing parameters in product_placement");
			itct=sscanf(line2,"%s",nm1);
			CHECKS(itct==1,"format for product_placement: rname type parameters");
			if(strbegin("product_",nm1,0)) {
				itct=strmathsscanf(nm1+8,"%mi",varnames,varvalues,nvar,&prd);
				CHECKS(itct==1,"error reading product number");
				prd--;
				CHECKS(prd>=0 && prd<rxn->nprod,"illegal product number"); }
			else {
				i=molstring2index1(sim,nm1,&ms,&index);
				CHECKS(i!=0,"wildcards are not permitted in product_placement");
				CHECKS(i!=-1,"cannot read molecule species name");
				CHECKS(i!=-2,"mismatched or improper parentheses around product state");
				CHECKS(i!=-3,"cannot read molecule state value");
				CHECKS(i!=-4,"molecule name not recognized");
				CHECKS(i!=-5,"'all' is not allowed in product_placement");
				CHECKS(i!=-6,"cannot read wildcard logic");
				CHECKS(i!=-7,"error allocating memory");
				CHECKS(ms<MSMAX1 || ms==MSall,"invalid state");
				for(prd=0;prd<rxn->nprod && rxn->prdident[prd]!=i && rxn->prdstate[prd]!=ms;prd++);
				CHECKS(prd<rxn->nprod,"molecule in product_placement is not a product of this reaction"); }
			CHECKS(line2=strnword(line2,2),"position vector missing for product_placement");
			for(d=0;d<dim;d++) {
				itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[d]);
				CHECKS(itct==1,"insufficient data for position vector for product_placement");
				line2=strnword(line2,2); }}
		else CHECKS(0,"unrecognized or not permitted product placement parameter");

		rxn=NULL;
		r=readrxnname(sim,rname,NULL,&rxn,NULL,1);
		if(r>=0) {
			i1=RxnSetRevparam(sim,rxn,rpart,flt1,prd,v1,dim);
			CHECKS(i1!=2,"reversible parameter value is out of bounds"); }
		r=readrxnname(sim,rname,NULL,&rxn,&vlist,2);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			for(i=0;i<vlist->n;i++) {
				i1=RxnSetRevparam(sim,(rxnptr) vlist->xs[i],rpart,flt1,prd,v1,dim);
				CHECKS(i1!=2,"reversible parameter value is out of bounds"); }
			ListFreeV(vlist); }
		r=readrxnname(sim,rname,NULL,&rxn,NULL,3);
		CHECKS(r!=-2,"out of memory reading reaction name");
		if(r>=0) {
			i1=RxnSetRevparam(NULL,rxn,rpart,flt1,prd,v1,dim);
			CHECKS(i1!=2,"reversible parameter value is out of bounds"); }
		CHECKS(!line2,"unexpected text following product_placement"); }

	else if(!strcmp(word,"expand_rules")) {																	// expand_rules
		itct=sscanf(line2,"%s",nm);
		if(!strcmp(nm,"all")) i=-1;
		else if(!strcmp(nm,"otf") || !strcmp(nm,"on-the-fly")) i=-2;
		else {
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i);
			CHECKS(itct==1,"expand_rules format: iterations");
			CHECKS(i>=0,"iterations needs to be >=0"); }
		er=RuleExpandRules(sim,i);
		CHECKS(er!=-1 && er!=-11,"out of memory while expanding rules");
		CHECKS(er!=-5,"generated species name exceeded maximum allowed length of %i characters",STRCHAR);
		CHECKS(er!=-41,"no rules to expand");
		CHECKS(!er,"BUG: error number %i arose in RuleExpandRules function",er);
		CHECKS(!strnword(line2,2),"unexpected text following expand_rules"); }

	// optimizing runtime

	else if(!strcmp(word,"rand_seed") || !strcmp(word,"random_seed")) {			// rand_seed, random_seed
		itct=sscanf(line2,"%s",nm);
		if(!strcmp(nm,"time"))
			li1=(long int) time(NULL);
		else {
			itct=sscanf(line2,"%li",&li1);
			CHECKS(itct==1,"random_seed needs to be 'time' or an integer"); }
		Simsetrandseed(sim,li1);
		CHECKS(!strnword(line2,2),"unexpected text following random_seed"); }

	else if(!strcmp(word,"accuracy")) {						// accuracy
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"accuracy needs to be a number");
		sim->accur=flt1;
		CHECKS(!strnword(line2,2),"unexpected text following accuracy"); }

	else if(!strcmp(word,"molperbox")) {					// molperbox
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"molperbox needs to be a number");
		er=boxsetsize(sim,"molperbox",flt1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"molperbox needs to be >0");
		CHECKS(er!=3,"need to enter dim before molperbox");
		CHECKS(!strnword(line2,2),"unexpected text following molperbox"); }

	else if(!strcmp(word,"boxsize")) {						// boxsize
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"boxsize needs to be a number");
		er=boxsetsize(sim,"boxsize",flt1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"boxsize needs to be >0");
		CHECKS(er!=3,"need to enter dim before boxsize");
		CHECKS(!strnword(line2,2),"unexpected text following boxsize"); }

	else if(!strcmp(word,"gauss_table_size")) {		// gauss_table_size
		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
		CHECKS(itct==1,"gauss_table_size needs to be an integer");
		er=molssetgausstable(sim,i1);
		CHECKS(er!=1,"out of memory");
		CHECKS(er!=2,"need to enter max_species before gauss_table_size");
		CHECKS(er!=3,"gauss_table_size needs to be an integer power of two");
		CHECKS(!strnword(line2,2),"unexpected text following gauss_table_size"); }

	else if(!strcmp(word,"epsilon")) {						// epsilon
		CHECKS(dim>0,"need to enter dim before epsilon");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"epsilon format: value");
		er=surfsetepsilon(sim,flt1);
		CHECKS(er!=2,"out of memory");
		CHECKS(er!=3,"epsilon value needs to be at least 0");
		CHECKS(!strnword(line2,2),"unexpected text following epsilon"); }

	else if(!strcmp(word,"margin")) {						// margin
		CHECKS(dim>0,"need to enter dim before margin");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"margin format: value");
		er=surfsetmargin(sim,flt1);
		CHECKS(er!=2,"out of memory");
		CHECKS(er!=3,"margin value needs to be at least 0");
		CHECKS(!strnword(line2,2),"unexpected text following margin"); }

	else if(!strcmp(word,"neighbor_dist") || !strcmp(word,"neighbour_dist")) {			// neighbor_dist
		CHECKS(dim>0,"need to enter dim before neighbor_dist");
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
		CHECKS(itct==1,"neighbor_dist format: value");
		er=surfsetneighdist(sim,flt1);
		CHECKS(er!=2,"out of memory");
		CHECKS(er!=3,"neighdist value needs to be at least 0");
		CHECKS(!strnword(line2,2),"unexpected text following neighbor_dist"); }

	else {																				// unknown word
		CHECKS(0,"syntax error: statement not recognized"); }

	return 0;

 failure:
	simParseError(sim,pfp);
	return 1; }


#ifdef VCELL
int loadJMS(simptr sim,ParseFilePtr *pfpptr,char *line2,char *erstr);
#endif


/* loadsim */
int loadsim(simptr sim,const char *fileroot,const char *filename,const char *flags) {
	int done,pfpcode,er;
	char word[STRCHAR],*line2,errstring[STRCHARLONG];
	ParseFilePtr pfp;

	strncpy(sim->filepath,fileroot,STRCHAR);
	strncpy(sim->filename,filename,STRCHAR);
	strncpy(sim->flags,flags,STRCHAR);
	strcpy(SimFlags,flags);
	done=0;
	ErrorLineAndString[0]='\0';

	pfp=Parse_Start(fileroot,filename,errstring);
	CHECKS(pfp,"%s",errstring);
	er=Parse_CmdLineArg(NULL,NULL,pfp);
	CHECKMEM(!er);
#ifdef OPTION_VCELL
	sim->volumeSamplesPtr = NULL;//initialize the volumesample to null
#endif

	while(!done) {
		if(pfp->lctr==0 && !strchr(flags,'q'))
			simLog(sim,2," Reading file: '%s'\n",pfp->fname);
		pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
		CHECKS(pfpcode!=3,"%s",errstring);
		er=0;

		if(pfpcode==0);																// already taken care of

		else if(pfpcode==2) {													// end reading
			done=1; }

#ifdef VCELL
		else if(!strcmp(word,"start_jms")) {			// jms settings
			er=loadJMS(sim,&pfp,line2, errstring);
		}

		else if(!strcmp(word,"start_highResVolumeSamples")) {			//highResVolumeSamplesFile
			er=loadHighResVolumeSamples(sim,&pfp,line2); }
#endif

		else if(!strcmp(word,"start_reaction")) {			// start_reaction
			CHECKS(1,"reactions cannot be entered in blocks; this feature was deprecated"); }

		else if(!strcmp(word,"start_surface")) {			// start_surface
			CHECKS(sim->dim>0,"need to enter dim before start_surface");
			er=loadsurface(sim,&pfp,line2); }

		else if(!strcmp(word,"start_compartment")) {	// start_compartment
			CHECKS(sim->dim>0,"need to enter dim before start_compartment");
			er=loadcompart(sim,&pfp,line2); }

		else if(!strcmp(word,"start_port")) {					// start_port
			er=loadport(sim,&pfp,line2); }

		else if(!strcmp(word,"start_lattice")) {			// start_lattice
			er=loadlattice(sim,&pfp,line2); }

		else if(!strcmp(word,"start_bng")) {					// start_bng
			er=loadbng(sim,&pfp,line2);
			if(er) return 1;
			er=bngupdate(sim); }

		else if(!strcmp(word,"start_filament_type")) {	// start_filament_type
			er=filloadtype(sim,&pfp,line2); }

		else if(!strcmp(word,"start_filament")) {			// start_filament
			er=filloadfil(sim,&pfp,line2,NULL); }

		else if(!strcmp(word,"start_rules")) {				// start_rules
			CHECKS(0,"Moleculizer support has been discontinued in Smoldyn"); }

		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }

		else {
			er=simreadstring(sim,pfp,word,line2); }

		if(er) return 1; }

	return 0;

 failure:																					// failure
	simParseError(sim,pfp);
	return 1; }


/* simupdate */
int simupdate(simptr sim) {
	int er;
	static int recurs=0;

	if(sim->condition==SCok) {
		return 0; }
	if(recurs>10) {
		recurs=0;
		return 2; }
	recurs++;

	if(sim->condition==SCinit && sim->mols)
		simLog(sim,2," setting up molecules\n");
	er=molsupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit)
		simLog(sim,2," setting up virtual boxes\n");
	er=boxesupdate(sim);
	CHECK(er!=1);
	CHECKS(er!=3,"simulation dimensions or boundaries are undefined");

	er=molsort(sim,0);
	CHECK(er!=1);

	if(sim->condition==SCinit && sim->cmptss)
		simLog(sim,2," setting up compartments\n");
	er=compartsupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit && (sim->rxnss[0] || sim->rxnss[1] || sim->rxnss[2]))
		simLog(sim,2," setting up reactions\n");
	er=rxnsupdate(sim);
	CHECK(er!=1);
	CHECKS(er!=3,"failed to set up reactions");

	if(sim->condition==SCinit && sim->srfss)
		simLog(sim,2," setting up surfaces\n");
	er=surfupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit && sim->portss)
		simLog(sim,2," setting up ports\n");
	er=portsupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit && sim->latticess)
		simLog(sim,2," setting up lattices\n");
	er=latticesupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit && sim->filss)
		simLog(sim,2," setting up filaments\n");
	er=filsupdate(sim);
	CHECK(er!=1);

	if(sim->condition==SCinit && sim->graphss)
		simLog(sim,2," setting up graphics\n");
	er=graphicsupdate(sim);
	CHECK(er!=1);

	if(sim->mols && sim->mols->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->boxs && sim->boxs->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->cmptss && sim->cmptss->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->rxnss[0] && sim->rxnss[0]->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->rxnss[1] && sim->rxnss[1]->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->rxnss[2] && sim->rxnss[2]->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->srfss && sim->srfss->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->portss && sim->portss->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->filss && sim->filss->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }
	if(sim->graphss && sim->graphss->condition!=SCok) {
		er=simupdate(sim);
		CHECK(!er); }

	er=reassignmolecs(sim,0,0);
	CHECK(!er);

	if(sim->cmds && sim->cmds->condition!=3) {
		er=scmdupdatecommands(sim->cmds,sim->tmin,sim->tmax,sim->dt);
		CHECK(!er); }

	simsetcondition(sim,SCok,1);
	recurs=0;

	return 0;

 failure:
	if(ErrorType!=1) simLog(sim,10,"%s",ErrorString);
	return 1; }


/* simInitAndLoad */
#ifdef OPTION_VCELL
int simInitAndLoad(const char *fileroot,const char *filename,simptr *smptr,const char *flags, ValueProviderFactory* valueProviderFactory, AbstractMesh* mesh) {

	simptr sim;
	int er,qflag,sflag;

	sim=*smptr;
	if(!sim) {
		qflag=strchr(flags,'q')?1:0;
		sflag=strchr(flags,'s')?1:0;
		if(!qflag && !sflag) {
			simLog(NULL,2,"--------------------------------------------------------------\n");
			simLog(NULL,2,"Running Smoldyn %s\n",VERSION);
			simLog(NULL,2,"\nCONFIGURATION FILE\n");
			simLog(NULL,2," Path: '%s'\n",fileroot);
			simLog(NULL,2," Name: '%s'\n",filename); }
		sim=simalloc(fileroot);
		CHECKMEM(sim);
		sim->valueProviderFactory = valueProviderFactory; //create value provider factory
		sim->mesh = mesh;
		er=loadsim(sim,fileroot,filename,flags);		// load sim
		CHECK(!er);
		if(sim && sim->valueProviderFactory) {
			sim->valueProviderFactory->setSimptr(sim); }

		simLog(sim,2," Loaded file successfully\n"); }

	*smptr=sim;
	return 0;

failure:
	if(ErrorType!=1) simLog(sim,10,ErrorString);
	if(!*smptr) simfree(sim);
	return 1; }

#else


/* simInitAndLoad */
int simInitAndLoad(const char *fileroot,const char *filename,simptr *smptr,const char *flags) {
	simptr sim;
	int er,qflag,sflag;

	sim=*smptr;
	if(!sim) {
		qflag=strchr(flags,'q')?1:0;
		sflag=strchr(flags,'s')?1:0;
		if(!qflag && !sflag) {
			simLog(NULL,2,"--------------------------------------------------------------\n");
			simLog(NULL,2,"Running Smoldyn %s\n",VERSION);
			simLog(NULL,2,"\nCONFIGURATION FILE\n");
			simLog(NULL,2," Path: '%s'\n",fileroot);
			simLog(NULL,2," Name: '%s'\n",filename); }
		sim=simalloc(fileroot);
		CHECKMEM(sim);
		CHECKMEM(strloadmathfunctions()==0);
		CHECKMEM(loadsmolfunctions(sim)==0);
		er=loadsim(sim,fileroot,filename,flags);		// load sim
		CHECK(!er);
		simLog(sim,2," Loaded file successfully\n"); }

	*smptr=sim;
	return 0;

failure:
	if(ErrorType!=1) simLog(sim,10,ErrorString);
	if(!*smptr) simfree(sim);
	return 1; }

#endif


/* simUpdateAndDisplay */
int simUpdateAndDisplay(simptr sim) {
	int er;

	er=simupdate(sim);														// update sim
	CHECK(!er);

	simLog(sim,2,"\n");														// diagnostics output
	simsystemoutput(sim);
	er=checksimparams(sim);
  CHECK(!er);
	return 0;

 failure:
	return 1; }


/******************************************************************************/
/************************** core simulation functions *************************/
/******************************************************************************/


/* simdocommands */
int simdocommands(simptr sim) {
	int er;
	enum CMDcode ccode;

	ccode=scmdexecute(sim->cmds,sim->time,sim->dt,-1,0);
	er=simupdate(sim);
	if(er) return 8;
	er=molsort(sim,0);														// sort live and dead
	if(er) return 6;
	if(ccode==CMDstop || ccode==CMDabort) return 7;
	return 0; }


/* debugcode */
void debugcode(simptr sim,const char *prefix) {
	int m;
	moleculeptr mptr;
	char string[STRCHAR];

	if(sim->time<189.243 || sim->time>189.247) return;
	for(m=0;m<sim->mols->nl[0];m++) {
		mptr=sim->mols->live[0][m];
		if(mptr->serno==1377166 || mptr->serno==1374858) {
			printf("%s: time=%g serno=%s",prefix,sim->time,molserno2string(mptr->serno,string));
			printf(" posx=(%g,%g,%g)",mptr->posx[0],mptr->posx[1],mptr->posx[2]);
			printf(" pos=(%g,%g,%g)",mptr->pos[0],mptr->pos[1],mptr->pos[2]);
			printf(" pnl=%s",mptr->pnl?mptr->pnl->pname:"NULL");
			printf(" posx side=%s",surfface2string(panelside(mptr->posx,sim->srfss->srflist[4]->panels[PSdisk][0],3,NULL,1,0),string));
			printf(" pos side=%s",surfface2string(panelside(mptr->pos,sim->srfss->srflist[4]->panels[PSdisk][0],3,NULL,1,0),string));
			printf("\n"); }}
	return; }



/* simulatetimestep */
int simulatetimestep(simptr sim) {
	int er,ll;

	er=RuleExpandRules(sim,-3);											// expand any reaction rules if needed
	if(er && er!=-41) return 13;

	er=simupdate(sim);															// update any data structure changes
	if(er) return 8;

	er=(*sim->diffusefn)(sim);											// diffuse
	if(er) return 9;

	if(sim->srfss) {																// deal with surface or wall collisions
		for(ll=0;ll<sim->srfss->nmollist;ll++) {
			if(sim->srfss->srfmollist[ll] & SMLdiffuse) {
		    (*sim->surfacecollisionsfn)(sim,ll,0); }}
		for(ll=0;ll<sim->srfss->nmollist;ll++)				// surface-bound molecule actions
			if(sim->srfss->srfmollist[ll] & SMLsrfbound)
				(*sim->surfaceboundfn)(sim,ll); }
	else {
		if(sim->mols)
			for(ll=0;ll<sim->mols->nlist;ll++)
				if(sim->mols->diffuselist[ll])
					(*sim->checkwallsfn)(sim,ll,0,NULL); }

	er=(*sim->assignmols2boxesfn)(sim,1,0);					// assign to boxes (diffusing molecs., not reborn)
	if(er) return 2;

	er=molsort(sim,0);	// sort live and dead
	if(er) return 6;

	er=(*sim->zeroreactfn)(sim);
	if(er) return 3;

	er=(*sim->unimolreactfn)(sim);
	if(er) return 4;

	er=(*sim->bimolreactfn)(sim,0);
	if(er) return 5;
	er=(*sim->bimolreactfn)(sim,1);
	if(er) return 5;

	er=molsort(sim,0);																// sort live and dead
	if(er) return 6;

	if(sim->latticess) {
		er=latticeruntimestep(sim);
		if(er) return 12;
        er=molsort(sim,1);
        if(er) return 6; }

	if(sim->srfss) {
		for(ll=0;ll<sim->srfss->nmollist;ll++)
			(*sim->surfacecollisionsfn)(sim,ll,1); }			// surfaces again, reborn molecs. only
	else {
		if(sim->mols)
			for(ll=0;ll<sim->mols->nlist;ll++)
					(*sim->checkwallsfn)(sim,ll,1,NULL); }

	er=(*sim->assignmols2boxesfn)(sim,0,1);					// assign again (all, reborn)
	if(er) return 2;

	er=filDynamics(sim);
	if(er) return 11;

#ifdef ENABLE_PYTHON_CALLBACK
        for(unsigned int i = 0; i < sim->ncallbacks; i++) {
            auto callback = sim->callbacks[i];
            if((0 == (sim->simstep % callback->getStep())) && callback->isValid()) {
                callback->evalAndUpdate(sim->time);
            }
        }
        sim->simstep += 1;
#endif
	sim->time+=sim->dt;													// --- end of time step ---
	simsetvariable(sim,"time",sim->time);
	er=simdocommands(sim);
	if(er) return er;


	if(sim->time>=sim->tmax) return 1;
	if(sim->time>=sim->tbreak) return 10;

	return 0; }


/* endsimulate */
void endsimulate(simptr sim,int er) {
	int sflag,tflag,*eventcount;

	gl2State(2);
	tflag=strchr(sim->flags,'t')?1:0;
	sflag=strchr(sim->flags,'s')?1:0;

	simLog(sim,2,"\n");
	if(er==1) simLog(sim,2,"Simulation complete\n");
	else if(er==2) simLog(sim,5,"Simulation terminated during molecule assignment\n  Out of memory\n");
	else if(er==3) simLog(sim,5,"Simulation terminated during order 0 reaction\n  Not enough molecules allocated\n Maximum allowed molecule number is %i",sim->mols->maxdlimit);
	else if(er==4) simLog(sim,5,"Simulation terminated during order 1 reaction\n  Not enough molecules allocated\n Maximum allowed molecule number is %i",sim->mols->maxdlimit);
	else if(er==5) simLog(sim,5,"Simulation terminated during order 2 reaction\n  Not enough molecules allocated\n Maximum allowed molecule number is %i",sim->mols->maxdlimit);
	else if(er==6) simLog(sim,5,"Simulation terminated during molecule sorting\n  Out of memory\n");
	else if(er==7) simLog(sim,5,"Simulation stopped by a runtime command\n");
	else if(er==8) simLog(sim,5,"Simulation terminated during simulation state updating\n  Out of memory\n");
	else if(er==9) simLog(sim,5,"Simulation terminated during diffusion\n  Out of memory\n");
	else if(er==11) simLog(sim,5,"Simulation terminated during filament dynamics\n");
	else if(er==12) simLog(sim,5,"Simulation terminated during lattice simulation\n");
	else if(er==13) simLog(sim,5,"Simulation terminated during reaction network expansion\n");
	else simLog(sim,2,"Simulation stopped by user\n");
	simLog(sim,2,"Current simulation time: %f\n",sim->time);

	eventcount=sim->eventcount;
	if(eventcount[ETwall]) simLog(sim,2,"%i wall interactions\n",eventcount[ETwall]);
	if(eventcount[ETsurf]) simLog(sim,2,"%i surface interactions\n",eventcount[ETsurf]);
	if(eventcount[ETdesorb]) simLog(sim,2,"%i desorptions\n",eventcount[ETdesorb]);
	if(eventcount[ETrxn0]) simLog(sim,2,"%i zeroth order reactions\n",eventcount[ETrxn0]);
	if(eventcount[ETrxn1]) simLog(sim,2,"%i unimolecular reactions\n",eventcount[ETrxn1]);
	if(eventcount[ETrxn2intra]) simLog(sim,2,"%i intrabox bimolecular reactions\n",eventcount[ETrxn2intra]);
	if(eventcount[ETrxn2inter]) simLog(sim,2,"%i interbox bimolecular reactions\n",eventcount[ETrxn2inter]);
	if(eventcount[ETrxn2wrap]) simLog(sim,2,"%i wrap-around bimolecular reactions\n",eventcount[ETrxn2wrap]);
	if(eventcount[ETrxn2hybrid]) simLog(sim,2,"%i bybrid bimolecular reactions\n",eventcount[ETrxn2hybrid]);
	if(eventcount[ETimport]) simLog(sim,2,"%i imported molecules\n",eventcount[ETimport]);
	if(eventcount[ETexport]) simLog(sim,2,"%i exported molecules\n",eventcount[ETexport]);

	simLog(sim,2,"total execution time: %g seconds\n",sim->elapsedtime);

  // TODO: Useful when running benchmarks
  // If SMOLDYN_NO_PROMPT is set by user then smoldyn quit at the
  // end of simultion without prompting user to press shift+Q.
  // It is useful when running tests in batch mode locally. And it is essential
  // for testing  examples on remote servers such as Travis CI and github actions.
  const char* dontPrompt = getenv("SMOLDYN_NO_PROMPT");
  if(dontPrompt != NULL && strlen(dontPrompt) > 0)
    sim->quitatend = 1;

  if(sim->graphss && sim->graphss->graphics>0 && !tflag && !sim->quitatend && !sflag)
    fprintf(stderr,"\nTo quit: Activate graphics window, then press shift-Q.\a\n");
  return; }


/* smolsimulate */
int smolsimulate(simptr sim) {
	int er;

	er=0;
	simLog(sim,2,"Simulating\n");
	sim->clockstt=time(NULL);
	er=simdocommands(sim);
	if(!er)
		while((er=simulatetimestep(sim))==0);
	if(er!=10) {
		scmdpop(sim->cmds,sim->tmax);
		scmdexecute(sim->cmds,sim->time,sim->dt,-1,1);
		scmdsetcondition(sim->cmds,0,0); }
	sim->elapsedtime+=difftime(time(NULL),sim->clockstt);
	return er; }
