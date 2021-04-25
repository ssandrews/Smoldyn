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
void scmdcopycommand(cmdptr cmdfrom,cmdptr cmdto);
char *scmdcode2string(enum CMDcode code,char *string);

cmdptr scmdalloc(void);
void scmdfree(cmdptr cmd);
int scmdcmdlistalloc(cmdssptr cmds,int newspaces);
int scmdqalloc(cmdssptr cmds,int n);
int scmdqalloci(cmdssptr cmds,int n);

void scmddocommandtiming(cmdptr cmd,double tmin,double tmax,double dt,int iter);


/* ***** Utility functions ***** */

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
	if(cmds->fsuffix[fid] && STRCHAR-strlen(str)>4) snprintf(str+strlen(str),sizeof(str)-strlen(str),"_%03i",cmds->fsuffix[fid]);
	if(dot) strncat(str,dot,STRCHAR-strlen(str));
	return; }


/* scmdcopycommand */
void scmdcopycommand(cmdptr cmdfrom,cmdptr cmdto) {
	if(!cmdfrom || !cmdto || cmdfrom==cmdto) return;
	cmdto->cmds=cmdfrom->cmds;
	cmdto->twin=cmdfrom;
	cmdto->timing=cmdfrom->timing;
	cmdto->on=cmdfrom->on;
	cmdto->off=cmdfrom->off;
	cmdto->dt=cmdfrom->dt;
	cmdto->xt=cmdfrom->xt;
	cmdto->oni=cmdfrom->oni;
	cmdto->offi=cmdfrom->offi;
	cmdto->dti=cmdfrom->dti;
	cmdto->invoke=0;
	strncpy(cmdto->str,cmdfrom->str,STRCHAR);
	strncpy(cmdto->erstr,"",STRCHAR);
	cmdto->i1=cmdto->i2=cmdto->i3=0;
	cmdto->f1=cmdto->f2=cmdto->f3=0;
	cmdto->v1=cmdto->v2=cmdto->v3=NULL;
	cmdto->freefn=NULL;
	return; }


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


/* Memory management */

/* scmdalloc */
cmdptr scmdalloc(void) {
	cmdptr cmd;

	cmd=(cmdptr) malloc(sizeof(struct cmdstruct));
	if(!cmd) return NULL;
	cmd->cmds=NULL;
	cmd->twin=NULL;
	cmd->timing='?';
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

	cmds->condition=0;
	cmds->cmdlist=NULL;
	cmds->maxcmdlist=0;
	cmds->ncmdlist=0;
	cmds->cmd=NULL;
	cmds->cmdi=NULL;
	cmds->cmdfn=cmdfn;
	cmds->cmdfnarg=cmdfnarg;
	cmds->iter=0;

	cmds->flag=0;

	cmds->maxfile=0;
	cmds->nfile=0;
	if(root) {
		strncpy(cmds->root,root,STRCHAR-1);
		cmds->root[STRCHAR-1]='\0'; }
	else cmds->root[0]='\0';
	cmds->froot[0]='\0';
	cmds->fname=NULL;
	cmds->fsuffix=NULL;
	cmds->fappend=NULL;
	cmds->fptr=NULL;
	cmds->precision=-1;
	cmds->outformat='s';

	cmds->maxdata=0;
	cmds->ndata=0;
	cmds->dname=NULL;
	cmds->data=NULL;
	return cmds; }


/* scmdssfree */
void scmdssfree(cmdssptr cmds) {
	int fid,did,i;
	void *voidptr;
	cmdptr cmd;

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

	if(cmds->cmdlist) {
		for(i=0;i<cmds->ncmdlist;i++)
			scmdfree(cmds->cmdlist[i]);
		free(cmds->cmdlist); }

	for(fid=0;fid<cmds->nfile;fid++)
		if(cmds->fptr && cmds->fptr[fid]) fclose(cmds->fptr[fid]);
	free(cmds->fptr);
	for(fid=0;fid<cmds->maxfile;fid++)
		if(cmds->fname) free(cmds->fname[fid]);
	free(cmds->fname);
	free(cmds->fsuffix);
	free(cmds->fappend);

	for(did=0;did<cmds->maxdata;did++) {
		if(cmds->dname) free(cmds->dname[did]);
		if(cmds->data) ListFreeDD(cmds->data[did]); }
	free(cmds->dname);
	free(cmds->data);

	free(cmds);
	return; }


