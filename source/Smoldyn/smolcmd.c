/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "Geometry.h"
#include "List.h"
#include "math2.h"
#include "opengl2.h"
#include "random2.h"
#include "Rn.h"
#include "RnSort.h"
#include "string2.h"
#include "SimCommand.h"
#include "Zn.h"

#ifdef OPTION_NSV
  #include "nsvc.h"
#endif

#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "smoldynconfigure.h"


/* Global variables */
int Nvar;
char **Varnames;
double *Varvalues;


/**********************************************************/
/******************** command declarations ****************/
/**********************************************************/

#ifdef VCELL
#include <vcellcmd.h>
#endif

// simulation control
enum CMDcode cmdstop(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdpause(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdbeep(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkeypress(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetflag(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetrandseed(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetgraphics(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetgraphic_iter(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdupdategraphics(simptr sim,cmdptr cmd,char *line2);

// file manipulation
enum CMDcode cmdoverwrite(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdincrementfile(simptr sim,cmdptr cmd,char *line2);

// conditional
enum CMDcode cmdifflag(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifprob(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifno(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifless(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifmore(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifincmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdifchange(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdif(simptr sim,cmdptr cmd,char *line2);

// system observation
enum CMDcode cmdecho(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdevaluate(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdwarnescapee(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdwarnescapeecmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountheader(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcount(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountinbox(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountincmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountincmpts(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountincmpt2(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountonsurf(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspace(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspace2d(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspaceradial(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspacepolarangle(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdradialdistribution(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdradialdistribution2(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspecies(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolcountspecieslist(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmollistsize(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlistmols(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlistmols2(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlistmols3(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlistmols4(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlistmolscmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolpos(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdtrackmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmolmoments(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsavesim(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmeansqrdisp(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmeansqrdisp2(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmeansqrdisp3(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdresidencetime(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmddiagnostics(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdexecutiontime(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdwriteVTK(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdprintdata(simptr sim,cmdptr cmd,char *line2);

enum CMDcode cmdprintLattice(simptr sim,cmdptr cmd,char *line2);

// system manipulation
enum CMDcode cmdset(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdpointsource(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdvolumesource(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdgaussiansource(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmovesurfacemol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkillmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkillmolprob(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkillmolinsphere(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkillmolincmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdkillmoloutsidesystem(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcount(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcountrange(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcountonsurf(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcountrangeonsurf(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcountincmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdfixmolcountrangeincmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdequilmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdreplacemol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdreplacexyzmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdreplacevolmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdreplacecmptmol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdmodulatemol(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdreact1(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetrateint(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdshufflemollist(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdshufflereactions(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsettimestep(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdporttransport(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdexcludebox(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdexcludesphere(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdincludeecoli(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdsetreactionratemolcount(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdexpandsystem(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdtranslatecmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmddiffusecmpt(simptr sim,cmdptr cmd,char *line2);
enum CMDcode cmdlongrangeforce(simptr sim,cmdptr cmd,char *line2);

// Smoldyn function declarations
double fnmolcount(void *voidsim,char *erstr,char *line2);
double fnmolcountonsurf(void *voidsim,char *erstr,char *line2);

// internal functions
void cmdv1free(cmdptr cmd);
void cmdv1v2free(cmdptr cmd);
void cmdListULVD4free(cmdptr cmd);
enum CMDcode conditionalcmdtype(simptr sim,cmdptr cmd,int nparam);
int insideecoli(double *pos,double *ofst,double rad,double length);
void putinecoli(double *pos,double *ofst,double rad,double length);
int molinpanels(simptr sim,moleculeptr mptr,int s,enum PanelShape ps);
void cmdmeansqrdispfree(cmdptr cmd);


/**********************************************************/
/********************* command processor ******************/
/**********************************************************/


/* docommand */
enum CMDcode docommand(void *simvd,cmdptr cmd,char *line) {
	simptr sim;
	char word[STRCHAR],*line2;
	int itct;

	if(!simvd) return CMDok;
	sim=(simptr) simvd;
	if(!line) return CMDok;
	itct=sscanf(line,"%s",word);
	if(itct<=0) return CMDok;
	line2=strnword(line,2);

	Nvar=sim->nvar;
	Varnames=sim->varnames;
	Varvalues=sim->varvalues;

	// simulation control
	if(!strcmp(word,"stop")) return cmdstop(sim,cmd,line2);
	else if(!strcmp(word,"pause")) return cmdpause(sim,cmd,line2);
	else if(!strcmp(word,"beep")) return cmdbeep(sim,cmd,line2);
	else if(!strcmp(word,"keypress")) return cmdkeypress(sim,cmd,line2);
	else if(!strcmp(word,"setflag")) return cmdsetflag(sim,cmd,line2);
	else if(!strcmp(word,"setrandseed")) return cmdsetrandseed(sim,cmd,line2);
	else if(!strcmp(word,"setgraphics")) return cmdsetgraphics(sim,cmd,line2);
	else if(!strcmp(word,"setgraphic_iter")) return cmdsetgraphic_iter(sim,cmd,line2);
	else if(!strcmp(word,"updategraphics")) return cmdupdategraphics(sim,cmd,line2);

	// file manipulation
	else if(!strcmp(word,"overwrite")) return cmdoverwrite(sim,cmd,line2);
	else if(!strcmp(word,"incrementfile")) return cmdincrementfile(sim,cmd,line2);

	// conditional
	else if(!strcmp(word,"ifflag")) return cmdifflag(sim,cmd,line2);
	else if(!strcmp(word,"ifprob")) return cmdifprob(sim,cmd,line2);
	else if(!strcmp(word,"ifno")) return cmdifno(sim,cmd,line2);
	else if(!strcmp(word,"ifless")) return cmdifless(sim,cmd,line2);
	else if(!strcmp(word,"ifmore")) return cmdifmore(sim,cmd,line2);
	else if(!strcmp(word,"ifincmpt")) return cmdifincmpt(sim,cmd,line2);
	else if(!strcmp(word,"ifchange")) return cmdifchange(sim,cmd,line2);
	else if(!strcmp(word,"if")) return cmdif(sim,cmd,line2);

	// system observation
	else if(!strcmp(word,"echo")) return cmdecho(sim,cmd,line2);
	else if(!strcmp(word,"evaluate")) return cmdevaluate(sim,cmd,line2);
	else if(!strcmp(word,"warnescapee")) return cmdwarnescapee(sim,cmd,line2);
	else if(!strcmp(word,"warnescapeecmpt")) return cmdwarnescapeecmpt(sim,cmd,line2);
	else if(!strcmp(word,"molcountheader")) return cmdmolcountheader(sim,cmd,line2);
	else if(!strcmp(word,"molcount")) return cmdmolcount(sim,cmd,line2);
	else if(!strcmp(word,"molcountinbox")) return cmdmolcountinbox(sim,cmd,line2);
	else if(!strcmp(word,"molcountincmpt")) return cmdmolcountincmpt(sim,cmd,line2);
	else if(!strcmp(word,"molcountincmpts")) return cmdmolcountincmpts(sim,cmd,line2);
	else if(!strcmp(word,"molcountincmpt2")) return cmdmolcountincmpt2(sim,cmd,line2);
	else if(!strcmp(word,"molcountonsurf")) return cmdmolcountonsurf(sim,cmd,line2);
	else if(!strcmp(word,"molcountspace")) return cmdmolcountspace(sim,cmd,line2);
	else if(!strcmp(word,"molcountspace2d")) return cmdmolcountspace2d(sim,cmd,line2);
	else if(!strcmp(word,"molcountspaceradial")) return cmdmolcountspaceradial(sim,cmd,line2);
	else if(!strcmp(word,"molcountspacepolarangle")) return cmdmolcountspacepolarangle(sim,cmd,line2);
	else if(!strcmp(word,"radialdistribution")) return cmdradialdistribution(sim,cmd,line2);
	else if(!strcmp(word,"radialdistribution2")) return cmdradialdistribution2(sim,cmd,line2);
	else if(!strcmp(word,"molcountspecies")) return cmdmolcountspecies(sim,cmd,line2);
	else if(!strcmp(word,"molcountspecieslist")) return cmdmolcountspecieslist(sim,cmd,line2);
	else if(!strcmp(word,"mollistsize")) return cmdmollistsize(sim,cmd,line2);
	else if(!strcmp(word,"listmols")) return cmdlistmols(sim,cmd,line2);
	else if(!strcmp(word,"listmols2")) return cmdlistmols2(sim,cmd,line2);
	else if(!strcmp(word,"listmols3")) return cmdlistmols3(sim,cmd,line2);
	else if(!strcmp(word,"listmols4")) return cmdlistmols4(sim,cmd,line2);
	else if(!strcmp(word,"listmolscmpt")) return cmdlistmolscmpt(sim,cmd,line2);
	else if(!strcmp(word,"molpos")) return cmdmolpos(sim,cmd,line2);
	else if(!strcmp(word,"trackmol")) return cmdtrackmol(sim,cmd,line2);
	else if(!strcmp(word,"molmoments")) return cmdmolmoments(sim,cmd,line2);
	else if(!strcmp(word,"savesim")) return cmdsavesim(sim,cmd,line2);
	else if(!strcmp(word,"meansqrdisp")) return cmdmeansqrdisp(sim,cmd,line2);
	else if(!strcmp(word,"meansqrdisp2")) return cmdmeansqrdisp2(sim,cmd,line2);
	else if(!strcmp(word,"meansqrdisp3")) return cmdmeansqrdisp3(sim,cmd,line2);
	else if(!strcmp(word,"residencetime")) return cmdresidencetime(sim,cmd,line2);
	else if(!strcmp(word,"diagnostics")) return cmddiagnostics(sim,cmd,line2);
	else if(!strcmp(word,"executiontime")) return cmdexecutiontime(sim,cmd,line2);
	else if(!strcmp(word,"writeVTK")) return cmdwriteVTK(sim,cmd,line2);
	else if(!strcmp(word,"printLattice")) return cmdprintLattice(sim,cmd,line2);
	else if(!strcmp(word,"printdata")) return cmdprintdata(sim,cmd,line2);

	// system manipulation
	else if(!strcmp(word,"set")) return cmdset(sim,cmd,line2);
	else if(!strcmp(word,"pointsource")) return cmdpointsource(sim,cmd,line2);
	else if(!strcmp(word,"volumesource")) return cmdvolumesource(sim,cmd,line2);
	else if(!strcmp(word,"gaussiansource")) return cmdgaussiansource(sim,cmd,line2);
	else if(!strcmp(word,"movesurfacemol")) return cmdmovesurfacemol(sim,cmd,line2);
	else if(!strcmp(word,"killmol")) return cmdkillmol(sim,cmd,line2);
	else if(!strcmp(word,"killmolprob")) return cmdkillmolprob(sim,cmd,line2);
	else if(!strcmp(word,"killmolinsphere")) return cmdkillmolinsphere(sim,cmd,line2);
	else if(!strcmp(word,"killmolincmpt")) return cmdkillmolincmpt(sim,cmd,line2);
	else if(!strcmp(word,"killmoloutsidesystem")) return cmdkillmoloutsidesystem(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcount")) return cmdfixmolcount(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcountrange")) return cmdfixmolcountrange(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcountonsurf")) return cmdfixmolcountonsurf(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcountrangeonsurf")) return cmdfixmolcountrangeonsurf(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcountincmpt")) return cmdfixmolcountincmpt(sim,cmd,line2);
	else if(!strcmp(word,"fixmolcountrangeincmpt")) return cmdfixmolcountrangeincmpt(sim,cmd,line2);
	else if(!strcmp(word,"equilmol")) return cmdequilmol(sim,cmd,line2);
	else if(!strcmp(word,"replacemol")) return cmdreplacemol(sim,cmd,line2);
	else if(!strcmp(word,"replacexyzmol")) return cmdreplacexyzmol(sim,cmd,line2);
	else if(!strcmp(word,"replacevolmol")) return cmdreplacevolmol(sim,cmd,line2);
	else if(!strcmp(word,"replacecmptmol")) return cmdreplacecmptmol(sim,cmd,line2);
	else if(!strcmp(word,"modulatemol")) return cmdmodulatemol(sim,cmd,line2);
	else if(!strcmp(word,"react1")) return cmdreact1(sim,cmd,line2);
	else if(!strcmp(word,"setrateint")) return cmdsetrateint(sim,cmd,line2);
	else if(!strcmp(word,"shufflemollist")) return cmdshufflemollist(sim,cmd,line2);
	else if(!strcmp(word,"shufflereactions")) return cmdshufflereactions(sim,cmd,line2);
	else if(!strcmp(word,"settimestep")) return cmdsettimestep(sim,cmd,line2);
	else if(!strcmp(word,"porttransport")) return cmdporttransport(sim,cmd,line2);
	else if(!strcmp(word,"excludebox")) return cmdexcludebox(sim,cmd,line2);
	else if(!strcmp(word,"excludesphere")) return cmdexcludesphere(sim,cmd,line2);
	else if(!strcmp(word,"includeecoli")) return cmdincludeecoli(sim,cmd,line2);
	else if(!strcmp(word,"setreactionratemolcount")) return cmdsetreactionratemolcount(sim,cmd,line2);
	else if(!strcmp(word,"expandsystem")) return cmdexpandsystem(sim,cmd,line2);
	else if(!strcmp(word,"translatecmpt")) return cmdtranslatecmpt(sim,cmd,line2);
	else if(!strcmp(word,"diffusecmpt")) return cmddiffusecmpt(sim,cmd,line2);
	else if(!strcmp(word,"longrangeforce")) return cmdlongrangeforce(sim,cmd,line2);

#ifdef VCELL
	// vcell commands
	else if(!strcmp(word,"vcellPrintProgress")) return cmdVCellPrintProgress(sim,cmd,line2);
	else if(!strcmp(word,"vcellWriteOutput")) return cmdVCellWriteOutput(sim,cmd,line2);
	else if(!strcmp(word,"vcellDataProcess")) return cmdVCellDataProcess(sim,cmd,line2);
#endif

	SCMDCHECK(0,"command not recognized");
	return CMDwarn; }


int loadsmolfunctions(simptr sim) {
	double er;
	char str1[STRCHAR],str2[STRCHAR];

	er=0;
	er+=strevalfunction(strcpy(str1,"molcount"),strcpy(str2,"dves"),(void*) sim,(void*) &fnmolcount,NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"molcountonsurf"),strcpy(str2,"dves"),(void*) sim,(void*) &fnmolcountonsurf,NULL,NULL,0);

	return (int) er; }


/**********************************************************/
/****************** simulation control ********************/
/**********************************************************/


/* cmdstop */
enum CMDcode cmdstop(simptr sim,cmdptr cmd,char *line2) {
	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	(void)sim;
	(void)cmd;
	return CMDstop; }


/* cmdpause */
enum CMDcode cmdpause(simptr sim,cmdptr cmd,char *line2) {
	char c;
	int tflag;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	if(!sim->graphss || sim->graphss->graphics==0) {
		fprintf(stderr,"Simulation paused at time %g.  Press enter to continue.",sim->time);
		int n = scanf("%c",&c);
        UNUSED(n);
		return CMDok; }
	tflag=strchr(sim->flags,'t')?1:0;
	SCMDCHECK(sim->graphss && sim->graphss->graphics!=0 && !tflag,"pause doesn't work without graphics");
	gl2State(1);
	return CMDpause; }


/* cmdbeep */
enum CMDcode cmdbeep(simptr sim,cmdptr cmd,char *line2) {
	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	(void)sim;
	(void)cmd;
	fprintf(stderr,"\7");
	return CMDok; }


/* cmdkeypress */
enum CMDcode cmdkeypress(simptr sim,cmdptr cmd,char *line2) {
	char c;
	int itct,tflag;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%c",&c);
	SCMDCHECK(itct==1,"cannot read character");
	tflag=strchr(sim->flags,'t')?1:0;
	SCMDCHECK(sim->graphss && sim->graphss->graphics!=0 && !tflag,"keypress doesn't work without graphics");
	gl2SetKeyPush((unsigned char) c);
	return CMDok; }


/* cmdsetflag */
enum CMDcode cmdsetflag(simptr sim,cmdptr cmd,char *line2) {
	double f1;
	int itct;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	SCMDCHECK(line2,"missing argument");
	itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&f1);
	SCMDCHECK(itct==1,"cannot read flag value");
	scmdsetflag(sim->cmds,f1);
	return CMDok; }


/* cmdsetrandseed */
enum CMDcode cmdsetrandseed(simptr sim,cmdptr cmd,char *line2) {
	int itct;
	long int seed;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	(void)sim;
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%li",&seed);
	SCMDCHECK(itct==1,"cannot read seed");
	if(seed<0) randomize((long int) time(NULL));
	else randomize((long int) seed);
	return CMDok; }


/* cmdsetgraphics */
enum CMDcode cmdsetgraphics(simptr sim,cmdptr cmd,char *line2) {
	int itct;
	char str[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	if(!sim->graphss || sim->graphss->graphics==0) return CMDok;
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",str);
	SCMDCHECK(itct==1,"cannot read graphics type");
	if(!strcmp(str,"opengl")) sim->graphss->graphics=1;
	else if(!strcmp(str,"opengl_good")) sim->graphss->graphics=2;
	else SCMDCHECK(0,"unrecognized graphics type");
	return CMDok; }

/* cmdsetgraphic_iter */
enum CMDcode cmdsetgraphic_iter(simptr sim,cmdptr cmd,char *line2) {
	int itct,iter;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	if(!sim->graphss || sim->graphss->graphics==0) return CMDok;
	SCMDCHECK(line2,"missing argument");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&iter);
	SCMDCHECK(itct==1,"cannot read graphics iterations");
	SCMDCHECK(iter>0,"graphics iterations must be >0");
	sim->graphss->graphicit=iter;
	return CMDok; }


/* cmdupdategraphics */
enum CMDcode cmdupdategraphics(simptr sim,cmdptr cmd,char *line2) {
	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	(void)cmd;
	if(!sim->graphss || sim->graphss->graphics==0) return CMDok;
	smolPostRedisplay();
	return CMDok; }


/**********************************************************/
/****************** file manipulation ********************/
/**********************************************************/


/* cmdoverwrite */
enum CMDcode cmdoverwrite(simptr sim,cmdptr cmd,char *line2) {
	int er;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	SCMDCHECK(line2,"missing argument");
	er=scmdoverwrite(sim->cmds,line2);
	SCMDCHECK(er!=1,"file not declared");
	SCMDCHECK(er!=2,"failed to open file for writing");
	return CMDok; }


/* cmdincrementfile */
enum CMDcode cmdincrementfile(simptr sim,cmdptr cmd,char *line2) {
	int er;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDcontrol;
	SCMDCHECK(line2,"missing argument");
	er=scmdincfile(sim->cmds,line2);
	SCMDCHECK(er!=1,"file name not declared");
	SCMDCHECK(er!=2,"failed to open new file for writing");
	return CMDok; }



/**********************************************************/
/********************** conditional ***********************/
/**********************************************************/


/* cmdifflag */
enum CMDcode cmdifflag(simptr sim,cmdptr cmd,char *line2) {
	int itct;
	char ch;
	double f1,flag;

	if(line2 && !strcmp(line2,"cmdtype")) return conditionalcmdtype(sim,cmd,2);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%c %mlg",Varnames,Varvalues,Nvar,&ch,&f1);
	SCMDCHECK(itct==2,"cannot read comparison symbol or flag value");
	SCMDCHECK(ch=='<' || ch=='=' || ch=='>',"comparison symbol has to be <, =, or >");
	flag=scmdreadflag(sim->cmds);
	if((ch=='<' && flag<f1) || (ch=='=' && flag==f1) || (ch=='>' && flag>f1))
		return docommand(sim,cmd,strnword(line2,3));
	return CMDok; }


/* cmdifprob */
enum CMDcode cmdifprob(simptr sim,cmdptr cmd,char *line2) {
	int itct;
	double f1;

	if(line2 && !strcmp(line2,"cmdtype")) return conditionalcmdtype(sim,cmd,1);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&f1);
	SCMDCHECK(itct==1,"cannot read probability value");
	SCMDCHECK(f1>=0 && f1<=1,"probability value needs to be between 0 and 1");
	if(randCOD()<f1)
		return docommand(sim,cmd,strnword(line2,2));
	return CMDok; }


/* cmdifno */
enum CMDcode cmdifno(simptr sim,cmdptr cmd,char *line2) {
	int i,count,*index;
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) {
		return conditionalcmdtype(sim,cmd,1); }

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	count=(i==-4)?0:molcount(sim,i,index,ms,1);
	if(count) return CMDok;
	return docommand(sim,cmd,strnword(line2,2)); }


/* cmdifless */
enum CMDcode cmdifless(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,*index,count,min;
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) {
		return conditionalcmdtype(sim,cmd,2); }

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(line2=strnword(line2,2),"missing value argument");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&min);
	SCMDCHECK(itct==1,"cannot read value argument");
	count=(i==-4)?0:molcount(sim,i,index,ms,min);
	if(count<min) return docommand(sim,cmd,strnword(line2,2));
	return CMDok; }


/* cmdifmore */
enum CMDcode cmdifmore(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,*index,count,min;
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) {
		return conditionalcmdtype(sim,cmd,2); }

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(line2=strnword(line2,2),"missing value argument");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&min);
	SCMDCHECK(itct==1,"cannot read value argument");
	count=(i==-4)?0:molcount(sim,i,index,ms,min+1);
	if(count>min) return docommand(sim,cmd,strnword(line2,2));
	return CMDok; }


/* cmdifincmpt */
enum CMDcode cmdifincmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,min,c,*index;
	enum MolecState ms;
	char cname[STRCHAR];
	compartssptr cmptss;
	char ch;
	moleculeptr mptr;
	static compartptr cmpt=NULL;
	static int inscan=0,count=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return conditionalcmdtype(sim,cmd,4);

	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(line2=strnword(line2,2),"missing value argument");
	itct=strmathsscanf(line2,"%c %mi %s",Varnames,Varvalues,Nvar,&ch,&min,cname);
	SCMDCHECK(itct==3,"cannot read symbol, value, and/or compartment arguments");
	SCMDCHECK(ch=='<' || ch=='=' || ch=='>',"comparison symbol has to be <, =, or >");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];
	line2=strnword(line2,4);

	if(i==-4) count=0;
	else {
		count=0;
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdifincmpt);
		inscan=0; }
	if((ch=='<' && count<min) || (ch=='=' && count==min) || (ch=='>' && count>min))
		return docommand(sim,cmd,line2);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0)) count++;
	return CMDok; }


/* cmdifchange */
enum CMDcode cmdifchange(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,*index,count,num,diff;
	enum MolecState ms;
  char change;

	if(line2 && !strcmp(line2,"cmdtype")) {
		return conditionalcmdtype(sim,cmd,2); }

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(line2=strnword(line2,2),"missing value argument");
	itct=strmathsscanf(line2,"%c %mi",Varnames,Varvalues,Nvar,&change,&num);
	SCMDCHECK(itct==2,"cannot read change or number arguments");
  SCMDCHECK(line2=strnword(line2,3),"missing conditional command");

  if(cmd->i1==0) {
    cmd->i1=1;
    cmd->i2=(i==-4)?0:molcount(sim,i,index,ms,-1); }
  else {
    count=(i==-4)?0:molcount(sim,i,index,ms,-1);
    diff=count-cmd->i2;
    cmd->i2=count;
    if((change=='>' && diff>num) || (change=='<' && diff<num) || (change=='=' && diff==num) || (change=='!' && diff!=num))
      return docommand(sim,cmd,line2); }
	return CMDok; }


/* cmdif */
enum CMDcode cmdif(simptr sim,cmdptr cmd,char *line2) {
	int itct;
  char symbol;
	double value1,value2;

	if(line2 && !strcmp(line2,"cmdtype")) {
		return conditionalcmdtype(sim,cmd,2); }

	itct=strmathsscanf(line2,"%mlg %c %mlg",Varnames,Varvalues,Nvar,&value1,&symbol,&value2);
	SCMDCHECK(itct==3,"cannot read command arguments");
  SCMDCHECK(line2=strnword(line2,4),"missing conditional command");

	if((symbol=='>' && value1>value2) || (symbol=='<' && value1<value2) || (symbol=='=' && value1==value2))
		return docommand(sim,cmd,line2);

	return CMDok; }


/**********************************************************/
/***************** observation commands *******************/
/**********************************************************/


/* cmdwarnescapee */
enum CMDcode cmdwarnescapee(simptr sim,cmdptr cmd,char *line2) {
	int i,escape,*index,er;
	enum MolecState ms;
	moleculeptr mptr;
	double *pos,*posx,*via;
	char string[STRCHAR];
	static int inscan=0;
	static FILE *fptr=NULL;
	static int dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdwarnescapee);
		inscan=0;
		scmdflush(fptr); }
	return CMDok;

 scanportion:
	mptr=(moleculeptr)line2;
	pos=mptr->pos;
	escape=!posinsystem(sim,pos);
	if(escape) {
		posx=mptr->posx;
		escape=!posinsystem(sim,posx);
		if(!escape) {
			via=mptr->via;
			if(sim->dim==1) {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s %g to %g via %g\n",sim->time,molserno2string(mptr->serno,string),posx[0],pos[0],via[0]);
				scmdappenddata(cmd->cmds,dataid,1,5,sim->time,(double)(mptr->serno),posx[0],pos[0],via[0]); }
			else if(sim->dim==2) {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s (%g,%g) to (%g,%g) via (%g,%g)\n",sim->time,molserno2string(mptr->serno,string),posx[0],posx[1],pos[0],pos[1],via[0],via[1]);
				scmdappenddata(cmd->cmds,dataid,1,8,sim->time,(double)(mptr->serno),posx[0],posx[1],pos[0],pos[1],via[0],via[1]); }
			else {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s (%g,%g,%g) to (%g,%g,%g) via (%g,%g,%g)\n",sim->time,molserno2string(mptr->serno,string),posx[0],posx[1],posx[2],pos[0],pos[1],pos[2],via[0],via[1],via[2]);
				scmdappenddata(cmd->cmds,dataid,1,11,sim->time,(double)(mptr->serno),posx[0],posx[1],posx[2],pos[0],pos[1],pos[2],via[0],via[1],via[2]); }}}
	return CMDok; }


/* cmdwarnescapeecmpt */
enum CMDcode cmdwarnescapeecmpt(simptr sim,cmdptr cmd,char *line2) {
	int i,escape,*index,er,itct,c;
	enum MolecState ms;
	moleculeptr mptr;
	compartssptr cmptss;
	double *pos,*posx,*via;
	char string[STRCHAR],nm[STRCHAR];
	static int inscan=0;
	static compartptr cmpt=NULL;
	static FILE *fptr=NULL;
	static int dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"cannot read argument");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdwarnescapeecmpt);
		inscan=0;
		scmdflush(fptr); }
	return CMDok;

 scanportion:
	mptr=(moleculeptr)line2;
	pos=mptr->pos;
	escape=!posincompart(sim,pos,cmpt,0);
	if(escape) {
		posx=mptr->posx;
		escape=!posincompart(sim,posx,cmpt,0);
		if(!escape) {
			via=mptr->via;
			if(sim->dim==1) {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s %g to %g via %g\n",sim->time,molserno2string(mptr->serno,string),posx[0],pos[0],via[0]);
				scmdappenddata(cmd->cmds,dataid,1,5,sim->time,(double)(mptr->serno),posx[0],pos[0],via[0]); }
			else if(sim->dim==2) {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s (%g,%g) to (%g,%g) via (%g,%g)\n",sim->time,molserno2string(mptr->serno,string),posx[0],posx[1],pos[0],pos[1],via[0],via[1]);
				scmdappenddata(cmd->cmds,dataid,1,8,sim->time,(double)(mptr->serno),posx[0],posx[1],pos[0],pos[1],via[0],via[1]); }
			else {
				scmdfprintf(cmd->cmds,fptr,"New escapee: %g #%s (%g,%g,%g) to (%g,%g,%g) via (%g,%g,%g)\n",sim->time,molserno2string(mptr->serno,string),posx[0],posx[1],posx[2],pos[0],pos[1],pos[2],via[0],via[1],via[2]);
				scmdappenddata(cmd->cmds,dataid,1,11,sim->time,(double)(mptr->serno),posx[0],posx[1],posx[2],pos[0],pos[1],pos[2],via[0],via[1],via[2]); }}}
	return CMDok; }


/* cmdecho */
enum CMDcode cmdecho(simptr sim,cmdptr cmd,char *line2) {
	int er;
	FILE *fptr;
	char *termqt,str[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,1,&fptr,NULL);
	SCMDCHECK(er!=-1,"file name not recognized");
	line2=strnword(line2,2);
	SCMDCHECK(line2=strchr(line2,'"'),"no starting quote on string");
	strncpy(str,line2+1,STRCHAR-1);
	str[STRCHAR-1]='\0';
	SCMDCHECK(termqt=strchr(str,'"'),"no terminal quote on string");
	*termqt='\0';
	strbslash2escseq(str);
	scmdfprintf(cmd->cmds,fptr,"%s",str);
	scmdflush(fptr);
	return CMDok; }


/* cmdevaluate */
enum CMDcode cmdevaluate(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	double answer;
	int it,er,dataid;
	char erstr[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing item to evaluate");
	it=strmathsscanf(line2,"%mlg",sim->varnames,sim->varvalues,sim->nvar,&answer);
	if(it!=1) {
		er=strmatherror(erstr,1);
		SCMDCHECK(!er,"%s",erstr); }
	scmdfprintf(cmd->cmds,fptr,"%g\n",answer);
	scmdappenddata(cmd->cmds,dataid,1,1,answer);
	scmdflush(fptr);
	return CMDok; }


/* cmdmolcountheader */
enum CMDcode cmdmolcountheader(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,er;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,1,&fptr,NULL);
	SCMDCHECK(er!=-1,"file name not recognized");
	SCMDCHECK(sim->mols,"molecules are undefined");
	scmdfprintf(cmd->cmds,fptr,"time");
	for(i=1;i<sim->mols->nspecies;i++)
		scmdfprintf(cmd->cmds,fptr,"%,%s",sim->mols->spname[i]);
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok; }


/* cmdmolcount */
enum CMDcode cmdmolcount(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,nspecies,*ctlat,ilat,er,dataid;
	latticeptr lat;
	moleculeptr mptr;
	static int inscan=0,*ct=NULL;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	SCMDCHECK(sim->mols,"molecules are undefined");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nspecies,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies;i++) ct[i]=0;									// clear counters

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdmolcount);
	inscan=0;

	if(sim->latticess) {
    if(cmd->i2!=nspecies) {
      free(cmd->v2);
      cmd->i2=nspecies;
      cmd->v2=calloc(nspecies,sizeof(int));
      if(!cmd->v2) {cmd->i2=-1;return CMDwarn;}}

    ctlat=(int*)cmd->v2;
		for(ilat=0;ilat<sim->latticess->nlattice;++ilat) {
			lat=sim->latticess->latticelist[ilat];
			for(i=1;i<nspecies;i++) ctlat[i]=0;
			if(lat->type==LATTICEnsv) {
				NSV_CALL(nsv_molcount(lat->nsv,ctlat)); }
			else if(lat->type==LATTICEpde) {
				//not implemented
			}
			for(i=1;i<nspecies;i++) {
				ct[i]+=ctlat[i]; }}}

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies;i++) {
		scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	ct[mptr->ident]++;
	return CMDok; }


/* fnmolcount */
double fnmolcount(void *voidsim,char *erstr,char *line2) {
	simptr sim;
	enum MolecState ms;
	int i,*index;
	static char oldline2[STRCHAR]="\0";
	static int inscan=0,ct;
	static long int oldtouch=0;

	sim=(simptr) voidsim;
	if(inscan) goto scanportion;

	if(!sim->mols) return 0;
	if(sim->mols->touch==oldtouch && !strcmp(line2,oldline2)) return ct;
	strcpy(oldline2,line2);
	oldtouch=sim->mols->touch;

	SFNCHECK(line2,"missing arguments");
	i=molstring2index1(sim,line2,&ms,&index);
	SFNCHECK(i!=-1,"species is missing or cannot be read");
	SFNCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SFNCHECK(i!=-3,"cannot read molecule state value");
	SFNCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SFNCHECK(i!=-7,"error allocating memory");

	ct=0;
	inscan=1;
	molscanfn(sim,i,index,ms,erstr,fnmolcount);
	inscan=0;
	return ct;

 scanportion:
	ct++;
	return 0; }


/* fnmolcountonsurf */
double fnmolcountonsurf(void *voidsim,char *erstr,char *line2) {
	simptr sim;
	enum MolecState ms;
	int i,*index,s,comma,itct;
	static int inscan=0,ct;
	surfacessptr srfss;
	char nm[STRCHAR];
	static surfaceptr srf;
	moleculeptr mptr;
	static long int oldtouch=0;
	static char oldline2[STRCHAR]="\0";

	sim=(simptr) voidsim;
	if(inscan) goto scanportion;

	if(!sim->mols) return 0;
	if(sim->mols->touch==oldtouch && !strcmp(line2,oldline2)) return ct;
	strcpy(oldline2,line2);
	oldtouch=sim->mols->touch;

	srfss=sim->srfss;
	SFNCHECK(srfss,"no surfaces defined");
	SFNCHECK(line2,"missing arguments");
	SFNCHECK((comma=strChrBrackets(line2,-1,',',"([{,\"'"))>0,"missing parameter");
	line2[comma]='\0';
	i=molstring2index1(sim,line2,&ms,&index);
	SFNCHECK(i!=-1,"species is missing or cannot be read");
	SFNCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SFNCHECK(i!=-3,"cannot read molecule state value");
	SFNCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SFNCHECK(i!=-7,"error allocating memory");
	line2+=comma+1;
	itct=sscanf(line2,"%s",nm);
	SFNCHECK(itct==1,"cannot read surface name");
	s=stringfind(srfss->snames,srfss->nsrf,nm);
	SFNCHECK(s>=0,"surface name '%s' not recognized",nm);
	srf=srfss->srflist[s];

	ct=0;
	inscan=1;
	molscanfn(sim,i,index,ms,erstr,fnmolcountonsurf);
	inscan=0;
	return ct;

 scanportion:							//?? This is very inefficient; should use surface molecule list
	mptr=(moleculeptr) line2;
	if(mptr->mstate!=MSsoln && mptr->pnl->srf==srf) ct++;
	return 0; }


/* cmdmolcountinbox */
enum CMDcode cmdmolcountinbox(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int d,dim,itct,i,nspecies,er,dataid;
	moleculeptr mptr;
	static double low[3]={0,0,0},high[3]={0,0,0};
	static int inscan=0,*ct;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	SCMDCHECK(sim->mols,"molecules are undefined");
	dim=sim->dim;
	for(d=0;d<dim;d++) {
		SCMDCHECK(line2,"missing argument");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&low[d],&high[d]);
		SCMDCHECK(itct==2,"read failure");
		line2=strnword(line2,3); }
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1free;
		cmd->v1=calloc(nspecies,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies;i++) ct[i]=0;

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdmolcountinbox);
	inscan=0;

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies;i++) {
		scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(d=0;d<sim->dim;d++)
		if(mptr->pos[d]<low[d] || mptr->pos[d]>high[d]) return CMDok;
	ct[mptr->ident]++;
	return CMDok; }


/* cmdmolcountincmpt */
enum CMDcode cmdmolcountincmpt(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	char nm[STRCHAR];
	compartssptr cmptss;
	int itct,c,i,nspecies,er,dataid;
	moleculeptr mptr;
	static compartptr cmpt=NULL;
	static int inscan=0,*ct;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"cannot read argument");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1free;
		cmd->v1=calloc(nspecies,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies;i++) ct[i]=0;

	inscan=1;
	molscancmd(sim,-1,NULL,MSsoln,cmd,cmdmolcountincmpt);
	inscan=0;

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies;i++) {
		scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0)) ct[mptr->ident]++;
	return CMDok; }


/* cmdmolcountincmpts */
enum CMDcode cmdmolcountincmpts(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	char nm[STRCHAR];
	compartssptr cmptss;
	int itct,c,i,ic,er,dataid;
	moleculeptr mptr;
	static int inscan=0,*ct,ncmpt,nspecies;
	static compartptr cmptlist[16];

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	ncmpt=wordcount(line2)-1;
	SCMDCHECK(ncmpt>=1,"no compartment or no output file listed");
	for(ic=0;ic<ncmpt;ic++) {
		itct=sscanf(line2,"%s",nm);
		SCMDCHECK(itct==1,"cannot read compartment name");
		c=stringfind(cmptss->cnames,cmptss->ncmpt,nm);
		SCMDCHECK(c>=0,"compartment name not recognized");
		cmptlist[ic]=cmptss->cmptlist[c];
		line2=strnword(line2,2);
		SCMDCHECK(line2,"missing argument"); }
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1free;
		cmd->v1=calloc(nspecies*ncmpt,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies*ncmpt;i++) ct[i]=0;

	inscan=1;
	molscancmd(sim,-1,NULL,MSsoln,cmd,cmdmolcountincmpts);
	inscan=0;

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies*ncmpt;i++)
		if(i%nspecies!=0) {
			scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(ic=0;ic<ncmpt;ic++)
		if(posincompart(sim,mptr->pos,cmptlist[ic],0)) ct[ic*nspecies+mptr->ident]++;
	return CMDok; }


/* cmdmolcountincmpt2 */
enum CMDcode cmdmolcountincmpt2(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	char nm[STRCHAR],state[STRCHAR];
	compartssptr cmptss;
	int itct,c,i,nspecies,er,dataid;
	moleculeptr mptr;
	enum MolecState ms;
	static compartptr cmpt;
	static int inscan=0,*ct;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s %s",nm,state);
	SCMDCHECK(itct==2,"cannot read arguments");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment name not recognized");
	ms=molstring2ms(state);
	SCMDCHECK(ms!=MSnone,"molecule state not recognized");
	SCMDCHECK(ms!=MSbsoln,"bsoln molecule state not permitted");
	cmpt=cmptss->cmptlist[c];
	line2=strnword(line2,3);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1free;
		cmd->v1=calloc(nspecies,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies;i++) ct[i]=0;

	inscan=1;
	molscancmd(sim,-1,NULL,ms,cmd,cmdmolcountincmpt2);
	inscan=0;

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies;i++) {
		scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0)) ct[mptr->ident]++;
	return CMDok; }


/* cmdmolcountonsurf */
enum CMDcode cmdmolcountonsurf(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	char nm[STRCHAR];
	surfacessptr srfss;
	int itct,s,i,nspecies,er,dataid;
	moleculeptr mptr;
	static int inscan=0,*ct;
	static surfaceptr srf;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again
	srfss=sim->srfss;
	SCMDCHECK(srfss,"no surfaces defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"cannot read argument");
	s=stringfind(srfss->snames,srfss->nsrf,nm);
	SCMDCHECK(s>=0,"surface name '%s' not recognized",nm);
	srf=srfss->srflist[s];
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	nspecies=sim->mols->nspecies;
	if(cmd->i1!=nspecies) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nspecies;
		cmd->freefn=&cmdv1free;
		cmd->v1=calloc(nspecies,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	for(i=0;i<nspecies;i++) ct[i]=0;

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdmolcountonsurf);
	inscan=0;

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	for(i=1;i<nspecies;i++) {
		scmdfprintf(cmd->cmds,fptr,"%,%i",ct[i]);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[i]); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(mptr->mstate!=MSsoln && mptr->pnl->srf==srf) ct[mptr->ident]++;
	return CMDok; }


/* cmdmolcountspace */
enum CMDcode cmdmolcountspace(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int dim,i,itct,ax2,d,bin,average,*ctlat,ilat,*index,j,er,dataid;
	enum MolecState ms;
	char axisstr[STRCHAR];
	moleculeptr mptr;
	latticeptr lat;
	static double low[DIMMAX],high[DIMMAX],scale;
	static int inscan=0,nbin,*ct,axis;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	dim=sim->dim;
	SCMDCHECK(line2,"missing arguments");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	itct=sscanf(line2,"%s",axisstr);
	SCMDCHECK(itct==1,"cannot read axis value");
	if(!strcmp(axisstr,"0") || !strcmp(axisstr,"x")) axis=0;
	else if(!strcmp(axisstr,"1") || !strcmp(axisstr,"y")) axis=1;
	else if(!strcmp(axisstr,"2") || !strcmp(axisstr,"z")) axis=2;
	else axis=3;
	SCMDCHECK(axis>=0 && axis<dim,"illegal axis value");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mlg %mlg %mi",Varnames,Varvalues,Nvar,&low[axis],&high[axis],&nbin);
	SCMDCHECK(itct==3,"cannot read arguments: low high bins");
	SCMDCHECK(low[axis]<high[axis],"low value needs to be less than high value");
	SCMDCHECK(nbin>0,"bins value needs to be > 0");
	line2=strnword(line2,4);
	ax2=0;
	for(d=0;d<dim-1;d++) {
		if(ax2==axis) ax2++;
		SCMDCHECK(line2,"missing position arguments");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&low[ax2],&high[ax2]);
		SCMDCHECK(itct==2,"cannot read (or insufficient) position arguments");
		SCMDCHECK(low[ax2]<=high[ax2],"low value needs to be less than or equal to high value");
		line2=strnword(line2,3);
		ax2++; }
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&average);
	SCMDCHECK(itct==1,"cannot read average number");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1)
		for(bin=0;bin<nbin;bin++) ct[bin]=0;
	scale=(double)nbin/(high[axis]-low[axis]);

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolcountspace);
		inscan=0;

		if(sim->latticess) {
			if(cmd->i2!=nbin) {
				free(cmd->v2);
				cmd->i2=nbin;
				cmd->v2=calloc(nbin,sizeof(int));
				if(!cmd->v2) {cmd->i2=-1;return CMDwarn;} }

			ctlat=(int*)cmd->v2;
			for(ilat=0;ilat<sim->latticess->nlattice;ilat++) {
				lat=sim->latticess->latticelist[ilat];
				if(lat->type==LATTICEnsv) {
					for(j=0;j<index[PDnresults];j++) {
						NSV_CALL(nsv_molcountspace(lat->nsv,index[PDMAX+j],low,high,dim,nbin,axis,ctlat));
						for(bin=0;bin<nbin;++bin)
							ct[bin]+=ctlat[bin]; }}
				else if(lat->type==LATTICEpde) {			//not implemented
					}}}}

	if(average<=1) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%i",ct[bin]);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[bin]); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	else if(cmd->invoke%average==0) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",(double)(ct[bin])/(double)average);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)(ct[bin])/(double)average); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(d=0;d<sim->dim;d++)
		if(mptr->pos[d]<=low[d] || mptr->pos[d]>=high[d]) return CMDok;
	bin=(int)floor(scale*(mptr->pos[axis]-low[axis]));
	if(bin==nbin) bin--;
	ct[bin]++;
	return CMDok; }


/* cmdmolcountspace2d */
enum CMDcode cmdmolcountspace2d(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int dim,i,itct,d,bin,average,*index,curaxis,bin1,bin2,er,dataid;
	enum MolecState ms;
	char axisstr[STRCHAR];
	moleculeptr mptr;

	static double low[DIMMAX],high[DIMMAX],scale1,scale2;
	static int inscan=0,nbin1,nbin2,*ct,axis,axis1,axis2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	dim=sim->dim;
	SCMDCHECK(line2,"missing arguments");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);

	SCMDCHECK(line2,"missing arguments");
	itct=sscanf(line2,"%s",axisstr);							// perpendicular axis
	SCMDCHECK(itct==1,"cannot read axis value");
	if(!strcmp(axisstr,"0") || !strcmp(axisstr,"x")) axis=0;
	else if(!strcmp(axisstr,"1") || !strcmp(axisstr,"y")) axis=1;
	else if(!strcmp(axisstr,"2") || !strcmp(axisstr,"z")) axis=2;
	else axis=3;
	SCMDCHECK((dim==2 && axis==2) || (dim==3 && axis<3),"illegal axis value");
	line2=strnword(line2,2);

	SCMDCHECK(line2,"missing arguments");
	curaxis=0;
	if(curaxis==axis) curaxis++;									// first parallel axis
	itct=strmathsscanf(line2,"%mlg %mlg %mi",Varnames,Varvalues,Nvar,&low[curaxis],&high[curaxis],&nbin1);
	SCMDCHECK(itct==3,"cannot read arguments: low high bins");
	SCMDCHECK(low[curaxis]<high[curaxis],"low value needs to be less than high value");
	SCMDCHECK(nbin1>0,"bins value needs to be > 0");
	axis1=curaxis;
	line2=strnword(line2,4);

	SCMDCHECK(line2,"missing arguments");
	curaxis++;
	if(curaxis==axis) curaxis++;									// second parallel axis
	itct=strmathsscanf(line2,"%mlg %mlg %mi",Varnames,Varvalues,Nvar,&low[curaxis],&high[curaxis],&nbin2);
	SCMDCHECK(itct==3,"cannot read arguments: low high bins");
	SCMDCHECK(low[curaxis]<high[curaxis],"low value needs to be less than high value");
	SCMDCHECK(nbin2>0,"bins value needs to be > 0");
	axis2=curaxis;
	line2=strnword(line2,4);

	if(dim==3) {
		curaxis=axis;																// perpendicular axis
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&low[curaxis],&high[curaxis]);
		SCMDCHECK(itct==2,"cannot read (or insufficient) position arguments");
		SCMDCHECK(low[curaxis]<=high[curaxis],"low value needs to be less than or equal to high value");
		line2=strnword(line2,3); }

	SCMDCHECK(line2,"missing arguments");					// average
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&average);
	SCMDCHECK(itct==1,"cannot read average number");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin1*nbin2) {											// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin1*nbin2;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin1*nbin2,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1)
		for(bin=0;bin<nbin1*nbin2;bin++) ct[bin]=0;
	scale1=(double)nbin1/(high[axis1]-low[axis1]);
	scale2=(double)nbin2/(high[axis2]-low[axis2]);

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolcountspace2d);
		inscan=0; }

	if(average<=1 || cmd->invoke%average==0) {
		scmdfprintf(cmd->cmds,fptr,"%g\n",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin2=0;bin2<nbin2;bin2++) {
			bin1=0;
			if(average<=1) {
				scmdfprintf(cmd->cmds,fptr,"%i",ct[bin2*nbin1+bin1]);
				scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[bin2*nbin1+bin1]); }
			else {
				scmdfprintf(cmd->cmds,fptr,"%g",(double)(ct[bin2*nbin1+bin1]/(double)average));
				scmdappenddata(cmd->cmds,dataid,0,1,(double)(ct[bin2*nbin1+bin1]/(double)average)); }
			for(bin1=1;bin1<nbin1;bin1++) {
				if(average<=1) {
					scmdfprintf(cmd->cmds,fptr,"%,%i",ct[bin2*nbin1+bin1]);
					scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[bin2*nbin1+bin1]); }
				else {
					scmdfprintf(cmd->cmds,fptr,"%,%g",(double)(ct[bin2*nbin1+bin1]/(double)average));
					scmdappenddata(cmd->cmds,dataid,0,1,(double)(ct[bin2*nbin1+bin1]/(double)average)); }}
			scmdfprintf(cmd->cmds,fptr,"\n"); }
		scmdflush(fptr); }

	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(d=0;d<sim->dim;d++)
		if(mptr->pos[d]<=low[d] || mptr->pos[d]>=high[d]) return CMDok;
	bin1=(int)floor(scale1*(mptr->pos[axis1]-low[axis1]));
	bin2=(int)floor(scale2*(mptr->pos[axis2]-low[axis2]));
	if(bin1==nbin1) bin1--;
	if(bin2==nbin2) bin2--;
	bin=bin2*nbin1+bin1;
	ct[bin]++;
	return CMDok; }


/* cmdmolcountspaceradial */
enum CMDcode cmdmolcountspaceradial(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,itct,d,bin,average,*index,er,dataid;
	enum MolecState ms;
	double radius,molrad;
	moleculeptr mptr;
	static double center[DIMMAX],scale,radius2;
	static int inscan=0,nbin,*ct;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	SCMDCHECK(line2,"missing arguments");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	for(d=0;d<sim->dim;d++) {
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&center[d]);
		SCMDCHECK(itct==1,"missing center value");
		line2=strnword(line2,2);
		SCMDCHECK(line2,"missing arguments"); }
	itct=strmathsscanf(line2,"%mlg %mi",Varnames,Varvalues,Nvar,&radius,&nbin);
	SCMDCHECK(itct==2,"cannot read arguments: radius bins");
	SCMDCHECK(radius>0,"radius needs to be greater than 0");
	SCMDCHECK(nbin>0,"bins value needs to be > 0");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&average);
	SCMDCHECK(itct==1,"cannot read average number");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1)
		for(bin=0;bin<nbin;bin++) ct[bin]=0;
	scale=(double)nbin/radius;

	radius2=radius*radius;

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolcountspaceradial);
		inscan=0; }

	if(average<=1) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%i",ct[bin]);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[bin]); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	else if(cmd->invoke%average==0) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",(double)(ct[bin])/(double)average);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)(ct[bin])/(double)average); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	molrad=0;
	for(d=0;d<sim->dim;d++)
		molrad+=(mptr->pos[d]-center[d])*(mptr->pos[d]-center[d]);
	if(molrad<radius2) {
		molrad=sqrt(molrad);
		bin=(int)floor(scale*molrad);
		if(bin==nbin) bin--;
		ct[bin]++; }
	return CMDok; }


/* cmdmolcountspacepolarangle */
enum CMDcode cmdmolcountspacepolarangle(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,itct,d,bin,average,*index,er,dataid;
	enum MolecState ms;
	double radiusmin,radiusmax,molrad,poleleninv,angle;
	moleculeptr mptr;
	static double center[DIMMAX],pole[DIMMAX],poleangle,scale,radiusmin2,radiusmax2;
	static int inscan=0,nbin,*ct;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	SCMDCHECK(line2,"missing arguments");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	for(d=0;d<sim->dim;d++) {
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&center[d]);
		SCMDCHECK(itct==1,"missing center value");
		line2=strnword(line2,2);
		SCMDCHECK(line2,"missing arguments"); }
	for(d=0;d<sim->dim;d++) {
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&pole[d]);
		SCMDCHECK(itct==1,"missing pole value");
		line2=strnword(line2,2);
		SCMDCHECK(line2,"missing arguments"); }
	itct=strmathsscanf(line2,"%mlg %mlg %mi",Varnames,Varvalues,Nvar,&radiusmin,&radiusmax,&nbin);
	SCMDCHECK(itct==3,"cannot read arguments: radius_min radius_max bins");
	SCMDCHECK(nbin>0,"bins value needs to be > 0");
	line2=strnword(line2,4);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&average);
	SCMDCHECK(itct==1,"cannot read average number");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1)
		for(bin=0;bin<nbin;bin++) ct[bin]=0;
	scale=(double)nbin/(sim->dim==2?2*PI:PI);

	radiusmin2=radiusmin>=0?radiusmin*radiusmin:0;
	radiusmax2=radiusmax>=0?radiusmax*radiusmax:-1;
	if(sim->dim==2) {
		SCMDCHECK(pole[0]!=0 || pole[1]!=0,"pole vector is equal to zero");
		poleangle=atan2(pole[1],pole[0]); }
	else {
		poleangle=0;
		poleleninv=sqrt(pole[0]*pole[0]+pole[1]*pole[1]+pole[2]*pole[2]);
		SCMDCHECK(poleleninv>0,"pole vector is equal to zero");
		poleleninv=1.0/poleleninv;
		pole[0]*=poleleninv;
		pole[1]*=poleleninv;
		pole[2]*=poleleninv; }

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolcountspacepolarangle);
		inscan=0; }

	if(average<=1) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%i",ct[bin]);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)ct[bin]); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	else if(cmd->invoke%average==0) {
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",(double)(ct[bin])/(double)average);
			scmdappenddata(cmd->cmds,dataid,0,1,(double)(ct[bin])/(double)average); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	molrad=0;
	for(d=0;d<sim->dim;d++)
		molrad+=(mptr->pos[d]-center[d])*(mptr->pos[d]-center[d]);
	if(molrad>=radiusmin2 && (radiusmax2==-1 || molrad<=radiusmax2)) {
		if(sim->dim==2) {
			angle=atan2(mptr->pos[1]-center[1],mptr->pos[0]-center[0]);
			angle-=poleangle;
			if(angle<0) angle+=2*PI;
			else if(angle>2*PI) angle-=2*PI; }
		else {
			angle=acos(((mptr->pos[0]-center[0])*pole[0]+(mptr->pos[1]-center[1])*pole[1]+(mptr->pos[2]-center[2])*pole[2])/sqrt(molrad)); }
		bin=(int)floor(scale*angle);
		if(bin==nbin) bin--;
		ct[bin]++; }
	return CMDok; }


/* cmdradialdistribution */
enum CMDcode cmdradialdistribution(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int dim,i1,itct,d,bin,average,*index1,i,ll,m,wrap[DIMMAX],er,dataid;
	enum MolecState ms1,mslo,mshi,ms;
	moleculeptr mptr,mptr2;
	boxptr bptr;
	double dist,scale2,rdf;
	static double scale,radius,syswidth[DIMMAX];
	static int inscan=0,nbin,*ct,i2,*index2,lllo,llhi,mcount;
	static enum MolecState ms2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	SCMDCHECK(line2,"missing arguments");
	i1=molstring2index1(sim,line2,&ms1,&index1);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	i2=molstring2index1(sim,line2,&ms2,&index2);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	itct=strmathsscanf(line2,"%mlg %mi %mi",Varnames,Varvalues,Nvar,&radius,&nbin,&average);
	SCMDCHECK(itct==3,"cannot read arguments: radius bins average");
	SCMDCHECK(radius>0,"radius needs to be greater than 0");
	SCMDCHECK(nbin>0,"bins value needs to be > 0");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,4);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	dim=sim->dim;
	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1) {
		for(bin=0;bin<nbin;bin++) ct[bin]=0;
		mcount=0; }
	scale=(double)nbin/radius;										// 1/scale is radial distance per bin
	if(dim==1) scale2=2.0/scale;									// scale2*bin^dim = volume inside bin where bin=0 for first bin
	else if(dim==2) scale2=PI/(scale*scale);
	else scale2=(4.0*PI/3.0)/(scale*scale*scale);

	lllo=llhi=-1;
	if(ms2<MSMAX) {
		mslo=ms2;
		mshi=(enum MolecState)(ms2+1); }
	else {
		mslo=(enum MolecState) 0;
		mshi=(enum MolecState) MSMAX; }
	for(i=0;i<index2[PDnresults];i++)
		for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1)) {
			ll=sim->mols->listlookup[index2[PDMAX+i]][ms];
			if(ll<lllo || lllo==-1) lllo=ll;
			if(ll>=llhi || llhi==-1) llhi=ll+1; }

	for(d=0;d<dim;d++)
		syswidth[d]=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;

	inscan=1;
	molscancmd(sim,i1,index1,ms1,cmd,cmdradialdistribution);
	inscan=0;

	if(average<=1 || cmd->invoke%average==0) {
		if(average<1) average=1;
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			if(dim==1) rdf=(double)ct[bin]/((double)mcount*scale2);
			else if(dim==2) rdf=(double)ct[bin]/((double)mcount*scale2*(2*bin+1));			// (bin+1)^2-bin^2=(2*bin+1)
			else rdf=(double)ct[bin]/((double)mcount*scale2*(3*bin*bin+3*bin+1));		// (bin+1)^3-bin^3=(3*bin^2+3*bin+1)
			scmdfprintf(cmd->cmds,fptr,"%,%g",rdf);
			scmdappenddata(cmd->cmds,dataid,0,1,rdf);	}
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	scmdflush(fptr);
	return CMDok;

 scanportion:
	dim=sim->dim;
	mptr=(moleculeptr) line2;
	mcount++;																									// mcount is number of "center" molecules
	bptr=boxscansphere(sim,mptr->pos,radius,NULL,wrap);
	while(bptr) {
		for(ll=lllo;ll<llhi;ll++)
			for(m=0;m<bptr->nmol[ll];m++) {
				mptr2=bptr->mol[ll][m];
				if(mptr2!=mptr && molismatch(mptr2,i2,index2,ms2)) {
					dist=0;
					for(d=0;d<dim;d++)
						dist+=(mptr2->pos[d]+wrap[d]*syswidth[d]-mptr->pos[d])*(mptr2->pos[d]+wrap[d]*syswidth[d]-mptr->pos[d]);
					dist=sqrt(dist);
					bin=(int)floor(scale*dist);
					if(bin<nbin)
						ct[bin]++; }}
		bptr=boxscansphere(sim,mptr->pos,radius,bptr,wrap); }
	return CMDok;	}


/* cmdradialdistribution2 */
enum CMDcode cmdradialdistribution2(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int dim,i1,itct,d,bin,average,*index1,i,ll,m,wrap[DIMMAX],er,dataid;
	enum MolecState ms1,mslo,mshi,ms;
	moleculeptr mptr,mptr2;
	boxptr bptr;
	double dist,scale2,rdf;
	static double scale,radius,syswidth[DIMMAX],lowpos[DIMMAX],highpos[DIMMAX];
	static int inscan=0,nbin,*ct,i2,*index2,lllo,llhi,mcount;
	static enum MolecState ms2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(cmd->i1!=-1,"error on setup");					// failed before, don't try again

	SCMDCHECK(line2,"missing arguments");
	i1=molstring2index1(sim,line2,&ms1,&index1);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	i2=molstring2index1(sim,line2,&ms2,&index2);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing arguments");
	for(d=0;d<sim->dim;d++) {
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&lowpos[d],&highpos[d]);
		SCMDCHECK(itct==2,"missing arguments");
		SCMDCHECK(lowpos[d]<=highpos[d],"low position value needs to be <= high position value");
		line2=strnword(line2,3);
		SCMDCHECK(line2,"missing arguments"); }
	itct=strmathsscanf(line2,"%mlg %mi %mi",Varnames,Varvalues,Nvar,&radius,&nbin,&average);
	SCMDCHECK(itct==3,"cannot read arguments: radius bins average");
	SCMDCHECK(radius>0,"radius needs to be greater than 0");
	SCMDCHECK(nbin>0,"bins value needs to be > 0");
	SCMDCHECK(average>=0,"illegal average value");
	line2=strnword(line2,4);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	if(cmd->i1!=nbin) {														// allocate counter if required
		cmdv1free(cmd);
		cmd->i1=nbin;
		cmd->freefn=&cmdv1v2free;
		cmd->v1=calloc(nbin,sizeof(int));
		if(!cmd->v1) {cmd->i1=-1;return CMDwarn;} }

	dim=sim->dim;
	ct=(int*)cmd->v1;
	if(average<=1 || cmd->invoke%average==1) {
		for(bin=0;bin<nbin;bin++) ct[bin]=0;
		mcount=0; }
	scale=(double)nbin/radius;										// 1/scale is radial distance per bin
	if(dim==1) scale2=2.0/scale;									// scale2*bin^dim = volume inside bin where bin=0 for first bin
	else if(dim==2) scale2=PI/(scale*scale);
	else scale2=(4.0*PI/3.0)/(scale*scale*scale);

	lllo=llhi=-1;
	if(ms2<MSMAX) {
		mslo=ms2;
		mshi=(enum MolecState)(ms2+1); }
	else {
		mslo=(enum MolecState) 0;
		mshi=(enum MolecState) MSMAX; }
	for(i=0;i<index2[PDnresults];i++)
		for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1)) {
			ll=sim->mols->listlookup[index2[PDMAX+i]][ms];
			if(ll<lllo || lllo==-1) lllo=ll;
			if(ll>=llhi || llhi==-1) llhi=ll+1; }

	for(d=0;d<dim;d++)
		syswidth[d]=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;

	inscan=1;
	molscancmd(sim,i1,index1,ms1,cmd,cmdradialdistribution2);
	inscan=0;

	if(average<=1 || cmd->invoke%average==0) {
		if(average<1) average=1;
		scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
		scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
		for(bin=0;bin<nbin;bin++) {
			if(dim==1) rdf=(double)ct[bin]/((double)mcount*scale2);
			else if(dim==2) rdf=(double)ct[bin]/((double)mcount*scale2*(2*bin+1));			// (bin+1)^2-bin^2=(2*bin+1)
			else rdf=(double)ct[bin]/((double)mcount*scale2*(3*bin*bin+3*bin+1));		// (bin+1)^3-bin^3=(3*bin^2+3*bin+1)
			scmdfprintf(cmd->cmds,fptr,"%,%g",rdf);
			scmdappenddata(cmd->cmds,dataid,0,1,rdf); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }
	scmdflush(fptr);
	return CMDok;

 scanportion:
	dim=sim->dim;
	mptr=(moleculeptr) line2;
	for(d=0;d<dim;d++) {
		if(mptr->pos[d]<lowpos[d] || mptr->pos[d]>highpos[d]) return CMDok; }
	mcount++;																									// mcount is number of "center" molecules
	bptr=boxscansphere(sim,mptr->pos,radius,NULL,wrap);
	while(bptr) {
		for(ll=lllo;ll<llhi;ll++)
			for(m=0;m<bptr->nmol[ll];m++) {
				mptr2=bptr->mol[ll][m];
				if(mptr2!=mptr && molismatch(mptr2,i2,index2,ms2)) {
					dist=0;
					for(d=0;d<dim;d++)
						dist+=(mptr2->pos[d]+wrap[d]*syswidth[d]-mptr->pos[d])*(mptr2->pos[d]+wrap[d]*syswidth[d]-mptr->pos[d]);
					dist=sqrt(dist);
					bin=(int)floor(scale*dist);
					if(bin<nbin)
						ct[bin]++; }}
		bptr=boxscansphere(sim,mptr->pos,radius,bptr,wrap); }
	return CMDok;	}


/* cmdmolcountspecies */
enum CMDcode cmdmolcountspecies(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,*index,count,er,dataid;
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	count=(i==-4)?0:molcount(sim,i,index,ms,-1);
	scmdfprintf(cmd->cmds,fptr,"%g%,%i\n",sim->time,count);
	scmdappenddata(cmd->cmds,dataid,1,2,sim->time,(double)count);
	scmdflush(fptr);
	return CMDok; }


/* cmdmolcountspecieslist */
enum CMDcode cmdmolcountspecieslist(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int i,*index,count,er,dataid;
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	while((line2=strnword(line2,2))) {
		i=molstring2index1(sim,line2,&ms,&index);
		SCMDCHECK(i!=-1,"species is missing or cannot be read");
		SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
		SCMDCHECK(i!=-3,"cannot read molecule state value");
		SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
		SCMDCHECK(i!=-7,"error allocating memory");
		count=(i==-4)?0:molcount(sim,i,index,ms,-1);
		scmdfprintf(cmd->cmds,fptr,"%,%i",count);
		scmdappenddata(cmd->cmds,dataid,0,1,(double)count); }

	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok; }


/* cmdmollistsize */
enum CMDcode cmdmollistsize(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int ll,itct,er,dataid;
	char listname[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	itct=sscanf(line2,"%s",listname);
	SCMDCHECK(itct==1,"cannot read molecule list name");
	SCMDCHECK(sim->mols && sim->mols->nlist>0,"no molecule lists defined");
	ll=stringfind(sim->mols->listname,sim->mols->nlist,listname);
	SCMDCHECK(ll>=0,"molecule list name not recognized");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	scmdfprintf(cmd->cmds,fptr,"%g%,%i\n",sim->time,sim->mols->nl[ll]);
	scmdappenddata(cmd->cmds,dataid,1,2,sim->time,(double)(sim->mols->nl[ll]));
	scmdflush(fptr);
	return CMDok; }


/* cmdlistmols */
enum CMDcode cmdlistmols(simptr sim,cmdptr cmd,char *line2) {
	int d,er;
	char string[STRCHAR];
	moleculeptr mptr;
	static FILE *fptr;
	static int inscan=0;
	static int dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(sim->mols,"molecules are undefined");

	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdlistmols);
	inscan=0;

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	scmdfprintf(cmd->cmds,fptr,"%s(%s)",sim->mols->spname[mptr->ident],molms2string(mptr->mstate,string));
	scmdappenddata(cmd->cmds,dataid,1,2,(double)(mptr->ident),(double)(mptr->mstate));
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
	scmdfprintf(cmd->cmds,fptr,"%,%s\n",molserno2string(mptr->serno,string));
	scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));
	return CMDok; }


/* cmdlistmols2 */
enum CMDcode cmdlistmols2(simptr sim,cmdptr cmd,char *line2) {
	int d,er;
	moleculeptr mptr;
	static FILE *fptr;
	static int inscan=0,invk,dataid=-1;
	char string[STRCHAR];

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(sim->mols,"molecules are undefined");

	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	invk=cmd?cmd->invoke:0;

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdlistmols2);
	inscan=0;

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	scmdfprintf(cmd->cmds,fptr,"%i%,%i%,%i",invk,mptr->ident,mptr->mstate);
	scmdappenddata(cmd->cmds,dataid,1,3,(double)invk,(double)(mptr->ident),(double)(mptr->mstate));
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
	scmdfprintf(cmd->cmds,fptr,"%,%s\n",molserno2string(mptr->serno,string));
	scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));
	return CMDok; }


/* cmdlistmols3 */
enum CMDcode cmdlistmols3(simptr sim,cmdptr cmd,char *line2) {
	int i,*index,d,er;
	moleculeptr mptr;
	enum MolecState ms;
	static FILE *fptr;
	static int inscan=0,invk,dataid=-1;
	char string[STRCHAR];

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	invk=cmd?cmd->invoke:0;

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdlistmols3);
		inscan=0; }

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	scmdfprintf(cmd->cmds,fptr,"%i%,%i%,%i",invk,mptr->ident,mptr->mstate);
	scmdappenddata(cmd->cmds,dataid,1,3,(double)invk,(double)(mptr->ident),(double)(mptr->mstate));
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
	scmdfprintf(cmd->cmds,fptr,"%,%s\n",molserno2string(mptr->serno,string));
	scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));
	return CMDok; }


