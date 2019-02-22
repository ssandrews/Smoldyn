/* Steven Andrews, 1/10/04.
Library for runtime command interpreter used for various simulations.
Copyright 2004-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "SimCommand.h"
#include "Zn.h"


/* Define SMOLDYN to 1 if this library is used in Smoldyn, or undefine this definition
 if it is not used in Smoldyn.  */

#define SMOLDYN 1

#if defined(SMOLDYN)
	#include "smoldynfuncs.h"
	#define SCMDPRINTF(A,...) simLog(NULL,A,__VA_ARGS__)
#else
	#define SCMDPRINTF(A,...) printf(__VA_ARGS__)
#endif


void scmdcatfname(cmdssptr cmds,int fid,char *str);


/* ***** internal routine ***** */

/* scmdcatfname */
void scmdcatfname(cmdssptr cmds,int fid,char *str) {
	char *dot;
	int min;

	strncpy(str,cmds->root,STRCHAR);
	strncat(str,cmds->froot,STRCHAR-strlen(str));
	dot=strrchr(cmds->fname[fid],'.');
	if(dot)	{
		min=STRCHAR-strlen(str)<(unsigned int)(dot-cmds->fname[fid])?STRCHAR-strlen(str):dot-cmds->fname[fid];
		strncat(str,cmds->fname[fid],min); }
	else strncat(str,cmds->fname[fid],STRCHAR);
	if(cmds->fsuffix[fid] && STRCHAR-strlen(str)>4) sprintf(str+strlen(str),"_%03i",cmds->fsuffix[fid]);
	if(dot) strncat(str,dot,STRCHAR-strlen(str));
	return; }


/* ***** external routines ***** */

/* scmdcode2string */
char *scmdcode2string(enum CMDcode code,char *string) {
	if(code==CMDok) strcpy(string,"ok");
	else if(code==CMDpause) strcpy(string,"pause");
	else if(code==CMDstop) strcpy(string,"stop");
	else if(code==CMDwarn) strcpy(string,"warn");
	else if(code==CMDabort) strcpy(string,"abort");
	else if(code==CMDcontrol) strcpy(string,"control");
	else if(code==CMDobserve) strcpy(string,"observe");
	else if(code==CMDmanipulate) strcpy(string,"manipulate");
	else strcpy(string,"none");
	return string; }


/* scmdalloc */
cmdptr scmdalloc(void) {
	cmdptr cmd;

	cmd=(cmdptr) malloc(sizeof(struct cmdstruct));
	if(!cmd) return NULL;
	cmd->cmds=NULL;
	cmd->on=cmd->off=cmd->dt=cmd->xt=0;
	cmd->oni=cmd->offi=cmd->dti=0;
	cmd->invoke=0;
	cmd->str=cmd->erstr=NULL;
	cmd->str=EmptyString();
	if(!cmd->str) {scmdfree(cmd);return NULL;}
	cmd->erstr=EmptyString();
	if(!cmd->erstr) {scmdfree(cmd);return NULL;}
	cmd->i1=cmd->i2=cmd->i3=0;
	cmd->f1=cmd->f2=cmd->f3=0;
	cmd->v1=cmd->v2=cmd->v3=NULL;
	cmd->freefn=NULL;
	return cmd; }


/* scmdfree */
void scmdfree(cmdptr cmd) {
	if(!cmd) return;
	if(cmd->freefn) (*cmd->freefn)(cmd);
	if(cmd->str) free(cmd->str);
	if(cmd->erstr) free(cmd->erstr);
	free(cmd);
	return; }


/* scmdssalloc */
cmdssptr scmdssalloc(enum CMDcode (*cmdfn)(void*,cmdptr,char*),void *cmdfnarg,const char *root) {
	cmdssptr cmds;

	cmds=(cmdssptr) malloc(sizeof(struct cmdsuperstruct));
	if(!cmds) return NULL;
	cmds->cmd=NULL;
	cmds->cmdi=NULL;
	cmds->cmdfn=cmdfn;
	cmds->cmdfnarg=cmdfnarg;
	cmds->iter=0;
	cmds->maxfile=0;
	cmds->nfile=0;
	if(root) strncpy(cmds->root,root,STRCHAR);
	else cmds->root[0]='\0';
	cmds->froot[0]='\0';
	cmds->fname=NULL;
	cmds->fsuffix=NULL;
	cmds->fappend=NULL;
	cmds->fptr=NULL;
	cmds->flag=0;
	cmds->precision=-1;
	cmds->outformat='s';
	return cmds; }


