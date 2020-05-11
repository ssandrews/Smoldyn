/* Steven Andrews, 1/10/04.
Library for runtime command interpreter used for various simulations.
Copyright 2004-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __SimCommand_h__
#define __SimCommand_h__

#include "queue.h"
#include "stdio.h"
#include "string2.h"

#include <cstring>
#include <cstdlib>
#include <vector>

#include <fmt/core.h>
#include <fmt/printf.h>

// std::vector<std::vector<double>> data_;
std::vector<double> data_;

#define SFNCHECK(A, ...)                                 \
    if(!(A)) {                                           \
        if(erstr)                                        \
            snprintf(erstr, sizeof(erstr), __VA_ARGS__); \
        return dblnan();                                 \
    }                                                    \
    else                                                 \
        (void)0
#define SCMDCHECK(A, ...)                                          \
    if(!(A)) {                                                     \
        if(cmd)                                                    \
            snprintf(cmd->erstr, sizeof(cmd->erstr), __VA_ARGS__); \
        return CMDwarn;                                            \
    }                                                              \
    else                                                           \
        (void)0

enum CMDcode {
    CMDok,
    CMDwarn,
    CMDpause,
    CMDstop,
    CMDabort,
    CMDnone,
    CMDcontrol,
    CMDobserve,
    CMDmanipulate,
    CMDctrlORobs,
    CMDall
};

typedef struct cmdstruct {
    struct cmdsuperstruct *cmds;          // owning command superstructure
    double                 on;            // first command run time
    double                 off;           // last command run time
    double                 dt;            // time interval between commands
    double                 xt;            // multiplicative time interval
    Q_LONGLONG             oni;           // first command run iteration
    Q_LONGLONG             offi;          // last command run iteration
    Q_LONGLONG             dti;           // iterations between commands
    Q_LONGLONG             invoke;        // number of times command has run
    char *                 str;           // command string
    char *                 erstr;         // storage space for error string
    int                    i1, i2, i3;    // integers for generic use
    double                 f1, f2, f3;    // doubles for generic use
    void *                 v1, *v2, *v3;  // pointers for generic use
    void (*freefn)(struct cmdstruct *);   // free command memory
} * cmdptr;

typedef struct cmdsuperstruct {
    queue cmd;   // queue of normal run-time commands
    queue cmdi;  // queue of integer time commands
    enum CMDcode (*cmdfn)(void *, cmdptr,
        char *);             // function that runs commands
    void *  cmdfnarg;        // function argument (e.g. sim)
    int     iter;            // number of times integer commands have run
    int     maxfile;         // number of files allocated
    int     nfile;           // number of output files
    char    root[STRCHAR];   // file path
    char    froot[STRCHAR];  // more file path, used after root
    char ** fname;           // file name [fid]
    int *   fsuffix;         // file suffix [fid]
    int *   fappend;         // 0 for overwrite, 1 for append [fid]
    FILE ** fptr;            // file pointers [fid]
    double  flag;            // global command structure flag
    int     precision;       // precision for output commands
    char    outformat;       // output format, 's' or 'c'
    double *data;            // store data
} * cmdssptr;

// non-file functions
char *   scmdcode2string(enum CMDcode code, char *string);
cmdptr   scmdalloc(void);
void     scmdfree(cmdptr cmd);
cmdssptr scmdssalloc(enum CMDcode (*cmdfn)(void *, cmdptr, char *),
    void *cmdfnarg, const char *root);
void     scmdssfree(cmdssptr cmds);
int      scmdqalloc(cmdssptr cmds, int n);
int      scmdqalloci(cmdssptr cmds, int n);
int scmdaddcommand(cmdssptr cmds, char ch, double tmin, double tmax, double dt,
    double on, double off, double step, double multiplier,
    const char *commandstring);

int scmdstr2cmd(cmdssptr cmds, char *line2, double tmin, double tmax, double dt,
    char **varnames, double *varvalues, int nvar);
void         scmdpop(cmdssptr cmds, double t);
enum CMDcode scmdexecute(
    cmdssptr cmds, double time, double simdt, Q_LONGLONG iter, int donow);
enum CMDcode scmdcmdtype(cmdssptr cmds, cmdptr cmd);
int          scmdnextcmdtime(cmdssptr cmds, double time, Q_LONGLONG iter,
             enum CMDcode type, int equalok, double *timeptr, Q_LONGLONG *iterptr);
void         scmdoutput(cmdssptr cmds);
void         scmdwritecommands(cmdssptr cmds, FILE *fptr, char *filename);
void         scmdsetflag(cmdssptr cmds, double flag);
double       scmdreadflag(cmdssptr cmds);
void         scmdsetprecision(cmdssptr cmds, int precision);
int          scmdsetoutputformat(cmdssptr cmds, char *format);

// file functions
int   scmdsetfroot(cmdssptr cmds, const char *root);
int   scmdsetfnames(cmdssptr cmds, char *str, int append);
int   scmdsetfsuffix(cmdssptr cmds, const char *fname, int i);
int   scmdopenfiles(cmdssptr cmds, int overwrite);
FILE *scmdoverwrite(cmdssptr cmds, char *line2);
FILE *scmdincfile(cmdssptr cmds, char *line2);
FILE *scmdgetfptr(cmdssptr cmds, char *line2);
void  scmdflush(FILE *fptr);

// std::vector<std::vector<double>> &getData();

std::vector<double> &getData();
void                 collectdata(double *vals, size_t n);

// Collect data.

void collectdata()
{
}

template <typename... Args>
void collectdata(const std::string& x, const Args&... arg)
{
    collectdata(arg...);
}

template <typename... Args>
void collectdata(const char x, const Args&... arg)
{
    data_.push_back((double)x);
    collectdata(arg...);
}

template <typename... Args>
void collectdata(unsigned int x, const Args &... arg)
{
    data_.push_back((double)x);
    collectdata(arg...);
}

template <typename... Args>
void collectdata(unsigned long x, const Args &... arg)
{
    data_.push_back((double)x);
    collectdata(arg...);
}

template <typename... Args>
void collectdata(int x, const Args &... arg)
{
    data_.push_back((double)x);
    collectdata(arg...);
}

template <typename... Args>
void collectdata(double x, const Args &... arg)
{
    data_.push_back(x);
    collectdata(arg...);
}

/* scmdfprintf */
template <typename... Args>
int scmdfprintf(
    cmdssptr cmds, FILE *fptr, const char *format, const Args &... args)
{
    char newformat[STRCHAR], replacestr[STRCHAR];

    std::strncpy(newformat, format, STRCHAR - 1);
    newformat[STRCHAR - 1] = '\0';
    if(!cmds) {
        strstrreplace(newformat, "%,", " ", STRCHAR);
    }
    else {
        if(cmds->precision >= 0) {
            snprintf(replacestr, STRCHAR, "%%.%ig", cmds->precision);
            strstrreplace(newformat, "%g", replacestr, STRCHAR);
        }
        if(cmds->outformat == 'c')
            strstrreplace(newformat, "%,", ",", STRCHAR);
        else
            strstrreplace(newformat, "%,", " ", STRCHAR);
    }
    int r = fmt::fprintf(fptr, newformat, args...);
    collectdata(args...);
    return r;
}

#endif