/* scmdcmdlistalloc */
int scmdcmdlistalloc(cmdssptr cmds,int newspaces) {
	int i,newmax;
	cmdptr *newcmdlist;

	if(!cmds) return 0;
	if(newspaces<=0) return 0;
	newmax=cmds->maxcmdlist+newspaces;
	newcmdlist=(cmdptr*) calloc(newmax,sizeof(cmdptr));
	if(!newcmdlist) return 1;

	for(i=0;i<cmds->ncmdlist;i++)
		newcmdlist[i]=cmds->cmdlist[i];
	for(;i<newmax;i++)
		newcmdlist[i]=NULL;

	if(cmds->cmdlist)
		free(cmds->cmdlist);
	cmds->cmdlist=newcmdlist;
	cmds->maxcmdlist=newmax;
	return 0; }


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


/* scmdsetcondition */
void scmdsetcondition(cmdssptr cmds,int cond,int upgrade) {
	if(!cmds) return;
	if(upgrade==0 && cmds->condition>cond) cmds->condition=cond;
	else if(upgrade==1 && cmds->condition<cond) cmds->condition=cond;
	else if(upgrade==2) cmds->condition=cond;
	return; }


/* scmdaddcommand */
int scmdaddcommand(cmdssptr cmds,char timing,double on,double off,double step,double multiplier,const char *commandstring) {
	cmdptr cmd;
	int er;

	if(!cmds) return 2;
	if(!commandstring || strlen(commandstring)==0) return 3;
	if(!(cmd=scmdalloc())) return 1;
	cmd->cmds=cmds;
	cmd->timing=timing;
	if(strchr("baBAEe",timing));
	else if(strchr("@",timing))
		cmd->on=cmd->off=on;
	else if(strchr("i",timing)) {
		cmd->on=on;
		cmd->off=off;
		cmd->dt=step; }
	else if(strchr("x",timing)) {
		cmd->on=on;
		cmd->off=off;
		cmd->dt=step;
		cmd->xt=multiplier; }
	else if(strchr("&",timing)) {
		cmd->oni=cmd->offi=(Q_LONGLONG)on;
		cmd->dti=1; }
	else if(strchr("Ij",timing)) {
		cmd->oni=(Q_LONGLONG)on;
		cmd->offi=(Q_LONGLONG)off;
		cmd->dti=(Q_LONGLONG)step; }
	else if(strchr("Nn",timing))
		cmd->dti=(Q_LONGLONG)step;
	else {
		scmdfree(cmd);
		return 6; }

	strncpy(cmd->str,commandstring,STRCHAR);
	if(cmd->str[strlen(cmd->str)-1]=='\n')
		cmd->str[strlen(cmd->str)-1]='\0';

	if(cmds->ncmdlist==cmds->maxcmdlist) {
		er=scmdcmdlistalloc(cmds,1+cmds->maxcmdlist);
		if(er) {scmdfree(cmd);return 1;} }
	cmds->cmdlist[cmds->ncmdlist]=cmd;
	cmds->ncmdlist++;
	scmdsetcondition(cmds,2,0);
	return 0; }