/* cmdlistmols4 */
enum CMDcode cmdlistmols4(simptr sim,cmdptr cmd,char *line2) {
	int i,d,*index,er;
	moleculeptr mptr;
	enum MolecState ms;
	static FILE *fptr;
	static int inscan=0,invk,dataid=-1;
	char string[STRCHAR];

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	invk=cmd?cmd->invoke:0;

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdlistmols4);
		inscan=0; }

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	scmdfprintf(cmd->cmds,fptr,"%i%,%i%,%i",invk,mptr->ident,mptr->mstate);
	scmdappenddata(cmd->cmds,dataid,1,3,(double)invk,(double)(mptr->ident),(double)(mptr->mstate));
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]+mptr->posoffset[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]+mptr->posoffset[d]); }
	scmdfprintf(cmd->cmds,fptr,"%,%s\n",molserno2string(mptr->serno,string));
	scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));
	return CMDok; }


/* cmdlistmolscmpt */
enum CMDcode cmdlistmolscmpt(simptr sim,cmdptr cmd,char *line2) {
	int i,*index,c,itct,d,er;
	moleculeptr mptr;
	enum MolecState ms;
	char cname[STRCHAR],string[STRCHAR];
	compartssptr cmptss;
	static FILE *fptr;
	static compartptr cmpt;
	static int inscan=0,invk,dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing compartment name");
	itct=sscanf(line2,"%s",cname);
	SCMDCHECK(itct==1,"cannot read compartment name");
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
	invk=cmd?cmd->invoke:0;

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdlistmolscmpt);
		inscan=0; }

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0)) {
		scmdfprintf(cmd->cmds,fptr,"%i%,%i%,%i",invk,mptr->ident,mptr->mstate);
		scmdappenddata(cmd->cmds,dataid,1,3,(double)invk,(double)(mptr->ident),(double)(mptr->mstate));
		for(d=0;d<sim->dim;d++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
			scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
		scmdfprintf(cmd->cmds,fptr,"%,%s\n",molserno2string(mptr->serno,string));
		scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));	}
	return CMDok; }