/* scmdssfree */
void scmdssfree(cmdssptr cmds) {
	cmdptr cmd;
	void *voidptr;
	int fid;

	if(!cmds) return;

	if(cmds->cmd) {
		while(q_pop(cmds->cmd,NULL,NULL,NULL,NULL,&voidptr)>=0) {
			cmd=(cmdptr)voidptr;
			scmdfree(cmd); }
		q_free(cmds->cmd,0,0); }

	if(cmds->cmdi) {
		while(q_pop(cmds->cmdi,NULL,NULL,NULL,NULL,&voidptr)>=0) {
			cmd=(cmdptr)voidptr;
			scmdfree(cmd); }
		q_free(cmds->cmdi,0,0); }

	for(fid=0;fid<cmds->nfile;fid++)
		if(cmds->fptr && cmds->fptr[fid]) fclose(cmds->fptr[fid]);
	free(cmds->fptr);

	for(fid=0;fid<cmds->maxfile;fid++)
		if(cmds->fname) free(cmds->fname[fid]);
	free(cmds->fname);

	free(cmds->fsuffix);
	free(cmds->fappend);
	free(cmds);
	return; }


/* scmdqalloc */
int scmdqalloc(cmdssptr cmds,int n) {
	if(!cmds) return 2;
	if(n<=0) return 0;
	if(cmds->cmd) return 3;
	cmds->cmd=q_alloc(n+1,Qdouble,NULL);
	if(!cmds->cmd) return 1;
	return 0; }


/* scmdqalloci */
int scmdqalloci(cmdssptr cmds,int n) {
	if(!cmds) return 2;
	if(n<=0) return 0;
	if(cmds->cmdi) return 3;
	cmds->cmdi=q_alloc(n+1,Qlong,NULL);
	if(!cmds->cmdi) return 1;
	return 0; }


/* scmdaddcommand */
int scmdaddcommand(cmdssptr cmds,char ch,double tmin,double tmax,double dt,double on,double off,double step,double multiplier,const char *commandstring) {
	cmdptr cmd;

	if(!cmds) return 2;
	if(!commandstring) return 0;
	if(!(cmd=scmdalloc())) return 1;
	cmd->cmds=cmds;

	if(ch=='b' || ch=='a' || ch=='@' || ch=='i' || ch=='x') {
		cmd->dt=dt;
		if(ch=='b') cmd->on=cmd->off=tmin-dt;
		else if(ch=='a') cmd->on=cmd->off=tmax+dt;
		else if(ch=='@') {
			cmd->on=on;
			cmd->off=cmd->on; }
		else if(ch=='i') {
			cmd->on=on;
			cmd->off=off;
			cmd->dt=step;
			if(cmd->on<tmin) cmd->on=tmin;
			if(cmd->off>tmax) cmd->off=tmax;
			if(cmd->dt<=0) return 5; }
		else if(ch=='x') {
			cmd->on=on;
			cmd->off=off;
			cmd->dt=step;
			cmd->xt=multiplier;
			if(cmd->on<tmin) cmd->on=tmin;
			if(cmd->off>tmax) cmd->off=tmax;
			if(cmd->dt<=0) return 5;
			if(cmd->xt<=1) return 8; }
		if(!cmds->cmd)
			if(scmdqalloc(cmds,10)==1) {scmdfree(cmd);return 7;}
		if(q_insert(NULL,0,cmd->on,0,(void*)cmd,cmds->cmd)==1)
			if(q_expand(cmds->cmd,q_length(cmds->cmd))) {scmdfree(cmd);return 7; }}
	
	else if(ch=='B' || ch=='A' || ch=='&' || ch=='j' || ch=='I' || ch=='E' || ch=='N' || ch=='e' || ch=='n') {
		cmd->oni=0;
		if(dt==0 || tmin>=tmax) cmd->offi=Q_LLONG_MAX;
		else cmd->offi=(Q_LONGLONG)((tmax-tmin)/dt+0.5);
		cmd->dti=1;
		if(ch=='B') cmd->oni=cmd->offi=-1;
		else if(ch=='A') cmd->oni=cmd->offi=(cmd->offi==Q_LLONG_MAX?cmd->offi:cmd->offi+1);
		else if(ch=='&') {
			cmd->oni=(Q_LONGLONG)on;
			cmd->offi=cmd->oni; }
		else if(ch=='j' || ch=='I') {
			cmd->oni=(Q_LONGLONG)on;
			cmd->offi=(Q_LONGLONG)off;
			cmd->dti=(Q_LONGLONG)step;
			if(cmd->dti<=0) return 5; }
		else if(ch=='e' || ch=='E');
		else if(ch=='n' || ch=='N') {
			cmd->dti=(Q_LONGLONG)step;
			if(cmd->dti<1) return 5; }
		if(!cmds->cmdi)
			if(scmdqalloci(cmds,10)==1) {scmdfree(cmd);return 7;}
		if(q_insert(NULL,0,0,cmd->oni,(void*)cmd,cmds->cmdi)==1)
			if(q_expand(cmds->cmdi,q_length(cmds->cmdi))) {scmdfree(cmd);return 7; }}
	
	else return 6;
	strncpy(cmd->str,commandstring,STRCHAR);
	if(cmd->str[strlen(cmd->str)-1]=='\n')
		cmd->str[strlen(cmd->str)-1]='\0';
	return 0; }


