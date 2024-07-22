/* Steven Andrews, started 10/22/2001.
 This is the entry point for the Smoldyn program.
 See documentation called SmoldynUsersManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "opengl2.h"
#include "random2.h"
#include "SimCommand.h"
#include "string2.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"
#ifdef OPTION_VCELL
	#include "SimpleValueProvider.h"
	#include "SimpleMesh.h"
#endif

/* ***************************************************************** */
/* ********************** main() segment *************************** */
/* ***************************************************************** */



/* main */
int main(int argc,char **argv) {
	int exitCode = 0;

	try {
	  simptr sim;
	  int i,er,pflag,wflag,tflag,Vflag,oflag;
	  char root[STRCHAR],fname[STRCHAR],flags[STRCHAR],*cptr,logfile[STRCHAR];

		for(i=0;i<STRCHAR;i++) root[i]=fname[i]=flags[i]=logfile[i]='\0';
		er=0;
		if(argc<=1) {																		// only software name
			fprintf(stderr,"Welcome to Smoldyn version %s.\n\n",VERSION);
			fprintf(stderr,"Enter name of configuration file: ");
			char* _x=fgets(root,STRCHAR,stdin);
			if(strchr(root,'\n')) *(strchr(root,'\n'))='\0';
			fprintf(stderr,"Enter runtime flags (q=quiet, p=parameters only, etc.), or '-'=none: ");
			_x=fgets(flags,STRCHAR,stdin);
            UNUSED(_x);
			if(strchr(flags,'\n')) *(strchr(flags,'\n'))='\0'; }

		if(argc>1) {																		// also have filename or --version
			if(!strcmp(argv[1],"--version")) strncat(flags,"V",STRCHAR);
			else {
				strncpy(root,argv[1],STRCHAR-1);
				root[STRCHAR-1]='\0'; }
			argc--;
			argv++; }

		er=Parse_CmdLineArg(&argc,argv,NULL);						// process any --define arguments
		if(er) {
			if(er==1) fprintf(stderr,"Out of memory");
			else fprintf(stderr,"Follow command line '--define' options with key=replacement\n");
			return 0; }

		while(argc>1) {																	// process flags with -- or with -
			if(!strcmp(argv[1],"--version")) strncat(flags,"V",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--output")) strncat(flags,"o",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--parameters")) strncat(flags,"p",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--params")) strncat(flags,"p",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--quiet")) strncat(flags,"q",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--silent")) strncat(flags,"s",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--text")) strncat(flags,"t",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--verbose")) strncat(flags,"v",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--warnings")) strncat(flags,"w",STRCHAR-strlen(flags));
			else if(!strcmp(argv[1],"--logfile")) {
				if(argc>2) {
					strncpy(logfile,argv[2],STRCHAR-1);
					argc--;
					argv++; }
				else {
					fprintf(stderr,"Missing logfile name\n");
					return 0; }}
			else if(argv[1][0]=='-' && argv[1][1]!='-') strncat(flags,argv[1],STRCHAR-strlen(flags));
			else {
				fprintf(stderr,"Command line format: smoldyn [config_file] [-options]\n");
				return 0; }
			argc--;
			argv++; }
		strcpy(SimFlags,flags);

		cptr=strrpbrk(root,":\\/");
		if(cptr) cptr++;
		else cptr=root;
		strcpy(fname,cptr);
		*cptr='\0';

		oflag=strchr(flags,'o')?1:0;
		pflag=strchr(flags,'p')?1:0;
		Vflag=strchr(flags,'V')?1:0;
		if(!strcmp(fname,"-V")) Vflag=1;
		wflag=strchr(flags,'w')?1:0;
		tflag=strchr(flags,'t')?1:0;

		if(Vflag) {
			simLog(NULL,4,"%s\n",VERSION);
			return 0; }
		sim=NULL;

#ifdef OPTION_VCELL
		er=simInitAndLoad(root,fname,&sim,flags,new SimpleValueProviderFactory(), new SimpleMesh());
#else
		er=simInitAndLoad(root,fname,&sim,flags,logfile);
#endif
		if(!er) {
			if(!tflag && sim->graphss && sim->graphss->graphics!=0)
				gl2glutInit(&argc,argv);
			er=simUpdateAndDisplay(sim); }
		if(!oflag && !pflag && !er)
			er=scmdopenfiles((cmdssptr) sim->cmds,wflag);
		if(pflag || er) {
			simLog(sim,4,"%sSimulation skipped\n",er?"\n":""); }
		else {
			fflush(stdout);
			fflush(stderr);
			if(tflag || !sim->graphss || sim->graphss->graphics==0) {
				er=smolsimulate(sim);
				endsimulate(sim,er); }
			else {
				smolsimulategl(sim); }}
		simfree(sim);
		simfuncfree(); }

	catch (const char* errmsg) {
		fprintf(stderr, "%s\n", errmsg);
		exitCode = 1; }
	catch (...) {
		fprintf(stderr, "unknown error\n");
		exitCode = 1; }
	return exitCode; }