/* cmdmolpos */
enum CMDcode cmdmolpos(simptr sim,cmdptr cmd,char *line2) {
	int i,d,*index,er;
	moleculeptr mptr;
	enum MolecState ms;
	static FILE *fptr;
	static int inscan=0,dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	scmdfprintf(cmd->cmds,fptr,"%g",sim->time);
	scmdappenddata(cmd->cmds,dataid,1,1,sim->time);
	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolpos);
		inscan=0; }

	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
	return CMDok; }


/* cmdtrackmol */
enum CMDcode cmdtrackmol(simptr sim,cmdptr cmd,char *line2) {
	int itct,d,c,er;
	moleculeptr mptr;
	char string[STRCHAR];
	static FILE *fptr;
	static unsigned long long serno;
	static int inscan=0,dataid=-1;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	itct=sscanf(line2,"%s",string);
	SCMDCHECK(itct==1,"cannot read molecule serial number");
	serno=molstring2serno(string);
	SCMDCHECK(serno>0,"cannot read molecule serial number");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdtrackmol);
	inscan=0;

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(!(mptr->serno==serno || (serno<0xFFFFFFFF && (mptr->serno&0xFFFFFFFF)==serno) || (serno<0xFFFFFFFF && mptr->serno>0xFFFFFFFF && (mptr->serno)>>32==serno)))
		return CMDok;
	scmdfprintf(cmd->cmds,fptr,"%g%,%s%,%s",sim->time,sim->mols->spname[mptr->ident],molms2string(mptr->mstate,string));
	scmdappenddata(cmd->cmds,dataid,1,3,sim->time,(double)(mptr->ident),(double)(mptr->mstate));
	scmdfprintf(cmd->cmds,fptr,"%,%s",molserno2string(mptr->serno,string));
	scmdappenddata(cmd->cmds,dataid,0,1,(double)(mptr->serno));
	for(d=0;d<sim->dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",mptr->pos[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,mptr->pos[d]); }
	if(sim->cmptss)
		for(c=0;c<sim->cmptss->ncmpt;c++) {
			if(posincompart(sim,mptr->pos,sim->cmptss->cmptlist[c],0)) {
				scmdfprintf(cmd->cmds,fptr,"%,in");
				scmdappenddata(cmd->cmds,dataid,0,1,(double)1.0); }
			else {
				scmdfprintf(cmd->cmds,fptr,"%,out");
				scmdappenddata(cmd->cmds,dataid,0,1,(double)0.0); }}
	scmdfprintf(cmd->cmds,fptr,"\n");
	return CMDstop; }