/* scmdstr2cmd */
int scmdstr2cmd(cmdssptr cmds,char *line2,char **varnames,double *varvalues,int nvar) {
	int itct,er;
	char timing;
	double on,off,step,multiplier;

	if(!cmds) return 2;
	if(!line2) return 3;

	on=off=step=multiplier=0;
	itct=sscanf(line2,"%c",&timing);
	if(itct!=1) return 3;
	if(!(line2=strnword(line2,2))) return 3;
	if(strchr("baBAEe",timing));
	else if(strchr("@&",timing)) {
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&on);
		if(itct!=1) return 3;
		if(!(line2=strnword(line2,2))) return 3; }
	else if(strchr("Nn",timing)) {
		itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&step);
		if(itct!=1) return 3;
		if(!(line2=strnword(line2,2))) return 3; }
	else if(strchr("iIj",timing)) {
		itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&on,&off,&step);
		if(itct!=3) return 3;
		if(!(line2=strnword(line2,4))) return 3; }
	else if(strchr("x",timing)) {
		itct=strmathsscanf(line2,"%mlg %mlg %mlg %mlg",varnames,varvalues,nvar,&on,&off,&step,&multiplier);
		if(itct!=4) return 3;
		if(!(line2=strnword(line2,5))) return 3; }
	else return 6;

	er=scmdaddcommand(cmds,timing,on,off,step,multiplier,line2);
	return er; }


/* scmddocommandtiming */
void scmddocommandtiming(cmdptr cmd,double tmin,double tmax,double dt,int iter) {
	char timing;

	timing=cmd->timing;
	if(timing=='b') {
		cmd->on=cmd->off=tmin-dt;
		cmd->dt=dt; }
	else if(timing=='a') {
		cmd->on=cmd->off=tmax+dt;
		cmd->dt=dt; }
	else if(timing=='@') {
		cmd->dt=dt; }
	else if(timing=='i') {
		if(cmd->on<tmin) cmd->on=tmin;
		if(cmd->off>tmax) cmd->off=tmax; }
	else if(timing=='x') {
		if(cmd->on<tmin) cmd->on=tmin;
		if(cmd->off>tmax) cmd->off=tmax; }
	else if(timing=='B') {
		cmd->oni=cmd->offi=iter-1;
		cmd->dti=1; }
	else if(timing=='A') {
		cmd->oni=cmd->offi=iter+(Q_LONGLONG)((tmax-tmin)/dt+0.5)+1;
		cmd->dti=1; }
	else if(timing=='&');
	else if(strchr("Ij",timing)) {
		if(cmd->oni<0) cmd->oni=iter+1; }
	else if(strchr("Ee",timing)) {
		cmd->oni=iter;
		cmd->offi=iter+(Q_LONGLONG)((tmax-tmin)/dt+0.5);
		cmd->dti=1; }
	else if(strchr("Nn",timing)) {
		cmd->oni=iter;
		cmd->offi=iter+(Q_LONGLONG)((tmax-tmin)/dt+0.5); }

	return; }