/* scmdstr2cmd */
int scmdstr2cmd(cmdssptr cmds,char *line2,double tmin,double tmax,double dt,char **varnames,double *varvalues,int nvar) {
	int itct,i1;
	char ch;
	cmdptr cmd;

	if(!cmds) return 2;
	if(!line2) return 0;
	if(!(cmd=scmdalloc())) return 1;
	cmd->cmds=cmds;
	itct=sscanf(line2,"%c",&ch);
	if(itct!=1) return 3;
	if(!(line2=strnword(line2,2))) return 3;

	if(strchr("ba@ix",ch)) {
		cmd->dt=dt;
		if(ch=='b') cmd->on=cmd->off=tmin-dt;
		else if(ch=='a') cmd->on=cmd->off=tmax+dt;
		else if(ch=='@') {
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&cmd->on);
			if(itct!=1) return 3;
			cmd->off=cmd->on;
			if(!(line2=strnword(line2,2))) return 4; }
		else if(ch=='i') {
			itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&cmd->on,&cmd->off,&cmd->dt);
			if(itct!=3) return 3;
			if(cmd->on<tmin) cmd->on=tmin;
			if(cmd->off>tmax) cmd->off=tmax;
			if(cmd->dt<=0) return 5;
			if(!(line2=strnword(line2,4))) return 4; }
		else if(ch=='x') {
			itct=strmathsscanf(line2,"%mlg %mlg %mlg %mlg",varnames,varvalues,nvar,&cmd->on,&cmd->off,&cmd->dt,&cmd->xt);
			if(itct!=4) return 3;
			if(cmd->on<tmin) cmd->on=tmin;
			if(cmd->off>tmax) cmd->off=tmax;
			if(cmd->dt<=0) return 5;
			if(cmd->xt<=1) return 8;
			if(!(line2=strnword(line2,5))) return 4; }
		if(!cmds->cmd)
			if(scmdqalloc(cmds,10)==1) {scmdfree(cmd);return 7;}
		if(q_insert(NULL,0,cmd->on,0,(void*)cmd,cmds->cmd)==1)
			if(q_expand(cmds->cmd,q_length(cmds->cmd))) {scmdfree(cmd);return 7; }}

	else if(strchr("BA&jIENen",ch)) {
		cmd->oni=0;
		if(dt==0 || tmin>=tmax) cmd->offi=Q_LLONG_MAX;
		else cmd->offi=(Q_LONGLONG)((tmax-tmin)/dt+0.5);
		cmd->dti=1;
		if(ch=='B') cmd->oni=cmd->offi=-1;
		else if(ch=='A') cmd->oni=cmd->offi=(cmd->offi==Q_LLONG_MAX?cmd->offi:cmd->offi+1);
		else if(ch=='&') {
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
			if(itct!=1) return 3;
			cmd->oni=i1;
			cmd->offi=cmd->oni;
			if(!(line2=strnword(line2,2))) return 4; }
		else if(ch=='j' || ch=='I') {
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
			if(itct!=1) return 3;
			cmd->oni=i1;
			if(!(line2=strnword(line2,2))) return 4;
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
			if(itct!=1) return 3;
			cmd->offi=i1;
			if(!(line2=strnword(line2,2))) return 4;
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
			if(itct!=1) return 3;
			cmd->dti=i1;
			if(cmd->dti<=0) return 5;
			if(!(line2=strnword(line2,2))) return 4; }
		else if(ch=='e' || ch=='E');
		else if(ch=='n' || ch=='N') {
			itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&i1);
			if(itct!=1) return 3;
			cmd->dti=i1;
			if(cmd->dti<1) return 5;
			if(!(line2=strnword(line2,2))) return 4; }
		if(!cmds->cmdi)
			if(scmdqalloci(cmds,10)==1) {scmdfree(cmd);return 7;}
		if(q_insert(NULL,0,0,cmd->oni,(void*)cmd,cmds->cmdi)==1)
			if(q_expand(cmds->cmdi,q_length(cmds->cmdi))) {scmdfree(cmd);return 7; }}

	else return 6;
	strncpy(cmd->str,line2,STRCHAR);
	if(cmd->str[strlen(cmd->str)-1]=='\n')
		cmd->str[strlen(cmd->str)-1]='\0';
	return 0; }
 