/* cmdmolmoments */
enum CMDcode cmdmolmoments(simptr sim,cmdptr cmd,char *line2) {
	int i,*index,d,d2,dim,er,dataid;
	FILE *fptr;
	moleculeptr mptr;
	enum MolecState ms;
	static double v1[DIMMAX],m1[DIMMAX*DIMMAX];
	static int inscan=0,ctr;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	dim=sim->dim;
	ctr=0;
	for(d=0;d<dim;d++) v1[d]=0;
	for(d=0;d<dim*dim;d++) m1[d]=0;

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdmolmoments);
		for(d=0;d<dim;d++) v1[d]/=1.0*ctr;
		inscan=2;
		molscancmd(sim,i,index,ms,cmd,cmdmolmoments);
		inscan=0; }

	scmdfprintf(cmd->cmds,fptr,"%g%,%i",sim->time,ctr);
	scmdappenddata(cmd->cmds,dataid,1,2,sim->time,(double)ctr);
	for(d=0;d<dim;d++) {
		scmdfprintf(cmd->cmds,fptr,"%,%g",v1[d]);
		scmdappenddata(cmd->cmds,dataid,0,1,v1[d]); }
	for(d=0;d<dim;d++)
		for(d2=0;d2<dim;d2++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",m1[d*dim+d2]/ctr);
			scmdappenddata(cmd->cmds,dataid,0,1,m1[d*dim+d2]/ctr); }
	scmdfprintf(cmd->cmds,fptr,"\n");
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(inscan==1) {
		ctr++;
		for(d=0;d<sim->dim;d++) v1[d]+=mptr->pos[d]; }
	else {
		for(d=0;d<sim->dim;d++)
			for(d2=0;d2<sim->dim;d2++)
				m1[d*sim->dim+d2]+=(mptr->pos[d]-v1[d])*(mptr->pos[d2]-v1[d2]); }
	return CMDok; }


/* cmdsavesim */
enum CMDcode cmdsavesim(simptr sim,cmdptr cmd,char *line2) {
	int er;
	FILE *fptr;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,1,&fptr,NULL);
	SCMDCHECK(er!=-1,"file name not recognized");
	if(line2) {
		strcutwhite(line2,2); }

	scmdfprintf(cmd->cmds,fptr,"# Configuration file automatically created by Smoldyn\n\n");
	writesim(sim,fptr);
	writegraphss(sim,fptr);
	writemols(sim,fptr);
	writewalls(sim,fptr);
	writesurfaces(sim,fptr);
	writecomparts(sim,fptr);
	writereactions(sim,fptr);
	writerules(sim,fptr);
	writelattices(sim,fptr);
	scmdwritecommands(sim->cmds,fptr,line2);
	writemolecules(sim,fptr);
	scmdfprintf(cmd->cmds,fptr,"\nend_file\n");
	scmdflush(fptr);
	return CMDok; }


/* cmdmeansqrdispfree */
void cmdmeansqrdispfree(cmdptr cmd) {
	int j;

	if(cmd->v2 && cmd->i1)
		for(j=0;j<cmd->i1;j++)
			free(((double**)(cmd->v2))[j]);
	free(cmd->v2);
	free(cmd->v1);
	return; }


/* cmdmeansqrdisp */
        enum CMDcode cmdmeansqrdisp(simptr sim,cmdptr cmd,char *line2) {
	char dimstr[STRCHAR];
	int i,*index,j,d,itct,dim,er,dataid;
	FILE *fptr;
	double r2,diff,**v2;
	long int *v1;
	enum MolecState ms;
	moleculeptr mptr;
	static double sum,sum4;
	static int inscan=0,ctr,msddim;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing dimension information");
	itct=sscanf(line2,"%s",dimstr);
	SCMDCHECK(itct==1,"cannot read dimension information");
	msddim=1;
	if(!strcmp(dimstr,"all")) msddim=-1;
	else if(!strcmp(dimstr,"0") || !strcmp(dimstr,"x")) msddim=0;
	else if(!strcmp(dimstr,"1") || !strcmp(dimstr,"y")) msddim=1;
	else if(!strcmp(dimstr,"2") || !strcmp(dimstr,"z")) msddim=2;
	else msddim=3;
	SCMDCHECK(msddim<sim->dim,"invalid dimension value");
	line2=strnword(line2,2);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	SCMDCHECK(cmd->i2!=2,"error on setup");					// failed before, don't try again

	dim=sim->dim;

	if(!cmd->i2) {										// test for first run
		cmd->i2=1;											// if first run, set up data structures
		ctr=(i==-4)?0:molcount(sim,i,index,ms,-1);
		cmd->i1=ctr;										// size of arrays
		SCMDCHECK(ctr>0,"no molecules to track");
		cmd->freefn=&cmdmeansqrdispfree;
		cmd->v1=calloc(ctr,sizeof(long int));	// v1 is serial numbers
		if(!cmd->v1) {cmd->i2=2;return CMDwarn;}
		for(j=0;j<ctr;j++) ((long int*)cmd->v1)[j]=0;
		cmd->v2=calloc(ctr,sizeof(double**));	// v2 is positions
		if(!cmd->v2) {cmd->i2=2;return CMDwarn;}
		for(j=0;j<ctr;j++) ((double**)cmd->v2)[j]=NULL;
		for(j=0;j<ctr;j++) {
			((double**)cmd->v2)[j]=(double*)calloc(dim,sizeof(double));
			if(!((double**)cmd->v2)[j]) {cmd->i2=2;return CMDwarn;}
			for(d=0;d<dim;d++) ((double**)cmd->v2)[j][d]=0; }
		ctr=0;
		if(i!=-4) {
			inscan=1;
			molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp);
			inscan=0; }
		sortVliv((long int*)cmd->v1,(void**)cmd->v2,cmd->i1); }

	ctr=0;														// start of code that is run every invocation
	sum=0;
	sum4=0;

	if(i!=-4) {
		inscan=2;
		molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp);
		inscan=0; }

	scmdfprintf(cmd->cmds,fptr,"%g%,%g%,%g\n",sim->time,sum/ctr,sum4/ctr);
	scmdappenddata(cmd->cmds,dataid,1,3,sim->time,sum/ctr,sum4/ctr);

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(inscan==1) {
		((long int*)cmd->v1)[ctr]=(long int)(mptr->serno&0xFFFFFFFF);
		for(d=0;d<sim->dim;d++)
			((double**)cmd->v2)[ctr][d]=mptr->posoffset[d]+mptr->pos[d];
		ctr++; }
	else {
		v1=(long int*)cmd->v1;
		v2=(double**)cmd->v2;
		j=locateVli(v1,(long int)(mptr->serno&0xFFFFFFFF),cmd->i1);
		if(j>=0) {
			ctr++;
			if(msddim<0) {
				r2=0;
				for(d=0;d<sim->dim;d++) {
					diff=mptr->posoffset[d]+mptr->pos[d]-v2[j][d];
					r2+=diff*diff; }
				sum+=r2;
				sum4+=r2*r2; }
			else {
				diff=mptr->posoffset[msddim]+mptr->pos[msddim]-v2[j][msddim];
				sum+=diff*diff;
				sum4+=diff*diff*diff*diff; }}}
	return CMDok; }


/* cmdmeansqrdisp2 */
enum CMDcode cmdmeansqrdisp2(simptr sim,cmdptr cmd,char *line2) {
	char dimstr[STRCHAR];
	int i,j,d,itct,dim,msddim,maxmoment,mom,*index,er,dataid;
	FILE *fptr;
	moleculeptr mptr;
	double sum[17];
	double r2,diff,**v2,*dblptr;
	long int *v1;
	enum MolecState ms;
	char reportchar;
	static char startchar;
	static int inscan=0,maxmol,ctr;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing dimension information");
	itct=sscanf(line2,"%s",dimstr);
	SCMDCHECK(itct==1,"cannot read dimension information");
	if(!strcmp(dimstr,"all")) msddim=-1;
	else if(!strcmp(dimstr,"0") || !strcmp(dimstr,"x")) msddim=0;
	else if(!strcmp(dimstr,"1") || !strcmp(dimstr,"y")) msddim=1;
	else if(!strcmp(dimstr,"2") || !strcmp(dimstr,"z")) msddim=2;
	else msddim=3;
	SCMDCHECK(msddim<sim->dim,"invalid dimension value");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"insufficient arguments");
	itct=strmathsscanf(line2,"%c %c %mi %mi",Varnames,Varvalues,Nvar,&startchar,&reportchar,&maxmol,&maxmoment);
	SCMDCHECK(itct==4,"cannot read start, report, max_mol, or max_moment information");
	SCMDCHECK(maxmol>0,"max_mol has to be at least 1");
	SCMDCHECK(maxmoment>0,"maxmoment has to be at least 1");
	SCMDCHECK(maxmoment<=16,"max_moment cannot exceed 16");
	line2=strnword(line2,5);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	SCMDCHECK(cmd->i2!=2,"error on setup");					// failed before, don't try again

	dim=sim->dim;

	if(!cmd->i2) {										// test for first run
		cmd->i2=1;											// if first run, set up data structures
		cmd->i1=maxmol;
		cmd->i3=0;
		cmd->freefn=&cmdmeansqrdispfree;
		cmd->v1=calloc(maxmol,sizeof(long int));	// v1 is serial numbers
		if(!cmd->v1) {cmd->i2=2;return CMDwarn;}
		v1=(long int*)cmd->v1;
		for(j=0;j<maxmol;j++) v1[j]=0;
		cmd->v2=calloc(maxmol,sizeof(double**));	// v2 is positions
		if(!cmd->v2) {cmd->i2=2;return CMDwarn;}
		v2=(double**)cmd->v2;
		for(j=0;j<maxmol;j++) v2[j]=NULL;
		for(j=0;j<maxmol;j++) {
			v2[j]=(double*)calloc(2*dim+1,sizeof(double));
			if(!v2[j]) {cmd->i2=2;return CMDwarn;}
			for(d=0;d<2*dim+1;d++) v2[j][d]=0; }
		ctr=0;
		if(i!=-4) {
			inscan=1;
			molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp2);
			inscan=0; }
		SCMDCHECK(ctr<maxmol,"insufficient allocated space");
		cmd->i3=ctr;
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3); }

	v1=(long int*)cmd->v1;						// start of code that is run every invocation
	v2=(double**)cmd->v2;

	if(i!=-4) {
		inscan=2;												// update tracking information for all tracked molecules
		molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp2);
		inscan=0; }
	if(cmd->i3==cmd->i1) {SCMDCHECK(0,"not enough allocated space");}
	if(startchar!='i')								// resort lists if appropriate
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

	for(mom=0;mom<=maxmoment;mom++)
		sum[mom]=0;
	for(j=0;j<cmd->i3;j++) {					// find moments of all reported results
		if((reportchar=='e' && v2[j][0]==3.0) || (reportchar=='r' && v2[j][0]==2.0)) { // molecule should be recorded
			if(msddim<0) {
				r2=0;
				for(d=0;d<dim;d++) {
					diff=v2[j][dim+1+d]-v2[j][1+d];
					r2+=diff*diff; }
				r2=sqrt(r2);
				for(mom=0;mom<=maxmoment;mom++)
					sum[mom]+=pow(r2,mom); }
			else {
				diff=v2[j][dim+1+msddim]-v2[j][1+msddim];
				for(mom=0;mom<=maxmoment;mom++)
					sum[mom]+=pow(diff,mom); }}}

	if(sum[0]>0) {
		scmdfprintf(cmd->cmds,fptr,"%g%,%g",sim->time,sum[0]);					// display results
		scmdappenddata(cmd->cmds,dataid,1,2,sim->time,sum[0]);
		for(mom=1;mom<=maxmoment;mom++) {
			scmdfprintf(cmd->cmds,fptr,"%,%g",sum[mom]/sum[0]);
			scmdappenddata(cmd->cmds,dataid,0,1,sum[mom]/sum[0]); }
		scmdfprintf(cmd->cmds,fptr,"\n"); }

	for(j=0;j<cmd->i3;j++) {							// stop tracking expired molecules
		if(v2[j][0]==0 || v2[j][0]==2.0) {
			v1[j]=v1[cmd->i3-1];
			v1[cmd->i3-1]=0;
			dblptr=v2[j];
			v2[j]=v2[cmd->i3-1];
			v2[cmd->i3-1]=dblptr;
			v2[cmd->i3-1][0]=0;
			j--;
			cmd->i3--; }
		else
			v2[j][0]-=1.0; }
	if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	v1=(long int*)cmd->v1;
	v2=(double**)cmd->v2;
	if(inscan==1) {
		if(ctr==maxmol) return CMDstop;
			v1[ctr]=(long int)(mptr->serno&0xFFFFFFFF);
			if(startchar=='c') v2[ctr][0]=0;
			else v2[ctr][0]=2.0;
			for(d=0;d<sim->dim;d++)
				v2[ctr][1+d]=v2[ctr][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d];
			ctr++; }
	else {
		j=locateVli(v1,(long int)(mptr->serno&0xFFFFFFFF),cmd->i3);
		if(j>=0) {										// molecule was found
			v2[j][0]+=1.0;
			if(v2[j][0]==3.0) {					// molecule is being tracked and exists, so record current positions
				for(d=0;d<sim->dim;d++)
					v2[j][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d]; }}
		else if(startchar!='i') {			// molecule was not found but should be tracked
			if(cmd->i3==cmd->i1) return CMDstop;
			j=cmd->i3++;					// find empty spot
			v1[j]=(long int)(mptr->serno&0xFFFFFFFF);
			v2[j][0]=3.0;
			for(d=0;d<sim->dim;d++)
				v2[j][1+d]=v2[j][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d]; }}
	return CMDok; }