/* scmdupdatecommands */
int scmdupdatecommands(cmdssptr cmds,double tmin,double tmax,double dt) {
	int i;
	char timing;
	cmdptr cmd,cmdtemplate;
	void *voidptr;

	if(!cmds || !cmds->cmdlist) return 0;
	if(cmds->condition==3) return 0;		// already ready to simulate
	if(dt<=0 || tmax<tmin) return 0;		// can't assign times to commands

	if(cmds->condition==0) {
		if(cmds->cmd) {
			while(q_pop(cmds->cmd,NULL,NULL,NULL,NULL,&voidptr)>=0) {
				cmd=(cmdptr)voidptr;
				scmdfree(cmd); }}
		if(cmds->cmdi) {
			while(q_pop(cmds->cmdi,NULL,NULL,NULL,NULL,&voidptr)>=0) {
				cmd=(cmdptr)voidptr;
				scmdfree(cmd); }}
		for(i=0;i<cmds->ncmdlist;i++) {
			cmdtemplate=cmds->cmdlist[i];
			cmdtemplate->twin=NULL;
			cmdtemplate->invoke=0; }}

	for(i=0;i<cmds->ncmdlist;i++) {
		cmdtemplate=cmds->cmdlist[i];
		if(!cmdtemplate->twin && cmdtemplate->invoke==0) {
			cmd=scmdalloc();
			if(!cmd) return 1;
			scmdcopycommand(cmdtemplate,cmd);
			scmddocommandtiming(cmd,tmin,tmax,dt,cmds->iter);
			timing=cmd->timing;
			if(strchr("ba@ix",timing)) {
				if(!cmds->cmd)
					if(scmdqalloc(cmds,10)==1) {scmdfree(cmd);return 1;}
				if(q_insert(NULL,0,cmd->on,0,(void*)cmd,cmds->cmd)==1)
					if(q_expand(cmds->cmd,q_length(cmds->cmd))) {scmdfree(cmd);return 1; }}
			else if(strchr("BA&jIENen",timing)) {
				if(!cmds->cmdi)
					if(scmdqalloci(cmds,10)==1) {scmdfree(cmd);return 1;}
				if(q_insert(NULL,0,0,cmd->oni,(void*)cmd,cmds->cmdi)==1)
					if(q_expand(cmds->cmdi,q_length(cmds->cmdi))) {scmdfree(cmd);return 1; }}
			else {
				scmdfree(cmd);
				return 6; }
			cmdtemplate->twin=cmd;
			cmdtemplate->invoke=1; }
		else if(cmds->condition==1) {
			cmd=cmdtemplate->twin;
			if(cmd)
				scmddocommandtiming(cmd,tmin,tmax,dt,cmds->iter); }}

	scmdsetcondition(cmds,3,1);
	return 0; }


/* scmdpop */
void scmdpop(cmdssptr cmds,double t) {
	cmdptr cmd;
	void *voidptr;

	if(!cmds || !cmds->cmd) return;
	while(q_length(cmds->cmd)>0 && q_frontkeyD(cmds->cmd)<=t) {
		q_pop(cmds->cmd,NULL,NULL,NULL,NULL,&voidptr);
		cmd=(cmdptr)voidptr;
		cmd->twin->twin=NULL;
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
			else {
				cmd->twin->twin=NULL;
				scmdfree(cmd); }
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
			else {
				cmd->twin->twin=NULL;
				scmdfree(cmd); }
			if(code1==CMDabort) return code1;
			if(code1>code2) code2=code1; }

	return code2; }


/* scmdcmdtype */
enum CMDcode scmdcmdtype(cmdssptr cmds,cmdptr cmd) {
	char string[STRCHAR];

	sscanf(cmd->str,"%s",string);
	strncat(string," cmdtype",STRCHAR-strlen(string));
	return (*cmds->cmdfn)(cmds->cmdfnarg,cmd,string); }