/* scmdpop */
void scmdpop(cmdssptr cmds,double t) {
	cmdptr cmd;
	void *voidptr;

	if(!cmds || !cmds->cmd) return;
	while(q_length(cmds->cmd)>0 && q_frontkeyD(cmds->cmd)<=t) {
		q_pop(cmds->cmd,NULL,NULL,NULL,NULL,&voidptr);
		cmd=(cmdptr)voidptr;
		scmdfree(cmd); }
	return; }


/* scmdexecute */
enum CMDcode scmdexecute(cmdssptr cmds,double time,double simdt,Q_LONGLONG iter,int donow) {
	enum CMDcode code1,code2;
	cmdptr cmd;
	double dt;
	void *voidptr;

	if(!cmds) return CMDok;
	code2=CMDok;
	if(iter<0) iter=cmds->iter++;
	else cmds->iter=iter;

	if(cmds->cmdi)			// integer execution times
		while((q_length(cmds->cmdi)>0) && (q_frontkeyL(cmds->cmdi)<=iter || donow)) {
			q_pop(cmds->cmdi,NULL,NULL,NULL,NULL,&voidptr);
			cmd=(cmdptr)voidptr;
			cmd->invoke++;
			code1=(*cmds->cmdfn)(cmds->cmdfnarg,cmd,cmd->str);
			if(code1==CMDwarn) {
				if(strlen(cmd->erstr)) SCMDPRINTF(7,"command '%s' error: %s\n",cmd->str,cmd->erstr);
				else SCMDPRINTF(7,"error with command: '%s'\n",cmd->str); }
			if(cmd->oni+cmd->dti<=cmd->offi && !donow && (code1==CMDok || code1==CMDpause)) {
				cmd->oni+=cmd->dti;
				q_insert(NULL,0,0,cmd->oni,(void*)cmd,cmds->cmdi); }
			else scmdfree(cmd);
			if(code1==CMDabort) return code1;
			if(code1>code2) code2=code1; }