/* cmdmeansqrdisp3 */
enum CMDcode cmdmeansqrdisp3(simptr sim,cmdptr cmd,char *line2) {
	char dimstr[STRCHAR];
	int i,j,d,itct,dim,msddim,*index,er,dataid;
	FILE *fptr;
	moleculeptr mptr;
	double sum,wgt;
	double r2,diff,**v2,*dblptr,change;
	long int *v1;
	enum MolecState ms;
	char reportchar;
	static char startchar;
	static int inscan=0,ctr,maxmol;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing dimension information");
	itct=sscanf(line2,"%s",dimstr);
	SCMDCHECK(itct==1,"cannot read dimension information");
	if(!strcmp(dimstr,"all")) msddim=-1;
	else if(!strcmp(dimstr,"0") || !strcmp(dimstr,"x")) msddim=0;
	else if(!strcmp(dimstr,"1") || !strcmp(dimstr,"y")) msddim=1;
	else if(!strcmp(dimstr,"2") || !strcmp(dimstr,"z")) msddim=2;
	else msddim=3;
	SCMDCHECK(msddim<sim->dim,"invalid dimension value");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"insufficient arguments");
	itct=strmathsscanf(line2,"%c %c %i %mlg",Varnames,Varvalues,Nvar,&startchar,&reportchar,&maxmol,&change);
	SCMDCHECK(itct==4,"cannot read start, report, max_mol, or change information");
	SCMDCHECK(maxmol>0,"max_mol has to be at least 1");
	line2=strnword(line2,5);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");
  line2=strnword(line2,2);
  SCMDCHECK(change<=0 || line2,"missing task to be accomplished if change is small");

	SCMDCHECK(cmd->i2!=2,"error on setup");					// failed before, don't try again

	dim=sim->dim;

	if(!cmd->i2) {										// test for first run
		cmd->i2=1;											// if first run, set up data structures
		cmd->i1=maxmol;
		cmd->i3=0;
    cmd->f1=-1;
		cmd->freefn=&cmdmeansqrdispfree;
		cmd->v1=calloc(maxmol,sizeof(long int));	// v1 is serial numbers
		if(!cmd->v1) {cmd->i2=2;return CMDwarn;}
		v1=(long int*)cmd->v1;
		for(j=0;j<maxmol;j++) v1[j]=0;
		cmd->v2=calloc(maxmol,sizeof(double**));	// v2 is positions
		if(!cmd->v2) {cmd->i2=2;return CMDwarn;}
		v2=(double**)cmd->v2;
		for(j=0;j<maxmol;j++) v2[j]=NULL;
		for(j=0;j<maxmol;j++) {
			v2[j]=(double*)calloc(2*dim+2,sizeof(double));
			if(!v2[j]) {cmd->i2=2;return CMDwarn;}
			for(d=0;d<2*dim+2;d++) v2[j][d]=0; }
		ctr=0;
		if(i!=-4) {
			inscan=1;
			molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp3);
			inscan=0; }
		SCMDCHECK(ctr<maxmol,"insufficient allocated space");
		cmd->i3=ctr;
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3); }

	v1=(long int*)cmd->v1;						// start of code that is run every invocation
	v2=(double**)cmd->v2;

	if(i!=-4) {
		inscan=2;
		molscancmd(sim,i,index,ms,cmd,cmdmeansqrdisp3);
		inscan=0; }
	if(cmd->i3==cmd->i1) {SCMDCHECK(0,"not enough allocated space");}
	if(startchar!='i')								// resort lists if appropriate
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

  sum=0;
  ctr=0;
  wgt=0;
	for(j=0;j<cmd->i3;j++) {					// find effective diffusion coefficients of all reported results
		if((reportchar=='e' && v2[j][0]==3.0) || (reportchar=='r' && v2[j][0]==2.0)) { // molecule should be recorded
      ctr++;
			if(msddim<0) {
				r2=0;
				for(d=0;d<dim;d++) {
					diff=v2[j][dim+1+d]-v2[j][1+d];
					r2+=diff*diff; }
        sum+=r2;
        wgt+=sim->time-v2[j][2*dim+1]; }
			else {
				diff=v2[j][dim+1+msddim]-v2[j][1+msddim];
        r2=diff*diff;
        sum+=r2;
        wgt+=sim->time-v2[j][2*dim+1]; }}}
  if(msddim<0) sum/=(2.0*dim);
  else sum/=2.0;

  scmdfprintf(cmd->cmds,fptr,"%g%,%i%,%g\n",sim->time,ctr,sum/wgt);					// display results
	scmdappenddata(cmd->cmds,dataid,1,3,sim->time,(double)ctr,sum/wgt);

	for(j=0;j<cmd->i3;j++) {							// stop tracking expired molecules
		if(v2[j][0]==0 || v2[j][0]==2.0) {
			v1[j]=v1[cmd->i3-1];
			v1[cmd->i3-1]=0;
			dblptr=v2[j];
			v2[j]=v2[cmd->i3-1];
			v2[cmd->i3-1]=dblptr;
			v2[cmd->i3-1][0]=0;
			j--;
			cmd->i3--; }
		else
			v2[j][0]-=1.0; }
	if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

  if(change>0 && ctr>0 && cmd->f1>0 && fabs((sum/ctr-cmd->f1)/cmd->f1)<change)
    return docommand(sim,cmd,line2);
  cmd->f1=sum/ctr;
	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	v1=(long int*)cmd->v1;
	v2=(double**)cmd->v2;
	if(inscan==1) {
		if(ctr==maxmol) return CMDstop;
		v1[ctr]=(long int)(mptr->serno&0xFFFFFFFF);
		if(startchar=='c') v2[ctr][0]=0;
		else v2[ctr][0]=2.0;
		for(d=0;d<sim->dim;d++)
			v2[ctr][1+d]=v2[ctr][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d];
		v2[ctr][2*sim->dim+1]=sim->time;
		ctr++; }
	else {
		j=locateVli(v1,(long int)(mptr->serno&0xFFFFFFFF),cmd->i3);
		if(j>=0) {										// molecule was found
			v2[j][0]+=1.0;
			if(v2[j][0]==3.0) {					// molecule is being tracked and exists, so record current positions
				for(d=0;d<sim->dim;d++)
					v2[j][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d]; }}
		else if(startchar!='i') {			// molecule was not found but should be tracked
			if(cmd->i3==cmd->i1) return CMDstop;
			j=cmd->i3++;					// find empty spot
			v1[j]=(long int)(mptr->serno&0xFFFFFFFF);
			v2[j][0]=3.0;
			for(d=0;d<sim->dim;d++)
				v2[j][1+d]=v2[j][sim->dim+1+d]=mptr->posoffset[d]+mptr->pos[d];
			v2[j][2*sim->dim+1]=sim->time; }}
	return CMDok; }


/* cmdresidencetime */
enum CMDcode cmdresidencetime(simptr sim,cmdptr cmd,char *line2) {
	int i,j,d,itct,summaryout,listout,*index,er,dataid;
	FILE *fptr;
	moleculeptr mptr;
	double sum;
	double **v2,*dblptr;
	long int *v1;
	enum MolecState ms;
	char reportchar;
	static char startchar;
	static int inscan=0,ctr,maxmol;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"insufficient arguments");
	itct=strmathsscanf(line2,"%c %c %mi %mi %mi",Varnames,Varvalues,Nvar,&startchar,&reportchar,&summaryout,&listout,&maxmol);
	SCMDCHECK(itct==5,"cannot read start, report, summary_out, list_out, or max_mol information");
	SCMDCHECK(maxmol>0,"max_mol has to be at least 1");
	line2=strnword(line2,6);
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	SCMDCHECK(cmd->i2!=2,"error on setup");					// failed before, don't try again

	if(!cmd->i2) {										// test for first run
		cmd->i2=1;											// if first run, set up data structures
		cmd->i1=maxmol;
		cmd->i3=0;
		cmd->freefn=&cmdmeansqrdispfree;
		cmd->v1=calloc(maxmol,sizeof(long int));	// v1 is serial numbers
		if(!cmd->v1) {cmd->i2=2;return CMDwarn;}
		v1=(long int*)cmd->v1;
		for(j=0;j<maxmol;j++) v1[j]=0;
		cmd->v2=calloc(maxmol,sizeof(double**));	// v2 is creation times
		if(!cmd->v2) {cmd->i2=2;return CMDwarn;}
		v2=(double**)cmd->v2;
		for(j=0;j<maxmol;j++) v2[j]=NULL;
		for(j=0;j<maxmol;j++) {
			v2[j]=(double*)calloc(2,sizeof(double));
			if(!v2[j]) {cmd->i2=2;return CMDwarn;}
			for(d=0;d<2;d++) v2[j][d]=0; }
		ctr=0;
		if(i!=-4) {
			inscan=1;
			molscancmd(sim,i,index,ms,cmd,cmdresidencetime);
			inscan=0; }
		SCMDCHECK(ctr<maxmol,"insufficient allocated space");
		cmd->i3=ctr;
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3); }

	v1=(long int*)cmd->v1;						// start of code that is run every invocation
	v2=(double**)cmd->v2;

	if(i!=-4) {
		inscan=2;
		molscancmd(sim,i,index,ms,cmd,cmdresidencetime);
		inscan=0; }

	if(cmd->i3==cmd->i1) {SCMDCHECK(0,"not enough allocated space");}
	if(startchar!='i')								// resort lists if appropriate
		if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

  sum=0;
  ctr=0;
	for(j=0;j<cmd->i3;j++) {					// find effective diffusion coefficients of all reported results
		if((reportchar=='e' && v2[j][0]==3.0) || (reportchar=='r' && v2[j][0]==2.0)) { // molecule should be recorded
      ctr++;
      sum+=sim->time-v2[j][1];
      if(listout>0 && cmd->invoke>0 && cmd->invoke%listout==0) {
        scmdfprintf(cmd->cmds,fptr,"%li%,%g\n",v1[j],sim->time-v2[j][1]);
        scmdappenddata(cmd->cmds,dataid,1,2,(double)v1[j],sim->time-v2[j][1]); }}}

  if(summaryout>0 && cmd->invoke>0 && cmd->invoke%summaryout==0) {
    scmdfprintf(cmd->cmds,fptr,"%g%,%i%,%g\n",sim->time,ctr,sum/(double)ctr);		// display results
		scmdappenddata(cmd->cmds,dataid,1,3,sim->time,(double)ctr,sum/(double)ctr); }

	for(j=0;j<cmd->i3;j++) {							// stop tracking expired molecules
		if(v2[j][0]==0 || v2[j][0]==2.0) {
			v1[j]=v1[cmd->i3-1];
			v1[cmd->i3-1]=0;
			dblptr=v2[j];
			v2[j]=v2[cmd->i3-1];
			v2[cmd->i3-1]=dblptr;
			v2[cmd->i3-1][0]=0;
			j--;
			cmd->i3--; }
		else
			v2[j][0]-=1.0; }
	if(cmd->i3>0) sortVliv(v1,(void**)cmd->v2,cmd->i3);

	scmdflush(fptr);
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	v1=(long int*)cmd->v1;
	v2=(double**)cmd->v2;
	if(inscan==1) {
		if(ctr==maxmol) return CMDstop;
		v1[ctr]=(long int)(mptr->serno&0xFFFFFFFF);
		if(startchar=='c') v2[ctr][0]=0;
		else v2[ctr][0]=2.0;
		v2[ctr][1]=sim->time;
		ctr++; }
	else {
		j=locateVli(v1,(long int)(mptr->serno&0xFFFFFFFF),cmd->i3);
		if(j>=0) {										// molecule was found
			v2[j][0]+=1.0; }
		else if(startchar!='i') {			// molecule was not found but should be tracked
			if(cmd->i3==cmd->i1) return CMDstop;
			j=cmd->i3++;                // find empty spot
			v1[j]=(long int)(mptr->serno&0xFFFFFFFF);
			v2[j][0]=3.0;
			v2[j][1]=sim->time; }}
	return CMDok; }


/* cmddiagnostics */
enum CMDcode cmddiagnostics(simptr sim,cmdptr cmd,char *line2) {
	int itct,order;
	static char nm[STRCHAR];
	enum SmolStruct ss;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"read failure");
	ss=simstring2ss(nm);
	SCMDCHECK(ss!=SSnone,"diagnostic type not recognized");

	if(ss==SSsim || ss==SSall) simoutput(sim);
	if(ss==SSwall || ss==SSall) walloutput(sim);
	if(ss==SSmolec || ss==SSall) molssoutput(sim);
	if(ss==SSsurf || ss==SSall) surfaceoutput(sim);
	if(ss==SScmd || ss==SSall) scmdoutput(sim->cmds);
	if(ss==SSbox || ss==SSall) boxssoutput(sim);
	if(ss==SSrxn || ss==SSall)
		for(order=0;order<MAXORDER;order++) rxnoutput(sim,order);
	if(ss==SSrule || ss==SSall) ruleoutput(sim);
	if(ss==SScmpt || ss==SSall) compartoutput(sim);
	if(ss==SSport || ss==SSall) portoutput(sim);
	if(ss==SScheck || ss==SSall) checksimparams(sim);
	return CMDok; }

/* cmdexecutiontime */
enum CMDcode cmdexecutiontime(simptr sim,cmdptr cmd,char *line2) {
	int er,dataid;
	FILE *fptr;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"file or data name not recognized");

	scmdfprintf(cmd->cmds,fptr,"%g%,%g\n",sim->time,sim->elapsedtime+difftime(time(NULL),sim->clockstt));
	scmdappenddata(cmd->cmds,dataid,1,2,sim->time,sim->elapsedtime+difftime(time(NULL),sim->clockstt));

	scmdflush(fptr);
	return CMDok; }


/* cmdprintLattice */
enum CMDcode cmdprintLattice(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	latticeptr lattice;
	char *buffer;
	int n,i,er;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;
	er=scmdgetfptr(sim->cmds,line2,1,&fptr,NULL);
	SCMDCHECK(er!=-1,"file name not recognized");

	n=sim->latticess->nlattice;
	buffer=NULL;
	for(i=0;i<n;++i) {
		lattice=sim->latticess->latticelist[i];
		scmdfprintf(cmd->cmds,fptr,"Lattice %d: %s:\n",i,lattice->latticename);
		NSV_CALL(nsv_print(lattice->nsv,&buffer));
		scmdfprintf(cmd->cmds,fptr,"%s",buffer?buffer:"Error");
		buffer=NULL; }
	scmdflush(fptr);
	return CMDok; }


/* cmdwriteVTK */
enum CMDcode cmdwriteVTK(simptr sim,cmdptr cmd,char *line2) {

#ifndef OPTION_VTK
	(void)sim;
	(void)cmd;
	(void)line2;
	simLog(NULL,11,"ERROR: VTK option not set. Recompile with OPTION_VTK = ON\n");
#else

	char nm[STRCHAR];
	moleculeptr mptr;
	int itct;
	static vtkUnstructuredGrid* grid;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

  SCMDCHECK(line2,"file name not given");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"read failure");

//	printf("Writing out VTK files....\n");
	grid = vtkCreateMolGrid();

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdwriteVTK);
	inscan=0;
	vtkWriteGrid(nm,"Molecules",cmd->invoke,grid);
	vtkDeleteGrid(grid);

#ifdef OPTION_NSV
	static char nm2[STRCHAR];
	latticeptr lattice;
	int ll;

	if (sim->latticess) {
		for (ll = 0; ll < sim->latticess->nlattice; ++ll) {
			lattice = sim->latticess->latticelist[ll];
			snprintf(nm2,STRCHAR,"Lattice%02d_",ll);
			switch(lattice->type) {
        case LATTICEnsv:
          if (lattice->nsv) vtkWriteGrid(nm,nm2,cmd->invoke,nsv_get_grid(lattice->nsv));
          break;
        case LATTICEpde:
          //not implemented
          break;
        case LATTICEnone:
        break; }}}
#endif

	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	vtkAddPoint(grid,mptr->pos[0],mptr->pos[1],mptr->pos[2],(long int)(mptr->serno&0xFFFFFFFF),mptr->ident);
#endif
	return CMDok; }


