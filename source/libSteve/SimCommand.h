/* Steven Andrews, 1/10/04.
Library for runtime command interpreter used for various simulations.
Copyright 2004-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __SimCommand_h__
#define __SimCommand_h__

#include "queue.h"
#include "stdio.h"
#include "string2.h"
#include "List.h"

#define SFNCHECK(A,...) if(!(A)) {if(erstr) snprintf(erstr,STRCHAR*sizeof(erstr),__VA_ARGS__);return dblnan();} else (void)0
#define SCMDCHECK(A,...) if(!(A)) {if(cmd) snprintf(cmd->erstr,STRCHAR*sizeof(cmd->erstr),__VA_ARGS__);return CMDwarn;} else (void)0

enum CMDcode {CMDok,CMDwarn,CMDpause,CMDstop,CMDabort,CMDnone,CMDcontrol,CMDobserve,CMDmanipulate,CMDctrlORobs,CMDall};

typedef struct cmdstruct {
	struct cmdsuperstruct *cmds;	// owning command superstructure
	struct cmdstruct *twin;				// pointer to same command elsewhere
	char timing;					// timing character (e.g. B, A, i, @, x)
	double on;						// first command run time
	double off;						// last command run time
	double dt;						// time interval between commands
	double xt;						// multiplicative time interval
	Q_LONGLONG oni;				// first command run iteration
	Q_LONGLONG offi;			// last command run iteration
	Q_LONGLONG dti;				// iterations between commands
	Q_LONGLONG invoke;		// number of times command has run
	char *str;						// command string
	char *erstr;					// storage space for error string
	int i1,i2,i3;					// integers for generic use
	double f1,f2,f3;			// doubles for generic use
	void *v1,*v2,*v3;			// pointers for generic use
	void (*freefn)(struct cmdstruct*);	// free command memory
	} *cmdptr;

typedef struct cmdsuperstruct {
	int condition;				// 0=new, 1=update time, 2=new command, 3=ready
	int maxcmdlist;				// allocated size of command list
	int ncmdlist;					// actual size of command list
	cmdptr *cmdlist;			// list of all added commands
	queue cmd;						// queue of normal run-time commands
	queue cmdi;						// queue of integer time commands
	enum CMDcode (*cmdfn)(void*,cmdptr,char*);	// function that runs commands
	void *cmdfnarg;				// function argument (e.g. sim)
	int iter;							// number of times integer commands have run
	double flag;					// global command structure flag
	int maxfile;					// number of files allocated
	int nfile;						// number of output files
	char root[STRCHAR];		// file path
	char froot[STRCHAR];	// more file path, used after root
	char **fname;					// file name [fid]
	int *fsuffix;					// file suffix [fid]
	int *fappend;					// 0 for overwrite, 1 for append [fid]
	FILE **fptr;					// file pointers [fid]
	int precision;				// precision for output commands
	char outformat;				// output format, 's' or 'c'
	int maxdata;					// number of data lists allocated
	int ndata;						// number of data lists used
	char **dname;					// data list names [did]
	listptrdd *data;			// data lists
	} *cmdssptr;

// non-file functions
cmdssptr scmdssalloc(enum CMDcode (*cmdfn)(void*,cmdptr,char*),void *cmdfnarg,const char *root);
void scmdssfree(cmdssptr cmds);
void scmdsetcondition(cmdssptr cmds,int cond,int upgrade);
int scmdaddcommand(cmdssptr cmds,char timing,double on,double off,double step,double multiplier,const char *commandstring);
int scmdstr2cmd(cmdssptr cmds,char *line2,char **varnames,double *varvalues,int nvar);
int scmdupdatecommands(cmdssptr cmds,double tmin,double tmax,double dt);
void scmdpop(cmdssptr cmds,double t);
enum CMDcode scmdexecute(cmdssptr cmds,double time,double simdt,Q_LONGLONG iter,int donow);
enum CMDcode scmdcmdtype(cmdssptr cmds,cmdptr cmd);
int scmdnextcmdtime(cmdssptr cmds,double time,Q_LONGLONG iter,enum CMDcode type,int equalok,double *timeptr,Q_LONGLONG *iterptr);
void scmdoutput(cmdssptr cmds);
void scmdwritecommands(cmdssptr cmds,FILE *fptr,char *filename);
void scmdsetflag(cmdssptr cmds,double flag);
double scmdreadflag(cmdssptr cmds);
void scmdsetcondition(cmdssptr cmds,int condition);
void scmdsetprecision(cmdssptr cmds,int precision);
int scmdsetoutputformat(cmdssptr cmds,char *format);

// data functions

int scmdsetdnames(cmdssptr cmds,char *str);
void scmdappenddata(cmdssptr cmds,int dataid,int newrow, int narg, ...);

// file functions
int scmdsetfroot(cmdssptr cmds,const char *root);
int scmdsetfnames(cmdssptr cmds,char *str,int append);
int scmdsetfsuffix(cmdssptr cmds,const char *fname,int i);
int scmdopenfiles(cmdssptr cmds,int overwrite);
int scmdoverwrite(cmdssptr cmds,char *line2);
int scmdincfile(cmdssptr cmds,char *line2);
int scmdgetfptr(cmdssptr cmds,char *line2,int outstyle,FILE **fptrptr,int *dataidptr);
int scmdfprintf(cmdssptr cmds,FILE *fptr,const char *format,...);
void scmdflush(FILE *fptr);

# endif