	if(cmds->cmd)				// float execution times
		while((q_length(cmds->cmd)>0) && (q_frontkeyD(cmds->cmd)<=time || donow)) {
			q_pop(cmds->cmd,NULL,NULL,NULL,NULL,&voidptr);
			cmd=(cmdptr)voidptr;
			cmd->invoke++;
			code1=(*cmds->cmdfn)(cmds->cmdfnarg,cmd,cmd->str);
			if(code1==CMDwarn) {
				if(strlen(cmd->erstr)) SCMDPRINTF(7,"command '%s' error: %s\n",cmd->str,cmd->erstr);
				else SCMDPRINTF(7,"error with command: '%s'\n",cmd->str); }
			dt=(cmd->dt>=simdt)?cmd->dt:simdt;
			if(cmd->on+dt<=cmd->off && !donow && (code1==CMDok || code1==CMDpause)) {
				cmd->on+=dt;
				if(cmd->xt>1) cmd->dt*=cmd->xt;
				q_insert(NULL,0,cmd->on,0,(void*)cmd,cmds->cmd); }
			else scmdfree(cmd);
			if(code1==CMDabort) return code1;
			if(code1>code2) code2=code1; }

	return code2; }


/* scmdcmdtype */
enum CMDcode scmdcmdtype(cmdssptr cmds,cmdptr cmd) {
	char string[STRCHAR];

	sscanf(cmd->str,"%s",string);
	strncat(string," cmdtype",STRCHAR-strlen(string));
	return (*cmds->cmdfn)(cmds->cmdfnarg,cmd,string); }


/* scmdnextcmdtime */
int scmdnextcmdtime(cmdssptr cmds,double time,Q_LONGLONG iter,enum CMDcode type,int equalok,double *timeptr,Q_LONGLONG *iterptr) {
	double tbest,t,dt;
	int i,ans,done;
	Q_LONGLONG dti,it,itbest;
	cmdptr cmd;
	enum CMDcode cmdtype;
	void *voidptr;

	if(!cmds) return 0;
	ans=0;
	tbest=time-1;
	itbest=iter-1;

	if(cmds->cmdi) {
		i=-1;
		done=0;
		while(!done && (i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmds->cmdi))>=0) {
			cmd=(cmdptr)voidptr;
			if(type==CMDall || (cmdtype=scmdcmdtype(cmds,cmd))==type || (type==CMDctrlORobs && (cmdtype==CMDcontrol || cmdtype==CMDobserve))) {
				it=cmd->oni;
				dti=cmd->dti;
				if((equalok && it>=iter) || (!equalok && it>iter)) done=1;
				while(((equalok && it<iter) || (!equalok && it<=iter)) && it<=cmd->offi) it+=dti;
				if(it<=cmd->offi)
					if(itbest<iter || it<itbest) {
						ans|=1;
						itbest=it; }}}}

	if(cmds->cmd) {
		i=-1;
		done=0;
		while(!done && (i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmds->cmd))>=0) {
			cmd=(cmdptr)voidptr;
			if(type==CMDall || (cmdtype=scmdcmdtype(cmds,cmd))==type || (type==CMDctrlORobs && (cmdtype==CMDcontrol || cmdtype==CMDobserve))) {
				t=cmd->on;
				dt=cmd->dt;
				if((equalok && t>=time) || (!equalok && t>time)) done=1;
				while(((equalok && t<time) || (!equalok && t<=time)) && t<=cmd->off) {
					t+=dt;
					if(cmd->xt>1) dt*=cmd->xt; }
				if(t<=cmd->off)
					if(tbest<time || t<tbest) {
						ans|=2;
						tbest=t; }}}}

	if(timeptr) *timeptr=tbest;
	if(iterptr) *iterptr=itbest;
	return ans; }