/* cmdprintdata */
enum CMDcode cmdprintdata(simptr sim,cmdptr cmd,char *line2) {
	FILE *fptr;
	int itct,er,did,dataid,i,j,erase;
	listptrdd list;
	cmdssptr cmds;
	char dname[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDobserve;

	cmds=sim->cmds;
	SCMDCHECK(line2,"missing data name");
	itct=sscanf(line2,"%s",dname);
	SCMDCHECK(itct==1,"cannot read data name");
	SCMDCHECK(cmds->ndata,"no data files have been declared");
	did=stringfind(cmds->dname,cmds->ndata,dname);
	SCMDCHECK(did>=0,"data name not recognized");
	list=cmds->data[did];

	line2=strnword(line2,2);
	er=scmdgetfptr(cmds,line2,3,&fptr,&dataid);
	SCMDCHECK(er!=-1,"output file or data name not recognized");

	erase=0;
	if(line2 && (line2=strnword(line2,2))) {
		itct=sscanf(line2,"%i",&erase);
		SCMDCHECK(itct==1,"erase value needs to be 0 or 1"); }

	for(i=0;i<list->nrow;i++) {
		for(j=0;j<list->ncol;j++) {
			scmdfprintf(cmds,fptr,"%g",list->data[i*list->maxcol+j]);
			if(j<list->ncol-1) scmdfprintf(cmds,fptr,"%,");
			scmdappenddata(cmds,dataid,j==0?1:0,1,list->data[i*list->maxcol+j]); }
		scmdfprintf(cmds,fptr,"\n"); }
	scmdflush(fptr);

	if(erase) ListClearDD(list);

	return CMDok; }


/**********************************************************/
/****************** system manipulation ********************/
/**********************************************************/


/* cmdset */
enum CMDcode cmdset(simptr sim,cmdptr cmd,char *line2) {
	int er,itct;
	char word[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;
	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",word);
	SCMDCHECK(itct==1,"missing statement");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing statement text");
	er=simreadstring(sim,NULL,word,line2);
	SCMDCHECK(!er,"%s",ErrorString);
	return CMDok; }


/* cmdpointsource */
enum CMDcode cmdpointsource(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i;
	char nm[STRCHAR];
	double pos[DIMMAX];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;
	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	itct=strmathsscanf(line2,"%s %mi",Varnames,Varvalues,Nvar,nm,&num);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(num>=0,"number cannot be negative");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"name not recognized");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"missing location");
	if(sim->dim==1) itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&pos[0]);
	else if(sim->dim==2) itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&pos[0],&pos[1]);
	else itct=strmathsscanf(line2,"%mlg %mlg %mlg",Varnames,Varvalues,Nvar,&pos[0],&pos[1],&pos[2]);
	SCMDCHECK(itct==sim->dim,"insufficient location dimensions");
	SCMDCHECK(addmol(sim,num,i,pos,pos,1)==0,"not enough available molecules");
	return CMDok; }


/* cmdvolumesource */
enum CMDcode cmdvolumesource(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i,d;
	char nm[STRCHAR];
	double poslo[DIMMAX],poshi[DIMMAX],numdbl;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;
	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	itct=strmathsscanf(line2,"%s %mlg",Varnames,Varvalues,Nvar,nm,&numdbl);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(numdbl>=0,"number cannot be negative");
	num=(int) numdbl;
	if(num!=numdbl) num=poisrandD(numdbl);
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"name not recognized");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"missing location");
	for(d=0;d<sim->dim;d++) {
		SCMDCHECK(line2,"missing argument");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&poslo[d],&poshi[d]);
		SCMDCHECK(itct==2,"read failure");
		line2=strnword(line2,3); }
	SCMDCHECK(addmol(sim,num,i,poslo,poshi,1)==0,"not enough available molecules");
	return CMDok; }


/* guassiansource */
enum CMDcode cmdgaussiansource(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i,d,imol,dim;
	char nm[STRCHAR];
	double mean[DIMMAX],sigma[DIMMAX],pos[DIMMAX],lowcorner[DIMMAX],highcorner[DIMMAX],numdbl;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	itct=strmathsscanf(line2,"%s %mlg",Varnames,Varvalues,Nvar,nm,&numdbl);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(numdbl>=0,"number cannot be negative");
	num=(int) numdbl;
	if(num!=numdbl) num=poisrandD(numdbl);
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"name not recognized");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"missing location");
	for(d=0;d<dim;d++) {
		SCMDCHECK(line2,"missing argument");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&mean[d],&sigma[d]);
		SCMDCHECK(itct==2,"read failure");
		line2=strnword(line2,3); }

	systemcorners(sim,lowcorner,highcorner);
	for(imol=0;imol<num;imol++) {
		for(d=0;d<dim;d++) {
			do {
				pos[d]=mean[d]+sigma[d]*gaussrandD(); }
				while(pos[d]<lowcorner[d] || pos[d]>highcorner[d]); }
		SCMDCHECK(addmol(sim,1,i,pos,pos,0)==0,"not enough available molecules"); }
	return CMDok; }


/* cmdmovesurfacemol */
enum CMDcode cmdmovesurfacemol(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,s1,s2,d,*index;
	char nm[STRCHAR],nm2[STRCHAR];
	enum MolecState ms1;
	moleculeptr mptr;
	double pos[DIMMAX];
	static panelptr pnl2;
	static enum MolecState ms2;
	static surfaceptr srf,srf2;
	static double prob;
	static enum PanelShape ps1,ps2;
	static int inscan=0,p1,p2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing arguments");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(sim->srfss,"surfaces are undefined");
	itct=strmathsscanf(line2,"%s %mlg",Varnames,Varvalues,Nvar,nm,&prob);
	SCMDCHECK(itct==2,"failed to read molecule name or probability");

	i=molstring2index1(sim,line2,&ms1,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(ms1==MSfront || ms1==MSback || ms1==MSup || ms1==MSdown || ms1==MSall,"illegal molecule state");
	SCMDCHECK(prob>=0 && prob<=1,"probability out of bounds");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"missing originating surface:panel");
	itct=sscanf(line2,"%s %s",nm,nm2);
	SCMDCHECK(itct==2,"failed to read surfaces and panels");
	s1=readsurfacename(sim,nm,&ps1,&p1);
	SCMDCHECK(s1>=0,"failed to read surface1");
	SCMDCHECK(p1>=0 || p1==-5,"failed to read panel1");
	s2=readsurfacename(sim,nm2,&ps2,&p2);
	SCMDCHECK(s2>=0,"failed to read surface2");
	SCMDCHECK(p2>=0 || p2==-5,"failed to read panel2");
	line2=strnword(line2,3);
	if(line2) {
		itct=sscanf(line2,"%s",nm);
		SCMDCHECK(itct==1,"failed to read final state");
		ms2=molstring2ms(nm);
		SCMDCHECK(ms2!=MSnone,"failed to read final state");
		line2=strnword(line2,2); }
	else ms2=MSnone;

	srf=sim->srfss->srflist[s1];
	srf2=sim->srfss->srflist[s2];
	if(p2==-5) pnl2=NULL;
	else pnl2=srf2->panels[ps2][p2];

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms1,cmd,cmdmovesurfacemol);
		inscan=0; }

	sim->mols->touch++;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(mptr->pnl && mptr->pnl->srf==srf && (p1==-5 || mptr->pnl==srf->panels[ps1][p1]))
		if(randCOD()<prob) {
			if(p2==-5) pnl2=surfrandpos(srf2,pos,sim->dim);
			else panelrandpos(pnl2,pos,sim->dim);
			for(d=0;d<sim->dim;d++) {
				mptr->posoffset[d]=mptr->pos[d]-pos[d];
				mptr->posx[d]=mptr->pos[d]=pos[d]; }
			molchangeident(sim,mptr,-1,-1,mptr->ident,ms2==MSnone?mptr->mstate:ms2,pnl2); }
	return CMDok; }


/* cmdkillmol */
enum CMDcode cmdkillmol(simptr sim,cmdptr cmd,char *line2) {
	int i,*index;
	moleculeptr mptr;
	enum MolecState ms;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdkillmol);
		inscan=0; }
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	molkill(sim,mptr,mptr->list,-1);
	return CMDok; }


/* cmdkillmolprob */
enum CMDcode cmdkillmolprob(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,*index;
	moleculeptr mptr;
	enum MolecState ms;
	static double prob;
	static char probstr[STRCHAR];
	static int xyzvar;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing probability value");
	if(strhasname(line2,"x") || strhasname(line2,"y") || strhasname(line2,"z")) {
		xyzvar=1;
		strcpy(probstr,line2); }
	else {
		xyzvar=0;
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&prob);
		SCMDCHECK(itct==1,"killmolprob format: name[(state)] probability");
		SCMDCHECK(prob>=0 && prob<=1,"probability needs to be between 0 and 1"); }

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdkillmolprob);
		inscan=0; }

	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(xyzvar) {
		simsetvariable(sim,"x",mptr->pos[0]);
		if(sim->dim>1) simsetvariable(sim,"y",mptr->pos[1]);
		if(sim->dim>2) simsetvariable(sim,"z",mptr->pos[2]);
		strmathsscanf(probstr,"%mlg",Varnames,Varvalues,Nvar,&prob); }
	if(coinrandD(prob)) molkill(sim,mptr,mptr->list,-1);
	return CMDok; }


/* cmdkillmolinsphere */
enum CMDcode cmdkillmolinsphere(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,*index;
	char nm[STRCHAR];
	moleculeptr mptr;
	static enum MolecState ms;
	static int inscan=0,s;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	if(!sim->srfss) return CMDok;
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing surface name");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"cannot read surface name");
	if(!strcmp(nm,"all")) s=-1;
	else {
		s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
		SCMDCHECK(s>=0,"surface not recognized"); }

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdkillmolinsphere);
		inscan=0; }
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(molinpanels(sim,mptr,s,PSsph))
		molkill(sim,mptr,mptr->list,-1);
	return CMDok; }


/* cmdkillmolincmpt */
enum CMDcode cmdkillmolincmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,c,*index;
	char cname[STRCHAR];
	moleculeptr mptr;
	enum MolecState ms;
	compartssptr cmptss;
	static compartptr cmpt;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(line2,"missing argument");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(line2=strnword(line2,2),"missing value argument");
	itct=sscanf(line2,"%s",cname);
	SCMDCHECK(itct==1,"cannot read compartment name");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdkillmolincmpt);
		inscan=0; }
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0))
		molkill(sim,mptr,mptr->list,-1);
	return CMDok; }


/* cmdkillmoloutsidesystem */
enum CMDcode cmdkillmoloutsidesystem(simptr sim,cmdptr cmd,char *line2) {
	int i,*index;
	moleculeptr mptr;
	enum MolecState ms;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	if(!sim->srfss) return CMDok;
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdkillmoloutsidesystem);
		inscan=0; }
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(!posinsystem(sim,mptr->pos)) molkill(sim,mptr,mptr->list,-1);
	return CMDok; }


/* cmdfixmolcount */
enum CMDcode cmdfixmolcount(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i,ll,m,ct,numl;
	char nm[STRCHAR];
	double pos1[DIMMAX],pos2[DIMMAX];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	itct=sscanf(line2,"%s %i",nm,&num);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(num>=0,"number cannot be negative");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"name not recognized");

	ll=sim->mols->listlookup[i][MSsoln];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++)
		if(sim->mols->live[ll][m]->ident==i) ct++;

	if(ct==num);
	else if(ct<num) {
		systemcorners(sim,pos1,pos2);
		SCMDCHECK(addmol(sim,num-ct,i,pos1,pos2,1)==0,"not enough available molecules"); }
	else {
		num=ct-num;
		for(;num>0;num--) {
			m=intrand(numl);
			while(sim->mols->live[ll][m]->ident!=i) m=(m==numl-1)?0:m+1;
			molkill(sim,sim->mols->live[ll][m],ll,m); }}

	return CMDok; }


/* cmdfixmolcountrange */
enum CMDcode cmdfixmolcountrange(simptr sim,cmdptr cmd,char *line2) {
	int itct,lownum,highnum,i,ll,m,ct,numl;
	char nm[STRCHAR];
	double pos1[DIMMAX],pos2[DIMMAX];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	itct=strmathsscanf(line2,"%s %mi %mi",Varnames,Varvalues,Nvar,nm,&lownum,&highnum);
	SCMDCHECK(itct==3,"read failure");
	SCMDCHECK(lownum>=0 && highnum>=0 && highnum>=lownum,"molecule numbers are out of bounds");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"species name not recognized");

	ll=sim->mols->listlookup[i][MSsoln];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++)
		if(sim->mols->live[ll][m]->ident==i) ct++;

	if(ct>=lownum && ct<=highnum);
	else if(ct<lownum) {
		systemcorners(sim,pos1,pos2);
		SCMDCHECK(addmol(sim,lownum-ct,i,pos1,pos2,1)==0,"not enough available molecules"); }
	else {
		highnum=ct-highnum;
		for(;highnum>0;highnum--) {
			m=intrand(numl);
			while(sim->mols->live[ll][m]->ident!=i) m=(m==numl-1)?0:m+1;
			molkill(sim,sim->mols->live[ll][m],ll,m); }}

	return CMDok; }


/* cmdfixmolcountonsurf */
enum CMDcode cmdfixmolcountonsurf(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i,ll,m,ct,numl,s,*index;
	char nm[STRCHAR];
	enum MolecState ms;
	surfaceptr sptr;
	moleculeptr mptr;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;
	SCMDCHECK(line2,"missing argument");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(i>0,"molecule name needs to be for a single species");
	SCMDCHECK(ms!=MSsoln && ms!=MSbsoln,"molecule state needs to be surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"fixmolcountonsurf format: species(state) number surface");
	itct=strmathsscanf(line2,"%mi %s",Varnames,Varvalues,Nvar,&num,nm);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(num>=0,"number cannot be negative");
	SCMDCHECK(sim->srfss,"no surfaces defined");
	s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
	SCMDCHECK(s>=0,"surface not recognized");
	sptr=sim->srfss->srflist[s];

	ll=sim->mols->listlookup[i][ms];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++) {
		mptr=sim->mols->live[ll][m];
		if(mptr->ident==i && mptr->mstate==ms && mptr->pnl->srf==sptr) ct++; }

	if(ct==num);
	else if(ct<num) {
		SCMDCHECK(addsurfmol(sim,num-ct,i,ms,NULL,NULL,s,PSall,NULL)==0,"not enough available molecules"); }
	else {
		num=ct-num;
		for(;num>0;num--) {
			m=intrand(numl);
			mptr=sim->mols->live[ll][m];
			while(!(mptr->ident==i && mptr->mstate==ms && mptr->pnl->srf==sptr)) {
				m=(m==numl-1)?0:m+1;
				mptr=sim->mols->live[ll][m]; }
			molkill(sim,mptr,ll,m); }}

	return CMDok; }


/* cmdfixmolcountrangeonsurf */
enum CMDcode cmdfixmolcountrangeonsurf(simptr sim,cmdptr cmd,char *line2) {
	int itct,lownum,highnum,i,ll,m,ct,numl,s,*index;
	char nm[STRCHAR];
	enum MolecState ms;
	surfaceptr sptr;
	moleculeptr mptr;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(i>0,"molecule name needs to be for a single species");
	SCMDCHECK(ms!=MSsoln && ms!=MSbsoln,"molecule state needs to be surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"fixmolcountrangeonsurf format: species(state) low_number high_number surface");
	itct=strmathsscanf(line2,"%mi %mi %s",Varnames,Varvalues,Nvar,&lownum,&highnum,nm);
	SCMDCHECK(itct==3,"read failure");
	SCMDCHECK(lownum>=0 && highnum>=0 && highnum>=lownum,"molecule numbers are out of bounds");
	SCMDCHECK(sim->srfss,"no surfaces defined");
	s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
	SCMDCHECK(s>=0,"surface not recognized");
	sptr=sim->srfss->srflist[s];

	ll=sim->mols->listlookup[i][ms];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++) {
		mptr=sim->mols->live[ll][m];
		if(mptr->ident==i && mptr->mstate==ms && mptr->pnl->srf==sptr) ct++; }

	if(ct>=lownum && ct<=highnum);
	else if(ct<lownum) {
		SCMDCHECK(addsurfmol(sim,lownum-ct,i,ms,NULL,NULL,s,PSall,NULL)==0,"not enough available molecules"); }
	else {
		highnum=ct-highnum;
		for(;highnum>0;highnum--) {
			m=intrand(numl);
			mptr=sim->mols->live[ll][m];
			while(!(mptr->ident==i && mptr->mstate==ms && mptr->pnl->srf==sptr)) {
				m=(m==numl-1)?0:m+1;
				mptr=sim->mols->live[ll][m]; }
			molkill(sim,mptr,ll,m); }}

	return CMDok; }


/* cmdfixmolcountincmpt */
enum CMDcode cmdfixmolcountincmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,num,i,ll,m,ct,numl,c;
	char nm[STRCHAR];
	moleculeptr mptr;
	compartptr cmpt;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(sim->cmptss,"compartments are undefined");
	itct=strmathsscanf(line2,"%s %mi",Varnames,Varvalues,Nvar,nm,&num);
	SCMDCHECK(itct==2,"read failure");
	SCMDCHECK(num>=0,"number cannot be negative");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"molecule name not recognized");
	line2=strnword(line2,3);
	SCMDCHECK(line2,"compartment name missing");
	itct=sscanf(line2,"%s",nm);
	c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment not recognized");
	cmpt=sim->cmptss->cmptlist[c];

	ll=sim->mols->listlookup[i][MSsoln];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++) {
		mptr=sim->mols->live[ll][m];
		if(mptr->ident==i && mptr->mstate==MSsoln && posincompart(sim,mptr->pos,cmpt,0)) ct++; }

	if(ct==num);
	else if(ct<num) {
		SCMDCHECK(addcompartmol(sim,num-ct,i,cmpt)==0,"not enough available molecules"); }
	else {
		num=ct-num;
		for(;num>0;num--) {
			m=intrand(numl);
			mptr=sim->mols->live[ll][m];
			while(!(mptr->ident==i && mptr->mstate==MSsoln && posincompart(sim,mptr->pos,cmpt,0))) {
				m=(m==numl-1)?0:m+1;
				mptr=sim->mols->live[ll][m]; }
			molkill(sim,mptr,ll,m); }}

	return CMDok; }


/* cmdfixmolcountrangeincmpt */
enum CMDcode cmdfixmolcountrangeincmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,ll,m,ct,numl,c,lownum,highnum;
	char nm[STRCHAR];
	moleculeptr mptr;
	compartptr cmpt;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->mols,"molecules are undefined");
	SCMDCHECK(sim->cmptss,"compartments are undefined");
	itct=strmathsscanf(line2,"%s %mi %mi",Varnames,Varvalues,Nvar,nm,&lownum,&highnum);
	SCMDCHECK(itct==3,"read failure");
	i=stringfind(sim->mols->spname,sim->mols->nspecies,nm);
	SCMDCHECK(i>=1,"molecule name not recognized");
	line2=strnword(line2,4);
	SCMDCHECK(line2,"compartment name missing");
	itct=sscanf(line2,"%s",nm);
	c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment not recognized");
	cmpt=sim->cmptss->cmptlist[c];

	ll=sim->mols->listlookup[i][MSsoln];
	numl=sim->mols->nl[ll];
	ct=0;
	for(m=0;m<numl;m++) {
		mptr=sim->mols->live[ll][m];
		if(mptr->ident==i && mptr->mstate==MSsoln && posincompart(sim,mptr->pos,cmpt,0)) ct++; }

	if(ct>=lownum && ct<=highnum);
	else if(ct<lownum) {
		SCMDCHECK(addcompartmol(sim,lownum-ct,i,cmpt)==0,"not enough available molecules"); }
	else {
		highnum=ct-highnum;
		for(;highnum>0;highnum--) {
			m=intrand(numl);
			mptr=sim->mols->live[ll][m];
			while(!(mptr->ident==i && mptr->mstate==MSsoln && posincompart(sim,mptr->pos,cmpt,0))) {
				m=(m==numl-1)?0:m+1;
				mptr=sim->mols->live[ll][m]; }
			molkill(sim,mptr,ll,m); }}

	return CMDok; }


/* cmdequilmol */
enum CMDcode cmdequilmol(simptr sim,cmdptr cmd,char *line2) {
	int itct,*index;
	moleculeptr mptr;
	static enum MolecState ms1,ms2;
	static double prob;
	static int xyzvar;
	static char probstr[STRCHAR];
	static int inscan=0,i1,i2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	SCMDCHECK(i1>0,"molecule name has to be for a single species");
	SCMDCHECK(ms1!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	i2=molstring2index1(sim,line2,&ms2,&index);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	SCMDCHECK(i2>0,"molecule name has to be for a single species");
	SCMDCHECK(ms2!=MSall,"molecule state cannot be 'all'");
	SCMDCHECK((ms1==MSsoln && ms2==MSsoln) || (ms1!=MSsoln && ms2!=MSsoln),"cannot equilibrate between solution and surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing probability argument");
	if(strhasname(line2,"x") || strhasname(line2,"y") || strhasname(line2,"z")) {
		xyzvar=1;
		strcpy(probstr,line2); }
	else {
		xyzvar=0;
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&prob);
		SCMDCHECK(itct==1,"failed to read probability");
		SCMDCHECK(prob>=0 && prob<=1,"probability is out of bounds"); }