/* scmdoutput */
void scmdoutput(cmdssptr cmds) {
	int fid,i,did;
	queue cmdq;
	cmdptr cmd;
	void* voidptr;
	char timing,string[STRCHAR],string2[STRCHAR];

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
	if(cmds->ndata) {
		SCMDPRINTF(2," Output data table names:\n"); }
	else
		SCMDPRINTF(2," No output data tables\n");
	for(did=0;did<cmds->ndata;did++)
		SCMDPRINTF(2,"  %s\n",cmds->dname[did]);

	if(!cmds->cmdlist || cmds->ncmdlist==0)
		SCMDPRINTF(2," No commands\n");
	else {
		SCMDPRINTF(2," Commands:\n");
		for(i=0;i<cmds->ncmdlist;i++) {
			cmd=cmds->cmdlist[i];
			timing=cmd->timing;
			SCMDPRINTF(2,"  %c",timing);
			if(strchr("baBAEe",timing));
			else if(strchr("@",timing))
				SCMDPRINTF(2," time: %g",cmd->on);
			else if(strchr("&",timing))
				SCMDPRINTF(2," iteration: %i",cmd->oni);
			else if(strchr("Nn",timing))
				SCMDPRINTF(2," every: %i",cmd->dti);
			else if(strchr("i",timing))
				SCMDPRINTF(2," from: %g to: %g step: %g",cmd->on,cmd->off,cmd->dt);
			else if(strchr("Ij",timing))
				SCMDPRINTF(2," from: %i to: %i step: %i",cmd->oni,cmd->offi,cmd->dti);
			else if(strchr("x",timing))
				SCMDPRINTF(2," from: %g to: %g step: %g mult: %g",cmd->on,cmd->off,cmd->dt,cmd->xt);
			SCMDPRINTF(2," '%s' (%s)\n",cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }}

	cmdq=cmds->cmd;
	if(cmdq) {
		SCMDPRINTF(1," Time queue:\n");
		SCMDPRINTF(1,"  %i queue spaces used of %i total\n",q_length(cmdq),q_maxlength(cmdq)-1);
		SCMDPRINTF(1,"  Times to start, stop, and step, strings, and command type:\n");
		i=-1;
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmdq))>=0) {
			cmd=(cmdptr)voidptr;
			SCMDPRINTF(1,"  %g %g%s%g '%s' (%s)\n",cmd->on,cmd->off,cmd->xt>1?" *":" ",cmd->xt>1?cmd->xt:cmd->dt,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }}
	cmdq=cmds->cmdi;
	if(cmdq) {
		SCMDPRINTF(1," Integer queue:\n");
		SCMDPRINTF(1,"  %i queue spaces used of %i total\n",q_length(cmdq),q_maxlength(cmdq)-1);
		SCMDPRINTF(1,"  Iterations to start, stop, and step, strings, and command type:\n");
		i=-1;
		while((i=q_next(i,NULL,NULL,NULL,NULL,&voidptr,cmdq))>=0) {
			cmd=(cmdptr)voidptr;
			if(cmd->offi!=Q_LLONG_MAX) {
				snprintf(string2,STRCHAR,"  %s %s %s '%%s' (%%s)\n",Q_LLI,Q_LLI,Q_LLI);
				SCMDPRINTF(1,string2,cmd->oni,cmd->offi,cmd->dti,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }
			else {
				snprintf(string2,STRCHAR,"  %s end %s '%%s' (%%s)\n",Q_LLI,Q_LLI);
				SCMDPRINTF(1,string2,cmd->oni,cmd->dti,cmd->str,scmdcode2string(scmdcmdtype(cmds,cmd),string)); }}}

	SCMDPRINTF(2,"\n");
	return; }


