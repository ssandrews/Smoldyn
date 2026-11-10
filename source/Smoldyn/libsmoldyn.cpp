/* Steven Andrews, started 10/22/2001.
 This is an application programming interface for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <string.h>
#include "../libSteve/List.h"
#include "opengl2.h"
#include "SimCommand.h"
#include "string2.h"
#include "List.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "libsmoldyn.h"
#ifdef OPTION_VCELL
	#include "SimpleValueProvider.h"
	#include "SimpleMesh.h"
#endif

#define LCHECK(A,B,C,D) if(!(A)) {smolSetError(B,C,D,sim?sim->flags:"");if(C<ECwarning) goto failure;} else (void)0
#define LCHECKNT(A,B,C,D) if(!(A)) {smolSetErrorNT(B,C,D);if(C<ECwarning) goto failure;} else (void)0

#ifdef __cplusplus
    #define CSTRING "C"
#else
    #define CSTRING
#endif


enum ErrorCode Liberrorcode=ECok;
enum ErrorCode Libwarncode=ECok;
char Liberrorfunction[STRCHARLONG]="";
char Liberrorstring[STRCHARLONG]="";
int Libdebugmode=1;
int LibThrowThreshold=11;

PanelShape AllPanels_arr[6] = {PanelShape::PSrect, PanelShape::PStri,
    PanelShape::PSsph, PanelShape::PScyl, PanelShape::PShemi, PanelShape::PSdisk};


/******************************************************************************/
/******************************* Miscellaneous ********************************/
/******************************************************************************/


/* smolGetVersion */
extern CSTRING double smolGetVersion(void) {
	return simversionnumber(); }


/******************************************************************************/
/*********************************** Errors ***********************************/
/******************************************************************************/

/* smolSetLogging */
extern CSTRING void smolSetLogging(const char *logfile,void (*logFunction)(simptr,int,const char*,...)) {
	simSetLogging(NULL,logfile,logFunction);
	return; }


/* smolSetThrowing */
extern CSTRING void smolSetThrowing(int corethreshold,int libthreshold) {
	simSetThrowing(corethreshold);
	LibThrowThreshold=libthreshold;
	return; }


/* smolSetError */
extern CSTRING void smolSetError(const char *errorfunction,enum ErrorCode errorcode,const char *errorstring,const char *flags) {
	char string[STRCHARLONG];
	int qflag,sflag,wflag;
//	int severity;

	if(flags) {
		qflag=strchr(flags,'q')?1:0;
		sflag=strchr(flags,'s')?1:0;
		wflag=strchr(flags,'w')?1:0; }
	else {
		qflag=sflag=wflag=0; }

	if(errorcode!=ECsame) {
		Liberrorcode=errorcode;
		Libwarncode=(errorcode>=ECwarning)?errorcode:ECok;
		if(errorstring)
			strncpy(Liberrorstring,errorstring,STRCHARLONG-1);
		else Liberrorstring[0]='\0'; }
	if(errorfunction)
		strncpy(Liberrorfunction,errorfunction,STRCHARLONG-1);
	else Liberrorfunction[0]='\0';

//	severity=-(int)errorcode;
//??	if(LibThrowThreshold<severity) throw;	//?? This is disabled for now because I can't link libsmoldyn statically if not

	if(Libdebugmode && Liberrorfunction[0]!='\0') {
		if(Liberrorcode==ECnotify) {
			if(!qflag && !sflag)
				fprintf(stderr,"Libsmoldyn notification from %s: %s\n",Liberrorfunction,Liberrorstring); }
		else if(Liberrorcode==ECwarning) {
			if(!sflag && !wflag)
				fprintf(stderr,"Libsmoldyn warning in %s: %s\n",Liberrorfunction,Liberrorstring); }
		else {
			if(!sflag)
				fprintf(stderr,"Libsmoldyn '%s' error in %s: %s\n",smolErrorCodeToString(Liberrorcode,string),Liberrorfunction,Liberrorstring); }}
	return; }


/* smolSetErrorNT */
extern CSTRING void smolSetErrorNT(const char *errorfunction,enum ErrorCode errorcode,const char *errorstring) {
	if(errorcode!=ECsame) {
		Liberrorcode=errorcode;
		Libwarncode=(errorcode>=ECwarning)?errorcode:ECok;
		if(errorstring) {
			strncpy(Liberrorstring,errorstring,STRCHARLONG-1);
			Liberrorstring[STRCHARLONG-1] = '\0'; }
		else Liberrorstring[0]='\0'; }
	if(errorfunction)
		strncpy(Liberrorfunction,errorfunction,STRCHARLONG-1);
	else Liberrorfunction[0]='\0';
	return; }


/* smolGetError */
extern CSTRING enum ErrorCode smolGetError(char *errorfunction,char *errorstring,int clearerror) {
	enum ErrorCode erc;

	erc=Liberrorcode;
	if(errorfunction) strcpy(errorfunction,Liberrorfunction);
	if(errorstring) strcpy(errorstring,Liberrorstring);
	if(clearerror) smolClearError();
	return erc; }


/* smolClearError */
extern CSTRING void smolClearError(void) {
	Liberrorcode=ECok;
	Libwarncode=ECok;
	Liberrorfunction[0]='\0';
	Liberrorstring[0]='\0';
	return; }


/* smolSetDebugMode */
extern CSTRING void smolSetDebugMode(int debugmode) {
	Libdebugmode=debugmode;
	return; }


/* smolErrorCodeToString */
extern CSTRING char *smolErrorCodeToString(enum ErrorCode erc,char *string) {
	if(erc==ECok) strcpy(string,"ok");
	else if(erc==ECnotify) strcpy(string,"notify");
	else if(erc==ECwarning) strcpy(string,"warning");
	else if(erc==ECnonexist) strcpy(string,"nonexistant");
	else if(erc==ECall) strcpy(string,"all");
	else if(erc==ECmissing) strcpy(string,"missing");
	else if(erc==ECbounds) strcpy(string,"bounds");
	else if(erc==ECsyntax) strcpy(string,"syntax");
	else if(erc==ECerror) strcpy(string,"error");
	else if(erc==ECmemory) strcpy(string,"memory");
	else if(erc==ECbug) strcpy(string,"Smoldyn bug");
	else if(erc==ECsame) strcpy(string,"same as before");
	else strcpy(string,"undefined");
	return string; }


/******************************************************************************/
/******************************** Sim structure *******************************/
/******************************************************************************/

/* smolNewSim */
extern CSTRING simptr smolNewSim(int dim,double *lowbounds,double *highbounds) {
	const char *funcname="smolNewSim";
	simptr sim;
	int d,er;

	sim=NULL;
	LCHECK(dim>0,funcname,ECbounds,"dim must be >0");
	LCHECK(dim<=3,funcname,ECbounds,"dim must be <=3");
	LCHECK(lowbounds,funcname,ECmissing,"missing lowbounds");
	LCHECK(highbounds,funcname,ECmissing,"missing highbounds");
	for(d=0;d<dim;d++)
		LCHECK(lowbounds[d]<highbounds[d],funcname,ECbounds,"lowbounds must be < highbounds");

	sim=simalloc(NULL);
	LCHECK(sim,funcname,ECmemory,"allocating sim");
	er=simsetdim(sim,dim);
	LCHECK(!er,funcname,ECbug,"simsetdim bug");
	for(d=0;d<dim;d++) {
		er=walladd(sim,d,0,lowbounds[d],'t');
		LCHECK(!er,funcname,ECmemory,"allocating wall");
		er=walladd(sim,d,1,highbounds[d],'t');
		LCHECK(!er,funcname,ECmemory,"allocating wall"); }

	return sim;
 failure:
	if(sim) simfree(sim);
	return NULL; }


/* smolUpdateSim */
extern CSTRING enum ErrorCode smolUpdateSim(simptr sim) {
	const char *funcname="smolUpdateSim";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=simupdate(sim);
	LCHECK(!er,funcname,ECerror,ErrorString);	//?? check error handling
	return ECok;
 failure:
	return Liberrorcode; }