	inscan=1;
	molscancmd(sim,-1,index,MSall,cmd,cmdequilmol);
	inscan=0;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if((mptr->ident==i1 && mptr->mstate==ms1) || (mptr->ident==i2 && mptr->mstate==ms2)) {
		if(xyzvar) {
			simsetvariable(sim,"x",mptr->pos[0]);
			if(sim->dim>1) simsetvariable(sim,"y",mptr->pos[1]);
			if(sim->dim>2) simsetvariable(sim,"z",mptr->pos[2]);
			strmathsscanf(probstr,"%mlg",Varnames,Varvalues,Nvar,&prob); }
		if(coinrandD(prob))
			molchangeident(sim,mptr,-1,-1,i2,ms2,mptr->pnl);
		else
			molchangeident(sim,mptr,-1,-1,i1,ms1,mptr->pnl); }
	return CMDok; }


/* cmdreplacemol */
enum CMDcode cmdreplacemol(simptr sim,cmdptr cmd,char *line2) {
	int itct,i1,*index1,*index2;
	enum MolecState ms1;
	moleculeptr mptr;
	static enum MolecState ms2;
	static double prob;
	static char probstr[STRCHAR];
	static int xyzvar;
	static int inscan=0,i2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index1);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	SCMDCHECK(ms1!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing second species name");
	i2=molstring2index1(sim,line2,&ms2,&index2);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	SCMDCHECK(i2>0,"molecule name has to be for a single species");
	SCMDCHECK(ms2!=MSall,"molecule state cannot be 'all'");
	SCMDCHECK((ms1==MSsoln && ms2==MSsoln) || (ms1!=MSsoln && ms2!=MSsoln),"cannot equilibrate between solution and surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing probability information");
	itct=sscanf(line2,"%s",probstr);
	SCMDCHECK(itct==1,"missing probability information");
	if(strhasname(probstr,"x") || strhasname(probstr,"y") || strhasname(probstr,"z"))
		xyzvar=1;
	else {
		xyzvar=0;
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&prob);
		SCMDCHECK(itct==1,"cannot read fraction");
		SCMDCHECK(prob>=0 && prob<=1,"fraction out of bounds"); }

	inscan=1;
	molscancmd(sim,i1,index1,ms1,cmd,cmdreplacemol);
	inscan=0;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(xyzvar) {
		simsetvariable(sim,"x",mptr->pos[0]);
		if(sim->dim>1) simsetvariable(sim,"y",mptr->pos[1]);
		if(sim->dim>2) simsetvariable(sim,"z",mptr->pos[2]);
		strmathsscanf(probstr,"%mlg",Varnames,Varvalues,Nvar,&prob); }
	if(coinrandD(prob))
		molchangeident(sim,mptr,-1,-1,i2,ms2,mptr->pnl);
	return CMDok; }


/* cmdreplacexyzmol */
enum CMDcode cmdreplacexyzmol(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,m,d,ll,*index;
	moleculeptr *mlist;
	boxptr bptr;
	double pos[DIMMAX];
	enum MolecState ms;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(i>0,"molecule name has to be for a single species");
	SCMDCHECK(ms!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing position information");
	if(sim->dim==1) itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&pos[0]);
	else if(sim->dim==2) itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&pos[0],&pos[1]);
	else itct=strmathsscanf(line2,"%mlg %mlg %mlg",Varnames,Varvalues,Nvar,&pos[0],&pos[1],&pos[2]);
	SCMDCHECK(itct==sim->dim,"insufficient dimensions entered");
	bptr=pos2box(sim,pos);
	ll=sim->mols->listlookup[i][ms];
	mlist=bptr->mol[ll];
	for(m=0;m<bptr->nmol[ll];m++) {
		for(d=0;d<sim->dim;d++)
			if(mlist[m]->pos[d]!=pos[d]) d=sim->dim+1;
		if(d==sim->dim) {
			molchangeident(sim,mlist[m],ll,-1,i,ms,mlist[m]->pnl);
			m=bptr->nmol[ll]+1; }}
	return CMDok; }


/* cmdreplacevolmol */
enum CMDcode cmdreplacevolmol(simptr sim,cmdptr cmd,char *line2) {
	int m,itct,dim,d,b,b1,b2,i1,i2,ll,*index;
	double *pos,poslo[DIMMAX],poshi[DIMMAX],frac;
	boxptr bptr1,bptr2,bptr;
	boxssptr boxs;
	moleculeptr *mlist,mptr;
	enum MolecState ms1,ms2;
	char probstr[STRCHAR];
	int xyzvar;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	SCMDCHECK(i1>0,"molecule name has to be for a single species");
	SCMDCHECK(ms1!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	i2=molstring2index1(sim,line2,&ms2,&index);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	SCMDCHECK(i2>0,"molecule name has to be for a single species");
	SCMDCHECK(ms2!=MSall,"molecule state cannot be 'all'");
	SCMDCHECK((ms1==MSsoln && ms2==MSsoln) || (ms1!=MSsoln && ms2!=MSsoln),"cannot equilibrate between solution and surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing fraction information");
	itct=sscanf(line2,"%s",probstr);
	SCMDCHECK(itct==1,"cannot read fraction");
	if(strhasname(probstr,"x") || strhasname(probstr,"y") || strhasname(probstr,"z"))
		xyzvar=1;
	else {
		xyzvar=0;
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&frac);
		SCMDCHECK(itct==1,"cannot read fraction");
		SCMDCHECK(frac>=0 && frac<=1,"fraction out of bounds"); }
	line2=strnword(line2,2);
	dim=sim->dim;
	boxs=sim->boxs;
	for(d=0;d<dim;d++) {
		SCMDCHECK(line2,"missing argument");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&poslo[d],&poshi[d]);
		SCMDCHECK(itct==2,"read failure");
		line2=strnword(line2,3); }

	bptr1=pos2box(sim,poslo);
	bptr2=pos2box(sim,poshi);
	b1=indx2addZV(bptr1->indx,boxs->side,dim);
	b2=indx2addZV(bptr2->indx,boxs->side,dim);
	ll=sim->mols->listlookup[i1][ms1];
	for(b=b1;b<=b2;b=nextaddZV(b,bptr1->indx,bptr2->indx,boxs->side,dim)) {
		bptr=boxs->blist[b];
		mlist=bptr->mol[ll];
		for(m=0;m<bptr->nmol[ll];m++) {
			mptr=mlist[m];
			if(mptr->ident==i1 && mptr->mstate==ms1) {
				pos=mptr->pos;
				for(d=0;d<dim;d++) if(pos[d]<poslo[d] || pos[d]>poshi[d]) d=dim+1;
				if(d==dim) {
					if(xyzvar) {
						simsetvariable(sim,"x",mptr->pos[0]);
						if(sim->dim>1) simsetvariable(sim,"y",mptr->pos[1]);
						if(sim->dim>2) simsetvariable(sim,"z",mptr->pos[2]);
						strmathsscanf(probstr,"%mlg",Varnames,Varvalues,Nvar,&frac); }
				 	if(coinrandD(frac)) {
						molchangeident(sim,mptr,ll,-1,i2,ms2,mptr->pnl); }}}}}
	return CMDok; }


/* cmdreplacecmptmol */
enum CMDcode cmdreplacecmptmol(simptr sim,cmdptr cmd,char *line2) {
	int itct,i1,c,*index1,*index2;
	char nm[STRCHAR];
	enum MolecState ms1;
	moleculeptr mptr;
	static enum MolecState ms2;
	static compartptr cmpt;
	static double frac;
	static char probstr[STRCHAR];
	static int xyzvar;
	static int inscan=0,i2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index1);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	SCMDCHECK(ms1!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing second species name");
	i2=molstring2index1(sim,line2,&ms2,&index2);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	SCMDCHECK(i2>0,"molecule name has to be for a single species");
	SCMDCHECK(ms2!=MSall,"molecule state cannot be 'all'");
	SCMDCHECK((ms1==MSsoln && ms2==MSsoln) || (ms1!=MSsoln && ms2!=MSsoln),"cannot equilibrate between solution and surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing fraction information");
	itct=sscanf(line2,"%s",probstr);
	SCMDCHECK(itct==1,"missing fraction information");
	if(strhasname(probstr,"x") || strhasname(probstr,"y") || strhasname(probstr,"z"))
		xyzvar=1;
	else {
		xyzvar=0;
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&frac);
		SCMDCHECK(itct==1,"cannot read fraction");
		SCMDCHECK(frac>=0 && frac<=1,"fraction out of bounds"); }
	line2=strnword(line2,2);
	SCMDCHECK(line2,"compartment name missing");
	itct=sscanf(line2,"%s",nm);
	c=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
	SCMDCHECK(c>=0,"compartment not recognized");
	cmpt=sim->cmptss->cmptlist[c];

	inscan=1;
	molscancmd(sim,i1,index1,ms1,cmd,cmdreplacecmptmol);
	inscan=0;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(posincompart(sim,mptr->pos,cmpt,0)) {
		if(xyzvar) {
			simsetvariable(sim,"x",mptr->pos[0]);
			if(sim->dim>1) simsetvariable(sim,"y",mptr->pos[1]);
			if(sim->dim>2) simsetvariable(sim,"z",mptr->pos[2]);
			strmathsscanf(probstr,"%mlg",Varnames,Varvalues,Nvar,&frac); }
	 	if(coinrandD(frac))
			molchangeident(sim,mptr,-1,-1,i2,ms2,mptr->pnl); }
	return CMDok; }


/* cmdmodulatemol */
enum CMDcode cmdmodulatemol(simptr sim,cmdptr cmd,char *line2) {
	int itct,*index;
	moleculeptr mptr;
	double freq,shift;
	static double prob;
	static enum MolecState ms1,ms2;
	static int inscan=0,i1,i2;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	SCMDCHECK(i1>0,"molecule name has to be for a single species");
	SCMDCHECK(ms1!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	i2=molstring2index1(sim,line2,&ms2,&index);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	SCMDCHECK(i2>0,"molecule name has to be for a single species");
	SCMDCHECK(ms2!=MSall,"molecule state cannot be 'all'");
	SCMDCHECK((ms1==MSsoln && ms2==MSsoln) || (ms1!=MSsoln && ms2!=MSsoln),"cannot equilibrate between solution and surface-bound");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"missing frequency and shift");
	itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&freq,&shift);
	SCMDCHECK(itct==2,"failure reading frequency or shift");

	inscan=1;
	molscancmd(sim,-1,index,MSall,cmd,cmdmodulatemol);
	inscan=0;

	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if((mptr->ident==i1 && mptr->mstate==ms1) || (mptr->ident==i2 && mptr->mstate==ms2)) {
		if(coinrandD(prob))
			molchangeident(sim,mptr,-1,-1,i2,ms2,mptr->pnl);
		else
			molchangeident(sim,mptr,-1,-1,i1,ms1,mptr->pnl); }
	return CMDok; }


/* cmdreact1 */
enum CMDcode cmdreact1(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,r,*index;
	char rnm[STRCHAR];
	moleculeptr mptr;
	enum MolecState ms;
	static rxnptr rxn;
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i=molstring2index1(sim,line2,&ms,&index);
	SCMDCHECK(i!=-1,"species is missing or cannot be read");
	SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i!=-3,"cannot read molecule state value");
	SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
	SCMDCHECK(i!=-7,"error allocating memory");
	SCMDCHECK(ms!=MSall,"molecule state cannot be 'all'");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"reaction name is missing");
	itct=sscanf(line2,"%s",rnm);
	SCMDCHECK(itct==1,"cannot read reaction name");
	SCMDCHECK(sim->rxnss[1],"no first order reactions defined");
	r=stringfind(sim->rxnss[1]->rname,sim->rxnss[1]->totrxn,rnm);
	SCMDCHECK(r>=0,"reaction not recognized");
	rxn=sim->rxnss[1]->rxn[r];

	if(i!=-4) {
		inscan=1;
		molscancmd(sim,i,index,ms,cmd,cmdreact1);
		inscan=0; }
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	doreact(sim,rxn,mptr,NULL,-1,-1,-1,-1,NULL,NULL);
	return CMDok; }


/* cmdsetrateint */
enum CMDcode cmdsetrateint(simptr sim,cmdptr cmd,char *line2) {
	int itct,r,order;
	char rnm[STRCHAR];
	double rateint;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	itct=strmathsscanf(line2,"%s %mlg",Varnames,Varvalues,Nvar,rnm,&rateint);
	SCMDCHECK(itct==2,"read failure");
	r=-1;
	if(sim->rxnss[0]) r=stringfind(sim->rxnss[0]->rname,sim->rxnss[0]->totrxn,rnm);
	if(r>=0) order=0;
	else {
		if(sim->rxnss[1]) r=stringfind(sim->rxnss[1]->rname,sim->rxnss[1]->totrxn,rnm);
		if(r>=0) order=1;
		else {
			if(sim->rxnss[2]) r=stringfind(sim->rxnss[2]->rname,sim->rxnss[2]->totrxn,rnm);
			if(r>=0) order=2;
			else SCMDCHECK(0,"reaction name not recognized"); }}
	SCMDCHECK(rateint>=0,"internal rate cannot be negative");
	if(order<2) RxnSetValue(sim,"prob",sim->rxnss[order]->rxn[r],rateint);
	else RxnSetValue(sim,"bindrad",sim->rxnss[order]->rxn[r],rateint);
	return CMDok; }


/* cmdshufflemollist */
enum CMDcode cmdshufflemollist(simptr sim,cmdptr cmd,char *line2) {
	int itct,ll,lllo,llhi;
	char nm[STRCHAR];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s",nm);
	SCMDCHECK(itct==1,"read failure");
	SCMDCHECK(sim->mols && sim->mols->nlist>0,"no molecule lists");
	if(!strcmp(nm,"all")) ll=-1;
	else {
		ll=stringfind(sim->mols->listname,sim->mols->nlist,nm);
		SCMDCHECK(ll>=0,"list name not recognized"); }
	lllo=(ll==-1)?0:ll;
	llhi=(ll==-1)?sim->mols->nlist:ll+1;
	for(ll=lllo;ll<llhi;ll++)
		randshuffletableV((void**) sim->mols->live[ll],sim->mols->nl[ll]);
	return CMDok; }


/* cmdshufflereactions */
enum CMDcode cmdshufflereactions(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,j,i1,i2,isym,j1,j2,*index1,*index2;
	char nm1[STRCHAR],nm2[STRCHAR];
	enum MolecState ms1,ms2;
	rxnssptr rxnss;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	itct=sscanf(line2,"%s %s",nm1,nm2);
	SCMDCHECK(itct==2,"missing argument");
	i1=molstring2index1(sim,nm1,&ms1,&index1);
	SCMDCHECK(i1>=0 || i1==-5,"first species name not recognized");
	i2=molstring2index1(sim,nm2,&ms2,&index2);
	SCMDCHECK(i2>=0 || i2==-5,"second species name not recognized");
	rxnss=sim->rxnss[2];
	if(!rxnss) return CMDok;

	for(j1=0;j1<index1[PDnresults];j1++)
		for(j2=0;j2<index2[PDnresults];j2++) {
			i1=index1[PDMAX+j1];
			i2=index2[PDMAX+j2];
			i=i1*rxnss->maxspecies+i2;
			if(rxnss->nrxn[i]) {
				randshuffletableI(rxnss->table[i],rxnss->nrxn[i]);
				isym=i2*rxnss->maxspecies+i1;
				for(j=0;j<rxnss->nrxn[i];j++)
					rxnss->table[isym][j]=rxnss->table[i][j]; }}

	return CMDok; }


/* cmdsettimestep */
enum CMDcode cmdsettimestep(simptr sim,cmdptr cmd,char *line2) {
	int itct,er;
	double dt;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&dt);
	SCMDCHECK(itct==1,"read failure");
	SCMDCHECK(dt>0,"time step must be >0");

	er=simsettime(sim,dt,3);
	SCMDCHECK(er==0,"error while setting the simulation time step");
	return CMDok; }


/* cmdporttransport */
enum CMDcode cmdporttransport(simptr sim,cmdptr cmd,char *line2) {
	int itct,prt1,prt2;
	char nm1[STRCHAR],nm2[STRCHAR];
	portptr port1,port2;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	SCMDCHECK(sim->portss,"no port superstructure is defined");
	itct=sscanf(line2,"%s %s",nm1,nm2);
	SCMDCHECK(itct==2,"porttransport format: port1 port2");
	prt1=stringfind(sim->portss->portnames,sim->portss->nport,nm1);
	SCMDCHECK(prt1>=0,"name of port1 is not recognized");
	prt2=stringfind(sim->portss->portnames,sim->portss->nport,nm2);
	SCMDCHECK(prt2>=0,"name of port2 is not recognized");
	port1=sim->portss->portlist[prt1];
	port2=sim->portss->portlist[prt2];
	porttransport(sim,port1,sim,port2);
	return CMDok; }


/* cmdexcludebox */
enum CMDcode cmdexcludebox(simptr sim,cmdptr cmd,char *line2) {
	int m,itct,dim,d,b,b1,b2;
	double *pos,poslo[DIMMAX],poshi[DIMMAX];
	boxptr bptr1,bptr2,bptr;
	boxssptr boxs;
	moleculeptr *mlist;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	boxs=sim->boxs;
	for(d=0;d<dim;d++) {
		SCMDCHECK(line2,"missing argument");
		itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&poslo[d],&poshi[d]);
		SCMDCHECK(itct==2,"read failure");
		line2=strnword(line2,3); }

	bptr1=pos2box(sim,poslo);
	bptr2=pos2box(sim,poshi);
	b1=indx2addZV(bptr1->indx,boxs->side,dim);
	b2=indx2addZV(bptr2->indx,boxs->side,dim);
	for(b=b1;b<=b2;b=nextaddZV(b,bptr1->indx,bptr2->indx,boxs->side,dim)) {
		bptr=boxs->blist[b];
		mlist=bptr->mol[0];
		for(m=0;m<bptr->nmol[0];m++) {
			pos=mlist[m]->pos;
			for(d=0;d<dim;d++) if(pos[d]<poslo[d] || pos[d]>poshi[d]) d=dim+1;
			if(d==dim) {
				pos=mlist[m]->posx;
				for(d=0;d<dim;d++) if(pos[d]<poslo[d] || pos[d]>poshi[d]) d=dim+1;
				if(d>dim) copyVD(mlist[m]->posx,mlist[m]->pos,dim); }}}
	sim->mols->touch++;
	return CMDok; }


/* cmdexcludesphere */
enum CMDcode cmdexcludesphere(simptr sim,cmdptr cmd,char *line2) {
	int m,itct,dim,d,b,b1,b2;
	double *pos,poslo[DIMMAX],poshi[DIMMAX],poscent[DIMMAX],rad,dist;
	boxptr bptr1,bptr2,bptr;
	boxssptr boxs;
	moleculeptr *mlist;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	boxs=sim->boxs;
	for(d=0;d<dim;d++) {
		SCMDCHECK(line2,"missing center argument");
		itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&poscent[d]);
		SCMDCHECK(itct==1,"failure reading center");
		line2=strnword(line2,2); }
	SCMDCHECK(line2,"missing radius");
	itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&rad);
	SCMDCHECK(itct==1,"failure reading radius");

	dist=rad*sqrt((double)dim);
	for(d=0;d<dim;d++) {
		poslo[d]=poscent[d]-dist;
		poshi[d]=poscent[d]+dist; }
	rad*=rad;
	bptr1=pos2box(sim,poslo);
	bptr2=pos2box(sim,poshi);
	b1=indx2addZV(bptr1->indx,boxs->side,dim);
	b2=indx2addZV(bptr2->indx,boxs->side,dim);
	for(b=b1;b<=b2;b=nextaddZV(b,bptr1->indx,bptr2->indx,boxs->side,dim)) {
		bptr=boxs->blist[b];
		mlist=bptr->mol[0];
		for(m=0;m<bptr->nmol[0];m++) {
			pos=mlist[m]->pos;
			for(dist=0,d=0;d<dim;d++) if((dist+=(pos[d]-poscent[d])*(pos[d]-poscent[d]))>rad) d=dim+1;
			if(d==dim) {
				pos=mlist[m]->posx;
				for(dist=0,d=0;d<dim;d++) if((dist+=(pos[d]-poscent[d])*(pos[d]-poscent[d]))>rad) d=dim+1;
				if(d>dim) copyVD(mlist[m]->posx,mlist[m]->pos,dim); }}}
	sim->mols->touch++;
	return CMDok; }


