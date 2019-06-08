/* Steven Andrews, 11/01.
See documentation called string2_doc.doc
Copyright 2003-2014 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __string2_h
#define __string2_h

#define STRCHAR 256
#define STRCHARLONG 4000


/******************* String classification ************************/

int strisnumber(const char *str);
int strokname(const char *name);
int strhasname(const char *string,const char *name);
int strbegin(const char *strshort,const char *strlong,int casesensitive);
int strsymbolcount(const char *s,char c);

/*********************** Character locating ***********************/

char *strrpbrk(char *cs,const char *ct);
char *StrChrQuote(char *cs,char c);
int StrChrPQuote(const char *cs,char c);
int StrrChrPQuote(const char *cs,char c);
int strChrBrackets(const char *string,int n,char c,const char *delimit);
int strPbrkBrackets(const char *string,int n,const char *c,const char *delimit,int imin);
int strparenmatch(const char *string,int index);
int strsymbolmatch(char *str1,char *str2,const char *symbols);

/************************* Word operations ************************/

int wordcount(const char *s);
int wordcountpbrk(const char *s,const char *symbols);
int strwhichword(const char *s,const char *end);
char *strnword(char *s,int n);
const char *strnwordc(const char *s,int n);
char *strnword1(char *s,int n);
char *strnwordend(char *s,int n);
char *strwordcpy(char *destination,const char *source,int n);

/************************** String arrays *************************/

#ifdef __GNUG__
int stringfind(const char * const *slist,int n,const char *s);
int stringnfind(const char * const *slist,int n,const char *s,int nchar);
#else
int stringfind(char **slist,int n,const char *s);
int stringnfind(char **slist,int n,const char *s,int nchar);
#endif

/**************** Reading sequential items from strings ***********/

int strreadni(char *s,int n,int *a,char **endp);
int strreadnli(char *s,int n,long int *a,char **endp);
int strreadnf(char *s,int n,float *a,char **endp);
int strreadnd(char *s,int n,double *a,char **endp);
int strreadns(char *s,int n,char **a,char **endp);

/************ String copying with memory allocation ***************/

char *EmptyString();
char *StringCopy(const char *s);
unsigned char *PascalString(const char *s);

/************ String modifying without memory allocation **********/

char *strPreCat(char *str,const char *cat,int start,int stop);
char *strPostCat(char *str,const char *cat,int start,int stop);
char *strMidCat(char *str,int s1,int s2,const char *cat,int start,int stop);
int strchrreplace(char *str,char charfrom,char charto);
int strstrreplace(char *str,const char *strfrom,const char *strto,int max);
void strcutwhite(char *str,int end);
int strbslash2escseq(char *str);

/******************* Wildcards and enhanced wildcards *************/

int strcharlistmatch(const char *pat,const char ch,int n);
int strwildcardmatch(const char *pat,const char *str);
int strwildcardmatchandsub(const char *pat,const char *str,char *dest,int starextra);
int strexpandlogic(const char *pat,int start,int stop,char ***resultsptr);
int strEnhWildcardMatch(const char *pat,const char *str);
int strEnhWildcardMatchAndSub(const char *pat,const char *str,const char *destpat,char *dest);

/***************************** Math parsing ***********************/

double dblnan();
int strloadmathfunctions(void);
double strevalfunction(char *expression,char *parameters,void *voidptr,void *funcptr,char **varnames,const double *varvalues,int nvar);
double strmatheval(char *expression,char **varnames,const double *varvalues,int nvar);
int strmatherror(char *string,int clear);
int strmathsscanf(const char *str,const char *format,char **varnames,const double *varvalues,int nvar,...);

#endif