/* scmdwritecommands */
void scmdwritecommands(cmdssptr cmds,FILE *fptr,char *filename) {
	int i,fid,did;
	cmdptr cmd;
	char timing;

	if(!fptr) return;
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

	if(cmds->ndata) {
		fprintf(fptr,"output_data");
		for(did=0;did<cmds->ndata;did++)
			fprintf(fptr," %s",cmds->dname[did]);
		fprintf(fptr,"\n"); }

	for(i=0;i<cmds->ncmdlist;i++) {
		cmd=cmds->cmdlist[i];
		timing=cmd->timing;
		fprintf(fptr,"cmd %c",timing);
		if(strchr("baBAEe",timing));
		else if(strchr("@&",timing))
			fprintf(fptr," %g",cmd->on);
		else if(strchr("Nn",timing))
			fprintf(fptr," %g",cmd->dt);
		else if(strchr("iIj",timing))
			fprintf(fptr," %g %g %g",cmd->on,cmd->off,cmd->dt);
		else if(strchr("x",timing))
			fprintf(fptr," %g %g %g %g",cmd->on,cmd->off,cmd->dt,cmd->xt);
		fprintf(fptr," %s\n",cmd->str); }

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


/*********** Data functions *************/

/* scmdsetdnames */
int scmdsetdnames(cmdssptr cmds,char *str) {
	int did,itct,n,newmaxdata;
	char **newdname;
	listptrdd *newdata;

	if(!cmds) return 4;
	n=wordcount(str);

	if(cmds->ndata+n > cmds->maxdata) {
		newmaxdata=cmds->maxdata+n;

		newdname=(char**) calloc(newmaxdata,sizeof(char*));
		if(!newdname) return 1;
		for(did=0;did<cmds->maxdata;did++)
			newdname[did]=cmds->dname[did];
		for(;did<newmaxdata;did++)
			newdname[did]=NULL;
		for(did=cmds->maxdata;did<newmaxdata;did++) {
			newdname[did]=EmptyString();
			if(!newdname[did]) return 1; }

		newdata=(listptrdd*) calloc(newmaxdata,sizeof(listptrdd));
		if(!newdata) return 1;
		for(did=0;did<cmds->maxdata;did++)
			newdata[did]=cmds->data[did];
		for(;did<newmaxdata;did++)
			newdata[did]=NULL;

		cmds->maxdata=newmaxdata;
		free(cmds->dname);
		cmds->dname=newdname;
		free(cmds->data);
		cmds->data=newdata; }

	while(str) {
		did=cmds->ndata;
		itct=sscanf(str,"%s",cmds->dname[did]);
		if(itct!=1) return 2;
		cmds->ndata++;
		str=strnword(str,2); }

	return 0; }


/* scmdappenddata */
void scmdappenddata(cmdssptr cmds,int dataid,int newrow, int narg, ...) {
	va_list arguments;
	listptrdd data;

	if(dataid<0) return;
	va_start(arguments,narg);
	data=ListAppendItemsDDv(cmds->data[dataid],newrow,narg,arguments);
	va_end(arguments);
	if(data) cmds->data[dataid]=data;
	return; }


/************** file functions **************/	
	
/* scmdsetfroot */
int scmdsetfroot(cmdssptr cmds,const char *root) {
	if(!cmds || !root) return 1;
	strncpy(cmds->froot,root,STRCHAR-1);
	cmds->froot[STRCHAR-1]='\0';
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
	char str1[STRCHAR];
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
#ifdef COMPILE_AS_PY_MODULE
					// When built as a python module. There is no way to use scanf (probably)?
					fprintf(stderr, "File '%s' already exists. Refusing to overwrite.\n", cmds->fname[fid]);
					fprintf(stderr, "Tip: pass `overwrite=True` to `run()` method.\n");
					return 1;
#else
					// When compiled for c++ binary.
					char str2[STRCHAR];
					fprintf(stderr,"Overwrite existing output file '%s' (y/n)? ",cmds->fname[fid]);
					scanf("%s",str2);
					if(!(str2[0]=='y' || str2[0]=='Y')) return 1; 
#endif
					}}
			if(cmds->fappend[fid]) cmds->fptr[fid]=fopen(str1,"a");
			else cmds->fptr[fid]=fopen(str1,"w");
			if(!cmds->fptr[fid]) {
				SCMDPRINTF(7,"Failed to open file '%s' for writing\n",cmds->fname[fid]);
				return 1; }}}

	return 0; }


/* scmdoverwrite */
int scmdoverwrite(cmdssptr cmds,char *line2) {
	int itct,fid;
	static char fname[STRCHAR],str1[STRCHAR];

	if(!line2) return 0;
	itct=sscanf(line2,"%s",fname);
	if(itct!=1) return 0;
	if(!strcmp(fname,"stdout") || !strcmp(fname,"stderr")) return 0;

	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return 1;
	fclose(cmds->fptr[fid]);
	scmdcatfname(cmds,fid,str1);
	cmds->fptr[fid]=fopen(str1,"w");
	if(!cmds->fptr[fid]) return 2;
	return 0; }