/* scmdoutput */
void scmdoutput(cmdssptr cmds) {
	int fid,i;
	queue cmdq;
	cmdptr cmd;
	void* voidptr;
	char string[STRCHAR],string2[STRCHAR];

	SCMDPRINTF(2,"RUNTIME COMMAND INTERPRETER\n");
	if(!cmds) {
		SCMDPRINTF(2," No command superstructure defined\n\n");
		return; }
	if(!cmds->cmdfn) SCMDPRINTF(10," ERROR: Command executer undefined");
	if(!cmds->cmdfnarg) SCMDPRINTF(10," WARNING: No argument for command executer");
	if(cmds->iter) SCMDPRINTF(2," Commands iteration counter: %i\n",cmds->iter);
	if(cmds->nfile) {
		SCMDPRINTF(2," Output file root: '%s%s'\n",cmds->root,cmds->froot);
		SCMDPRINTF(2," Output file paths and names:\n"); }
	else
		SCMDPRINTF(2," No output files\n");
	for(fid=0;fid<cmds->nfile;fid++) {
		if(!strcmp(cmds->fname[fid],"stdout") || !strcmp(cmds->fname[fid],"stderr"))
			SCMDPRINTF(2,"  %s (file open): %s\n",cmds->fname[fid],cmds->fname[fid]);
		else {
			scmdcatfname(cmds,fid,string);
			SCMDPRINTF(2,"  %s (file %s): %s\n",cmds->fname[fid],cmds->fptr[fid]?"open":"closed",string); }}

	cmdq=cmds->cmd;
	if(cmdq) {
		SCMDPRINTF(2," Time queue:\n");
		SCMDPRINTF(2,"  %i queue spaces used of %i total\n",q_length(cmdq),q_maxlength(cmdq)-1);
		SCMDPRINTF(2,"  Times to start, stop, and step, strings, and command type:\n");
		i=-1;
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmdq))>=0) {
			cmd=(cmdptr)voidptr;
			SCMDPRINTF(2,"  %g %g%s%g '%s' (%s)\n",cmd->on,cmd->off,cmd->xt>1?" *":" ",cmd->xt>1?cmd->xt:cmd->dt,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }}
	cmdq=cmds->cmdi;
	if(cmdq) {
		SCMDPRINTF(2," Integer queue:\n");
		SCMDPRINTF(2,"  %i queue spaces used of %i total\n",q_length(cmdq),q_maxlength(cmdq)-1);
		SCMDPRINTF(2,"  Iterations to start, stop, and step, strings, and command type:\n");
		i=-1;
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmdq))>=0) {
			cmd=(cmdptr)voidptr;
			if(cmd->offi!=Q_LLONG_MAX) {
				sprintf(string2,"  %s %s %s '%%s' (%%s)\n",Q_LLI,Q_LLI,Q_LLI);
				SCMDPRINTF(2,string2,cmd->oni,cmd->offi,cmd->dti,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }
			else {
				sprintf(string2,"  %s end %s '%%s' (%%s)\n",Q_LLI,Q_LLI);
				SCMDPRINTF(2,string2,cmd->oni,cmd->dti,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }}}
	SCMDPRINTF(2,"\n");
	return; }


/* scmdwritecommands */
void scmdwritecommands(cmdssptr cmds,FILE *fptr,char *filename) {
	int i,fid;
	cmdptr cmd;
	void *voidptr;
	char string2[STRCHAR];

	fprintf(fptr,"# Command parameters\n");
	if(strlen(cmds->froot)) fprintf(fptr,"output_root %s\n",cmds->froot);

	if(!(cmds->nfile==1 && !strcmp(cmds->fname[0],filename))) {
		if(cmds->nfile) {
			fprintf(fptr,"output_files");
			for(fid=0;fid<cmds->nfile;fid++)
				if(!filename || strcmp(cmds->fname[fid],filename))
					fprintf(fptr," %s",cmds->fname[fid]);
			fprintf(fptr,"\n"); }
		for(fid=0;fid<cmds->nfile;fid++)
			if(cmds->fsuffix[fid])
				fprintf(fptr,"output_file_number %s %i\n",cmds->fname[fid],cmds->fsuffix[fid]); }

	i=-1;
	if(cmds->cmdi)
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmds->cmdi))>=0) {
			cmd=(cmdptr)voidptr;
			sprintf(string2,"cmd I %s %s %s %%s\n",Q_LLI,Q_LLI,Q_LLI);
			fprintf(fptr,string2,cmd->oni,cmd->offi,cmd->dti,cmd->str); }
	if(cmds->cmd)
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmds->cmd))>=0) {
			cmd=(cmdptr)voidptr;
			if(cmd->xt<=1) fprintf(fptr,"cmd i %g %g %g %s\n",cmd->on,cmd->off,cmd->dt,cmd->str);
			else fprintf(fptr,"cmd x %g %g %g %g %s\n",cmd->on,cmd->off,cmd->dt,cmd->xt,cmd->str); }
	fprintf(fptr,"\n");
	return; }