/* smolRunTimeStep */
extern CSTRING enum ErrorCode smolRunTimeStep(simptr sim) {
	const char *funcname="smolRunTimeStep";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simsettime(sim,sim->time+sim->dt/2,4);
	er=smolsimulate(sim);
	LCHECK(er!=1,funcname,ECnotify,"Simulation complete");
	LCHECK(er!=2,funcname,ECerror,"Simulation terminated during molecule assignment\n  Out of memory");
	LCHECK(er!=3,funcname,ECerror,"Simulation terminated during order 0 reaction\n");
	LCHECK(er!=4,funcname,ECerror,"Simulation terminated during order 1 reaction\n");
	LCHECK(er!=5,funcname,ECerror,"Simulation terminated during order 2 reaction\n");
	LCHECK(er!=6,funcname,ECerror,"Simulation terminated during molecule sorting\n  Out of memory");
	LCHECK(er!=7,funcname,ECnotify,"Simulation stopped by a runtime command");
	LCHECK(er!=8,funcname,ECerror,"Simulation terminated during simulation state updating\n  Out of memory");
	LCHECK(er!=9,funcname,ECerror,"Simulation terminated during diffusion\n  Out of memory");
	LCHECK(er!=11,funcname,ECerror,"Simulation terminated during filament dynamics");
	LCHECK(er!=12,funcname,ECerror,"Simulation terminated during lattice simulation");
	LCHECK(er!=13,funcname,ECerror,"Simulation terminated during reaction network expansion");
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolRunSim */
extern CSTRING enum ErrorCode smolRunSim(simptr sim) {
	const char *funcname="smolRunSim";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=smolOpenOutputFiles(sim, true);
	LCHECK(!er,funcname,ECerror,"Cannot open output files for writing");

	if(sim->graphss && sim->graphss->graphics>0 && !strchr(sim->flags,'t'))
		smolsimulategl(sim);
	else {
		er=smolsimulate(sim);
		LCHECK(er!=1,funcname,ECnotify,"Simulation complete");
		LCHECK(er!=2,funcname,ECerror,"Simulation terminated during molecule assignment\n  Out of memory");
		LCHECK(er!=3,funcname,ECerror,"Simulation terminated during order 0 reaction\n  Not enough molecules allocated");
		LCHECK(er!=4,funcname,ECerror,"Simulation terminated during order 1 reaction\n  Not enough molecules allocated");
		LCHECK(er!=5,funcname,ECerror,"Simulation terminated during order 2 reaction\n  Not enough molecules allocated");
		LCHECK(er!=6,funcname,ECerror,"Simulation terminated during molecule sorting\n  Out of memory");
		LCHECK(er!=7,funcname,ECnotify,"Simulation stopped by a runtime command");
		LCHECK(er!=8,funcname,ECerror,"Simulation terminated during simulation state updating\n  Out of memory");
		LCHECK(er!=9,funcname,ECerror,"Simulation terminated during diffusion\n  Out of memory"); }

        // FIXME. If run() is called again, previous Liberrorcode does not
        // reset to ECok but remain stuck to ECnotify.
        if(Libwarncode == ECnotify)
            Libwarncode = ECok;
        return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolRunSimUntil */
extern CSTRING enum ErrorCode smolRunSimUntil(simptr sim,double breaktime) {
	const char *funcname="smolRunSimUntil";
	double stoptime;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	stoptime=sim->tmax;
	simsettime(sim,breaktime,4);
	smolRunSim(sim);
	simsettime(sim,stoptime,4);
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolFreeSim */
extern CSTRING enum ErrorCode smolFreeSim(simptr sim) {
	simfree(sim);
	return ECok; }


/* smolDisplaySim */
extern CSTRING enum ErrorCode smolDisplaySim(simptr sim) {
	simsystemoutput(sim);
	checksimparams(sim);
	return ECok; }


/******************************************************************************/
/************************** Read configuration file ***************************/
/******************************************************************************/


/* smolPrepareSimFromFile */
extern CSTRING simptr smolPrepareSimFromFile(const char *filepath,const char *filename,const char *flags) {
	const char *funcname="smolPrepareSimFromFile";
	int er;
	char emptystring[STRCHAR];
	simptr sim;

	sim=NULL;
	LCHECK(filename,funcname,ECmissing,"missing filename");

	emptystring[0]='\0';
	if(!filepath) filepath=emptystring;
	if(!flags) flags=emptystring;
#ifdef OPTION_VCELL
	er=simInitAndLoad(filepath,filename,&sim,flags,new SimpleValueProviderFactory(),new SimpleMesh());
#else
	er=simInitAndLoad(filepath,filename,&sim,flags,NULL);
#endif
	LCHECKNT(!er,funcname,ECerror,ErrorLineAndString);
	er=simUpdateAndDisplay(sim);
	LCHECK(!er,funcname,ECerror,"Failed to update simulation");
	return sim;
 failure:
	simfree(sim);
	return NULL; }


/* smolLoadSimFromFile */
extern CSTRING enum ErrorCode smolLoadSimFromFile(const char *filepath,const char *filename,simptr *simpointer,const char *flags) {
	const char *funcname="smolLoadSimFromFile";
	int er;
	char emptystring[STRCHAR];
	simptr sim;

	sim=NULL;
	LCHECK(filename,funcname,ECmissing,"missing filename");
	LCHECK(simpointer,funcname,ECmissing,"missing simpointer");

	emptystring[0]='\0';
	if(!filepath) filepath=emptystring;
	if(!flags) flags=emptystring;

	sim=*simpointer;
	if(!sim) {
		sim=simalloc(filepath);
		LCHECK(sim,funcname,ECmemory,"allocating sim"); }
	er=loadsim(sim,filepath,filename,flags);
	LCHECKNT(!er,funcname,ECerror,ErrorLineAndString);

	*simpointer=sim;
	return ECok;
 failure:
	return Liberrorcode; }


/* smolReadConfigString */
extern CSTRING enum ErrorCode smolReadConfigString(simptr sim,const char *statement,char *parameters) {
	const char *funcname="smolReadConfigString";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(statement,funcname,ECmissing,"missing statement");
	er=simreadstring(sim,NULL,statement,parameters);
	LCHECK(!er,funcname,ECerror,"Error in configuration string");

	return ECok;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/***************************** Simulation settings ****************************/
/******************************************************************************/


/* smolSetSimFlags */
extern CSTRING enum ErrorCode smolSetSimFlags(simptr sim,const char *flags) {
	const char *funcname="smolSetSimFlags";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(flags,funcname,ECmissing,"missing flags");
	strncpy(sim->flags,flags,STRCHAR);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSimTimes */
extern CSTRING enum ErrorCode smolSetSimTimes(simptr sim,double timestart,double timestop,double timestep) {
	const char *funcname="smolSetSimTimes";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(timestep>0,funcname,ECbounds,"timestep value");
	simsettime(sim,timestart,0);
	simsettime(sim,timestart,1);
	simsettime(sim,timestop,2);
	simsettime(sim,timestep,3);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTimeStart */
extern CSTRING enum ErrorCode smolSetTimeStart(simptr sim,double timestart) {
	const char *funcname="smolSetTimeStart";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simsettime(sim,timestart,1);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTimeStop */
extern CSTRING enum ErrorCode smolSetTimeStop(simptr sim,double timestop) {
	const char *funcname="smolSetTimeStop";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simsettime(sim,timestop,2);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTimeNow */
extern CSTRING enum ErrorCode smolSetTimeNow(simptr sim,double timenow) {
	const char *funcname="smolSetTimeNow";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simsettime(sim,timenow,0);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTimeStep */
extern CSTRING enum ErrorCode smolSetTimeStep(simptr sim,double timestep) {
	const char *funcname="smolSetTimeStep";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(timestep>0,funcname,ECbounds,"timestep is not > 0");
	simsettime(sim,timestep,3);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSimParams */
extern CSTRING enum ErrorCode smolSetSimParams(simptr sim,const char *parameter,double value) {
	const char *funcname="smolSetSimParams";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(parameter,funcname,ECmissing,"missing parameter");
	if(!strcmp(parameter,"accuracy")) {
		sim->accur=value; }
	else if(!strcmp(parameter,"gauss_table_size")) {
		LCHECK(value==(double)(int)value,funcname,ECbounds,"gauss_table_size needs to be an integer");
		er=molssetgausstable(sim,(int)value);
		LCHECK(er!=1,funcname,ECmemory,"out of memory");
		LCHECK(er!=3,funcname,ECbounds,"gauss_table_size needs to be an integer power of two");
		LCHECK(!er,funcname,ECerror,"error setting gauss_table_size"); }
	else if(!strcmp(parameter,"quit_at_end")) {
		LCHECK(value==0 || value==1,funcname,ECbounds,"quit_at_end needs to be 0 or 1");
		sim->quitatend=(int)value; }
	else
		LCHECK(0,funcname,ECsyntax,"parameter name not recognized");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetRandomSeed */
extern CSTRING enum ErrorCode smolSetRandomSeed(simptr sim,long int seed) {
	const char *funcname="smolSetRandomSeed";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	Simsetrandseed(sim,seed);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetVariable */
extern CSTRING enum ErrorCode smolSetVariable(simptr sim,const char *name,double value) {
	const char *funcname="smolSetVariable";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(name && name[0]!='\0',funcname,ECmissing,"missing variable name");
	LCHECK(strlen(name)<STRCHAR,funcname,ECbounds,"variable name is too long");
	LCHECK(strcmp(name,"time"),funcname,ECsyntax,"'time' cannot be used as a variable name; it is pre-defined as the simulation time");
	LCHECK(strcmp(name,"x") && strcmp(name,"y") && strcmp(name,"z") && strcmp(name,"r"),funcname,ECsyntax,"x, y, z, and r are reserved variable names");
	LCHECK(strokname(name),funcname,ECsyntax,"variable name has to start with a letter and then be alphanumeric with optional underscores");
	er=simsetvariable(sim,name,value);
	LCHECK(!er,funcname,ECmemory,"out of memory allocating variable space");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetVariable */
extern CSTRING enum ErrorCode smolGetVariable(simptr sim,const char *name,double *value) {
	const char *funcname="smolGetVariable";
	int v;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(name,funcname,ECmissing,"missing variable name");
	LCHECK(value,funcname,ECmissing,"missing value pointer");
	v=stringfind(sim->varnames,sim->nvar,name);
	LCHECK(v>=0,funcname,ECnonexist,"variable not found");
	*value=sim->varvalues[v];
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetPartitions */
extern CSTRING enum ErrorCode smolSetPartitions(simptr sim,const char *method,double value) {
	const char *funcname="smolSetPartitions";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(method,funcname,ECmissing,"missing method string");
	LCHECK(value>0,funcname,ECbounds,"value needs to be > 0");
	er=boxsetsize(sim,method,value);
	LCHECK(er!=1,funcname,ECmemory,"out of memory");
	LCHECK(er!=2,funcname,ECsyntax,"method is not recognized");
	return ECok;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/********************************** Graphics **********************************/
/******************************************************************************/

/* smolSetGraphicsParams */
extern CSTRING enum ErrorCode smolSetGraphicsParams(simptr sim,const char *method,int timesteps,int delay) {
	const char *funcname="smolSetGraphicsParams";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=graphicsenablegraphics(sim,method);
	LCHECK(er!=1,funcname,ECmemory,"out of memory");
	LCHECK(er!=2,funcname,ECmissing,"missing sim");
	LCHECK(er!=3,funcname,ECsyntax,"graphics method not recognized");
	if(timesteps>0) {
		er=graphicssetiter(sim,timesteps);
		LCHECK(er!=1,funcname,ECmemory,"out of memory enabling graphics");
		LCHECK(er!=2,funcname,ECbug,"BUG: missing parameter");
		LCHECK(er!=3,funcname,ECbug,"BUG: timesteps needs to be >=1"); }
	if(delay>=0) {
		er=graphicssetdelay(sim,delay);
		LCHECK(er!=1,funcname,ECmemory,"out of memory enabling graphics");
		LCHECK(er!=2,funcname,ECbug,"BUG: missing parameter");
		LCHECK(er!=3,funcname,ECbug,"BUG: delay needs to be >=0"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTiffParams */
extern CSTRING enum ErrorCode smolSetTiffParams(simptr sim,int timesteps,const char *tiffname,int lowcount,int highcount) {
	const char *funcname="smolSetTiffParams";
	char nm1[STRCHAR];
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(timesteps>0) {
		er=graphicssettiffiter(sim,timesteps);
		LCHECK(er!=1,funcname,ECmemory,"out of memory enabling graphics");
		LCHECK(er!=2,funcname,ECbug,"BUG: missing parameter");
		LCHECK(er!=3,funcname,ECbug,"BUG: timesteps needs to be >=1"); }
	if(tiffname) {
		strcpy(nm1,sim->filepath);
		strncat(nm1,tiffname,STRCHAR-1-strlen(nm1));
		gl2SetOptionStr("TiffName",nm1); }
	if(lowcount>=0) {
		gl2SetOptionInt("TiffNumber",lowcount); }
	if(highcount>=0) {
		gl2SetOptionInt("TiffNumMax",highcount); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetLightParams */
extern CSTRING enum ErrorCode smolSetLightParams(simptr sim,int lightindex,double *ambient,double *diffuse,double *specular,double *position) {
	const char *funcname="smolSetLightParams";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(lightindex>=-1 && lightindex<MAXLIGHTS,funcname,ECbounds,"lightindex out of bounds");
	LCHECK(lightindex>=0 || (!diffuse && !specular && !position),funcname,ECsyntax,"can only set ambient for global light");

	if(ambient) {
		for(c=0;c<4;c++)
			LCHECK(ambient[c]>=0 && ambient[c]<=1,funcname,ECbounds,"ambient light value out of bounds");
		er=graphicssetlight(sim,NULL,lightindex,LPambient,ambient);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	if(diffuse) {
		for(c=0;c<4;c++)
			LCHECK(diffuse[c]>=0 && diffuse[c]<=1,funcname,ECbounds,"diffuse light value out of bounds");
		er=graphicssetlight(sim,NULL,lightindex,LPdiffuse,diffuse);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	if(specular) {
		for(c=0;c<4;c++)
			LCHECK(specular[c]>=0 && specular[c]<=1,funcname,ECbounds,"specular light value out of bounds");
		er=graphicssetlight(sim,NULL,lightindex,LPspecular,specular);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	if(position) {
		er=graphicssetlight(sim,NULL,lightindex,LPposition,position);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }

	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetBackgroundStyle */
extern CSTRING enum ErrorCode smolSetBackgroundStyle(simptr sim,double *color) {
	const char *funcname="smolSetBackgroundStyle";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=graphicssetbackcolor(sim,color);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetFrameStyle */
extern CSTRING enum ErrorCode smolSetFrameStyle(simptr sim,double thickness,double *color) {
	const char *funcname="smolSetFrameStyle";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(thickness>=0) {
		er=graphicssetframethickness(sim,thickness);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=graphicssetframecolor(sim,color);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetGridStyle */
extern CSTRING enum ErrorCode smolSetGridStyle(simptr sim,double thickness,double *color) {
	const char *funcname="smolSetGridStyle";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(thickness>=0) {
		er=graphicssetgridthickness(sim,thickness);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=graphicssetgridcolor(sim,color);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetTextStyle */
extern CSTRING enum ErrorCode smolSetTextStyle(simptr sim,double *color) {
	const char *funcname="smolSetTextStyle";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=graphicssettextcolor(sim,color);
		LCHECK(!er,funcname,ECmemory,"out of memory enabling graphics"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddTextDisplay */
extern CSTRING enum ErrorCode smolAddTextDisplay(simptr sim,char *item) {
	const char *funcname="smolAddTextDisplay";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=graphicssettextitem(sim,item);
	LCHECK(er!=1,funcname,ECmemory,"out of memory adding text display item");
	LCHECK(er!=2,funcname,ECsyntax,"listed item is not recognized or not supported");
	LCHECK(er!=3,funcname,ECwarning,"text display item was already listed");
	return Libwarncode;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/***************************** Runtime commands *******************************/
/******************************************************************************/

/* smolSetOutputPath */
extern CSTRING enum ErrorCode smolSetOutputPath(simptr sim,const char *path) {
	const char *funcname="smolSetOutputPath";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(path,funcname,ECmissing,"missing path");
	er=scmdsetfroot(sim->cmds,path);
	LCHECK(!er,funcname,ECbug,"scmdsetfroot bug");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddOutputFile */
extern CSTRING enum ErrorCode smolAddOutputFile(simptr sim,char *filename,int suffix,int append) {
	const char *funcname="smolAddOutputFile";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(filename,funcname,ECmissing,"missing filename");
	LCHECK(!strchr(filename,' '),funcname,ECwarning,"only first word of filename is used");
	er=scmdsetfnames(sim->cmds,filename,append);
	LCHECK(!er,funcname,ECmemory,"allocating filename");
	if(suffix>=0) {
		er=scmdsetfsuffix(sim->cmds,filename,suffix);
		LCHECK(!er,funcname,ECbug,"scmdsetfsuffix bug"); }

	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolAddOutputData */
extern CSTRING enum ErrorCode smolAddOutputData(simptr sim,char *dataname) {
	const char *funcname="smolAddOutputData";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(dataname,funcname,ECmissing,"missing dataname");
	LCHECK(!strchr(dataname,' '),funcname,ECwarning,"only first word of dataname is used");
	er=scmdsetdnames(sim->cmds,dataname);
	LCHECK(!er,funcname,ECmemory,"allocating dataname");
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolOpenOutputFiles */
enum ErrorCode smolOpenOutputFiles(simptr sim, int overwrite = 0)
{
    const char *funcname = "smolOpenOutputFiles";
    int err              = scmdopenfiles(sim->cmds, overwrite);
    LCHECK(!err, funcname, ECerror, "Unable (or forbidden) to open output files for writing");

    return Libwarncode;
failure:
    return Liberrorcode;
}

/* smolSetOutputFormat */
extern CSTRING enum ErrorCode smolSetOutputFormat(simptr sim,const char *format,int precision) {
	const char *funcname="smolSetOutputFormat";
	char fmt[STRCHAR];
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(format && format[0]!='\0') {
		LCHECK(strlen(format)<STRCHAR,funcname,ECsyntax,"output format not recognized");
		strcpy(fmt,format);
		er=scmdsetoutputformat(sim->cmds,fmt);
		LCHECK(!er,funcname,ECsyntax,"output format not recognized; use ssv or csv"); }
	if(precision>=0)
		scmdsetprecision(sim->cmds,precision);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddCommand */
extern CSTRING enum ErrorCode smolAddCommand(simptr sim,char type,double on,double off,double step,double multiplier,const char *commandstring) {
	const char *funcname="smolSetCommand";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=scmdaddcommand(sim->cmds,type,on,off,step,multiplier,commandstring);
	LCHECK(er!=1,funcname,ECmemory,"out of memory creating command");
	LCHECK(er!=2,funcname,ECbug,"missing sim->cmds");
	LCHECK(er!=3,funcname,ECsyntax,"missing command string");
	simsetcondition(sim,SCparams,0);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddCommandFromString */
extern CSTRING enum ErrorCode smolAddCommandFromString(simptr sim,char *string) {
	const char *funcname="smolSetCommandFromString";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(string,funcname,ECmissing,"missing string");
	er=scmdstr2cmd(sim->cmds,string,NULL,NULL,0);
	LCHECK(er!=1,funcname,ECmemory,"out of memory in cmd");
	LCHECK(er!=2,funcname,ECbug,"BUG: no command superstructure for cmd");
	LCHECK(er!=3,funcname,ECsyntax,"cmd format: type [on off dt] string");
	LCHECK(er!=5,funcname,ECbounds,"cmd time step needs to be >0");
	LCHECK(er!=8,funcname,ECbounds,"cmd time multiplier needs to be >1");
	simsetcondition(sim,SCparams,0);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetOutputData */
extern CSTRING enum ErrorCode smolGetOutputData(simptr sim,char *dataname,int *nrow,int *ncol,double **array,int erase) {
	const char *funcname="smolGetOutputData";
	int did,i,j;
	listptrdd list;
	double *datacopy;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(dataname,funcname,ECmissing,"missing dataname");
	LCHECK(nrow && ncol && array,funcname,ECmissing,"missing pointer for returned data");
	LCHECK(sim->cmds && sim->cmds->ndata>0,funcname,ECerror,"no data files in the sim");
	did=stringfind(sim->cmds->dname,sim->cmds->ndata,dataname);
	LCHECK(did>=0,funcname,ECerror,"no data file of the requested name");
	list=sim->cmds->data[did];

	if(!list) {
		// Data table was registered via smolAddOutputData but no command has
		// populated it yet — return an empty result rather than dereferencing.
		*nrow=0;
		*ncol=0;
		*array=NULL;
		return ECok; }

    datacopy=(double*) calloc(list->nrow*list->ncol,sizeof(double));
    LCHECK(datacopy,funcname,ECmemory,"out of memory");
    for(i=0;i<list->nrow;i++)
        for(j=0;j<list->ncol;j++)
            datacopy[i*list->ncol+j]=list->data[i*list->maxcol+j];
    *nrow=list->nrow;
    *ncol=list->ncol;
    *array=datacopy;
	if(erase) ListClearDD(list);

	return ECok;
 failure:
	return Liberrorcode; }


extern CSTRING enum ErrorCode smolRunCommand(simptr sim,const char *commandstring) {
	const char *funcname="smolRunCommand";
	char stringcopy[STRCHARLONG];
	cmdptr cmd;
	enum CMDcode cmdcode;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(commandstring,funcname,ECmissing,"missing command string");
	strncpy(stringcopy,commandstring,STRCHARLONG-1);
	cmd=scmdalloc();
	LCHECK(cmd,funcname,ECmemory,"failed to create a new command structure");
	strcpy(cmd->str,stringcopy);
	cmdcode=docommand((void*)sim,cmd,stringcopy);
	LCHECK(cmdcode==CMDok,funcname,ECwarning,cmd->erstr);
	scmdfree(cmd);
	return ECok;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/********************************* Molecules **********************************/
/******************************************************************************/

/* smolAddSpecies */
extern CSTRING enum ErrorCode smolAddSpecies(simptr sim,const char *species,const char *mollist) {
	const char *funcname="smolAddSpecies";
	int i,ll;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(species,funcname,ECmissing,"missing species");
	if(mollist && mollist[0]!='\0') {
		ll=smolGetMolListIndexNT(sim,mollist);
		LCHECK(ll>=0,funcname,ECsame,NULL);
		LCHECK(sim->mols->listtype[ll]==MLTsystem,funcname,ECsyntax,"mollist is not a system list"); }
	else ll=-1;
	i=moladdspecies(sim,species);
	LCHECK(i!=-1,funcname,ECbug,"out of memory");
	LCHECK(i!=-2,funcname,ECbug,"add species bug");
	LCHECK(i!=-3,funcname,ECbug,"more species are entered than are automatically allocated");
	LCHECK(i!=-4,funcname,ECsyntax,"'empty' is not a permitted species name");
	LCHECK(i!=-5,funcname,ECwarning,"this species has already been declared");
	LCHECK(i!=-6,funcname,ECsyntax,"'?' and '*' are not allowed in species names");
	if(mollist && mollist[0]!='\0')
		molsetlistlookup(sim,i,NULL,MSall,ll);
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolGetSpeciesIndex */
extern CSTRING int smolGetSpeciesIndex(simptr sim,const char *species) {
	const char *funcname="smolGetSpeciesIndex";
	int i;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(species,funcname,ECmissing,"missing species name");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	LCHECK(strcmp(species,"all"),funcname,ECall,"species is 'all'");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,species);
	if(i<=0) {
		char buffer[STRCHAR];
		snprintf(buffer,STRCHAR,"species '%s' not found",species);
		LCHECK(0,funcname,ECnonexist,buffer); }
	return i;
 failure:
	return (int)Liberrorcode; }


/* smolGetSpeciesIndexNT */
extern CSTRING int smolGetSpeciesIndexNT(simptr sim,const char *species) {
	const char *funcname="smolGetSpeciesIndexNT";
	int i;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(species,funcname,ECmissing,"missing species name");
	LCHECKNT(sim->mols,funcname,ECnonexist,"no species defined");
	LCHECKNT(strcmp(species,"all"),funcname,ECall,"species cannot be 'all'");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,species);
	if(i<=0) {
		char buffer[STRCHAR];
		snprintf(buffer,STRCHAR,"species '%s' not found",species);
		LCHECKNT(0,funcname,ECnonexist,buffer); }
	return i;
 failure:
	return (int)Liberrorcode; }


/* smolGetSpeciesName */
extern CSTRING void smolGetSpeciesName(simptr sim,int speciesindex,char *species) {
	const char *funcname="smolGetSpeciesName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	LCHECK(speciesindex>=0,funcname,ECbounds,"speciesindex < 0");
	LCHECK(speciesindex<sim->mols->nspecies,funcname,ECnonexist,"species doesn't exist");
	LCHECK(species,funcname,ECmissing,"missing species");
	strcpy(species,sim->mols->spname[speciesindex]);
 failure:
	return; }


/* smolGetSpecies */
extern CSTRING int smolGetSpecies(simptr sim,int speciesindex,char *speciesname,double *difc,double **color,double *displaysize,char **listname) {
	const char *funcname="smolGetSpecies";
	int i=0;
	enum MolecState ms;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	if(speciesindex>=0) {
		LCHECK(speciesindex<sim->mols->nspecies,funcname,ECnonexist,"species doesn't exist");
		if(speciesname) strcpy(speciesname,sim->mols->spname[speciesindex]); }
	else {
		LCHECK(speciesname,funcname,ECmissing,"missing species name");
		LCHECK(strcmp(speciesname,"all"),funcname,ECall,"species is 'all'");
		i=stringfind(sim->mols->spname,sim->mols->nspecies,speciesname);
		if(i<=0) {
			char buffer[STRCHAR];
			snprintf(buffer,STRCHAR,"species '%s' not found",speciesname);
			LCHECK(0,funcname,ECnonexist,buffer); }}
	for(ms=(enum MolecState)0;ms<(enum MolecState)MSMAX;ms=(enum MolecState)(ms+1)) {
		if(difc) difc[ms]=sim->mols->difc[i][ms];
		if(color) {
			color[ms][0]=sim->mols->color[i][ms][0];
			color[ms][1]=sim->mols->color[i][ms][1];
			color[ms][2]=sim->mols->color[i][ms][2];
			color[ms][3]=sim->mols->color[i][ms][3]; }
		if(displaysize) displaysize[ms]=sim->mols->display[i][ms];
		if(listname) strcpy(listname[ms],sim->mols->listname[sim->mols->listlookup[i][ms]]); }
	return i;
 failure:
	return (int)Liberrorcode; }


/* smolSetSpeciesMobility */
extern CSTRING enum ErrorCode smolSetSpeciesMobility(simptr sim,const char *species,enum MolecState state,double difc,double *drift,double *difmatrix) {
	const char *funcname="smolSetSpeciesMobility";
	int i,er,isall,ilow,ihigh;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	isall=0;
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();isall=1;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");

	if(isall) {
		ilow=1;
		ihigh=sim->mols->nspecies; }
	else {
		ilow=i;
		ihigh=i+1; }

	for(i=ilow;i<ihigh;i++) {
		if(difc>=0) {
			molsetdifc(sim,i,NULL,state,difc); }
		if(drift) {
			er=molsetdrift(sim,i,NULL,state,drift);
			LCHECK(!er,funcname,ECmemory,"allocating drift"); }
		if(difmatrix) {
			er=molsetdifm(sim,i,NULL,state,difmatrix);
			LCHECK(!er,funcname,ECmemory,"allocating difmatrix"); }}

	return ECok;
 failure:
	return Liberrorcode; }


//?? need to add function for smolSetSpeciesSurfaceDrift


/* smolAddMolList */
extern CSTRING enum ErrorCode smolAddMolList(simptr sim,const char *mollist) {
	const char *funcname="smolAddMolList";
	int ll;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(mollist,funcname,ECmissing,"missing mollist");
	ll=addmollist(sim,mollist,MLTsystem);
	LCHECK(ll!=-1,funcname,ECmemory,"out of memory");
	LCHECK(ll!=-2,funcname,ECwarning,"molecule list name has already been used");
	LCHECK(ll!=-3,funcname,ECbug,"illegal addmollist inputs");
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolGetMolListIndex */
extern CSTRING int smolGetMolListIndex(simptr sim,const char *mollist) {
	const char *funcname="smolGetMolListIndex";
	int ll;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(mollist,funcname,ECmissing,"missing mollist");
	LCHECK(sim->mols,funcname,ECnonexist,"no molecule lists defined");
	LCHECK(strcmp(mollist,"all"),funcname,ECall,"molecule list is 'all'");
	ll=stringfind(sim->mols->listname,sim->mols->nlist,mollist);
	LCHECK(ll>=0,funcname,ECnonexist,"list name not recognized");
	return ll;
 failure:
	return (int)Liberrorcode; }


/* smolGetMolListIndexNT */
extern CSTRING int smolGetMolListIndexNT(simptr sim,const char *mollist) {
	const char *funcname="smolGetMolListIndexNT";
	int ll;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(mollist,funcname,ECmissing,"missing mollist");
	LCHECKNT(sim->mols,funcname,ECnonexist,"no molecule lists defined");
	LCHECKNT(strcmp(mollist,"all"),funcname,ECall,"molecule list cannot be 'all'");
	ll=stringfind(sim->mols->listname,sim->mols->nlist,mollist);
	LCHECKNT(ll>=0,funcname,ECnonexist,"list name not recognized");
	return ll;
 failure:
	return (int)Liberrorcode; }


/* smolGetMolListName */
extern CSTRING char *smolGetMolListName(simptr sim,int mollistindex,char *mollist) {
	const char *funcname="smolGetMolListName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(mollistindex>=0,funcname,ECbounds,"mollistindex < 0");
	LCHECK(sim->mols,funcname,ECnonexist,"no molecule lists defined");
	LCHECK(mollistindex<sim->mols->nlist,funcname,ECnonexist,"molecule list doesn't exist");
	LCHECK(mollist,funcname,ECmissing,"missing mollist");
	strcpy(mollist,sim->mols->listname[mollistindex]);
	return mollist;
 failure:
	return NULL; }


/* smolSetMolList */
extern CSTRING enum ErrorCode smolSetMolList(simptr sim,const char *species,enum MolecState state,const char *mollist) {
	const char *funcname="smolSetMolList";
	int i,ll;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();i=-5;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");
	ll=smolGetMolListIndexNT(sim,mollist);
	LCHECK(ll>=0,funcname,ECsame,NULL);
	LCHECK(sim->mols->listtype[ll]==MLTsystem,funcname,ECerror,"list is not a system list");
	molsetlistlookup(sim,i,NULL,state,ll);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetMaxMolecules */
extern CSTRING enum ErrorCode smolSetMaxMolecules(simptr sim,int maxmolecules) {
	const char *funcname="smolSetMaxMolecules";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(maxmolecules>0,funcname,ECbounds,"maxmolecules needs to be > 0");
	er=molsetmaxmol(sim,maxmolecules);
	LCHECK(!er,funcname,ECmemory,"out of memory allocating molecules");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddSolutionMolecules */
extern CSTRING enum ErrorCode smolAddSolutionMolecules(simptr sim,const char *species,int number,double *lowposition,double *highposition) {
	const char *funcname="smolAddSolutionMolecules";
	int er,i,d;
	double *low,*high,lowpos[3],highpos[3];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK(number>=0,funcname,ECbounds,"number cannot be < 0");
	if(!lowposition) {
		for(d=0;d<sim->dim;d++) lowpos[d]=sim->wlist[d*2]->pos;
		low=lowpos; }
	else
		low=lowposition;
	if(!highposition) {
		for(d=0;d<sim->dim;d++) highpos[d]=sim->wlist[d*2+1]->pos;
		high=highpos; }
	else
		high=highposition;

	er=addmol(sim,number,i,low,high,0);
	LCHECK(!er,funcname,ECmemory,"out of memory adding molecules");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddCompartmentMolecules */
extern CSTRING enum ErrorCode smolAddCompartmentMolecules(simptr sim,const char *species,int number,const char *compartment) {
	const char *funcname="smolAddCompartmentMolecules";
	int i,er,c;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK(number>=0,funcname,ECbounds,"number < 0");
	c=smolGetCompartmentIndexNT(sim,compartment);
	LCHECK(c>=0,funcname,ECsame,NULL);
	er=addcompartmol(sim,number,i,sim->cmptss->cmptlist[c]);
	LCHECK(er!=2,funcname,ECerror,"compartment volume is zero or nearly zero");
	LCHECK(er!=3,funcname,ECmemory,"out of memory adding molecules");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddSurfaceMolecules */
extern CSTRING enum ErrorCode smolAddSurfaceMolecules(simptr sim,const char *species,enum MolecState state,int number,const char *surface,enum PanelShape panelshape,const char *panel,double *position) {
	const char *funcname="smolAddSurfaceMolecules";
	int i,s,p,er;
	panelptr pnl;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK(state>=0 && state<MSMAX,funcname,ECsyntax,"invalid state");
	LCHECK(number>=0,funcname,ECbounds,"number < 0");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();s=-5;}
	else LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK((panelshape>=0 && panelshape<PSMAX) || panelshape==PSall,funcname,ECnonexist,"invalid panelshape");
	pnl=NULL;
	p=smolGetPanelIndexNT(sim,surface,NULL,panel);
	if(p==(int)ECall) {smolClearError();p=-5;}
	else LCHECK(p>=0,funcname,ECsame,NULL);

	if(p>=0) {
		LCHECK(s>=0,funcname,ECsyntax,"needs to be specific surface");
		LCHECK(panelshape!=PSall,funcname,ECsyntax,"needs to be specific panelshape");
		pnl=sim->srfss->srflist[s]->panels[panelshape][p]; }
	else {
		LCHECK(!position,funcname,ECsyntax,"a panel must be specified if position is entered"); }
	er=addsurfmol(sim,number,i,state,position,pnl,s,panelshape,NULL);
	LCHECK(er!=1,funcname,ECmemory,"unable to allocate temporary storage space");
	LCHECK(er!=2,funcname,ECbug,"panel name not recognized");
	LCHECK(er!=3,funcname,ECmemory,"out of memory adding molecules");

	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetMoleculeCount */
extern CSTRING int smolGetMoleculeCount(simptr sim,const char *species,enum MolecState state) {
	const char *funcname="smolGetMoleculeCount";
	int i;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {i=-5;smolClearError();}
	else LCHECK(i>0,funcname,ECsame,NULL);
	return molcount(sim,i,NULL,state,-1);
 failure:
	return (int)Liberrorcode; }


/* smolSetSpeciesSurfaceDrift */
extern CSTRING enum ErrorCode smolSetSpeciesSurfaceDrift(simptr sim,const char *species,enum MolecState state,const char *surface,enum PanelShape panelshape,double *drift) {
	const char *funcname="smolSetSpeciesSurfaceDrift";
	int i,s,er,ilow,ihigh;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->dim>1,funcname,ECerror,"system dimensionality needs to be 2 or 3");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	LCHECK(drift,funcname,ECmissing,"missing drift vector");
	LCHECK((state>MSsoln && state<MSMAX) || state==MSall,funcname,ECsyntax,"state needs to be surface-bound");
	LCHECK((panelshape>=0 && panelshape<PSMAX) || panelshape==PSall,funcname,ECsyntax,"invalid panel shape");

	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {
		smolClearError();
		ilow=1;
		ihigh=sim->mols->nspecies; }
	else {
		LCHECK(i>0,funcname,ECsame,NULL);
		ilow=i;
		ihigh=i+1; }

	if(!surface || !strcmp(surface,"all")) s=-1;
	else {
		s=smolGetSurfaceIndexNT(sim,surface);
		LCHECK(s>=0,funcname,ECsame,NULL); }

	for(i=ilow;i<ihigh;i++) {
		er=molsetsurfdrift(sim,i,NULL,state,s,panelshape,drift);
		LCHECK(!er,funcname,ECmemory,"out of memory allocating surface drift"); }

	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetMoleculeColor */
extern "C" enum ErrorCode smolSetMoleculeColor(
    simptr sim, const char *species, enum MolecState state, double *color)
{
    const char *funcname = "smolSetMoleculeColor";
    int i, c;

    LCHECK(sim, funcname, ECmissing, "missing sim");
    i = smolGetSpeciesIndexNT(sim, species);
    if(i == (int)ECall) {
        smolClearError();
        i = -5;
    }
    else
        LCHECK(i > 0, funcname, ECsame, NULL);

    LCHECK((state >= 0 && state < MSMAX) || state == MSall, funcname, ECsyntax,
        "invalid state");

    for(c = 0; c < 3; c++)
        LCHECK(color[c] >= 0 && color[c] <= 1, funcname, ECbounds,
            "color value out of bounds");
    molsetcolor(sim, i, NULL, state, color);
    return ECok;
failure:
    return Liberrorcode;
}

extern "C" enum ErrorCode smolSetMoleculeSize(
    simptr sim, const char *species, enum MolecState state, double size)
{
    const char *funcname = "smolSetMoleculeSize";
    int i;
    LCHECK(sim, funcname, ECmissing, "missing sim");
    i = smolGetSpeciesIndexNT(sim, species);
    if(i == (int)ECall) {
        smolClearError();
        i = -5;
    }
    else
        LCHECK(i > 0, funcname, ECsame, NULL);
    LCHECK((state >= 0 && state < MSMAX) || state == MSall, funcname, ECsyntax,
        "invalid state");

    molsetdisplaysize(sim, i, NULL, state, size);
    return ECok;
failure:
    return Liberrorcode;
}

/* smolSetMoleculeStyle */
extern CSTRING enum ErrorCode smolSetMoleculeStyle(simptr sim,const char *species,enum MolecState state,double size,double *color) {
	const char *funcname="smolSetTextStyle";
	int i,c;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();i=-5;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");

	if(size>0) molsetdisplaysize(sim,i,NULL,state,size);

	if(color) {
		for(c=0;c<3;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		molsetcolor(sim,i,NULL,state,color); }
	return ECok;
failure:
	return Liberrorcode; }


/* smolAddSpeciesGroup */
extern CSTRING enum ErrorCode smolAddSpeciesGroup(simptr sim,const char *group,const char *species) {
	const char *funcname="smolAddSpeciesGroup";
	char spname[STRCHAR];
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECnonexist,"no species defined");
	LCHECK(group && group[0]!='\0',funcname,ECmissing,"missing group name");
	if(species && species[0]!='\0') {
		LCHECK(strlen(species)<STRCHAR,funcname,ECbounds,"species name is too long");
		strcpy(spname,species);
		er=moladdspeciesgroup(sim,group,spname,0); }
	else
		er=moladdspeciesgroup(sim,group,NULL,0);
	LCHECK(er!=-4,funcname,ECnonexist,"species name not recognized");
	LCHECK(er!=-6,funcname,ECsyntax,"cannot read wildcard logic");
	LCHECK(er!=-7,funcname,ECmemory,"error allocating memory");
	LCHECK(er!=-8,funcname,ECsyntax,"molecule states are not permitted");
	LCHECK(er!=-9,funcname,ECerror,"species group name cannot be a species name");
	LCHECK(!er,funcname,ECsyntax,"cannot read group or species name");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolrulepattern. Converts the species pattern string species into a rule
pattern in pattern, which needs to have STRCHAR space. Returns 0 for success
or the error code from molstring2pattern. */
static int smolrulepattern(const char *species,char *pattern) {
	enum MolecState ms;

	if(!species || species[0]=='\0') return -1;
	pattern[0]='\0';
	return molstring2pattern(species,&ms,pattern,0); }


/* smolSetSpeciesMobilityRule */
extern CSTRING enum ErrorCode smolSetSpeciesMobilityRule(simptr sim,const char *species,enum MolecState state,double difc,double *drift,double *difmatrix) {
	const char *funcname="smolSetSpeciesMobilityRule";
	char pattern[STRCHAR];
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->dim>0,funcname,ECerror,"need to set the system dimensionality first");
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");
	if(difc>=0) {
		er=RuleAddRule(sim,RTdifc,NULL,pattern,&state,NULL,difc,NULL,NULL);
		LCHECK(!er,funcname,ECmemory,"out of memory adding difc rule"); }
	if(drift) {
		er=RuleAddRule(sim,RTdrift,NULL,pattern,&state,NULL,0,NULL,drift);
		LCHECK(!er,funcname,ECmemory,"out of memory adding drift rule"); }
	if(difmatrix) {
		er=RuleAddRule(sim,RTdifm,NULL,pattern,&state,NULL,0,NULL,difmatrix);
		LCHECK(!er,funcname,ECmemory,"out of memory adding difm rule"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSpeciesSurfaceDriftRule */
extern CSTRING enum ErrorCode smolSetSpeciesSurfaceDriftRule(simptr sim,const char *species,enum MolecState state,const char *surface,enum PanelShape panelshape,double *drift) {
	const char *funcname="smolSetSpeciesSurfaceDriftRule";
	char pattern[STRCHAR];
	int er,d,detailsi[2];
	double v1[DIMMAX];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->dim>1,funcname,ECerror,"system dimensionality needs to be 2 or 3");
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	LCHECK(drift,funcname,ECmissing,"missing drift vector");
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK((state>MSsoln && state<MSMAX) || state==MSall,funcname,ECsyntax,"state needs to be surface-bound");
	LCHECK((panelshape>=0 && panelshape<PSMAX) || panelshape==PSall,funcname,ECsyntax,"invalid panel shape");
	if(!surface || !strcmp(surface,"all")) detailsi[0]=-1;
	else {
		detailsi[0]=smolGetSurfaceIndexNT(sim,surface);
		LCHECK(detailsi[0]>=0,funcname,ECsame,NULL); }
	detailsi[1]=(int)panelshape;
	for(d=0;d<DIMMAX;d++) v1[d]=(d<sim->dim-1)?drift[d]:0;		// RuleAddRule copies dim values
	er=RuleAddRule(sim,RTsurfdrift,NULL,pattern,&state,NULL,0,detailsi,v1);
	LCHECK(!er,funcname,ECmemory,"out of memory adding surface drift rule");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetMolListRule */
extern CSTRING enum ErrorCode smolSetMolListRule(simptr sim,const char *species,enum MolecState state,const char *mollist) {
	const char *funcname="smolSetMolListRule";
	char pattern[STRCHAR];
	int er,ll;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");
	ll=smolGetMolListIndexNT(sim,mollist);
	LCHECK(ll>=0,funcname,ECsame,NULL);
	LCHECK(sim->mols->listtype[ll]==MLTsystem,funcname,ECerror,"list is not a system list");
	er=RuleAddRule(sim,RTmollist,NULL,pattern,&state,NULL,0,&ll,NULL);
	LCHECK(!er,funcname,ECmemory,"out of memory adding molecule list rule");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetMoleculeStyleRule */
extern CSTRING enum ErrorCode smolSetMoleculeStyleRule(simptr sim,const char *species,enum MolecState state,double size,double *color) {
	const char *funcname="smolSetMoleculeStyleRule";
	char pattern[STRCHAR];
	int er,c;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");
	if(size>=0) {
		er=RuleAddRule(sim,RTdispsize,NULL,pattern,&state,NULL,size,NULL,NULL);
		LCHECK(!er,funcname,ECmemory,"out of memory adding display size rule"); }
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=RuleAddRule(sim,RTcolor,NULL,pattern,&state,NULL,0,NULL,color);
		LCHECK(!er,funcname,ECmemory,"out of memory adding color rule"); }
	return ECok;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/*********************************** Surfaces *********************************/
/******************************************************************************/

/* smolSetBoundaryType */
extern CSTRING enum ErrorCode smolSetBoundaryType(simptr sim,int dimension,int highside,char type) {
	const char *funcname="smolSetBoundaryType";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(dimension<sim->dim,funcname,ECbounds,"dimension cannot exceed system dimensionality");
	LCHECK(highside<=1,funcname,ECbounds,"highside must be -1, 0, or 1");
	LCHECK(type=='r' || type=='p' || type=='a' || type=='t',funcname,ECsyntax,"invalid type");
	er=wallsettype(sim,dimension,highside,type);
	LCHECK(!er,funcname,ECbug,"bug in wallsettype");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddSurface */
extern CSTRING enum ErrorCode smolAddSurface(simptr sim,const char *surface) {
	const char *funcname="smolAddSurface";
	int s;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECnonexist) smolClearError();
	else if(s<0) LCHECK(0,funcname,ECsame,NULL);
	else LCHECK(0,funcname,ECerror,"surface is already in system");
	srf=surfaddsurface(sim,surface);
	LCHECK(srf,funcname,ECmemory,"out of memory adding surface");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetSurfaceIndex */
extern CSTRING int smolGetSurfaceIndex(simptr sim,const char *surface) {
	const char *funcname="smolGetSurfaceIndex";
	int s;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(surface,funcname,ECmissing,"missing surface");
	LCHECK(sim->srfss && sim->srfss->nsrf,funcname,ECnonexist,"no surfaces defined");
	LCHECK(strcmp(surface,"all"),funcname,ECall,"surface cannot be 'all'");
	s=stringfind(sim->srfss->snames,sim->srfss->nsrf,surface);
	LCHECK(s>=0,funcname,ECnonexist,"surface not found");
	return s;
 failure:
	return (int)Liberrorcode; }


/* smolGetSurfaceIndexNT */
extern CSTRING int smolGetSurfaceIndexNT(simptr sim,const char *surface) {
	const char *funcname="smolGetSurfaceIndexNT";
	int s;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(surface,funcname,ECmissing,"missing surface");
	LCHECKNT(sim->srfss && sim->srfss->nsrf,funcname,ECnonexist,"no surfaces defined");
	LCHECKNT(strcmp(surface,"all"),funcname,ECall,"surface cannot be 'all'");
	s=stringfind(sim->srfss->snames,sim->srfss->nsrf,surface);
	LCHECKNT(s>=0,funcname,ECnonexist,"surface not found");
	return s;
 failure:
	return (int)Liberrorcode; }


/* smolGetSurfaceName */
extern CSTRING char *smolGetSurfaceName(simptr sim,int surfaceindex,char *surface) {
	const char *funcname="smolGetSurfaceName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(surfaceindex>=0,funcname,ECbounds,"invalid surface index");
	LCHECK(surface,funcname,ECmissing,"missing surface");
	LCHECK(sim->srfss && sim->srfss->nsrf,funcname,ECnonexist,"no surfaces defined");
	LCHECK(surfaceindex<sim->srfss->nsrf,funcname,ECnonexist,"surface does not exist");
	strcpy(surface,sim->srfss->snames[surfaceindex]);
	return surface;
 failure:
	return NULL; }


/* smolSetSurfaceAction */
extern CSTRING enum ErrorCode smolSetSurfaceAction(simptr sim,const char *surface,enum PanelFace face,const char *species,enum MolecState state,enum SrfAction action,const char *newspecies) {
	const char *funcname="smolSetSurfaceAction";
	int er,i,s,i2;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();s=-5;}
	else LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(face==PFfront || face==PFback || face==PFboth,funcname,ECbounds,"invalid face");
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();i=-5;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECbounds,"invalid state");
	LCHECK(action>=0 && action<=SAmult,funcname,ECbounds,"invalid action");
        if(newspecies && strlen(newspecies) > 0) {
            i2=smolGetSpeciesIndexNT(sim,newspecies);
            LCHECK(i2>0,funcname,ECnonexist,"unrecognized new species name"); }
        else
            i2=0;
        if(s>=0) {
            srf=sim->srfss->srflist[s];
            er=surfsetaction(srf,i,NULL,state,face,action,0);
            LCHECK(!er,funcname,ECbug,"bug in surfsetaction"); }
        else {
            for(s=0;s<sim->srfss->nsrf;s++) {
                srf=sim->srfss->srflist[s];
                er=surfsetaction(srf,i,NULL,state,face,action,0);
                LCHECK(!er,funcname,ECbug,"bug in surfsetaction"); }}
        return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSurfaceRate */
extern CSTRING enum ErrorCode smolSetSurfaceRate(simptr sim,const char *surface,const char *species,enum MolecState state,enum MolecState state1,enum MolecState state2,double rate,const char *newspecies,int isinternal) {
	const char *funcname="smolSetSurfaceRate";
	int er,i,i2,s;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();s=-5;}
	else LCHECK(s>=0,funcname,ECsame,NULL);
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();i=-5;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK(state>=0 && state<MSMAX,funcname,ECbounds,"invalid state");
	LCHECK(state1>=0 && state1<MSMAX1,funcname,ECbounds,"invalid state1");
	LCHECK(state==MSsoln || state1==MSsoln || state1==MSbsoln || state1==state,funcname,ECsyntax,"nonsensical state combination");
	LCHECK(state2>=0 && state2<MSMAX1,funcname,ECbounds,"invalid state2");
	LCHECK(state1!=state2,funcname,ECsyntax,"cannot set rate for state1 = state2");
	if(newspecies && newspecies[0]!='\0') {
		i2=smolGetSpeciesIndexNT(sim,newspecies);
		LCHECK(i2>0,funcname,ECerror,"invalid newspecies"); }
	else i2=-5;
	LCHECK(rate>=0,funcname,ECbounds,"rate needs to be non-negative");
	LCHECK(!(isinternal && rate>1),funcname,ECbounds,"internal rate needs to be <= 1");

	if(s>=0) {
		srf=sim->srfss->srflist[s];
		er=surfsetrate(srf,i,NULL,state,state1,state2,i2,rate,isinternal?2:1);
		LCHECK(!er,funcname,ECerror,"error in surfsetrate"); }
	else {
		for(s=0;s<sim->srfss->nsrf;s++) {
			srf=sim->srfss->srflist[s];
			er=surfsetrate(srf,i,NULL,state,state1,state2,i2,rate,isinternal?2:1);
			LCHECK(!er,funcname,ECerror,"error in surfsetrate"); }}
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSurfaceActionRule */
extern CSTRING enum ErrorCode smolSetSurfaceActionRule(simptr sim,const char *surface,enum PanelFace face,const char *species,enum MolecState state,enum SrfAction action) {
	const char *funcname="smolSetSurfaceActionRule";
	char pattern[STRCHAR];
	int er,s,slo,shi,detailsi[3];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();slo=0;shi=sim->srfss->nsrf;}
	else {
		LCHECK(s>=0,funcname,ECsame,NULL);
		slo=s;
		shi=s+1; }
	LCHECK(face==PFfront || face==PFback || face==PFboth,funcname,ECbounds,"invalid face");
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECbounds,"invalid state");
	LCHECK((action>=0 && action<=SAmult) || action==SAadsorb,funcname,ECbounds,"invalid action");
	for(s=slo;s<shi;s++) {
		detailsi[0]=s;
		detailsi[1]=(int)face;
		detailsi[2]=(int)action;
		er=RuleAddRule(sim,RTsurfaction,NULL,pattern,&state,NULL,0,detailsi,NULL);
		LCHECK(!er,funcname,ECmemory,"out of memory adding surface action rule"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSurfaceRateRule */
extern CSTRING enum ErrorCode smolSetSurfaceRateRule(simptr sim,const char *surface,const char *species,enum MolecState state,enum MolecState state1,enum MolecState state2,double rate,const char *newspecies,int isinternal) {
	const char *funcname="smolSetSurfaceRateRule";
	char pattern[STRCHAR];
	int er,s,slo,shi,i3,detailsi[4];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();slo=0;shi=sim->srfss->nsrf;}
	else {
		LCHECK(s>=0,funcname,ECsame,NULL);
		slo=s;
		shi=s+1; }
	er=smolrulepattern(species,pattern);
	LCHECK(!er,funcname,ECsyntax,"unable to read species pattern");
	LCHECK(state>=0 && state<MSbsoln,funcname,ECbounds,"invalid state");
	LCHECK(state1>=0 && state1<=MSbsoln,funcname,ECbounds,"invalid state1");
	LCHECK(state2>=0 && state2<=MSbsoln,funcname,ECbounds,"invalid state2");
	LCHECK(state1!=state2,funcname,ECsyntax,"cannot set rate for state1 = state2");
	LCHECK(state==MSsoln || state1==state || state1==MSsoln || state1==MSbsoln,funcname,ECsyntax,"nonsensical state combination");
	if(isinternal) {
		LCHECK(rate>=0 && rate<=1,funcname,ECbounds,"internal rate needs to be between 0 and 1"); }
	else {
		LCHECK(rate>=0 || rate==-1,funcname,ECbounds,"rate needs to be non-negative, or -1 for the maximum"); }
	if(newspecies && newspecies[0]!='\0') {
		i3=smolGetSpeciesIndexNT(sim,newspecies);
		LCHECK(i3>0,funcname,ECnonexist,"unrecognized new species name"); }
	else i3=0;
	for(s=slo;s<shi;s++) {
		detailsi[0]=s;
		detailsi[1]=(int)state1;
		detailsi[2]=(int)state2;
		detailsi[3]=i3;
		er=RuleAddRule(sim,isinternal?RTsurfrateint:RTsurfrate,NULL,pattern,&state,NULL,rate,detailsi,NULL);
		LCHECK(!er,funcname,ECmemory,"out of memory adding surface rate rule"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddPanel */
extern CSTRING enum ErrorCode smolAddPanel(simptr sim,const char *surface,enum PanelShape panelshape,const char *panel,const char *axisstring,double *params) {
	const char *funcname="smolAddPanel";
	int er,s;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(panelshape>=0 && panelshape<PSMAX,funcname,ECnonexist,"invalid panel shape");
	if(panelshape==PSrect) {
		LCHECK(axisstring,funcname,ECmissing,"missing axisstring"); }
	LCHECK(params,funcname,ECmissing,"missing params");
	srf=sim->srfss->srflist[s];
	er=surfaddpanel(srf,sim->dim,panelshape,axisstring,params,panel);
	LCHECK(er!=-1,funcname,ECmemory,"out of memory adding panel");
	LCHECK(er!=3,funcname,ECsyntax,"cannot parse axisstring");
	LCHECK(er!=4,funcname,ECbounds,"drawing slices and stacks need to be positive");
	LCHECK(er!=5,funcname,ECbounds,"cylinder ends cannot be at the same location");
	LCHECK(er!=6,funcname,ECbounds,"hemisphere outward pointing vector has zero length");
	LCHECK(er!=7,funcname,ECbounds,"radius needs to be positive");
	LCHECK(er!=8,funcname,ECbounds,"normal vector has zero length");
	LCHECK(er!=9,funcname,ECerror,"panel name was used before for a different panel shape");
	LCHECK(!er,funcname,ECbug,"bug in smolAddPanel");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetPanelIndex */
extern CSTRING int smolGetPanelIndex(simptr sim,const char *surface,enum PanelShape *panelshapeptr,const char *panel) {
	const char *funcname="smolGetPanelIndex";
	surfaceptr srf;
	int p,s;
	enum PanelShape ps;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(panel,funcname,ECmissing,"missing panel name");
	LCHECK(strcmp(panel,"all"),funcname,ECall,"panel cannot be 'all'");
	srf=sim->srfss->srflist[s];
	p=-1;

#if 0
	for(ps=(enum PanelShape)0;ps<(enum PanelShape)PSMAX && p<0;ps=(enum PanelShape)(ps+1))
		p=stringfind(srf->pname[ps],srf->npanel[ps],panel);
#else
        for(auto ips : AllPanels_arr) {
            int _ips = static_cast<int>(ips);
            p        = stringfind(srf->pname[_ips], srf->npanel[_ips], panel);
            if(p >= 0) {
                ps = ips;
                break;
            }
        }
#endif

	LCHECK(p>=0,funcname,ECnonexist,"panel not found");
	if(panelshapeptr) *panelshapeptr=ps;
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetPanelIndexNT */
extern CSTRING int smolGetPanelIndexNT(simptr sim,const char *surface,enum PanelShape *panelshapeptr,const char *panel) {
	const char *funcname="smolGetPanelIndexNT";
	surfaceptr srf;
	int p,s;
	enum PanelShape ps;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECKNT(s>=0,funcname,ECsame,NULL);
	LCHECKNT(panel,funcname,ECmissing,"missing panel name");
	LCHECKNT(strcmp(panel,"all"),funcname,ECall,"panel cannot be 'all'");
	srf=sim->srfss->srflist[s];
	p=-1;
#if 0
	for(ps=(enum PanelShape)0;ps<(enum PanelShape)PSMAX && p<0;ps=(enum PanelShape)(ps+1))
		p=stringfind(srf->pname[ps],srf->npanel[ps],panel);
#else
    for(auto ips : AllPanels_arr) {
        int _ips = static_cast<int>(ips);
        p        = stringfind(srf->pname[_ips], srf->npanel[_ips], panel);
        if(p >= 0) {
            ps = ips;
            break;
        }
    }
#endif
	LCHECKNT(p>=0,funcname,ECnonexist,"panel not found");
	if(panelshapeptr) *panelshapeptr=ps;
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetPanelName */
extern CSTRING char *smolGetPanelName(simptr sim,const char *surface,enum PanelShape panelshape,int panelindex,char *panel) {
	const char *funcname="smolGetPanelName";
	surfaceptr srf;
	int s;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(panelshape>=0 && panelshape<PSMAX,funcname,ECnonexist,"invalid panel shape");
	LCHECK(panelindex>=0,funcname,ECbounds,"invalid panel index");
	LCHECK(panel,funcname,ECmissing,"missing panel name");
	srf=sim->srfss->srflist[s];
	LCHECK(panelindex<srf->npanel[panelshape],funcname,ECnonexist,"no panel exists with this number");
	strcpy(panel,srf->pname[panelshape][panelindex]);
	return panel;
 failure:
	return NULL; }


/* smolSetPanelJump */
extern CSTRING enum ErrorCode smolSetPanelJump(simptr sim,const char *surface,const char *panel1,enum PanelFace face1,const char *panel2,enum PanelFace face2,int isbidirectional) {
	const char *funcname="smolSetPanelJump";
	int s,p1,p2,er;
	surfaceptr srf;
	enum PanelShape ps1,ps2;
	panelptr pnl1,pnl2;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	p1=smolGetPanelIndexNT(sim,surface,&ps1,panel1);
	LCHECK(p1>=0,funcname,ECsame,NULL);
	p2=smolGetPanelIndexNT(sim,surface,&ps2,panel2);
	LCHECK(p2>=0,funcname,ECsame,NULL);
	LCHECK(ps1==ps2,funcname,ECerror,"origin and destination jump panels need to have the same shape");
	LCHECK(p1!=p2,funcname,ECerror,"origin and destination jump panels cannot be the same panel");
	LCHECK(face1==PFfront || face1==PFback,funcname,ECsyntax,"jumping panel face has to be either front or back");
	LCHECK(face2==PFfront || face2==PFback,funcname,ECsyntax,"jumping panel face has to be either front or back");
	LCHECK(isbidirectional==0 || isbidirectional==1,funcname,ECsyntax,"bidirectional code has to be 0 or 1");

	srf=sim->srfss->srflist[s];
	pnl1=srf->panels[ps1][p1];
	pnl2=srf->panels[ps2][p2];
	er=surfsetjumppanel(srf,pnl1,face1,isbidirectional,pnl2,face2);
	LCHECK(!er,funcname,ECbug,"BUG: error code returned by surfsetjumppanel");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddSurfaceUnboundedEmitter */
extern CSTRING enum ErrorCode smolAddSurfaceUnboundedEmitter(simptr sim,const char *surface,enum PanelFace face,const char *species,double emitamount,double *emitposition) {
	const char *funcname="smolAddSurfaceUnboundedEmitter";
	int s,i,er;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(face==PFfront || face==PFback,funcname,ECsyntax,"jumping panel face has to be either front or back");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);

	srf=sim->srfss->srflist[s];
	er=surfaddemitter(srf,face,i,emitamount,emitposition,sim->dim);
	LCHECK(!er,funcname,ECmemory,"out of memory allocating unbounded emitter");

	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSurfaceNeighborAction */
extern CSTRING enum ErrorCode smolSetSurfaceNeighborAction(simptr sim,const char *surface,int hop) {
	const char *funcname="smolSetSurfaceNeighborAction";
	int s,slo,shi;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();slo=0;shi=sim->srfss->nsrf;}
	else {
		LCHECK(s>=0,funcname,ECsame,NULL);
		slo=s;
		shi=s+1; }
	for(s=slo;s<shi;s++)
		surfsetneighhop(sim->srfss->srflist[s],hop?1:0);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetSurfaceSimParams */
extern CSTRING enum ErrorCode smolSetSurfaceSimParams(simptr sim,const char *parameter,double value) {
	const char *funcname="smolSetSurfaceSimParams";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(parameter,funcname,ECmissing,"missing parameter name");
	if(!strcmp(parameter,"epsilon")) {
		er=surfsetepsilon(sim,value);
		LCHECK(er!=2,funcname,ECmemory,"out of memory enabling surfaces");
		LCHECK(er!=3,funcname,ECbounds,"epsilon needs to be >0"); }
	else if(!strcmp(parameter,"margin")) {
		er=surfsetmargin(sim,value);
		LCHECK(er!=2,funcname,ECmemory,"out of memory enabling surfaces");
		LCHECK(er!=3,funcname,ECbounds,"margin needs to be >=0"); }
	else if(!strcmp(parameter,"neighbordist")) {
		er=surfsetneighdist(sim,value);
		LCHECK(er!=2,funcname,ECmemory,"out of memory enabling surfaces");
		LCHECK(er!=3,funcname,ECbounds,"neighbor distance needs to be >0"); }
	else
		LCHECK(0,funcname,ECsyntax,"parameter name not recognized");

	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddPanelNeighbor */
extern CSTRING enum ErrorCode smolAddPanelNeighbor(simptr sim,const char *surface1,const char *panel1,const char *surface2,const char *panel2,int reciprocal) {
	const char *funcname="smolAddPanelNeighbor";
	int s1,s2,p1,p2,er;
	panelptr pnl1,pnl2;
	enum PanelShape ps1,ps2;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s1=smolGetSurfaceIndexNT(sim,surface1);
	LCHECK(s1>=0,funcname,ECsame,NULL);
	s2=smolGetSurfaceIndexNT(sim,surface2);
	LCHECK(s2>=0,funcname,ECsame,NULL);
	p1=smolGetPanelIndexNT(sim,surface1,&ps1,panel1);
	LCHECK(p1>=0,funcname,ECsame,NULL);
	p2=smolGetPanelIndexNT(sim,surface2,&ps2,panel2);
	LCHECK(p2>=0,funcname,ECsame,NULL);
	LCHECK(!(s1==s2 && p1==p2 && ps1 == ps2),funcname,ECerror,"panels cannot be their own neighbors");

	pnl1=sim->srfss->srflist[s1]->panels[ps1][p1];
	pnl2=sim->srfss->srflist[s2]->panels[ps2][p2];
	er=surfsetneighbors(pnl1,&pnl2,1,1);
	LCHECK(!er,funcname,ECmemory,"out of memory adding panel neighbor");
	if(reciprocal) {
		er=surfsetneighbors(pnl2,&pnl1,1,1);
		LCHECK(!er,funcname,ECmemory,"out of memory adding panel neighbor"); }

	return ECok;
failure:
	return Liberrorcode; }


/* smolSetSurfaceStyle */
extern CSTRING enum ErrorCode smolSetSurfaceStyle(simptr sim,const char *surface,enum PanelFace face,enum DrawMode mode,double thickness,double *color,int stipplefactor,int stipplepattern,double shininess) {
	const char *funcname="smolSetSurfaceFaceStyle";
	int s,c,er;
	surfaceptr srf;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();s=-5;}
	else LCHECK(s>=0,funcname,ECsame,NULL);
	srf=sim->srfss->srflist[s];

	if(mode!=DMnone) {
		LCHECK(mode>=0 && mode<DMnone,funcname,ECsyntax,"mode not recognized");
		er=surfsetdrawmode(srf,face,mode);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetdrawmode"); }
	if(thickness>=0) {
		er=surfsetedgepts(srf,thickness);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetedgepts"); }
	if(color) {
		for(c=0;c<4;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds");
		er=surfsetcolor(srf,face,color);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetcolor"); }
	if(stipplefactor>=0) {
		LCHECK(stipplefactor>0,funcname,ECbounds,"stipplefactor needs to be >0");
		er=surfsetstipple(srf,stipplefactor,-1);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetstipple"); }
	if(stipplepattern>=0) {
		LCHECK(stipplepattern<=0xFFFF,funcname,ECbounds,"stipplepattern needs to be between 0 and 0xFFFF");
		er=surfsetstipple(srf,-1,stipplepattern);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetstipple"); }
	if(shininess>=0) {
		LCHECK(shininess<=128,funcname,ECbounds,"shininess cannot exceed 128");
		er=surfsetshiny(srf,face,shininess);
		LCHECK(!er,funcname,ECbug,"BUG: error in surfsetshiny"); }

	return ECok;
failure:
	return Liberrorcode; }


/******************************************************************************/
/******************************** Compartments ********************************/
/******************************************************************************/

/* smolAddCompartment */
extern CSTRING enum ErrorCode smolAddCompartment(simptr sim,const char *compartment) {
	const char *funcname="smolAddCompartment";
	int c;
	compartptr cmpt;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	c=smolGetCompartmentIndexNT(sim,compartment);
	if(c==(int)ECnonexist) smolClearError();
	else if(c<0) LCHECK(0,funcname,ECsame,NULL);
	else LCHECK(0,funcname,ECerror,"compartment is already in system");
	cmpt=compartaddcompart(sim,compartment);
	LCHECK(cmpt,funcname,ECmemory,"out of memory adding compartment");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetCompartmentIndex */
extern CSTRING int smolGetCompartmentIndex(simptr sim,const char *compartment) {
	const char *funcname="smolGetCompartmentIndex";
	int c;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(compartment,funcname,ECmissing,"missing compartment");
	LCHECK(sim->cmptss && sim->cmptss->ncmpt,funcname,ECnonexist,"no compartments defined");
	LCHECK(strcmp(compartment,"all"),funcname,ECall,"compartment cannot be 'all'");
	c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,compartment);
	LCHECK(c>=0,funcname,ECnonexist,"compartment not found");
	return c;
 failure:
	return (int)Liberrorcode; }


/* smolGetCompartmentIndexNT */
extern CSTRING int smolGetCompartmentIndexNT(simptr sim,const char *compartment) {
	const char *funcname="smolGetCompartmentIndexNT";
	int c;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(compartment,funcname,ECmissing,"missing compartment");
	LCHECKNT(sim->cmptss && sim->cmptss->ncmpt,funcname,ECnonexist,"no compartments defined");
	LCHECKNT(strcmp(compartment,"all"),funcname,ECall,"compartment cannot be 'all'");
	c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,compartment);
	LCHECKNT(c>=0,funcname,ECnonexist,"compartment not found");
	return c;
 failure:
	return (int)Liberrorcode; }


/* smolGetCompartmentName */
extern CSTRING char *smolGetCompartmentName(simptr sim,int compartmentindex,char *compartment) {
	const char *funcname="smolGetCompartmentName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(compartmentindex>=0,funcname,ECbounds,"invalid compartment index");
	LCHECK(compartment,funcname,ECmissing,"missing compartment string");
	LCHECK(sim->cmptss && sim->cmptss->ncmpt,funcname,ECnonexist,"no compartments defined");
	LCHECK(compartmentindex<sim->cmptss->ncmpt,funcname,ECnonexist,"compartment does not exist");
	strcpy(compartment,sim->cmptss->cnames[compartmentindex]);
	return compartment;
 failure:
	return NULL; }


/* smolAddCompartmentSurface */
extern CSTRING enum ErrorCode smolAddCompartmentSurface(simptr sim,const char *compartment,const char *surface) {
	const char *funcname="smolAddCompartmentSurface";
	int c,s,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	c=smolGetCompartmentIndexNT(sim,compartment);
	LCHECK(c>=0,funcname,ECsame,NULL);
	s=smolGetSurfaceIndexNT(sim,surface);
	LCHECK(s>=0,funcname,ECsame,NULL);
	er=compartaddsurf(sim->cmptss->cmptlist[c],sim->srfss->srflist[s]);
	LCHECK(!er,funcname,ECmemory,"out of memory in compartaddsurf");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddCompartmentPoint */
extern CSTRING enum ErrorCode smolAddCompartmentPoint(simptr sim,const char *compartment,double *point) {
	const char *funcname="smolAddCompartmentPoint";
	int c,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	c=smolGetCompartmentIndexNT(sim,compartment);
	LCHECK(c>=0,funcname,ECsame,NULL);
	LCHECK(point,funcname,ECmissing,"missing point");
	er=compartaddpoint(sim->cmptss->cmptlist[c],sim->dim,point);
	LCHECK(!er,funcname,ECmemory,"out of memory in compartaddsurf");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddCompartmentLogic */
extern CSTRING enum ErrorCode smolAddCompartmentLogic(simptr sim,const char *compartment,enum CmptLogic logic,const char *compartment2) {
	const char *funcname="smolAddCompartmentLogic";
	int c,c2,er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	c=smolGetCompartmentIndexNT(sim,compartment);
	LCHECK(c>=0,funcname,ECsame,NULL);
	LCHECK(logic>=CLequal && logic<CLnone,funcname,ECsyntax,"invalid logic operation");
	c2=smolGetCompartmentIndexNT(sim,compartment2);
	LCHECK(c2>=0,funcname,ECerror,"error with compartment2");
	er=compartaddcmptl(sim->cmptss->cmptlist[c],sim->cmptss->cmptlist[c2],logic);
	LCHECK(!er,funcname,ECmemory,"out of memory in compartaddcmpt");
	return ECok;
 failure:
	return Liberrorcode; }



/******************************************************************************/
/********************************* Reactions **********************************/
/******************************************************************************/

/* smolAddReaction */
extern CSTRING enum ErrorCode smolAddReaction(simptr sim,const char *reaction,const char *reactant1,enum MolecState rstate1,const char *reactant2,enum MolecState rstate2,int nproduct,const char **productspecies,enum MolecState *productstates,double rate) {
	const char *funcname="smolAddReaction";
	rxnptr rxn;
	int order,prd,rctident[MAXORDER],prdident[MAXPRODUCT],er;
	enum MolecState rctstate[MAXORDER];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(reaction,funcname,ECmissing,"missing reaction name");
	order=0;
	rctident[0]=rctident[1]=0;
	rctstate[0]=rctstate[1]=MSnone;
	if(reactant1 && reactant1[0]!='\0') {
		rctident[order]=smolGetSpeciesIndexNT(sim,reactant1);
		LCHECK(rctident[order]>0,funcname,ECsame,NULL);
		LCHECK(rstate1>=0 && rstate1<=MSMAX,funcname,ECbounds,"invalid rstate1");
		rctstate[order]=rstate1;
		order++; }
	if(reactant2 && reactant2[0]!='\0') {
		rctident[order]=smolGetSpeciesIndexNT(sim,reactant2);
		LCHECK(rctident[order]>0,funcname,ECsame,NULL);
		LCHECK(rstate2>=0 && rstate2<=MSMAX,funcname,ECbounds,"invalid rstate2");
		rctstate[order]=rstate2;
		order++; }
	LCHECK(nproduct>=0,funcname,ECbounds,"invalid nproduct");
	if(nproduct) {
		LCHECK(productspecies,funcname,ECmissing,"missing product species");
		LCHECK(productstates,funcname,ECmissing,"missing product states");
		for(prd=0;prd<nproduct;prd++) {
			prdident[prd]=smolGetSpeciesIndexNT(sim,productspecies[prd]);
			LCHECK(prdident[prd]>0,funcname,ECsame,NULL);
			LCHECK(productstates[prd]>=MSsoln && productstates[prd]<=MSMAX,funcname,ECsyntax,"invalid product state"); }}
	rxn=RxnAddReaction(sim,reaction,order,rctident,rctstate,nproduct,prdident,productstates,NULL,NULL);
	LCHECK(rxn,funcname,ECmemory,"out of memory allocating reaction");

	if(rate>=0) {
		er=RxnSetValue(sim,"rate",rxn,rate);
		if(er==3) LCHECK(0,funcname,ECwarning,"rate was set previously");
		else LCHECK(!er,funcname,ECbug,"RxnSetValue error"); }

	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolGetReactionIndex */
extern CSTRING int smolGetReactionIndex(simptr sim,int *orderptr,const char *reaction) {
	const char *funcname="smolGetReactionIndex";
	int order,r;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(reaction,funcname,ECmissing,"missing reaction");
	LCHECK(strcmp(reaction,"all"),funcname,ECall,"reaction cannot be 'all'");
	if(orderptr && *orderptr>=0 && *orderptr<MAXORDER) {
		order=*orderptr;
		LCHECK(sim->rxnss[order] && sim->rxnss[order]->totrxn,funcname,ECnonexist,"no reactions defined of this order");
		r=stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,reaction);
		LCHECK(r>=0,funcname,ECnonexist,"reaction not found"); }
	else {
		r=-1;
		for(order=0;order<MAXORDER && r<0;order++)
			if(sim->rxnss[order])
				r=stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,reaction);
		LCHECK(r>=0,funcname,ECnonexist,"reaction not found");
		if(orderptr) *orderptr=order-1; }
	return r;
 failure:
	return (int)Liberrorcode; }


/* smolGetReactionIndexNT */
extern CSTRING int smolGetReactionIndexNT(simptr sim,int *orderptr,const char *reaction) {
	const char *funcname="smolGetReactionIndexNT";
	int order,r;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(reaction,funcname,ECmissing,"missing reaction");
	LCHECKNT(strcmp(reaction,"all"),funcname,ECall,"reaction cannot be 'all'");
	if(orderptr && *orderptr>=0 && *orderptr<MAXORDER) {
		order=*orderptr;
		LCHECKNT(sim->rxnss[order] && sim->rxnss[order]->totrxn,funcname,ECnonexist,"no reactions defined of this order");
		r=stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,reaction);
		LCHECKNT(r>=0,funcname,ECnonexist,"reaction not found"); }
	else {
		r=-1;
		for(order=0;order<MAXORDER && r<0;order++)
			if(sim->rxnss[order])
				r=stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,reaction);
		LCHECKNT(r>=0,funcname,ECnonexist,"reaction not found");
		if(orderptr) *orderptr=order-1; }
	return r;
 failure:
	return (int)Liberrorcode; }


/* smolGetReactionName */
extern CSTRING char *smolGetReactionName(simptr sim,int order,int reactionindex,char *reaction) {
	const char *funcname="smolGetReactionName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(order>=0 && order<MAXORDER,funcname,ECbounds,"invalid reaction order");
	LCHECK(reactionindex>=0,funcname,ECbounds,"invalid reaction name");
	LCHECK(reaction,funcname,ECmissing,"missing reaction");
	LCHECK(sim->rxnss[order] && sim->rxnss[order]->totrxn,funcname,ECnonexist,"no reactions defined of this order");
	LCHECK(reactionindex<sim->rxnss[order]->totrxn,funcname,ECnonexist,"reaction does not exist");
	strcpy(reaction,sim->rxnss[order]->rname[reactionindex]);
	return reaction;
failure:
	return NULL; }


/* smolSetReactionRate */
extern CSTRING enum ErrorCode smolSetReactionRate(simptr sim,const char *reaction,double rate,int isinternal) {
	const char *funcname="smolSetReactionRate";
	int r,er,order;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	rxn=sim->rxnss[order]->rxn[r];
	if(!isinternal)
		er=RxnSetValue(sim,"rate",rxn,rate);
	else if(order<2)
		er=RxnSetValue(sim,"prob",rxn,rate);
	else if(isinternal==1)
		er=RxnSetValue(sim,"bindrad",rxn,rate);
	else
		er=RxnSetValue(sim,"prob",rxn,rate);
	if(er==3) LCHECK(0,funcname,ECwarning,"rate was set previously");
	else LCHECK(!er,funcname,ECbug,"RxnSetValue error");
	return Libwarncode;
 failure:
	return Liberrorcode; }


/* smolGetReactionRate */
extern CSTRING enum ErrorCode smolGetReactionRate(simptr sim,const char *reaction,double *rate) {
	const char *funcname="smolGetReactionRate";
	int r,order;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(rate,funcname,ECmissing,"missing rate pointer");
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	rxn=sim->rxnss[order]->rxn[r];
	*rate=rxn->rate;
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetReactionRegion */
extern CSTRING enum ErrorCode smolSetReactionRegion(simptr sim,const char *reaction,const char *compartment,const char *surface) {
	const char *funcname="smolSetReactionRegion";
	int order,r,c,s;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	rxn=sim->rxnss[order]->rxn[r];

	if(compartment && compartment[0]!='\0') {
		c=smolGetCompartmentIndexNT(sim,compartment);
		LCHECK(c>=0,funcname,ECsame,NULL);
		RxnSetCmpt(rxn,sim->cmptss->cmptlist[c]); }
	else if(compartment && compartment[0]=='\0')
		RxnSetCmpt(rxn,NULL);

	if(surface && surface[0]!='\0') {
		s=smolGetSurfaceIndexNT(sim,surface);
		LCHECK(s>=0,funcname,ECsame,NULL);
		RxnSetSurface(rxn,sim->srfss->srflist[s]); }
	else if(surface && surface[0]=='\0')
		RxnSetSurface(rxn,NULL);

	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetReactionIntersurface */
enum ErrorCode smolSetReactionIntersurface(
    simptr sim, const char *reaction, int *rulelist)
{
    const char *funcname = "smolSetReactionIntersurface";
    int order, r, i;
    int er;
    listptrv vlist;
    rxnptr rxn = NULL;

    readrxnname(sim, reaction, &order, &rxn, NULL, 1);
    if(!rxn)
        readrxnname(sim, reaction, &order, &rxn, NULL, 2);
    if(!rxn)
        readrxnname(sim, reaction, &order, &rxn, NULL, 3);

    LCHECK(rxn, funcname, ECnonexist, NULL);
    LCHECK(order == 2, funcname, ECerror, NULL);

    rxn = NULL;
    r   = readrxnname(sim, reaction, NULL, &rxn, NULL, 1);
    if(r >= 0) {
        er = RxnSetIntersurfaceRules(rxn, rulelist);
        LCHECK(er != 1, funcname, ECmemory, NULL);
    }
    r = readrxnname(sim, reaction, NULL, &rxn, &vlist, 2);
    LCHECK(r != -2, funcname, ECmemory, NULL);
    if(r >= 0) {
        for(i = 0; i < vlist->n; i++) {
            er = RxnSetIntersurfaceRules((rxnptr)vlist->xs[i], rulelist);
            LCHECK(er != 1, funcname, ECmemory, NULL);
        }
        ListFreeV(vlist);
    }
    r = readrxnname(sim, reaction, NULL, &rxn, NULL, 3);
    LCHECK(r != -2, funcname, ECmemory, NULL);
    if(r >= 0) {
        er = RxnSetIntersurfaceRules(rxn, rulelist);
        LCHECK(er != 1, funcname, ECmemory, NULL);
    }

    return ECok;
failure:
    return Liberrorcode;
}

/* smolSetReactionProducts */
extern CSTRING enum ErrorCode smolSetReactionProducts(simptr sim,const char *reaction,enum RevParam method,double parameter,const char *product,double *position) {
	const char *funcname="smolSetReactionProducts";
	int order,r,done,prd,i,er;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	rxn=sim->rxnss[order]->rxn[r];

	prd=-1;
	if(product && strlen(product)>0) {
		i=smolGetSpeciesIndexNT(sim,product);
		LCHECK(i>0,funcname,ECsame,NULL);
		done=0;
		for(prd=0;prd<rxn->nprod && !done;prd++)
			if(rxn->prdident[prd]==i) done=1;
		prd--;
		LCHECK(done,funcname,ECerror,"listed product is not a product of the listed reaction"); }

	er=RxnSetRevparam(sim,rxn,method,parameter,prd,position,sim->dim);
	LCHECK(er!=1,funcname,ECwarning,"reaction product parameter was set before");
	LCHECK(er!=2,funcname,ECbounds,"reaction product parameter out of bounds");
	LCHECK(er!=3,funcname,ECnonexist,"invalid reaction product method");
	LCHECK(er!=4,funcname,ECmissing,"missing product name");
	LCHECK(er!=5,funcname,ECmissing,"missing product position");

	return Libwarncode;
failure:
	return Liberrorcode; }


/* smolfindreactions. Finds all reactions that match the name reaction, using
the same matching as the configuration file statements: a reaction with this
exact name, reactions generated from a rule with this name (name_number), and the
template reaction for a rule with this name. Returns 0 if any were found, -1 if
none were found, or -2 for out of memory. *vlistptr is returned with a list of
the matching reactions (or NULL), and *rulerxnptr with the rule template reaction
(or NULL); the template is not included in the list because it is not part of
the reaction superstructure. *ruleorderptr is set to the order of the rule
template. */
static int smolfindreactions(simptr sim,const char *reaction,listptrv *vlistptr,rxnptr *rulerxnptr,int *ruleorderptr) {
	int r,i,order;
	rxnptr rxn;
	listptrv vlist,vlist2;

	vlist=NULL;
	*vlistptr=NULL;
	*rulerxnptr=NULL;
	*ruleorderptr=-1;

	rxn=NULL;
	r=readrxnname(sim,reaction,NULL,&rxn,NULL,1);
	if(r>=0) {
		vlist=ListAppendItemV(NULL,(void*) rxn);
		if(!vlist) return -2; }

	vlist2=NULL;
	r=readrxnname(sim,reaction,NULL,NULL,&vlist2,2);
	if(r==-2) {ListFreeV(vlist);return -2;}
	if(r>=0) {
		for(i=0;i<vlist2->n;i++) {
			vlist=ListAppendItemV(vlist,vlist2->xs[i]);
			if(!vlist) {ListFreeV(vlist2);return -2;}}
		ListFreeV(vlist2); }

	rxn=NULL;
	r=readrxnname(sim,reaction,&order,&rxn,NULL,3);
	if(r==-2) {ListFreeV(vlist);return -2;}
	if(r>=0) {
		*rulerxnptr=rxn;
		*ruleorderptr=order; }

	*vlistptr=vlist;
	return (vlist || *rulerxnptr)?0:-1; }


/* smolAddReactionRule */
extern CSTRING enum ErrorCode smolAddReactionRule(simptr sim,const char *rule,const char *reactant1,enum MolecState rstate1,const char *reactant2,enum MolecState rstate2,int nproduct,const char **productspecies,enum MolecState *productstates,double rate,const char *compartment,const char *surface) {
	const char *funcname="smolAddReactionRule";
	char pattern[STRCHAR];
	int order,prd,er,detailsi[2];
	enum MolecState ms,rctstate[MAXORDER],prdstate[MAXPRODUCT];

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECerror,"need to enter species before reaction rules");
	LCHECK(rule && rule[0]!='\0',funcname,ECmissing,"missing rule name");
	for(order=0;order<MAXORDER;order++)
		if(sim->rxnss[order]) {
			LCHECK(stringfind(sim->rxnss[order]->rname,sim->rxnss[order]->totrxn,rule)<0,funcname,ECerror,"rule name has already been used as a reaction name"); }

	detailsi[0]=-1;																// compartment
	if(compartment && compartment[0]!='\0') {
		detailsi[0]=smolGetCompartmentIndexNT(sim,compartment);
		LCHECK(detailsi[0]>=0,funcname,ECsame,NULL); }
	detailsi[1]=-1;																// surface
	if(surface && surface[0]!='\0') {
		detailsi[1]=smolGetSurfaceIndexNT(sim,surface);
		LCHECK(detailsi[1]>=0,funcname,ECsame,NULL); }

	pattern[0]='\0';
	order=0;
	if(reactant1 && reactant1[0]!='\0') {
		LCHECK(rstate1>=0 && (rstate1<MSMAX1 || rstate1==MSall),funcname,ECbounds,"invalid rstate1");
		er=molstring2pattern(reactant1,&ms,pattern,1);
		LCHECK(er!=-4,funcname,ECerror,"reactant list exceeds maximum string length");
		LCHECK(!er,funcname,ECsyntax,"unable to read reactant1");
		rctstate[order++]=rstate1; }
	if(reactant2 && reactant2[0]!='\0') {
		LCHECK(rstate2>=0 && (rstate2<MSMAX1 || rstate2==MSall),funcname,ECbounds,"invalid rstate2");
		er=molstring2pattern(reactant2,&ms,pattern,1);
		LCHECK(er!=-4,funcname,ECerror,"reactant list exceeds maximum string length");
		LCHECK(!er,funcname,ECsyntax,"unable to read reactant2");
		rctstate[order++]=rstate2; }

	LCHECK(nproduct>=0 && nproduct<MAXPRODUCT,funcname,ECbounds,"invalid nproduct");
	LCHECK(!(order==0 && nproduct==0),funcname,ECerror,"reaction rule has neither reactants nor products");
	if(nproduct) {
		LCHECK(productspecies,funcname,ECmissing,"missing product species");
		LCHECK(productstates,funcname,ECmissing,"missing product states"); }
	for(prd=0;prd<nproduct;prd++) {
		LCHECK(productspecies[prd] && productspecies[prd][0]!='\0',funcname,ECmissing,"missing product species");
		LCHECK(productstates[prd]>=MSsoln && productstates[prd]<MSMAX1,funcname,ECsyntax,"invalid product state");
		LCHECK(!(order==0 && detailsi[1]<0 && productstates[prd]!=MSsoln),funcname,ECerror,"enter a surface for order 0 reaction rules with surface-bound products");
		er=molstring2pattern(productspecies[prd],&ms,pattern,2);
		LCHECK(er!=-4,funcname,ECerror,"reactant and product list together exceed maximum string length");
		LCHECK(!er,funcname,ECsyntax,"unable to read product species");
		prdstate[prd]=productstates[prd]; }
	if(nproduct==0) {
		er=molstring2pattern(NULL,&ms,pattern,2);
		LCHECK(!er,funcname,ECbug,"BUG: error in molstring2pattern"); }

	er=RuleAddRule(sim,RTreaction,rule,pattern,rctstate,prdstate,rate>=0?rate:-1,detailsi,NULL);
	LCHECK(!er,funcname,ECerror,"out of memory or other error adding rule");

	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetReactionSimParams */
extern CSTRING enum ErrorCode smolSetReactionSimParams(simptr sim,const char *reaction,const char *parameter,double value) {
	const char *funcname="smolSetReactionSimParams";
	const char *option=NULL;
	int er,i,ruleorder,order;
	rxnptr rxn,rulerxn;
	listptrv vlist;

	vlist=NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(reaction,funcname,ECmissing,"missing reaction");
	LCHECK(parameter,funcname,ECmissing,"missing parameter");

	if(!strcmp(parameter,"rate")) {
		LCHECK(value>=0,funcname,ECbounds,"reaction rate value must be non-negative");
		option="rate"; }
	else if(!strcmp(parameter,"multiplicity")) {
		LCHECK(value>=0,funcname,ECbounds,"reaction multiplicity value must be non-negative");
		value=(double)((int)value);
		option="multiplicity"; }
	else if(!strcmp(parameter,"confspread_radius")) {
		LCHECK(value>=0,funcname,ECbounds,"confspread radius value must be non-negative");
		option="confspreadrad"; }
	else if(!strcmp(parameter,"binding_radius")) {
		LCHECK(value>=0,funcname,ECbounds,"binding radius value must be non-negative");
		option="bindrad"; }
	else if(!strcmp(parameter,"probability")) {
		LCHECK(value>=0 && value<=1,funcname,ECbounds,"probability value must be between 0 and 1");
		option="prob"; }
	else if(!strcmp(parameter,"chi")) {
		LCHECK(value!=0 && value<1,funcname,ECbounds,"reaction chi value must be between 0 and 1 (or -1 to disable)");
		option="chi"; }
	else if(!strcmp(parameter,"production")) {
		LCHECK(value>=0,funcname,ECbounds,"production value must be at least 0");
		option="prob"; }
	else
		LCHECK(0,funcname,ECsyntax,"parameter name not recognized");

	er=smolfindreactions(sim,reaction,&vlist,&rulerxn,&ruleorder);
	LCHECK(er!=-2,funcname,ECmemory,"out of memory finding reactions");
	LCHECK(er!=-1,funcname,ECnonexist,"reaction not found");

	for(i=0;vlist && i<vlist->n;i++) {						// check reaction orders
		order=((rxnptr) vlist->xs[i])->rxnss->order;
		if(!strcmp(parameter,"chi")) {
			LCHECK(order==2,funcname,ECerror,"reaction chi value is only allowed for order 2 reactions"); }
		else if(!strcmp(parameter,"production")) {
			LCHECK(order==0,funcname,ECerror,"production is only allowed for order 0 reactions"); }}
	if(rulerxn) {
		if(!strcmp(parameter,"chi")) {
			LCHECK(ruleorder==2,funcname,ECerror,"reaction chi value is only allowed for order 2 reactions"); }
		else if(!strcmp(parameter,"production")) {
			LCHECK(ruleorder==0,funcname,ECerror,"production is only allowed for order 0 reactions"); }}

	for(i=0;vlist && i<vlist->n;i++) {
		rxn=(rxnptr) vlist->xs[i];
		er=RxnSetValue(sim,option,rxn,value);
		LCHECK(er!=4,funcname,ECbounds,"parameter value is out of bounds");
		LCHECK(!er,funcname,ECbug,"RxnSetValue error"); }
	if(rulerxn) {
		er=RxnSetValue(NULL,option,rulerxn,value);
		LCHECK(er!=4,funcname,ECbounds,"parameter value is out of bounds");
		LCHECK(!er,funcname,ECbug,"RxnSetValue error"); }

	ListFreeV(vlist);
	return ECok;
 failure:
	ListFreeV(vlist);
	return Liberrorcode; }


/* smolSetReactionPermit */
extern CSTRING enum ErrorCode smolSetReactionPermit(simptr sim,const char *reaction,enum MolecState *states,int permit) {
	const char *funcname="smolSetReactionPermit";
	int order,r,ord;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(states,funcname,ECmissing,"missing states");
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	LCHECK(order>0,funcname,ECerror,"reaction permit and forbid are not allowed for order 0 reactions");
	rxn=sim->rxnss[order]->rxn[r];
	for(ord=0;ord<order;ord++)
		LCHECK(states[ord]>=0 && (states[ord]<MSMAX1 || states[ord]==MSall),funcname,ECbounds,"invalid state");

	RxnSetPermit(sim,rxn,order,states,permit?1:0);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetReactionSerialnum */
extern CSTRING enum ErrorCode smolSetReactionSerialnum(simptr sim,const char *reaction,const char **codes) {
	const char *funcname="smolSetReactionSerialnum";
	char pattern[STRCHAR];
	int er,i,prd,nprod,ruleorder;
	long int sernolist[MAXPRODUCT];
	rxnptr rxn,rulerxn;
	listptrv vlist;

	vlist=NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(reaction,funcname,ECmissing,"missing reaction");
	er=smolfindreactions(sim,reaction,&vlist,&rulerxn,&ruleorder);
	LCHECK(er!=-2,funcname,ECmemory,"out of memory finding reactions");
	LCHECK(er!=-1,funcname,ECnonexist,"reaction not found");

	rxn=vlist?(rxnptr) vlist->xs[0]:rulerxn;
	nprod=rxn->nprod;
	LCHECK(nprod==0 || codes,funcname,ECmissing,"missing product codes");
	for(prd=0;prd<nprod;prd++) {
		LCHECK(codes[prd] && strlen(codes[prd])<STRCHAR,funcname,ECmissing,"missing product code");
		strcpy(pattern,codes[prd]);
		sernolist[prd]=rxnstring2sernocode(pattern,prd);
		LCHECK(sernolist[prd]!=0,funcname,ECsyntax,"error reading a product code"); }

	for(i=0;vlist && i<vlist->n;i++) {
		rxn=(rxnptr) vlist->xs[i];
		LCHECK(rxn->nprod==nprod,funcname,ECerror,"matching reactions have different numbers of products");
		er=RxnSetPrdSerno(rxn,sernolist);
		LCHECK(!er,funcname,ECmemory,"out of memory allocating product serial number list"); }
	if(rulerxn) {
		LCHECK(rulerxn->nprod==nprod,funcname,ECerror,"matching reactions have different numbers of products");
		er=RxnSetPrdSerno(rulerxn,sernolist);
		LCHECK(!er,funcname,ECmemory,"out of memory allocating product serial number list"); }

	ListFreeV(vlist);
	return ECok;
 failure:
	ListFreeV(vlist);
	return Liberrorcode; }


/* smolSetReactionRepresentation */
extern CSTRING enum ErrorCode smolSetReactionRepresentation(simptr sim,const char *reaction,const enum SpeciesRepresentation *rctrep,const enum SpeciesRepresentation *prdrep) {
	const char *funcname="smolSetReactionRepresentation";
	int er,i,rct,prd,order,nprod,ruleorder;
	rxnptr rxn,rulerxn;
	listptrv vlist;

	vlist=NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(reaction,funcname,ECmissing,"missing reaction");
	er=smolfindreactions(sim,reaction,&vlist,&rulerxn,&ruleorder);
	LCHECK(er!=-2,funcname,ECmemory,"out of memory finding reactions");
	LCHECK(er!=-1,funcname,ECnonexist,"reaction not found");

	rxn=vlist?(rxnptr) vlist->xs[0]:rulerxn;
	order=vlist?rxn->rxnss->order:ruleorder;
	nprod=rxn->nprod;
	LCHECK(order==0 || rctrep,funcname,ECmissing,"missing reactant representations");
	LCHECK(nprod==0 || prdrep,funcname,ECmissing,"missing product representations");
	for(rct=0;rct<order;rct++)
		LCHECK(rctrep[rct]==SRparticle || rctrep[rct]==SRlattice || rctrep[rct]==SRboth,funcname,ECsyntax,"invalid reactant representation");
	for(prd=0;prd<nprod;prd++)
		LCHECK(prdrep[prd]==SRparticle || prdrep[prd]==SRlattice,funcname,ECsyntax,"product representation must be particle or lattice");

	for(i=0;vlist && i<vlist->n;i++) {
		rxn=(rxnptr) vlist->xs[i];
		LCHECK(rxn->rxnss->order==order && rxn->nprod==nprod,funcname,ECerror,"matching reactions have different numbers of reactants or products");
		er=RxnSetRepresentationRules(rxn,order,rctrep,prdrep);
		LCHECK(!er,funcname,ECmemory,"out of memory allocating reaction representation rules"); }
	if(rulerxn) {
		LCHECK(ruleorder==order && rulerxn->nprod==nprod,funcname,ECerror,"matching reactions have different numbers of reactants or products");
		er=RxnSetRepresentationRules(rulerxn,order,rctrep,prdrep);
		LCHECK(!er,funcname,ECmemory,"out of memory allocating reaction representation rules"); }

	ListFreeV(vlist);
	return ECok;
 failure:
	ListFreeV(vlist);
	return Liberrorcode; }


/* smolSetReactionLog */
extern CSTRING enum ErrorCode smolSetReactionLog(simptr sim,const char *filename,const char *reaction,int nserial,const long int *serialnums,int turnon) {
	const char *funcname="smolSetReactionLog";
	char fname[STRCHAR];
	int er,i,ruleorder;
	rxnptr rulerxn;
	listptrv vlist;
	listptrli lilist;

	vlist=NULL;
	lilist=NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	if(turnon) {
		LCHECK(filename && filename[0]!='\0',funcname,ECmissing,"missing filename");
		LCHECK(strlen(filename)<STRCHAR,funcname,ECbounds,"filename is too long");
		strcpy(fname,filename); }
	else
		fname[0]='\0';

	if(nserial<=0 || !serialnums) {							// all serial numbers
		lilist=ListAppendItemLI(NULL,-1);
		LCHECK(lilist,funcname,ECmemory,"out of memory"); }
	else
		for(i=0;i<nserial;i++) {
			lilist=ListAppendItemLI(lilist,serialnums[i]);
			LCHECK(lilist,funcname,ECmemory,"out of memory"); }

	if(!reaction || reaction[0]=='\0' || !strcmp(reaction,"all")) {	// all current reactions, no rules
		er=RxnSetLog(sim,turnon?fname:NULL,NULL,lilist,turnon?1:0);
		LCHECK(er!=2,funcname,ECerror,"cannot log a reaction to multiple output files");
		LCHECK(!er,funcname,ECmemory,"out of memory in reaction log"); }
	else {
		er=smolfindreactions(sim,reaction,&vlist,&rulerxn,&ruleorder);
		LCHECK(er!=-2,funcname,ECmemory,"out of memory finding reactions");
		LCHECK(er!=-1,funcname,ECnonexist,"reaction not found");
		for(i=0;vlist && i<vlist->n;i++) {
			er=RxnSetLog(sim,turnon?fname:NULL,(rxnptr) vlist->xs[i],lilist,turnon?1:0);
			LCHECK(er!=2,funcname,ECerror,"cannot log a reaction to multiple output files");
			LCHECK(!er,funcname,ECmemory,"out of memory in reaction log"); }
		if(rulerxn) {
			er=RxnSetLog(NULL,turnon?fname:NULL,rulerxn,lilist,turnon?1:0);
			LCHECK(er!=2,funcname,ECerror,"cannot log a reaction to multiple output files");
			LCHECK(!er,funcname,ECmemory,"out of memory in reaction log"); }}

	ListFreeV(vlist);
	ListFreeLI(lilist);
	return ECok;
 failure:
	ListFreeV(vlist);
	ListFreeLI(lilist);
	return Liberrorcode; }


/* smolExpandRules */
extern CSTRING enum ErrorCode smolExpandRules(simptr sim,int iterations) {
	const char *funcname="smolExpandRules";
	int er;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(iterations>=-2,funcname,ECbounds,"iterations needs to be >=0, or -1 for all, or -2 for on-the-fly");
	er=RuleExpandRules(sim,iterations);
	LCHECK(er!=-1 && er!=-11,funcname,ECmemory,"out of memory while expanding rules");
	LCHECK(er!=-5,funcname,ECerror,"generated species name exceeded maximum allowed length");
	LCHECK(er!=-41,funcname,ECnonexist,"no rules to expand");
	LCHECK(!er,funcname,ECbug,"BUG: error in RuleExpandRules");
	return ECok;
 failure:
	return Liberrorcode; }



/******************************************************************************/
/*********************************** Ports ************************************/
/******************************************************************************/

/* smolAddPort */
extern CSTRING enum ErrorCode smolAddPort(simptr sim,const char *port,const char *surface,enum PanelFace face) {
	const char *funcname="smolAddPort";
	int s;
	portptr simport;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(port,funcname,ECmissing,"missing port");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECmissing) smolClearError();
	else LCHECK(s>=0,funcname,ECsame,NULL);
	LCHECK(face==PFfront || face==PFback || face==PFnone,funcname,ECsyntax,"invalid face");
	simport=portaddport(sim,port,sim->srfss->srflist[s],face);
	LCHECK(simport,funcname,ECmemory,"out of memory adding port");

	return ECok;
 failure:
	return Liberrorcode; }


/* smolGetPortIndex */
extern CSTRING int smolGetPortIndex(simptr sim,const char *port) {
	const char *funcname="smolGetPortIndex";
	int p;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(port,funcname,ECmissing,"missing port");
	LCHECK(sim->portss && sim->portss->nport,funcname,ECnonexist,"no ports defined");
	LCHECK(strcmp(port,"all"),funcname,ECall,"port cannot be 'all'");
	p=stringfind(sim->portss->portnames,sim->portss->nport,port);
	LCHECK(p>=0,funcname,ECnonexist,"port not found");
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetPortIndexNT */
extern CSTRING int smolGetPortIndexNT(simptr sim,const char *port) {
	const char *funcname="smolGetPortIndexNT";
	int p;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(port,funcname,ECmissing,"missing port");
	LCHECKNT(sim->portss && sim->portss->nport,funcname,ECnonexist,"no ports defined");
	LCHECKNT(strcmp(port,"all"),funcname,ECall,"port cannot be 'all'");
	p=stringfind(sim->portss->portnames,sim->portss->nport,port);
	LCHECKNT(p>=0,funcname,ECnonexist,"port not found");
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetPortName */
extern CSTRING char *smolGetPortName(simptr sim,int portindex,char *port) {
	const char *funcname="smolGetPortName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(portindex>=0,funcname,ECbounds,"invalid port index");
	LCHECK(port,funcname,ECmissing,"missing port string");
	LCHECK(sim->portss && sim->portss->nport,funcname,ECnonexist,"no ports defined");
	LCHECK(portindex<sim->portss->nport,funcname,ECnonexist,"port does not exist");
	strcpy(port,sim->portss->portnames[portindex]);
	return port;
 failure:
	return NULL; }


/* smolAddPortMolecules */
extern CSTRING enum ErrorCode smolAddPortMolecules(simptr sim,const char *port,int nmolec,const char *species,double **positions) {
	const char *funcname="smolAddPortMolecules";
	int prt,i,er;
	portptr simport;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	prt=smolGetPortIndexNT(sim,port);
	LCHECK(prt>=0,funcname,ECsame,NULL);
	simport=sim->portss->portlist[prt];
	if(nmolec==0) return ECok;
	LCHECK(nmolec>0,funcname,ECbounds,"nmolec cannot be negative");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	er=portputmols(sim,simport,nmolec,i,NULL,positions,NULL);
	LCHECK(er!=1,funcname,ECmemory,"out of memory");
	LCHECK(er!=2,funcname,ECnonexist,"no porting surface defined");
	LCHECK(er!=3,funcname,ECnonexist,"no porting face defined");
	LCHECK(er!=4,funcname,ECnonexist,"no panels on porting surface");

	return ECok;
failure:
	return Liberrorcode; }


/* smolGetPortMolecules */
extern CSTRING int smolGetPortMolecules(simptr sim,const char *port,const char *species,enum MolecState state,int remove) {
	const char *funcname="smolGetPortMolecules";
	int prt,i,num;
	portptr simport;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	prt=smolGetPortIndexNT(sim,port);
	LCHECK(prt>=0,funcname,ECsame,NULL);
	simport=sim->portss->portlist[prt];

	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {smolClearError();i=-5;}
	else LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK((state>=0 && state<MSMAX) || state==MSall,funcname,ECsyntax,"invalid state");

	num=portgetmols(sim,simport,i,state,remove);
	return num;
failure:
	return (int)Liberrorcode; }



/******************************************************************************/
/*********************************** Lattices *********************************/
/******************************************************************************/

/* smolAddLattice */
extern CSTRING enum ErrorCode smolAddLattice(simptr sim,const char *lattice,const double *min,const double *max,const double *dx,const char *btype) {
	const char *funcname="smolAddLattice";
	int er,p;
	latticeptr simlattice;
	simlattice = NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(lattice,funcname,ECmissing,"missing lattice");
	p=sim->latticess?stringfind(sim->latticess->latticenames,sim->latticess->nlattice,lattice):-1;
	LCHECK(p<0,funcname,ECsame,"lattice name already exists");
	er=latticeaddlattice(sim,&simlattice,lattice,min,max,dx,btype,LATTICEnsv);
	LCHECK(er==0,funcname,ECerror,"error adding lattice");
	LCHECK(simlattice,funcname,ECmemory,"out of memory adding lattice");
	return ECok;
 failure:
	return Liberrorcode;
}


/* smolGetLatticeIndex */
extern CSTRING int smolGetLatticeIndex(simptr sim,const char *lattice) {
	const char *funcname="smolGetLatticeIndex";
	int p;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(lattice,funcname,ECmissing,"missing lattice");
	LCHECK(sim->latticess && sim->latticess->nlattice,funcname,ECnonexist,"no lattices defined");
	LCHECK(strcmp(lattice,"all"),funcname,ECall,"lattice cannot be 'all'");
	p=stringfind(sim->latticess->latticenames,sim->latticess->nlattice,lattice);
	LCHECK(p>=0,funcname,ECnonexist,"lattice not found");
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetLatticeIndexNT */
extern CSTRING int smolGetLatticeIndexNT(simptr sim,const char *lattice) {
	const char *funcname="smolGetLatticeIndexNT";
	int p;

	LCHECKNT(sim,funcname,ECmissing,"missing sim");
	LCHECKNT(lattice,funcname,ECmissing,"missing lattice");
	LCHECKNT(sim->latticess && sim->latticess->nlattice,funcname,ECnonexist,"no lattices defined");
	LCHECKNT(strcmp(lattice,"all"),funcname,ECall,"lattice cannot be 'all'");
	p=stringfind(sim->latticess->latticenames,sim->latticess->nlattice,lattice);
	LCHECKNT(p>=0,funcname,ECnonexist,"lattice not found");
	return p;
 failure:
	return (int)Liberrorcode; }


/* smolGetLatticeName */
extern CSTRING char *smolGetLatticeName(simptr sim,int latticeindex,char *lattice) {
	const char *funcname="smolGetLatticeName";

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(latticeindex>=0,funcname,ECbounds,"invalid lattice index");
	LCHECK(lattice,funcname,ECmissing,"missing lattice string");
	LCHECK(sim->latticess && sim->latticess->nlattice,funcname,ECnonexist,"no lattices defined");
	LCHECK(latticeindex<sim->latticess->nlattice,funcname,ECnonexist,"lattice does not exist");
	strcpy(lattice,sim->latticess->latticenames[latticeindex]);
	return lattice;
 failure:
	return NULL; }

/* smolAddLatticeMolecules */
extern CSTRING enum ErrorCode smolAddLatticeMolecules(simptr sim,const char *lattice, const char *species,int number,double *lowposition,double *highposition) {
	const char *funcname="smolAddLatticeMolecules";
	int er,i,lat;
	double *low,*high;
	latticeptr simlattice;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	if(number==0) return ECok;
	LCHECK(number>0,funcname,ECbounds,"nmolec cannot be negative");
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	LCHECK(number>=0,funcname,ECbounds,"number cannot be < 0");
	if(!lowposition)
		low = simlattice->min;
	else
		low=lowposition;
	if(!highposition)
		high = simlattice->max;
	else
		high=highposition;
	er=latticeaddmols(simlattice,number,i,low,high,sim->dim);
	LCHECK(!er,funcname,ECmemory,"out of memory adding molecules");
	return ECok;
 failure:
	return Liberrorcode; }

/* smolAddLatticePort */
extern CSTRING enum ErrorCode smolAddLatticePort(simptr sim, const char *lattice, const char *port) {
	const char *funcname="smolAddLatticePort";
	int prt,lat;
	latticeptr simlattice;
	portptr simport;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	prt=smolGetPortIndexNT(sim,port);
	LCHECK(prt>=0,funcname,ECsame,NULL);
	simport=sim->portss->portlist[prt];
	latticeaddport(simlattice,simport);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddLatticeSpecies */
extern CSTRING enum ErrorCode smolAddLatticeSpecies(simptr sim,const char *lattice, const char *species) {
	const char *funcname="smolAddLatticeSpecies";
	int er,i,lat;
	latticeptr simlattice;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	i=smolGetSpeciesIndexNT(sim,species);
	LCHECK(i>0,funcname,ECsame,NULL);
	er=latticeaddspecies(simlattice,i,NULL);
	LCHECK(!er,funcname,ECmemory,"out of memory in latticeaddspecies");
	return ECok;
 failure:
	return Liberrorcode; }

/* smolAddLatticeReaction */
extern CSTRING enum ErrorCode smolAddLatticeReaction(simptr sim,const char *lattice, const char *reaction, const int move) {
	const char *funcname="smolAddLatticeReaction";
	int er,lat,r,order;
	latticeptr simlattice;
	rxnptr rxn;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	order=-1;
	r=smolGetReactionIndexNT(sim,&order,reaction);
	LCHECK(r>=0,funcname,ECsame,NULL);
	rxn=sim->rxnss[order]->rxn[r];

	er=latticeaddrxn(simlattice,rxn,move);
	LCHECK(!er,funcname,ECmemory,"out of memory in latticeaddrxn");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolAddLatticeSurface */
extern CSTRING enum ErrorCode smolAddLatticeSurface(simptr sim,const char *lattice,const char *surface) {
	const char *funcname="smolAddLatticeSurface";
	int er,lat,s,slo,shi;
	latticeptr simlattice;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	LCHECK(sim->srfss && sim->srfss->nsrf>0,funcname,ECnonexist,"no surfaces defined");
	s=smolGetSurfaceIndexNT(sim,surface);
	if(s==(int)ECall) {smolClearError();slo=0;shi=sim->srfss->nsrf;}
	else {
		LCHECK(s>=0,funcname,ECsame,NULL);
		slo=s;
		shi=s+1; }
	for(s=slo;s<shi;s++) {
		er=latticeaddsurface(simlattice,sim->srfss->srflist[s]);
		LCHECK(er!=1,funcname,ECmemory,"out of memory in latticeaddsurface"); }
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetLatticeMakeParticle */
extern CSTRING enum ErrorCode smolSetLatticeMakeParticle(simptr sim,const char *lattice,enum PanelFace face,const char *species,int makeparticle) {
	const char *funcname="smolSetLatticeMakeParticle";
	int er,lat,i,ilow,ihigh;
	latticeptr simlattice;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	lat=smolGetLatticeIndexNT(sim,lattice);
	LCHECK(lat>=0,funcname,ECsame,NULL);
	simlattice=sim->latticess->latticelist[lat];
	LCHECK(face==PFfront || face==PFback || face==PFboth,funcname,ECbounds,"invalid face");
	i=smolGetSpeciesIndexNT(sim,species);
	if(i==(int)ECall) {
		smolClearError();
		ilow=1;
		ihigh=sim->mols->nspecies; }
	else {
		LCHECK(i>0,funcname,ECsame,NULL);
		ilow=i;
		ihigh=i+1; }
	for(i=ilow;i<ihigh;i++) {
		er=latticeaddconvert(simlattice,i,NULL,face,makeparticle?1:0);
		LCHECK(!er,funcname,ECerror,"species needs to be added to the lattice with smolAddLatticeSpecies first"); }
	return ECok;
 failure:
	return Liberrorcode; }


/******************************************************************************/
/********************************** BioNetGen *********************************/
/******************************************************************************/


/* smolgetbng. Returns the BNG structure named bng, or NULL if it doesn't
exist. */
static bngptr smolgetbng(simptr sim,const char *bng) {
	int b;

	if(!sim->bngss || !bng) return NULL;
	b=stringfind(sim->bngss->bngnames,sim->bngss->nbng,bng);
	return b>=0?sim->bngss->bnglist[b]:NULL; }


/* smolAddBNG */
extern CSTRING enum ErrorCode smolAddBNG(simptr sim,const char *bng) {
	const char *funcname="smolAddBNG";
	bngptr simbng;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	LCHECK(sim->mols,funcname,ECnonexist,"need to define species before BioNetGen structures");
	LCHECK(bng && bng[0]!='\0',funcname,ECmissing,"missing BNG name");
	simbng=bngaddbng(sim,bng);
	LCHECK(simbng,funcname,ECmemory,"failed to add BNG structure");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetBNGMultiply */
extern CSTRING enum ErrorCode smolSetBNGMultiply(simptr sim,const char *bng,const char *parameter,double amount) {
	const char *funcname="smolSetBNGMultiply";
	char param[STRCHAR];
	int er;
	bngptr simbng;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simbng=smolgetbng(sim,bng);
	LCHECK(simbng,funcname,ECnonexist,"BNG structure not found");
	LCHECK(parameter && strlen(parameter)<STRCHAR,funcname,ECmissing,"missing parameter");
	strcpy(param,parameter);
	er=bngsetparam(simbng,param,amount);
	LCHECK(er!=1,funcname,ECsyntax,"parameter needs to be unimolecular_rate or bimolecular_rate");
	LCHECK(er!=2,funcname,ECbounds,"multiply amount needs to be at least 0");
	return ECok;
 failure:
	return Liberrorcode; }


/* smolSetBNGMonomer */
extern CSTRING enum ErrorCode smolSetBNGMonomer(simptr sim,const char *bng,const char *monomer,enum MolecState state,double difc,double displaysize,double *color) {
	const char *funcname="smolSetBNGMonomer";
	char name[STRCHAR];
	int er,c;
	bngptr simbng;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simbng=smolgetbng(sim,bng);
	LCHECK(simbng,funcname,ECnonexist,"BNG structure not found");
	LCHECK(monomer && monomer[0]!='\0' && strlen(monomer)<STRCHAR,funcname,ECmissing,"missing monomer name");
	strcpy(name,monomer);
	LCHECK((state>=0 && state<MSMAX1) || state==MSnone,funcname,ECbounds,"invalid state");
	if(color) {
		for(c=0;c<3;c++)
			LCHECK(color[c]>=0 && color[c]<=1,funcname,ECbounds,"color value out of bounds"); }

	if(state!=MSnone) er=bngsetmonomerstate(simbng,name,state);
	else if(strcmp(name,"all")) er=bngaddmonomer(simbng,name,MSsoln);
	else er=0;
	LCHECK(er!=-1,funcname,ECmemory,"out of memory adding monomer");
	LCHECK(er!=-2,funcname,ECsyntax,"monomer name is not permitted");
	if(difc>=0) bngsetmonomerdifc(simbng,name,difc);
	if(displaysize>=0) bngsetmonomerdisplaysize(simbng,name,displaysize);
	if(color) bngsetmonomercolor(simbng,name,color);
	return ECok;
 failure:
	return Liberrorcode; }


/* smolLoadBNGNetFile */
extern CSTRING enum ErrorCode smolLoadBNGNetFile(simptr sim,const char *bng,const char *filename) {
	const char *funcname="smolLoadBNGNetFile";
	char word[STRCHARLONG],errstring[STRCHARLONG],*line2;
	int done,pfpcode,er;
	bngptr simbng;
	ParseFilePtr pfp;

	pfp=NULL;
	LCHECK(sim,funcname,ECmissing,"missing sim");
	simbng=smolgetbng(sim,bng);
	LCHECK(simbng,funcname,ECnonexist,"BNG structure not found");
	LCHECK(filename && filename[0]!='\0',funcname,ECmissing,"missing file name");
	pfp=Parse_Start(NULL,filename,errstring);
	LCHECK(pfp,funcname,ECnonexist,"unable to open BioNetGen network file");

	done=0;
	while(!done) {
		pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
		LCHECK(pfpcode!=3,funcname,ECsyntax,errstring);
		if(pfpcode==2) done=1;
		else if(pfpcode==1 && !bngreadstring(sim,pfp,simbng,word,line2)) {
			pfp=NULL;																	// bngreadstring closed the file
			LCHECK(0,funcname,ECerror,"error reading BioNetGen network file"); }}

	er=bngupdate(sim);
	LCHECK(!er,funcname,ECerror,"error updating BioNetGen structures");
	return ECok;
 failure:
	if(pfp) Parse_ReadFailure(pfp,errstring);				// close any files still open
	return Liberrorcode; }


/* smolExpandBNGRules */
extern CSTRING enum ErrorCode smolExpandBNGRules(simptr sim,const char *bng,const char *filename,const char *BNG2path) {
	const char *funcname="smolExpandBNGRules";
	char fname[STRCHAR],outname[STRCHAR],path[STRCHARLONG];
	int er;
	bngptr simbng;

	LCHECK(sim,funcname,ECmissing,"missing sim");
	simbng=smolgetbng(sim,bng);
	LCHECK(simbng,funcname,ECnonexist,"BNG structure not found");
	LCHECK(filename && filename[0]!='\0',funcname,ECmissing,"missing file name");
	LCHECK(strlen(filename)<STRCHAR-5,funcname,ECbounds,"file name is too long");
	if(BNG2path && BNG2path[0]!='\0') {
		LCHECK(strlen(BNG2path)<STRCHARLONG,funcname,ECbounds,"BNG2.pl path is too long");
		strcpy(path,BNG2path);
		bngsetBNG2path(simbng,path); }
	strcpy(fname,filename);
	er=bngrunBNGL2(simbng,fname,outname);
	LCHECK(er!=1,funcname,ECnonexist,"BNG2.pl software not found");
	LCHECK(er!=2,funcname,ECnonexist,"BioNetGen file not found");
	LCHECK(er!=3,funcname,ECerror,"BNG2.pl failed to write a network file; try again with the 'v' flag for more information");
	LCHECK(er!=4,funcname,ECerror,"unable to run perl; check that it is installed");
	LCHECK(!er,funcname,ECerror,"error running BNG2.pl");
	return smolLoadBNGNetFile(sim,bng,outname);
 failure:
	return Liberrorcode; }