/* cmdincludeecoli */
enum CMDcode cmdincludeecoli(simptr sim,cmdptr cmd,char *line2) {
	moleculeptr mptr;
	wallptr *wlist;
	static double rad,length,pos[DIMMAX];
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(sim->dim==3,"system is not 3 dimensional");
	wlist=sim->wlist;
	rad=0.5*(wlist[3]->pos-wlist[2]->pos);
	length=wlist[1]->pos-wlist[0]->pos;
	pos[0]=wlist[0]->pos;
	pos[1]=0.5*(wlist[2]->pos+wlist[3]->pos);
	pos[2]=0.5*(wlist[4]->pos+wlist[5]->pos);

	inscan=1;
	molscancmd(sim,-1,NULL,MSsoln,cmd,cmdincludeecoli);
	inscan=0;
	sim->mols->touch++;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	if(!insideecoli(mptr->pos,pos,rad,length)) {
		if(insideecoli(mptr->posx,pos,rad,length))
			copyVD(mptr->posx,mptr->pos,3);
		else putinecoli(mptr->pos,pos,rad,length); }
	return CMDok; }


/* cmdsetreactionratemolcount */
enum CMDcode cmdsetreactionratemolcount(simptr sim,cmdptr cmd,char *line2) {
	int itct,order,r,count,i,*index,er;
  char nm1[STRCHAR];
  rxnptr rxn;
  double rate,coeff;
  enum MolecState ms;
	listptrv vlist;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	SCMDCHECK(line2,"missing argument");
	itct=strmathsscanf(line2,"%s %mlg",Varnames,Varvalues,Nvar,nm1,&coeff);
  SCMDCHECK(itct==2,"missing reaction name or coefficient c0");
  r=readrxnname(sim,nm1,&order,&rxn,&vlist,1);
  SCMDCHECK(r>=0,"unrecognized reaction name");
  line2=strnword(line2,3);
  rate=coeff;
  while(line2) {
    itct=strmathsscanf(line2,"%mlg %s",Varnames,Varvalues,Nvar,&coeff,nm1);
    SCMDCHECK(itct==2,"missing coefficient and/or species parameters");
		i=molstring2index1(sim,line2,&ms,&index);
		SCMDCHECK(i!=-1,"species is missing or cannot be read");
		SCMDCHECK(i!=-2,"mismatched or improper parentheses around molecule state");
		SCMDCHECK(i!=-3,"cannot read molecule state value");
		SCMDCHECK(i!=-4 || sim->ruless,"molecule name not recognized");
		SCMDCHECK(i!=-7,"error allocating memory");
    line2=strnword(line2,3);
    count=(i==-4)?0:molcount(sim,i,index,ms,-1);
    rate+=coeff*count; }
  if(rate<0) rate=0;
	if(!vlist) {
		er=RxnSetValue(sim,"rate",rxn,rate);
		SCMDCHECK(er==0,"error setting reaction rate"); }
	else {
		for(i=0;i<vlist->n;i++) {
			er=RxnSetValue(sim,"rate",(rxnptr) vlist->xs[i],rate);
			SCMDCHECK(er==0,"error setting reaction rate"); }}

  return CMDok; }


/* cmdexpandsystem */
enum CMDcode cmdexpandsystem(simptr sim,cmdptr cmd,char *line2) {
	int itct,d,dim,s,p,c,k,i,em;
	double zero[DIMMAX];
	moleculeptr mptr;
	surfaceptr srf;
	enum PanelShape ps;
	enum PanelFace face;
	panelptr pnl;
	compartptr cmpt;
	static double expand[DIMMAX],center[DIMMAX];
	static int inscan=0;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	SCMDCHECK(line2,"missing arguments");
	if(dim==1) itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&expand[0]);
	else if(dim==2) itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&expand[0],&expand[1]);
	else itct=strmathsscanf(line2,"%mlg %mlg %mlg",Varnames,Varvalues,Nvar,&expand[0],&expand[1],&expand[2]);
  SCMDCHECK(itct==dim,"cannot read or wrong number of expansion values");
	systemcenter(sim,center);

	inscan=1;
	molscancmd(sim,-1,NULL,MSall,cmd,cmdexpandsystem);
	inscan=0;

	if(sim->srfss) {
		zero[0]=zero[1]=zero[2]=0;
		for(s=0;s<sim->srfss->nsrf;s++) {
			srf=sim->srfss->srflist[s];
			for(ps=(enum PanelShape)0;ps<(enum PanelShape)PSMAX;ps=(enum PanelShape)(ps+1))
				for(p=0;p<srf->npanel[ps];p++) {
					pnl=srf->panels[ps][p];
					surftransformpanel(pnl,sim->dim,zero,center,expand); }
			if(srf->nemitter[PFfront] && srf->nemitter[PFback] && sim->mols)
				for(face=(enum PanelFace)0;face<(enum PanelFace)2;face=(enum PanelFace)(face+1))
					for(i=1;i<sim->mols->nspecies;i++)
						for(em=0;em<srf->nemitter[face][i];em++)
							for(d=0;d<dim;d++)
								srf->emitterpos[face][i][em][d]=center[d]+(srf->emitterpos[face][i][em][d]-center[d])*expand[d]; }}

	if(sim->cmptss) {
		for(c=0;c<sim->cmptss->ncmpt;c++) {
			cmpt=sim->cmptss->cmptlist[c];
			for(k=0;k<cmpt->npts;k++)
				for(d=0;d<dim;d++)
					cmpt->points[k][d]=center[d]+(cmpt->points[k][d]-center[d])*expand[d]; }
		compartsetcondition(sim->cmptss,SCparams,0); }

	sim->mols->touch++;
	return CMDok;

 scanportion:
	mptr=(moleculeptr) line2;
	for(d=0;d<sim->dim;d++) {
		mptr->pos[d]=center[d]+(mptr->pos[d]-center[d])*expand[d];
		mptr->posx[d]=center[d]+(mptr->posx[d]-center[d])*expand[d]; }
	return CMDok; }


/* cmdtranslatecmpt */
enum CMDcode cmdtranslatecmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,dim,c,code;
	compartssptr cmptss;
	compartptr cmpt;
	char cname[STRCHAR];
	double translate[DIMMAX];

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(line2,"first argument should be compartment name");
	itct=sscanf(line2,"%s",cname);
	SCMDCHECK(itct==1,"cannot read compartment name");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];

	SCMDCHECK(line2=strnword(line2,2),"second argument should be code value");;
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&code);
	SCMDCHECK(itct==1,"second argument should be code value");

	SCMDCHECK(line2=strnword(line2,2),"missing arguments for translation amount");;
	if(dim==1) itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&translate[0]);
	else if(dim==2) itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&translate[0],&translate[1]);
	else itct=strmathsscanf(line2,"%mlg %mlg %mlg",Varnames,Varvalues,Nvar,&translate[0],&translate[1],&translate[2]);
  SCMDCHECK(itct==dim,"cannot read translation values or wrong number of them");

	comparttranslate(sim,cmpt,code,translate);

	return CMDok; }


/* cmddiffusecmpt */
enum CMDcode cmddiffusecmpt(simptr sim,cmdptr cmd,char *line2) {
	int itct,dim,c,code,d,valid,pt,is,nsample,attempts;
	compartssptr cmptss;
	compartptr cmpt,cmptbound;
	char cname[STRCHAR];
	double stddev[DIMMAX],translate[DIMMAX],samplept[DIMMAX],testpt[DIMMAX],radius;

	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	dim=sim->dim;
	cmptss=sim->cmptss;
	SCMDCHECK(cmptss,"no compartments defined");
	SCMDCHECK(line2,"first argument should be compartment name");
	itct=sscanf(line2,"%s",cname);
	SCMDCHECK(itct==1,"cannot read compartment name");
	c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
	SCMDCHECK(c>=0,"compartment name not recognized");
	cmpt=cmptss->cmptlist[c];

	SCMDCHECK(line2=strnword(line2,2),"second argument should be code value");;
	itct=strmathsscanf(line2,"%mi",Varnames,Varvalues,Nvar,&code);
	SCMDCHECK(itct==1,"second argument should be code value");

	SCMDCHECK(line2=strnword(line2,2),"missing arguments for standard deviations");;
	if(dim==1) itct=strmathsscanf(line2,"%mlg",Varnames,Varvalues,Nvar,&stddev[0]);
	else if(dim==2) itct=strmathsscanf(line2,"%mlg %mlg",Varnames,Varvalues,Nvar,&stddev[0],&stddev[1]);
	else itct=strmathsscanf(line2,"%mlg %mlg %mlg",Varnames,Varvalues,Nvar,&stddev[0],&stddev[1],&stddev[2]);
  SCMDCHECK(itct==dim,"cannot read standard deviation values or wrong number of them");

	line2=strnword(line2,dim+1);
	if(line2) {
		itct=strmathsscanf(line2,"%s %mlg %mi",Varnames,Varvalues,Nvar,cname,&radius,&nsample);
		SCMDCHECK(itct==3,"cannot read bounding compartment name, radius, and/or number of samples");
		c=stringfind(cmptss->cnames,cmptss->ncmpt,cname);
		SCMDCHECK(c>=0,"bounding compartment name not recognized");
		cmptbound=cmptss->cmptlist[c];
		SCMDCHECK(cmptbound!=cmpt,"bounding compartment cannot be the same as the moving compartment");
		SCMDCHECK(radius>0,"bounding radius needs to be >0");
		SCMDCHECK(nsample>0,"number of samples needs to be >0"); }
	else {
		cmptbound=NULL;
		radius=0; }

	if(!cmptbound) {
		for(d=0;d<dim;d++)
			translate[d]=stddev[d]*gaussrandD();
		valid=1; }
	else {
		valid=0;
		attempts=0;
		while(!valid && attempts<10) {
			valid=1;
			attempts++;
			for(d=0;d<dim;d++)
				translate[d]=stddev[d]*gaussrandD();
			for(pt=0;pt<cmpt->npts && valid;pt++)
				for(is=0;is<nsample && valid;is++) {
					if(dim==1) samplept[0]=(coinrandD(0.5)==0)?-radius:radius;
					else if(dim==2) circlerandD(samplept,radius);
					else sphererandCCD(samplept,radius,radius);
					for(d=0;d<dim;d++)
						testpt[d]=cmpt->points[pt][d]+samplept[d]+translate[d];
					if(!posincompart(sim,testpt,cmptbound,0)) {
						valid=0; }}}}

	if(valid)
		comparttranslate(sim,cmpt,code,translate);

	return CMDok; }


/* cmdlongrangeforce */
enum CMDcode cmdlongrangeforce(simptr sim,cmdptr cmd,char *line2) {
	int itct,i,j,j2,ll,d,dim,wrap[DIMMAX],m,duplicate;
	enum MolecState ms;
	moleculeptr mptr,mptr2;
	double dt,mobility,dist,delta[DIMMAX],force;
	boxptr bptr;

	static int inscan=0,i1,i2,*index1,*index2,rvar,lllo,llhi;
	static enum MolecState ms1,ms2,mslo,mshi;
	static double mobility1,mobility2,rmin,rmax,forcemag,syswidth[DIMMAX],force0[4]={0,0,0,0};
	static char eqstring[STRCHAR];
	static listptrULVD4 moleclist;

	if(inscan) goto scanportion;
	if(line2 && !strcmp(line2,"cmdtype")) return CMDmanipulate;

	i1=molstring2index1(sim,line2,&ms1,&index1);
	SCMDCHECK(i1!=-1,"species is missing or cannot be read");
	SCMDCHECK(i1!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i1!=-3,"cannot read molecule state value");
	SCMDCHECK(i1!=-4,"molecule name not recognized");
	SCMDCHECK(i1!=-7,"error allocating memory");
	line2=strnword(line2,2);
	i2=molstring2index1(sim,line2,&ms2,&index2);
	SCMDCHECK(i2!=-1,"species is missing or cannot be read");
	SCMDCHECK(i2!=-2,"mismatched or improper parentheses around molecule state");
	SCMDCHECK(i2!=-3,"cannot read molecule state value");
	SCMDCHECK(i2!=-4,"molecule name not recognized");
	SCMDCHECK(i2!=-7,"error allocating memory");
	line2=strnword(line2,2);
	SCMDCHECK(line2,"longrangeforce format: species1(state) species2(state) mobility1 mobility2 r_min r_max equation");
	itct=strmathsscanf(line2,"%mlg %mlg %mlg %mlg %s",Varnames,Varvalues,Nvar,&mobility1,&mobility2,&rmin,&rmax,eqstring);
	SCMDCHECK(itct==5,"longrangeforce format: species1(state) species2(state) mobility1 mobility2 r_min r_max equation");
	SCMDCHECK(rmin>0,"minimum radius needs to be >0");
	SCMDCHECK(rmax>=0,"maximum radius needs to be >=0");

	dim=sim->dim;
	if(strhasname(eqstring,"r"))
		rvar=1;
	else {
		rvar=0;
		forcemag=strmatheval(eqstring,Varnames,Varvalues,Nvar);
		SCMDCHECK(forcemag==forcemag,"cannot compute equation value"); }

	lllo=llhi=-1;
	if(ms2<MSMAX) {
		mslo=ms2;
		mshi=(enum MolecState)(ms2+1); }
	else {
		mslo=(enum MolecState) 0;
		mshi=(enum MolecState) MSMAX; }
	for(i=0;i<index2[PDnresults];i++)
		for(ms=mslo;ms<mshi;ms=(enum MolecState)(ms+1)) {
			ll=sim->mols->listlookup[index2[PDMAX+i]][ms];
			if(ll<lllo || lllo==-1) lllo=ll;
			if(ll>=llhi || llhi==-1) llhi=ll+1; }

	if(!cmd->v1) {
		cmd->v1=(void*) ListAllocULVD4(256);	// start with 256 molecules, and expand as needed
		SCMDCHECK(cmd->v1,"memory allocation error");
		cmd->freefn=&cmdListULVD4free; }

	moleclist=(listptrULVD4) cmd->v1;
	for(j=0;j<moleclist->n;j++) {
		moleclist->datad4[j][3]=0;
		for(d=0;d<dim;d++)
			moleclist->datad4[j][d]=0; }

	for(d=0;d<dim;d++)
		syswidth[d]=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;

	inscan=1;
	molscancmd(sim,i1,index1,ms1,cmd,cmdlongrangeforce);
	inscan=0;

	dt=sim->dt;
	for(i=0;i<moleclist->n;i++) {
		if(moleclist->datad4[i][3]==0)
			moleclist->datav[i]=NULL;
		else {
			mptr=(moleculeptr) moleclist->datav[i];
			mobility=molismatch(mptr,i1,index1,ms1)?mobility1:mobility2;
			for(d=0;d<dim;d++)
				mptr->pos[d]+=moleclist->datad4[i][d]*mobility*dt; }}
	List_CleanULVD4(moleclist);

	return CMDok;

 scanportion:
	dim=sim->dim;
	mptr=(moleculeptr) line2;
	j=ListInsertItemULVD4(moleclist,mptr->serno,(void*)mptr,force0,1);
	SCMDCHECK(j>=0,"failed to allocate memory");
	moleclist->datad4[j][3]=1;
	bptr=boxscansphere(sim,mptr->pos,rmax,NULL,wrap);
	while(bptr) {
		for(ll=lllo;ll<llhi;ll++)
			for(m=0;m<bptr->nmol[ll];m++) {
				mptr2=bptr->mol[ll][m];
				if(molismatch(mptr2,i2,index2,ms2) && mptr2!=mptr) {
					dist=0;
					for(d=0;d<dim;d++) {
						delta[d]=mptr2->pos[d]+wrap[d]*syswidth[d]-mptr->pos[d];
						dist+=delta[d]*delta[d]; }
					dist=sqrt(dist);
					if(dist>=rmin && dist<=rmax) {
						if(rvar) {
							simsetvariable(sim,"r",dist);
							forcemag=strmatheval(eqstring,Varnames,Varvalues,Nvar); }
						duplicate=molismatch(mptr2,i1,index1,ms1);
						if(!duplicate) {
							j2=ListInsertItemULVD4(moleclist,mptr2->serno,(void*)mptr2,force0,1);
							moleclist->datad4[j2][3]=1;
							j=ListInsertItemULVD4(moleclist,mptr->serno,NULL,NULL,0); } // in case adding j2 moved the position for j
						for(d=0;d<dim;d++) {
							force=delta[d]/dist*forcemag;
							moleclist->datad4[j][d]-=force;
							if(!duplicate)
								moleclist->datad4[j2][d]+=force; }}}}
		bptr=boxscansphere(sim,mptr->pos,rmax,bptr,wrap); }
	return CMDok;	}





/**********************************************************/
/******************* internal routines ********************/
/**********************************************************/


/* cmdv1free */
void cmdv1free(cmdptr cmd) {
	free(cmd->v1);
	cmd->v1=NULL;
	return; }


/* cmdv1v2free */
void cmdv1v2free(cmdptr cmd) {
	free(cmd->v1);
	free(cmd->v2);
	cmd->v1=NULL;
	cmd->v2=NULL;
	return; }


/* cmdListULVD3free */
void cmdListULVD4free(cmdptr cmd) {
	ListFreeULVD4((listptrULVD4) cmd->v1);
	cmd->v1=NULL;
	return; }


/* conditionalcmdtype */
enum CMDcode conditionalcmdtype(simptr sim,cmdptr cmd,int nparam) {
	char string[STRCHAR],*strptr;
	enum CMDcode ans;

	if(!cmd->str) return CMDnone;
	strptr=strnword(cmd->str,nparam+2);
	if(!strptr) return CMDnone;
	strcpy(string,strptr);
	strptr=cmd->str;
	cmd->str=string;
	ans=scmdcmdtype(sim->cmds,cmd);
	cmd->str=strptr;
	return ans; }


/* insideecoli */
int insideecoli(double *pos,double *ofst,double rad,double length) {
	double dist;

	dist=(pos[1]-ofst[1])*(pos[1]-ofst[1])+(pos[2]-ofst[2])*(pos[2]-ofst[2]);
	if(pos[0]-ofst[0]<rad) dist+=(pos[0]-ofst[0]-rad)*(pos[0]-ofst[0]-rad);
	else if(pos[0]-ofst[0]>length-rad) dist+=(pos[0]-ofst[0]-length+rad)*(pos[0]-ofst[0]-length+rad);
	return dist<rad*rad; }


/* putinecoli */
void putinecoli(double *pos,double *ofst,double rad,double length) {
	double dist;

	dist=(pos[1]-ofst[1])*(pos[1]-ofst[1])+(pos[2]-ofst[2])*(pos[2]-ofst[2]);
	if(pos[0]-ofst[0]<rad) {
		dist+=(pos[0]-ofst[0]-rad)*(pos[0]-ofst[0]-rad);
		dist=sqrt(rad*rad/dist);
		pos[0]=ofst[0]+rad+dist*(pos[0]-ofst[0]-rad); }
	else if(pos[0]-ofst[0]>length-rad) {
		dist+=(pos[0]-ofst[0]-length+rad)*(pos[0]-ofst[0]-length+rad);
		dist=sqrt(rad*rad/dist);
		pos[0]=ofst[0]+length-rad+dist*(pos[0]-ofst[0]-length+rad); }
	else
		dist=sqrt(rad*rad/dist);
	pos[1]=ofst[1]+dist*(pos[1]-ofst[1]);
	pos[2]=ofst[2]+dist*(pos[2]-ofst[2]);
	return; }


/* molinpanels */
int molinpanels(simptr sim,moleculeptr mptr,int s,enum PanelShape ps) {
	int p,dim,npnl;
	double *pos;
	panelptr *pnls,pnl;

	if(ps!=PSsph) return 0;

	if(s<0) {
		for(s=0;s<sim->srfss->nsrf;s++)
			if(molinpanels(sim,mptr,s,ps)) return 1;
		return 0; }

	dim=sim->dim;
	pnls=sim->srfss->srflist[s]->panels[ps];
	npnl=sim->srfss->srflist[s]->npanel[ps];
	pos=mptr->pos;
	for(p=0;p<npnl;p++) {
		pnl=pnls[p];
		if(Geo_PtInSphere(pos,pnl->point[0],pnl->point[1][0],dim)) return 1; }
	return 0; }