/* scmdsetflag */
void scmdsetflag(cmdssptr cmds,double flag) {
	cmds->flag=flag;
	return; }


/* scmdreadflag */
double scmdreadflag(cmdssptr cmds) {
	return cmds->flag; }


/* scmdsetprecision */
void scmdsetprecision(cmdssptr cmds,int precision) {
	cmds->precision=precision;
	return; }


/* scmdsetoutputformat */
int scmdsetoutputformat(cmdssptr cmds,char *format) {
	if(!strcmp(format,"ssv") || !strcmp(format,"SSV")) cmds->outformat='s';
	else if(!strcmp(format,"csv") || !strcmp(format,"CSV")) cmds->outformat='c';
	else return 1;
	return 0; }


/************** file functions **************/	
	
/* scmdsetfroot */
int scmdsetfroot(cmdssptr cmds,const char *root) {
	if(!cmds || !root) return 1;
	strncpy(cmds->froot,root,STRCHAR);
	return 0; }


/* scmdsetfnames */
int scmdsetfnames(cmdssptr cmds,char *str,int append) {
	int fid,itct,n,newmaxfile,*newfsuffix,*newfappend;
	char **newfname;
	FILE **newfptr;

	if(!cmds) return 4;
	n=wordcount(str);

	if(cmds->nfile+n>cmds->maxfile) {
		newmaxfile=cmds->maxfile+n;

		newfname=(char**)calloc(newmaxfile,sizeof(char*));
		if(!newfname) return 1;
		for(fid=0;fid<cmds->maxfile;fid++)
			newfname[fid]=cmds->fname[fid];
		for(;fid<newmaxfile;fid++)
			newfname[fid]=NULL;
		for(fid=cmds->maxfile;fid<newmaxfile;fid++) {
			newfname[fid]=EmptyString();
			if(!newfname[fid]) return 1; }

		newfsuffix=(int*)calloc(newmaxfile,sizeof(int));
		if(!newfsuffix) return 1;
		for(fid=0;fid<cmds->maxfile;fid++)
			newfsuffix[fid]=cmds->fsuffix[fid];
		for(;fid<newmaxfile;fid++)
			newfsuffix[fid]=0;
		
		newfappend=(int*)calloc(newmaxfile,sizeof(int));
		if(!newfappend) return 1;
		for(fid=0;fid<cmds->maxfile;fid++)
			newfappend[fid]=cmds->fappend[fid];
		for(;fid<newmaxfile;fid++)
			newfappend[fid]=0;

		newfptr=(FILE **)calloc(newmaxfile,sizeof(FILE*));
		if(!newfptr) return 1;
		for(fid=0;fid<cmds->maxfile;fid++)
			newfptr[fid]=cmds->fptr[fid];
		for(;fid<newmaxfile;fid++)
			newfptr[fid]=NULL;
		
		cmds->maxfile=newmaxfile;
		free(cmds->fname);
		cmds->fname=newfname;
		free(cmds->fsuffix);
		cmds->fsuffix=newfsuffix;
		free(cmds->fappend);
		cmds->fappend=newfappend;
		free(cmds->fptr);
		cmds->fptr=newfptr; }

	while(str) {
		fid=cmds->nfile;
		itct=sscanf(str,"%s",cmds->fname[fid]);
		if(itct!=1) return 2;
		cmds->fappend[fid]=append;
		cmds->nfile++;
		str=strnword(str,2); }

	return 0; }


/* scmdsetfsuffix */
int scmdsetfsuffix(cmdssptr cmds,const char *fname,int i) {
	int fid;

	if(!cmds || !cmds->nfile) return 1;
	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return 1;
	cmds->fsuffix[fid]=i;
	return 0; }