/* scmdincfile */
int scmdincfile(cmdssptr cmds,char *line2) {
	int itct,fid;
	static char fname[STRCHAR],str1[STRCHAR];

	if(!line2) return 0;
	itct=sscanf(line2,"%s",fname);
	if(itct!=1) return 0;
	if(!strcmp(fname,"stdout") || !strcmp(fname,"stderr")) return 0;

	fid=stringfind(cmds->fname,cmds->nfile,fname);
	if(fid<0) return 1;
	fclose(cmds->fptr[fid]);
	cmds->fsuffix[fid]++;
	scmdcatfname(cmds,fid,str1);
	if(cmds->fappend[fid])
		cmds->fptr[fid]=fopen(str1,"a");
	else
		cmds->fptr[fid]=fopen(str1,"w");
	if(!cmds->fptr[fid]) return 2;
	return 0; }


/* scmdgetfptr */
int scmdgetfptr(cmdssptr cmds,char *line2,int outstyle,FILE **fptrptr,int *dataidptr) {
	int itct,fid,did;
	char name[STRCHAR];

	if(fptrptr) *fptrptr=NULL;
	if(dataidptr) *dataidptr=-1;
	if(outstyle==0) return 0;

	fid=did=-1;
	itct=0;
	if(line2)
		itct=sscanf(line2,"%s",name);
	if(itct!=1) *fptrptr=stdout;
	else if(fptrptr && !strcmp(name,"stdout")) *fptrptr=stdout;
	else if(fptrptr && !strcmp(name,"stderr")) *fptrptr=stderr;
	else {
		if(fptrptr) fid=stringfind(cmds->fname,cmds->nfile,name);
		if(dataidptr) did=stringfind(cmds->dname,cmds->ndata,name); }
	if(fptrptr && *fptrptr && outstyle&1);
	else if(fid>=0 && outstyle&1) *fptrptr=cmds->fptr[fid];
	else if(did>=0 && outstyle&2) *dataidptr=did;
	else return -1;
	if(outstyle<3) return 1;

	line2=strnword(line2,2);
	if(!line2) return 1;

	fid=did=-1;
	itct=sscanf(line2,"%s",name);
	if(itct!=1) return 1;
	else if(fptrptr && !(*fptrptr) && !strcmp(name,"stdout")) *fptrptr=stdout;
	else if(fptrptr && !(*fptrptr) && !strcmp(name,"stderr")) *fptrptr=stderr;
	else if(fptrptr && !(*fptrptr)) fid=stringfind(cmds->fname,cmds->nfile,name);
	else if(dataidptr && *dataidptr<0)	did=stringfind(cmds->dname,cmds->ndata,name);
	if(fid>=0) *fptrptr=cmds->fptr[fid];
	else if(did>=0) *dataidptr=did;
	else return 1;	// got 1 good answer, word 2 doesn't make sense

	return 2; }


/* scmdfprintf */
int scmdfprintf(cmdssptr cmds,FILE *fptr,const char *format,...) {
	char message[STRCHARLONG],newformat[STRCHAR],replacestr[STRCHAR];
	va_list arguments;
	int code;

	if(!fptr) return 0;
	strncpy(newformat,format,STRCHAR-1);
	newformat[STRCHAR-1]='\0';
	if(!cmds) {
		strstrreplace(newformat,"%,"," ",STRCHAR); }
	else {
		if(cmds->precision>=0) {
			snprintf(replacestr,STRCHAR,"%%.%ig",cmds->precision);
			strstrreplace(newformat,"%g",replacestr,STRCHAR); }
		if(cmds->outformat=='c')
			strstrreplace(newformat,"%,",",",STRCHAR);
		else
			strstrreplace(newformat,"%,"," ",STRCHAR); }
	va_start(arguments,format);
	vsnprintf(message,STRCHARLONG,newformat,arguments);
	va_end(arguments);
	code=fprintf(fptr,"%s",message);	
	return code; }


/* scmdflush */
void scmdflush(FILE *fptr) {
	if(fptr) fflush(fptr);
	return; }