/* scmdopenfiles */
int scmdopenfiles(cmdssptr cmds,int overwrite) {
	int fid;
	char str1[STRCHAR],str2[STRCHAR];
	FILE *fptr;

	if(!cmds) return 0;
	for(fid=0;fid<cmds->nfile;fid++) {
		if(cmds->fptr[fid] && strcmp(cmds->fname[fid],"stdout") && strcmp(cmds->fname[fid],"stderr"))
			fclose(cmds->fptr[fid]);
		cmds->fptr[fid]=NULL; }

	for(fid=0;fid<cmds->nfile;fid++) {
		if(!strcmp(cmds->fname[fid],"stdout")) cmds->fptr[fid]=stdout;
		else if(!strcmp(cmds->fname[fid],"stderr")) cmds->fptr[fid]=stderr;
		else {
			scmdcatfname(cmds,fid,str1);
			if(!cmds->fappend[fid] && !overwrite) {
				fptr=fopen(str1,"r");
				if(fptr) {
					fclose(fptr);
					fprintf(stderr,"Overwrite existing output file '%s' (y/n)? ",cmds->fname[fid]);
					scanf("%s",str2);
					if(!(str2[0]=='y' || str2[0]=='Y')) return 1; }}
			if(cmds->fappend[fid]) cmds->fptr[fid]=fopen(str1,"a");
			else cmds->fptr[fid]=fopen(str1,"w"); }
		if(!cmds->fptr[fid]) {
			SCMDPRINTF(7,"Failed to open file '%s' for writing\n",cmds->fname[fid]);
			return 1; }}

	return 0; }


/* scmdoverwrite */
FILE *scmdoverwrite(cmdssptr cmds,char *line2) {
	int itct,fid;
	static char fname[STRCHAR],str1[STRCHAR];

	if(!line2) return NULL;
	itct=sscanf(line2,"%s",fname);
	if(itct!=1) return NULL;
	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return NULL;
	if(strcmp(cmds->fname[fid],"stdout") && strcmp(cmds->fname[fid],"stderr")) {
		fclose(cmds->fptr[fid]);
		scmdcatfname(cmds,fid,str1);
		cmds->fptr[fid]=fopen(str1,"w"); }
	return cmds->fptr[fid]; }


/* scmdincfile */
FILE *scmdincfile(cmdssptr cmds,char *line2) {
	int itct,fid;
	static char fname[STRCHAR],str1[STRCHAR];

	if(!line2) return NULL;
	itct=sscanf(line2,"%s",fname);
	if(itct!=1) return NULL;
	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return NULL;
	if(strcmp(cmds->fname[fid],"stdout") && strcmp(cmds->fname[fid],"stderr")) {
		fclose(cmds->fptr[fid]);
		cmds->fsuffix[fid]++;
		scmdcatfname(cmds,fid,str1);
		if(cmds->fappend[fid])
			cmds->fptr[fid]=fopen(str1,"a");
		else
			cmds->fptr[fid]=fopen(str1,"w"); }
	return cmds->fptr[fid]; }


/* scmdgetfptr */
FILE *scmdgetfptr(cmdssptr cmds,char *line2) {
	int itct,fid;
	static char fname[STRCHAR];

	if(!line2) return stdout;
	itct=sscanf(line2,"%s",fname);
	if(itct!=1) return NULL;
	if(!strcmp(fname,"stdout")) return stdout;
	if(!strcmp(fname,"stderr")) return stderr;
	if(!cmds) return NULL;
	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return NULL;
	return cmds->fptr[fid]; }


/* scmdfprintf */
int scmdfprintf(cmdssptr cmds,FILE *fptr,const char *format,...) {
	char message[STRCHARLONG],newformat[STRCHAR],replacestr[STRCHAR];
	va_list arguments;
	int code;
	
	strncpy(newformat,format,STRCHAR);
	if(!cmds) {
		strstrreplace(newformat,"%,"," ",STRCHAR); }
	else {
		if(cmds->precision>=0) {
			sprintf(replacestr,"%%.%ig",cmds->precision);
			strstrreplace(newformat,"%g",replacestr,STRCHAR); }
		if(cmds->outformat=='c')
			strstrreplace(newformat,"%,",",",STRCHAR);
		else
			strstrreplace(newformat,"%,"," ",STRCHAR); }
	va_start(arguments,format);
	vsprintf(message,newformat,arguments);
	va_end(arguments);
	code=fprintf(fptr,"%s",message);	
	return code; }


/* scmdflush */
void scmdflush(FILE *fptr) {
	fflush(fptr);
	return; }

