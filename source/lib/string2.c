/* Steven Andrews, 1/99
See documentation called string2_doc.doc
Copyright 2003-2007 by Steven Andrews.	This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "random2.h"
#include "string2.h"

#ifndef NAN			// NAN is defined in math.h, so should already be defined
	#include <limits>
	#define NAN std::numeric_limits<double>::quiet_NaN( );
#endif


/******************************************************************/
/************ Declarations for internal functions ****************/
/******************************************************************/

#define CHECKS(A,...)		if(!(A)) {snprintf(StrErrorString,sizeof(StrErrorString),__VA_ARGS__); goto failure;} else (void)0
#define CHECK(A)		if(!(A)) {goto failure;} else (void)0

char StrErrorString[STRCHAR];
int MathParseError=0;

int permutelex(int *seq,int n);
int allocresults(char ***resultsptr,int *maxrptr,int nchar);


/******************************************************************/
/******************* String classification ************************/
/******************************************************************/


/* strisnumber */
int strisnumber(const char *str) {
  char *ptr;
  int len;

  if(!(len=strlen(str))) return 0;
  strtod(str,&ptr);
  return(ptr==str+len); }


/* strokname */
int strokname(const char *name) {
	int ok;

	ok=1;
	ok=ok && isalpha(*name);
	for(name++;*name && ok;name++)
		ok=ok && (isalnum(*name) || *name=='_');
	return ok; }


/* strhasname */
int strhasname(const char *string,const char *name) {
	int i,len;

	len=strlen(name);
	while(*string) {
		for(i=0;i<len && *string==name[i];i++) string++;
		if(i==len && (*string=='\0' || !(isalnum(*string) || *string=='_'))) return 1;
		while(*string && (*string!=name[0] || isalnum(*(string-1)) || *(string-1)=='_')) string++; }
	return 0; }


/* strbegin */
int strbegin(const char *strshort,const char *strlong,int casesensitive) {
	int i;
	
	i=0;
	if(casesensitive)
		while(strshort[i]!='\0' && strshort[i]==strlong[i]) i++;
	else
		while(strshort[i]!='\0' && tolower(strshort[i])==tolower(strlong[i])) i++;
	if(strshort[i]=='\0' && i>0) return 1;
	return 0; }


/* strsymbolcount */
int strsymbolcount(const char *s,char c) {
	int n;
	
	n=0;
	for(;*s;s++)
		if(*s==c) n++;
	return n; }


/* strisfunctionform */
int strisfunctionform(char *str,char **parenptr) {
	int i,len,ok;
	char *ptr;

	len=strlen(str);
	if(len<3 || str[len-1]!=')') return 0;
	if(!(ptr=strchr(str+1,'('))) return 0;

	len=ptr-str;
	ok=isalpha(*str);
	for(i=1;i<len && ok;i++)
		ok=isalnum(str[i]) || str[i]=='_';
	if(parenptr) *parenptr=ptr;
	return ok; }


/******************************************************************/
/*********************** Character locating ***********************/
/******************************************************************/


/* strchrindex */
int strchrindex(const char *cs,char c,int i) {
  while(cs[i]!='\0') {
    if(cs[i]==c) return i;
    i++; }
  return -1; }


/* strrpbrk */
char *strrpbrk(char *cs,const char *ct) {
	char *s2;
	int i,n;

	if(!cs || !ct) return NULL;
	n=strlen(ct);
	for(s2=cs+strlen(cs)-1;s2>=cs;s2--)
		for(i=0;i<n;i++)
			if(*s2==ct[i]) return s2;
	return NULL; }


/* StrChrQuote */
char *StrChrQuote(char *cs,char c) {
	int qt;
	
	qt=0;
	for(;*cs;cs++)	{
		if(*cs=='"')	qt=!qt;
		else if(*cs==c && !qt) return cs;	}
	return NULL; }


/* StrChrPQuote */
int StrChrPQuote(const char *cs,char c) {
	unsigned int i,pn,qt;
	
	pn=qt=0;
	for(i=0;i<strlen(cs);i++)
		if(cs[i]=='"') qt=!qt;
		else if(cs[i]==c && !qt && !pn) return i;
		else if(cs[i]=='(' && !qt) pn++;
		else if(cs[i]==')' && !qt && pn) pn--;
		else if(cs[i]==')' && !qt) return -2;
	if(pn) return -2;
	if(qt) return -3;
	return -1; }


/* StrrChrPQuote */
int StrrChrPQuote(const char *cs,char c) {
	int i,pn,qt;
	
	pn=qt=0;
	for(i=strlen(cs)-1;i>=0;i--)
		if(cs[i]=='"') qt=!qt;
		else if(cs[i]==c && !qt && !pn) return i;
		else if(cs[i]==')' && !qt) pn++;
		else if(cs[i]=='(' && !qt && pn) pn--;
		else if(cs[i]=='(' && !qt) return -2;
	if(pn) return -2;
	if(qt) return -3;
	return -1; }


/* strChrBrackets */
int strChrBrackets(const char *string,int n,char c,const char *delimit) {
	int i,pn,bk,bc,qt,sq,ckpn,ckbk,ckbc,ckqt,cksq;
	char ch;
	
	pn=bk=bc=qt=sq=0;
	ckpn=strchr(delimit,'(')?1:0;
	ckbk=strchr(delimit,'[')?1:0;
	ckbc=strchr(delimit,'{')?1:0;
	ckqt=strchr(delimit,'"')?1:0;
	cksq=strchr(delimit,'\'')?1:0;
	if(n<0)
		n=strlen(string);
  
	for(i=0;i<n;i++) {
		ch=string[i];
		if(ch==c) {
			if(pn!=0 || bk!=0 || bc!=0 || qt!=0 || sq!=0);
			else return i; }
		else if(ckpn && ch=='(') pn++;
		else if(ckbk && ch=='[') bk++;
		else if(ckbc && ch=='{') bc++;
		else if(ckqt && ch=='"') qt=!qt;
		else if(cksq && ch=='\'') sq=!sq;
		else if(ckpn && ch==')') {if(--pn<0) return -2;}
		else if(ckbk && ch==']') {if(--bk<0) return -3;}
		else if(ckbc && ch=='}') {if(--bc<0) return -4;} }
	
	return -1; }


/* strPbrkBrackets */
int strPbrkBrackets(const char *string,int n,const char *c,const char *delimit,int imin) {
	int i,pn,bk,bc,qt,sq,ckpn,ckbk,ckbc,ckqt,cksq;
	char ch;
	
	pn=bk=bc=qt=sq=0;
	ckpn=strchr(delimit,'(')?1:0;
	ckbk=strchr(delimit,'[')?1:0;
	ckbc=strchr(delimit,'{')?1:0;
	ckqt=strchr(delimit,'"')?1:0;
	cksq=strchr(delimit,'\'')?1:0;
	if(n<0)
		n=strlen(string);
  
	for(i=0;i<n;i++) {
		ch=string[i];
		if(strchr(c,ch)) {
			if(pn!=0 || bk!=0 || bc!=0 || qt!=0 || sq!=0 || i<imin);
			else return i; }
		else if(ckpn && ch=='(') pn++;
		else if(ckbk && ch=='[') bk++;
		else if(ckbc && ch=='{') bc++;
		else if(ckqt && ch=='"') qt=!qt;
		else if(cksq && ch=='\'') sq=!sq;
		else if(ckpn && ch==')') {if(--pn<0) return -2;}
		else if(ckbk && ch==']') {if(--bk<0) return -3;}
		else if(ckbc && ch=='}') {if(--bc<0) return -4;} }
	
	return -1; }


/* strparenmatch */
int strparenmatch(const char *string,int index) {
	char ch1,ch2;
	int dir,i,count;
	
	ch1=string[index];
	if(ch1=='(') {dir=1;ch2=')';}
	else if(ch1=='[') {dir=1;ch2=']';}
	else if(ch1=='{') {dir=1;ch2='}';}
	else if(ch1==')') {dir=-1;ch2='(';}
	else if(ch1==']') {dir=-1;ch2='[';}
	else if(ch1=='}') {dir=-1;ch2='{';}
	else return -1;
	
	i=index+dir;
	count=0;
	for(;;) {
		while(i>=0 && string[i]!='\0' && string[i]!=ch1 && string[i]!=ch2) i+=dir;
		if(i<0 || string[i]=='\0') return -2;
		if(string[i]==ch1) count++;
		else if(count==0) return i;
		else count--;
		i+=dir; }
	return 0; }


/* strsymbolmatch */
int strsymbolmatch(char *str1,char *str2,const char *symbols) {
	char *s1,*s2;

	s1=str1;
	s2=str2;
	while(1) {
		s1=strpbrk(s1,symbols);
		s2=strpbrk(s2,symbols);
		if(!s1 && !s2) return 1;
		if(!s1 || !s2) return 0;
		if(*s1!=*s2) return 0;
		s1++;
		s2++; }
	return 0; }



/******************************************************************/
/************************* Word operations ************************/
/******************************************************************/


/* wordcount */
int wordcount(const char *s) {
	int n,sp;
	const char *s2;
	
	n=0;
	sp=1;
	for(s2=s;*s2;s2++) {
		if(sp && !isspace(*s2)) n++;
		sp=isspace(*s2); }
	return n; }


/* wordcountpbrk */
int wordcountpbrk(const char *s,const char *symbols) {
	int n,sp,ischar;
	const char *s2;
	
	n=0;
	sp=1;
	ischar=0;
	for(s2=s;*s2;s2++) {
		if(sp && (ischar=!strchr(symbols,*s2))) n++;
		sp=!ischar; }
	return n; }


/* strwhichword */
int strwhichword(const char *s,const char *end) {
	int n,sp;
	const char *s2;
	
	n=0;
	sp=1;
	for(s2=s;*s2 && s2!=end+1;s2++) {
		if(sp && !isspace(*s2)) n++;
		sp=isspace(*s2); }
	return n; }


/* strnword */
char *strnword(char *s,int n) {
	char *s2;

	if(!s) return NULL;
	s2=s;
	if(!isspace(*s)) n--;
	for(;n>0 && *s2;n--) {
		while(!isspace(*s2) && *s2) s2++;
		while(isspace(*s2)) s2++; }
	return *s2?s2:NULL; }


/* strnwordc */
const char *strnwordc(const char *s,int n) {
	const char *s2;

	if(!s) return NULL;
	s2=s;
	if(!isspace(*s)) n--;
	for(;n>0 && *s2;n--) {
		while(!isspace(*s2) && *s2) s2++;
		while(isspace(*s2)) s2++; }
	return *s2?s2:NULL; }


/* strnword1 */
char *strnword1(char *s,int n) {
	char *s2;

	if(!s) return NULL;
	s2=s;
	for(n--;n>0 && *s2;n--) {
		while(*s2 && *s2!=' ' && *s2!='\t') s2++;
		if(*s2) s2++; }
	if(*s2==' ' || *s2=='\t' || !*s2) return NULL;
	else return s2;}


/* strnwordend */
char *strnwordend(char *s,int n) {
	char *s2;

	if(!s) return NULL;
	s2=s;
	for(;n>0 && *s2;n--) {
		while(isspace(*s2)) s2++;
		while(!isspace(*s2) && *s2) s2++; }
	return n==0?s2:NULL; }


/* strwordcpy */
char *strwordcpy(char *destination,const char *source,int n) {
	int i;

	i=0;
	while(n) {
		while(isspace(source[i])) {
			destination[i]=source[i];
			i++; }
		while(!isspace(source[i])) {
			destination[i]=source[i];
			i++; }
		n--; }

	if(i>0 && destination[i-1]!='\0')
		destination[i]='\0';

	return destination; }


/******************************************************************/
/************************** String arrays *************************/
/******************************************************************/


/* stringfind */
// C++ supports const in nested lists but C does not
#ifdef __GNUG__
int stringfind(const char * const *slist,int n,const char *s) {
#else
int stringfind(char **slist,int n,const char *s) {
#endif
	int i;
	
	for(i=0;i<n && strcmp(slist[i],s);i++);
	return i<n?i:-1; }


/* stringnfind */
// C++ supports const in nested lists but C does not
#ifdef __GNUG__
int stringnfind(const char * const *slist,int n,const char *s,int nchar) {
#else
int stringnfind(char **slist,int n,const char *s,int nchar) {
#endif
	int i;
	
	for(i=0;i<n && strncmp(slist[i],s,nchar);i++);
	return i<n?i:-1; }


/******************************************************************/
/**************** Reading sequential items from strings ***********/
/******************************************************************/


/* strreadni */
int strreadni(char *s,int n,int *a,char **endp) {
	int i,ok;
	char *s2;
	
	s2=s;
	ok=1;
	for(i=0;i<n && ok;i++) {
		a[i]=(int) strtol(s,&s2,10);
		if(s2==s) ok=0;
		s=s2; }
	if(endp) *endp=s2;
	return ok?i:i-1; }


/* strreadnli */
int strreadnli(char *s,int n,long int *a,char **endp) {
	int i,ok;
	char *s2;
	
	s2=s;
	ok=1;
	for(i=0;i<n && ok;i++) {
		a[i]=(long int) strtol(s,&s2,10);
		if(s2==s) ok=0;
		s=s2; }
	if(endp) *endp=s2;
	return ok?i:i-1; }


/* strreadnf */
int strreadnf(char *s,int n,float *a,char **endp) {
	int i,ok;
	char *s2;
	
	s2=s;
	ok=1;
	for(i=0;i<n && ok;i++) {
		a[i]=(float) strtod(s,&s2);
		if(s2==s) ok=0;
		s=s2; }
	if(endp) *endp=s2;
	return ok?i:i-1; }


/* strreadnd */
int strreadnd(char *s,int n,double *a,char **endp) {
	int i,ok;
	char *s2;
	
	s2=s;
	ok=1;
	for(i=0;i<n && ok;i++) {
		a[i]=strtod(s,&s2);
		if(s2==s) ok=0;
		s=s2; }
	if(endp) *endp=s2;
	return ok?i:i-1; }


/* strreadns */
int strreadns(char *s,int n,char **a,char **endp) {
	int i,j;
	char *s2;
	
	s2=s;
	j=1;
	for(i=0;i<n && *s2;i++) {
		while(isspace(*s2)) s2++;
		for(j=0;!isspace(*s2) && *s2;j++) a[i][j]=*(s2++);
		if(j) a[i][j]='\0'; }
	if(endp) *endp=s2;
	return j?i:i-1; }


/******************************************************************/
/************ String copying with memory allocation ***************/
/******************************************************************/


/* EmptyString */
char *EmptyString() {
	char *s;
	int i;
	
	s=(char*) calloc(STRCHAR,sizeof(char));
	if(s)	for(i=0;i<STRCHAR;i++)	s[i]='\0';
	return s; }


/* StringCopy */
char *StringCopy(const char *s) {
	char *s2;
	int i;

	s2=(char *) calloc(strlen(s)+1,sizeof(char));
	if(!s2) return NULL;
	for(i=0;s[i];i++)	s2[i]=s[i];
	s2[i]='\0';
	return s2; }


/* PascalString */
unsigned char *PascalString(const char *s) {
	unsigned char *s2;
	int i;

	s2=(unsigned char *) calloc(strlen(s)+1,sizeof(unsigned char));
	if(!s2) return 0;
	for(i=0;s[i];i++)	s2[i+1]=s[i];
	s2[0]=(unsigned char) i;
	return s2; }


/******************************************************************/
/************ String modifying without memory allocation **********/
/******************************************************************/


/* strPreCat */
char *strPreCat(char *str,const char *cat,int start,int stop) {
	int i,n,len;
	
	n=stop-start;
	len=strlen(str);
	for(i=len+n;i>=n;i--) str[i]=str[i-n];
	for(;i>=0;i--) str[i]=cat[start+i];
	return str; }


/* strPostCat */
char *strPostCat(char *str,const char *cat,int start,int stop) {
	int i,n,len;
	
	n=stop-start;
	len=strlen(str);
	for(i=0;i<n;i++) str[len+i]=cat[start+i];
	str[len+i]='\0';
	return str; }


/* strMidCat */
char *strMidCat(char *str,int s1,int s2,const char *cat,int start,int stop) {
	int i,n,len,shift;

	if(stop<0) stop=strlen(cat);
	n=stop-start;
	shift=n-(s2-s1);
	len=strlen(str);
	if(shift>0)
		for(i=len+shift;i>=s2+shift;i--) str[i]=str[i-shift];
	else if(shift<0)
		for(i=s2+shift;i<=len+shift;i++) str[i]=str[i-shift];
	for(i=0;i<n;i++) str[i+s1]=cat[start+i];
	return str; }


/* strchrreplace */
int strchrreplace(char *str,char charfrom,char charto) {
	int n;
	char *s1;
	
	n=0;
	while((s1=strchr(str,charfrom))) {
		*s1=charto;
		n++; }
	return n; }


/* strstrreplace */
int strstrreplace(char *str,const char *strfrom,const char *strto,int max) {
	int n,diff,lento,lenfrom,over;
	char *s1,*s2;
	int i,j;
	
	if(strto) lento=strlen(strto);
	else lento=0;
	lenfrom=strlen(strfrom);
	diff=lento-lenfrom;
	over=0;
	n=0;
	s2=str;
	while((s1=strstr(s2,strfrom))) {
		i=s1-str;
		if(diff<0) {
			for(j=i+lenfrom;str[j-1] && j<max;j++)
				str[j+diff]=str[j]; }
		else if(diff>0) {
			for(j=strlen(str);j>=i+lenfrom;j--)
				if(j+diff<max) str[j+diff]=str[j];
			else over=1; }
		for(j=0;j<lento;j++)
			if(i+j<max) str[i+j]=strto[j];
			else over=1;
		if(i+lento<max) s2=s1+lento;
		else s2=str+strlen(str);
		n++; }
	if(over) return -n;
	return n; }


/* strcutwhite */
void strcutwhite(char *str,int end) {
	int i,j;
	
	if(end&2) {	// end of string
		for(i=strlen(str)-1;i>=0 && isspace(str[i]);i--);
		str[i+1]='\0'; }
	if(end&1) {	// start of string
		for(i=0;str[i]!='\0' && isspace(str[i]);i++);
		j=0;
		for(;str[i]!='\0';i++) str[j++]=str[i];
		str[j]='\0'; }
	return; }


/* strbslash2escseq */
int strbslash2escseq(char *str) {
	char *s1,*s2;
	
	s1=s2=str;
	while(*s2) {
		if(*s2=='\\') {
			s2++;
			if(*s2=='a') *s1='\a';
			else if(*s2=='b') *s1='\b';
			else if(*s2=='t') *s1='\t';
			else if(*s2=='n') *s1='\n';
			else if(*s2=='v') *s1='\v';
			else if(*s2=='f') *s1='\f';
			else if(*s2=='r') *s1='\r';
			else if(*s2=='\\') *s1='\\';
			else if(*s2=='"') *s1='\"';
			else *s1='\\'; }
		else
			*s1=*s2;
		s1++;
		s2++; }
	*s1=*s2;
	return s2-s1; }


/******************************************************************/
/**************************** Wildcards ***************************/
/******************************************************************/


/* strcharlistmatch */
int strcharlistmatch(const char *pat,const char ch,int n) {
	int p;

	if(n<0) n=strlen(pat);
	for(p=0;p<n;p++) {
		if(pat[p]=='-') {															// initial -
			if(p+1==n) return 1;												// only -
			if(ch<=pat[p+1]) return 1;									// match initial -
			p++; }
		else if(p+1<n && pat[p+1]=='-') {							// internal or terminal -
			if(p+2==n) {																// terminal -
				if(ch>=pat[p]) return 1;									// terminal - match
				return 0; }																// terminal - no match
			if(ch>=pat[p] && ch<=pat[p+2]) return 1;		// internal - match
			p+=2; }
		else if(pat[p]==ch) return 1; }								// character match
	return 0; }


/* strwildcardmatch */
int strwildcardmatch(const char *pat,const char *str) {
	int s,p,s1,p1,match,blen,brend;

	p1=s1=-1;
	s=p=0;
	while(str[s]) {
		if(pat[p]=='*') {															// star found in pat
			if(!pat[p+1]) return 1;											// terminal star, so this is a match
			p++;
			p1=p;																				// p and p1 are at '*'+1, where post-star text will resume
			s1=s; }																			// s1 is at s, where any star text will start
		else if(pat[p]=='?') {s++;p++;}								// '?' wildcard character
		else if(pat[p]=='[') {												// character list or range
			brend=strchrindex(pat,']',p);
      if(brend==-1) return -1;
      blen=brend-p-1;
			match=strcharlistmatch(pat+p+1,str[s],blen);
			if(match) {
				s++;
				p+=blen+2; }
			else if(p1>=0) goto starlabel;							// act as if this option weren't ever selected
			else return 0; }
		else if(pat[p]==str[s]) {s++;p++;}						// same character
		else if(p1>=0) {															// in star text
		starlabel:
			if(p==p1) s++;
			else {
				p=p1;
				s1++;
				s=s1; }}
		else return 0; }															// no match
	while(pat[p]=='*') p++;
	if(!pat[p]) return 1;														// match
	return 0; }																			// more pattern, so no match


/* strwildcardmatchandsub */
int strwildcardmatchandsub(const char *pat,const char *str,char *dest,int starextra) {
	int s,p,s1,p1,starpt,isub,nsub,isubstar,match,blen,d,brend,starxc;
	int maxsub=16;
	char subst[16][STRCHAR],patst[16][STRCHAR];

	p1=s1=-1;																				// p1 and s1 are pat and str indicies following star text
	p=s=0;																					// p and s are current pat and str indicies
	starpt=0;																				// starpt is star start location in str
	isub=0;																					// isub is current substitution string index
	isubstar=0;																			// isubstar is substitution string index for most recent open star
	starxc=0;																				// starxc is extra characters that were included in first star text
	while(str[s]) {																	// walk through pat and str, determining if match and recording pattern and substitution strings
		if(pat[p]=='*') {
			if(p1>=0) {																	// resolve any open star
				if(starxc<starextra) {
					starxc++;
					goto starlabel; }
				if(isub==maxsub) return -6;
				strncpy(subst[isubstar],str+starpt,s1-starpt);
				subst[isubstar][s1-starpt]='\0';
				strcpy(patst[isubstar],"**");
				p1=-1; }
			starpt=s;																		// point in str where star was reached
			if(!pat[p+1]) {															// terminal star
				s=strlen(str);
				if(isub==maxsub) return -6;
				strncpy(subst[isub],str+starpt,s-starpt);
				subst[isub][s-starpt]='\0';
				strcpy(patst[isub],"**");
				isub++;
				p++; }
			else {																			// new star, which is not terminal
				isubstar=isub++;
				p++;
				p1=p;
				s1=s; }}

		else if(pat[p]=='?') {												// '?' wildcard character
			if(isub==maxsub) return -6;
			subst[isub][0]=str[s];
			subst[isub][1]='\0';
			strcpy(patst[isub],"??");
			isub++;
			s++;
			p++; }

		else if(pat[p]=='[') {												// character list or range
      brend=strchrindex(pat,']',p);
      if(brend==-1) return -7;
      blen=brend-p-1;                             // length of text within brackets not including brackets
			match=strcharlistmatch(pat+p+1,str[s],blen);
			if(match) {
				if(isub==maxsub) return -6;
				subst[isub][0]=str[s];
				subst[isub][1]='\0';
				patst[isub][0]='[';
				strncpy(patst[isub]+1,pat+p,blen+2);
				patst[isub][blen+3]='\0';
				isub++;
				s++;
				p+=blen+2; }
			else if(p1>=0) goto starlabel;							// act as if this option weren't ever selected
			else return 0; }														// bracket pattern doesn't match and no open star, so no match

		else if(pat[p]==str[s]) {											// same character
			s++;
			p++; }

		else if(p1>=0) {															// in star text
		starlabel:
			if(p==p1) {																	// str and pat still don't match so keep going
				s++;
				s1++; }
			else {																			// str and pat did match but now don't, so try one larger star text
				isub=isubstar+1;
				p=p1;
				s1++;
				s=s1; }}

		else return 0; }															// no match

	if(starxc<starextra) return 0;									// doesn't qualify as match because extra characters weren't used

	if(p1>=0) {																			// resolve any open star
		if(isub==maxsub) return -6;
		strncpy(subst[isubstar],str+starpt,s1-starpt);
		subst[isubstar][s1-starpt]='\0';
		strcpy(patst[isubstar],"**");
		p1=-1; }
	while(pat[p]=='*') {														// any additional stars in pat represent no characters
		if(isub==maxsub) return -6;
		subst[isub][0]='\0';
		strcpy(patst[isub],"**");
		isub++;
		p++; }
	if(pat[p]) return 0;														// more text remains in pat, so no match
	nsub=isub;

// At this point, have returned 0 if no match; continuing if match.  Have nsub substitutions, recorded in substr and subpat
//	for(isub=0;isub<nsub;isub++) {									// debug code
//		printf("%i: pat='%s' subst='%s'\n",isub,patst[isub],subst[isub]); }

	if(dest) {																			// perform substitutions
		for(d=0;dest[d]!='\0';d++) {
			if(dest[d]=='*') {
				for(isub=0;isub<nsub && (patst[isub][0]=='x' || strcmp(patst[isub],"**"));isub++);
				if(isub==nsub) return -9;
				patst[isub][0]='x';
				strMidCat(dest,d,d+1,subst[isub],0,-1); }
			else if(dest[d]=='?') {
				for(isub=0;isub<nsub && (patst[isub][0]=='x' || strcmp(patst[isub],"??"));isub++);
				if(isub==nsub) return -9;
				patst[isub][0]='x';
				strMidCat(dest,d,d+1,subst[isub],0,-1); }
			else if(dest[d]=='[') {
        brend=strchrindex(dest,']',d);
        if(brend==-1) return -8;
        blen=brend-d-1;
				for(isub=0;isub<nsub && (patst[isub][0]=='x' || strncmp(patst[isub]+1,dest+d,blen+2));isub++);
				if(isub==nsub) return -9;
				patst[isub][0]='x';
				strMidCat(dest,d,d+blen+2,subst[isub],0,-1); }
			else if(dest[d]=='$') {
				if(!dest[d+1]) return -10;
				isub=((int)dest[d+1])-((int)'1');
				if(isub<0 || isub>nsub-1) return -10;
				strMidCat(dest,d,d+2,subst[isub],0,-1); }}}

	return 1; }


/* permutelex - internal function, duplicate of a Zn.c function */
int permutelex(int *seq,int n) {
	int i,j;
	int temp;
	
	i=n-1;
	while(i>0 && seq[i-1]>=seq[i]) i--;
	if(i==0) {						// input was final sequence
		i=0;
		j=n-1;
		while(i<j) {
			temp=seq[i];			// swap values at positions i and j
			seq[i]=seq[j];
			seq[j]=temp;
			i++;
			j--; }
		return 2; }
	
	j=n;
	while(seq[j-1]<=seq[i-1]) j--;
	
	temp=seq[i-1];			// swap values at positions (i-1) and (j-1)
	seq[i-1]=seq[j-1];
	seq[j-1]=temp;
	
	i++;
	j=n;
	while(i<j) {
		temp=seq[i-1];			// swap values at positions (i-1) and (j-1)
		seq[i-1]=seq[j-1];
		seq[j-1]=temp;
		i++;
		j--; }
	
	i=n-1;
	while(i>0 && seq[i-1]>=seq[i]) i--;
	if(i==0) return 1;	// at final sequence
	
	return 0; }


/* allocresults - internal function used by strexpandlogic */
int allocresults(char ***resultsptr,int *maxrptr,int nchar) {
	char **results,**newresults;
	int maxr,newmaxr,i;

	results=*resultsptr;
	maxr=*maxrptr;
	if(!results) maxr=0;

	if(nchar<0) {													// free memory
		if(results) {
			for(i=0;i<maxr;i++) free(results[i]);
			free(results);
			results=NULL; }
		maxr=0; }

	else {																// expand current list or make new one
		newmaxr=maxr*2+2;
		newresults=(char **) calloc(newmaxr,sizeof(char*));
		if(!newresults) return 1;
		for(i=0;i<newmaxr;i++) newresults[i]=NULL;
		for(i=0;i<newmaxr;i++) {
			newresults[i]=(char*) calloc(nchar,sizeof(char));
			if(!newresults[i]) return 1;
			newresults[i][0]='\0'; }
		for(i=0;i<maxr;i++) {
			strncpy(newresults[i],results[i],nchar-1);
			newresults[i][nchar-1]='\0'; }
		allocresults(resultsptr,maxrptr,-1);
		results=newresults;
		maxr=newmaxr; }

	*resultsptr=results;
	*maxrptr=maxr;
	return 0; }


/* strexpandlogic */
int strexpandlogic(const char *pat,int start,int stop,char ***resultsptr) {
	char **results;
	int isym,il,ir,lnum,rnum,i2,ires;
	char **llist,**rlist;

	results=*resultsptr=NULL;
	if(stop<0) stop=strlen(pat);

	if(stop==start) return 0;																				// empty pattern

	isym=start+strChrBrackets(pat+start,stop-start,' ',"{");				// word separator
	if(isym>=start) {																								// space operator found at i2
		if(isym==start || isym==stop-1) return -2;										// missing space operand
		lnum=strexpandlogic(pat,start,isym,&llist);
		if(lnum<0) return lnum;
		rnum=strexpandlogic(pat,isym+1,stop,&rlist);
		if(rnum<0) return rnum;
		if(lnum*rnum>0) {
			results=(char**) calloc(lnum*rnum,sizeof(char*));
			if(!results) return -1; }
		for(il=0;il<lnum;il++)
			for(ir=0;ir<rnum;ir++) {
				ires=il*rnum+ir;
				results[ires]=(char*)calloc(strlen(llist[il])+1+strlen(rlist[ir])+1,sizeof(char));
				if(!results[ires]) return -1;
				strcpy(results[ires],llist[il]);
				strcat(results[ires]," ");
				strcat(results[ires],rlist[ir]); }
		for(il=0;il<lnum;il++) free(llist[il]);
		for(ir=0;ir<rnum;ir++) free(rlist[ir]);
		free(llist);
		free(rlist);
		*resultsptr=results;
		return lnum*rnum; }

	isym=start+strChrBrackets(pat+start,stop-start,'|',"{");				// OR operator
	if(isym>=start) {																								// OR operator found at i2
		if(isym>start) {						// left operand
			lnum=strexpandlogic(pat,start,isym,&llist);
			if(lnum<0) return lnum; }
		else {											// empty left operand
			lnum=1;
			llist=(char**)malloc(sizeof(char*));
			if(!llist) return -1;
			llist[0]=(char*)malloc(sizeof(char));
			if(!llist[0]) return -1;
			llist[0][0]='\0'; }
		if(isym<stop-1) {						// right operand
			rnum=strexpandlogic(pat,isym+1,stop,&rlist);
			if(rnum<0) return rnum; }
		else {											// empty right operand
			rnum=1;
			rlist=(char**)malloc(sizeof(char*));
			if(!rlist) return -1;
			rlist[0]=(char*)malloc(sizeof(char));
			if(!rlist[0]) return -1;
			rlist[0][0]='\0'; }
		if(lnum==0 && rnum==0) return 0;
		results=(char**) calloc(lnum+rnum,sizeof(char*));
		if(!results) return -1;
		for(il=0;il<lnum;il++) results[il]=llist[il];
		for(ir=0;ir<rnum;ir++) results[lnum+ir]=rlist[ir];
		free(llist);
		free(rlist);
		*resultsptr=results;
		return lnum+rnum; }

	isym=start+strChrBrackets(pat+start,stop-start,'&',"{");				// AND operator
	if(isym>=start) {																								// AND operator found at i2
		if(isym==start || isym==stop-1) return -3;											// missing AND operand
		lnum=strexpandlogic(pat,start,isym,&llist);
		if(lnum<0) return lnum;
		rnum=strexpandlogic(pat,isym+1,stop,&rlist);
		if(rnum<0) return rnum;
		if(lnum*rnum>0) {
			results=(char**) calloc(2*lnum*rnum,sizeof(char*));
			if(!results) return -1; }
		for(il=0;il<lnum;il++)
			for(ir=0;ir<rnum;ir++) {
				ires=il*rnum+ir;
				results[ires]=(char*)calloc(strlen(llist[il])+strlen(rlist[ir])+1,sizeof(char));
				if(!results[ires]) return -1;
				strcpy(results[ires],llist[il]);
				strcat(results[ires],rlist[ir]); }
		for(il=0;il<lnum;il++)
			for(ir=0;ir<rnum;ir++) {
				ires=lnum*rnum+il*rnum+ir;
				results[ires]=(char*)calloc(strlen(llist[il])+strlen(rlist[ir])+1,sizeof(char));
				if(!results[ires]) return -1;
				strcpy(results[ires],rlist[ir]);
				strcat(results[ires],llist[il]); }
		for(il=0;il<lnum;il++) free(llist[il]);
		for(ir=0;ir<rnum;ir++) free(rlist[ir]);
		free(llist);
		free(rlist);
		*resultsptr=results;
		return 2*lnum*rnum; }

	isym=start+strChrBrackets(pat+start,stop-start,'{',"");		 // braces
	if(isym>=start) {
		i2=strparenmatch(pat,isym);
		if(i2<0) return -5;																				// no matching brace
		lnum=strexpandlogic(pat,isym+1,i2,&llist);
		if(lnum<0) return lnum;
		if(i2+1==stop) {																					// right brace at stop
			*resultsptr=llist;
			for(il=0;il<lnum;il++)
				strPreCat(llist[il],pat,start,isym);
			return lnum; }
		rnum=strexpandlogic(pat,i2+1,stop,&rlist);
		if(rnum<0) return rnum;
		if(lnum*rnum>0) {
			results=(char**) calloc(lnum*rnum,sizeof(char*));
			if(!results) return -1; }
		for(il=0;il<lnum;il++)
			for(ir=0;ir<rnum;ir++) {
				ires=il*rnum+ir;
				results[ires]=(char*)calloc(isym+strlen(llist[il])+strlen(rlist[ir])+1,sizeof(char));
				if(!results[ires]) return -1;
				strncpy(results[ires],pat+start,isym-start);
				strcat(results[ires],llist[il]);
				strcat(results[ires],rlist[ir]); }
		for(il=0;il<lnum;il++) free(llist[il]);
		for(ir=0;ir<rnum;ir++) free(rlist[ir]);
		free(llist);
		free(rlist);
		*resultsptr=results;
		return lnum*rnum; }

	results=(char**) malloc(sizeof(char*));												// no special symbols
	if(!results) return -1;
	results[0]=(char*)calloc(stop-start+1,sizeof(char));
	if(!results[0]) return -1;
	strncpy(results[0],pat+start,stop-start);
	results[0][stop-start]='\0';
	*resultsptr=results;
	return 1; }


/* strEnhWildcardMatch */
int strEnhWildcardMatch(const char *pat,const char *str) {
	static char *localpat=NULL;
	static char **results=NULL;
	static int nr=0;
	int i;

	if(!pat || !localpat || strcmp(pat,localpat)) {								// create list of patterns
		if(nr>0) {
			for(i=0;i<nr;i++) free(results[i]);
			free(results);
			results=NULL;
			nr=0; }
		if(localpat) {
			free(localpat);
			localpat=NULL; }
		if(pat) {
			localpat=(char*)calloc(strlen(pat)+1,sizeof(char));
			if(!localpat) return -1;
			strcpy(localpat,pat);
			nr=strexpandlogic(localpat,0,-1,&results);
			if(nr<0) return nr; }}

//	for(i=0;i<nr;i++) printf("%s\n",results[i]);	//?? debug

	if(str)
		for(i=0;i<nr;i++)
			if(strwildcardmatch(results[i],str)) return 1;
	
	return 0; }


/* strEnhWildcardMatchAndSub */
int strEnhWildcardMatchAndSub(const char *pat,const char *str,const char *destpat,char *dest) {
	static char *localpat=NULL,*localdestpat=NULL;
	static char **patlist=NULL,**destlist=NULL;
	static int npr=0,ndr=0,iresults=0,starextra=0;
	int i,ismatch,destwords;

	if(!pat || !localpat || strcmp(pat,localpat)) {
		if(npr>0) {																		// npr is number of pattern results, which is items in patlist
			for(i=0;i<npr;i++) free(patlist[i]);
			free(patlist);
			patlist=NULL;
			npr=0; }
		if(localpat) {
			free(localpat);
			localpat=NULL; }
		if(pat) {
			localpat=(char*)calloc(strlen(pat)+1,sizeof(char));
			if(!localpat) return -1;
			strcpy(localpat,pat);													// localpat is local copy of pat
			npr=strexpandlogic(localpat,0,-1,&patlist);		// patlist lists the same pattern as pat but with the logic expanded
			if(npr<0) return npr; }												// error occured
		iresults=0;																			// which result is being returned
		starextra=0; }

//	for(i=0;i<npr;i++) printf("%s\n",patlist[i]);	//?? debug

	if(!destpat || !localdestpat || strcmp(destpat,localdestpat)) {
		if(ndr>0) {																			// ndr is number of destination results, which is items in destlist
			for(i=0;i<ndr;i++) free(destlist[i]);
			free(destlist);
			destlist=NULL;
			ndr=0; }
		if(localdestpat) {
			free(localdestpat);
			localdestpat=NULL; }
		if(destpat) {
			localdestpat=(char*)calloc(strlen(destpat)+1,sizeof(char));
			if(!localdestpat) return -1;
			strcpy(localdestpat,destpat);
			ndr=strexpandlogic(localdestpat,0,-1,&destlist);	// destlist lists the same pattern as destpat but with logic expanded
			if(ndr<0) return ndr; }
		iresults=0;
		starextra=0; }

	if(ndr>1 && npr>1 && ndr!=npr) return -10;

	if(!str) {
		iresults=0;
		starextra=0;
		return 0; }

	destwords=wordcount(destpat);

	if((ndr==0 || ndr==1) && npr>0) {
		while(iresults<npr) {																// iresults scans of list of patterns
			if(strwildcardmatch(patlist[iresults],str)) {
				strncpy(dest,destlist?destlist[0]:"",STRCHAR-1);
				dest[STRCHAR-1]='\0';
				ismatch=strwildcardmatchandsub(patlist[iresults],str,dest,starextra);
				if(ismatch) starextra++;												// starextra looks for multiple matches when there are 2 stars
				else {
					starextra=0;
					iresults++; }
				if(ismatch && wordcount(dest)==destwords) return 1; }
			else {
				starextra=0;
				iresults++; }}}

	else if((npr==0 || npr==1) && ndr>0) {
		if(patlist?strwildcardmatch(patlist[0],str):str[0]=='\0') {
			while(iresults<ndr) {															// iresults scans of list of destinations
				strncpy(dest,destlist[iresults],STRCHAR-1);
				dest[STRCHAR-1]='\0';
				ismatch=strwildcardmatchandsub(patlist?patlist[0]:"",str,dest,starextra);
				if(ismatch) starextra++;												// starextra looks for multiple matches when there are 2 stars
				else {
					starextra=0;
					iresults++; }
				if(ismatch && wordcount(dest)==destwords) return 1; }}}

	else if(npr>0 && ndr>0) {															// ndr==npr and ndr,ndr>0
		while(iresults<npr) {																// iresults scans list of patterns
			if(strwildcardmatch(patlist[iresults],str)) {
				strncpy(dest,destlist[iresults],STRCHAR-1);
				dest[STRCHAR-1]='\0';
				ismatch=strwildcardmatchandsub(patlist[iresults],str,dest,starextra);
				if(ismatch) starextra++;												// starextra looks for multiple matches when there are 2 stars
				else {
					starextra=0;
					iresults++; }
				if(ismatch && wordcount(dest)==destwords) return 1; }
			else {
				starextra=0;
				iresults++; }}}

	iresults=0;

	return 0; }



/******************************************************************/
/***************************** Math parsing ***********************/
/******************************************************************/



/* dblnan */
double dblnan() {
  return (double)NAN; }


/* strloadmathfunctions */
int strloadmathfunctions(void) {
	double er;
	char str1[STRCHAR],str2[STRCHAR];
	double (*fnptrdd)(double);
	double (*fnptrddd)(double,double);

	er=0;
	// functions from standard library math.h
	er+=strevalfunction(strcpy(str1,"acos"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&acos),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"asin"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&asin),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"atan"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&atan),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"cos"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&cos),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"cosh"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&cosh),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"sin"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&sin),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"sinh"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&sinh),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"tan"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&tan),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"tanh"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&tanh),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"exp"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&exp),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"log"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&log),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"log10"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&log10),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"sqrt"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&sqrt),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"ceil"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&ceil),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"fabs"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&fabs),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"floor"),strcpy(str2,"dd"),NULL,(void*) (fnptrdd=&floor),NULL,NULL,0);

	er+=strevalfunction(strcpy(str1,"atan2"),strcpy(str2,"ddd"),NULL,(void*) (fnptrddd=&atan2),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"pow"),strcpy(str2,"ddd"),NULL,(void*) (fnptrddd=&pow),NULL,NULL,0);
	er+=strevalfunction(strcpy(str1,"rand"),strcpy(str2,"ddd"),NULL,(void*) (fnptrddd=&unirandCCD),NULL,NULL,0);

	return (int) er; }


/* strevalfunction */
double strevalfunction(char *expression,char *parameters,void *voidptr,void *funcptr,char **varnames,const double *varvalues,int nvar) {
	static int maxfunc=0,nfunc=0;
	static char **funclist=NULL,**paramlist=NULL;
	static void **funcptrs=NULL,**voidptrs=NULL;
	int newmaxfunc;
	char **newfunclist,**newparamlist;
	void **newfuncptrs,**newvoidptrs;
	char erstr[STRCHAR];

	int i,comma;
	double (*fnptrdd)(double);
	double (*fnptrddd)(double,double);
	double (*fnptrdves)(void*,char*,char*);

	double answer,f1,f2;
	char *s1;

	if(!expression) {
		for(i=0;i<nfunc;i++) {
			free(funclist[i]);
			free(paramlist[i]); }
		free(funclist);
		free(paramlist);
		free(funcptrs);
		free(voidptrs);
		maxfunc=0;
		nfunc=0;
		return 0; }

	if(funcptr) {
		if(nfunc==maxfunc) {
			newmaxfunc=2*maxfunc+1;
			newfunclist=(char **) calloc(newmaxfunc,sizeof(char *));
			if(!newfunclist) return 1;
			for(i=0;i<nfunc;i++)
				newfunclist[i]=funclist[i];
			for(;i<newmaxfunc;i++)
				newfunclist[i]=NULL;

			newparamlist=(char **) calloc(newmaxfunc,sizeof(char *));
			if(!newparamlist) return 1;
			for(i=0;i<nfunc;i++)
				newparamlist[i]=paramlist[i];
			for(;i<newmaxfunc;i++)
				newparamlist[i]=NULL;

			newfuncptrs=(void **) calloc(newmaxfunc,sizeof(void *));
			if(!newfuncptrs) return 1;
			for(i=0;i<nfunc;i++)
				newfuncptrs[i]=funcptrs[i];
			for(;i<newmaxfunc;i++)
				newfuncptrs[i]=NULL;

			newvoidptrs=(void **) calloc(newmaxfunc,sizeof(void *));
			if(!newvoidptrs) return 1;
			for(i=0;i<nfunc;i++)
				newvoidptrs[i]=voidptrs[i];
			for(;i<newmaxfunc;i++)
				newvoidptrs[i]=NULL;

			free(funclist);
			free(paramlist);
			free(funcptrs);
			free(voidptrs);
			funclist=newfunclist;
			paramlist=newparamlist;
			funcptrs=newfuncptrs;
			voidptrs=newvoidptrs;
			maxfunc=newmaxfunc; }

		funclist[nfunc]=StringCopy(expression);
		if(!funclist[nfunc]) return 1;
		paramlist[nfunc]=StringCopy(parameters);
		if(!paramlist[nfunc]) return 1;
		funcptrs[nfunc]=funcptr;
		voidptrs[nfunc]=voidptr;
		nfunc++;
		return 0; }

	for(i=0;i<nfunc && strcmp(expression,funclist[i]);i++);		// i is function number
	CHECKS(i<nfunc,"unknown function name");

	if(!strcmp(paramlist[i],"dd")) {
		fnptrdd=(double(*)(double)) funcptrs[i];
		f1=strmatheval(parameters,varnames,varvalues,nvar);
		answer=(*fnptrdd)(f1); }
	else if(!strcmp(paramlist[i],"ddd")) {
		fnptrddd=(double(*)(double,double)) funcptrs[i];
		CHECKS((comma=strChrBrackets(parameters,-1,',',"([{,\"'"))>0,"missing parameter");
		parameters[comma]='\0';
		f1=strmatheval(parameters,varnames,varvalues,nvar);
		parameters+=comma+1;
		f2=strmatheval(parameters,varnames,varvalues,nvar);
		answer=(*fnptrddd)(f1,f2); }
	else if(!strcmp(paramlist[i],"dves")) {
		fnptrdves=(double(*)(void*,char*,char*)) funcptrs[i];
		s1=parameters;
		answer=(*fnptrdves)(voidptrs[i],erstr,s1);
		CHECKS(answer>=0 || answer<0,"%s",erstr); }
	else
		CHECKS(0,"BUG: unknown function format");

	return answer;

 failure:
	MathParseError=1;
	return dblnan(); }


/* strmatheval */
double strmatheval(char *expression,char **varnames,const double *varvalues,int nvar) {
  static int unarysymbol=0;
  int length,i1,i2;
  double answer,term;
  char *ptr,*ptr2,ptrchar,ptr2char;

//	printf("strmatheval expression: '%s'\n",expression);	// DEBUG

  MathParseError=0;
  length=strlen(expression);
  CHECKS(length>0,"missing expression");

  if(strisnumber(expression)) {																		// number
    unarysymbol=0;
    answer=strtod(expression,NULL); }

  else if((i1=stringfind(varnames,nvar,expression))>=0) {					// variable
    unarysymbol=0;
    answer=varvalues[i1]; }

  else if(strchr("([{",expression[0]) && strparenmatch(expression,0)==length-1) {   // {[()]}
    unarysymbol=0;
    ptr=expression+length-1;
    ptrchar=*ptr;
    *ptr='\0';
    answer=strmatheval(expression+1,varnames,varvalues,nvar);
    *ptr=ptrchar; }

	else if(strisfunctionform(expression,&ptr)) {										// function
		unarysymbol=0;
		expression[length-1]='\0';
		*ptr='\0';
		ptr++;
		answer=strevalfunction(expression,ptr,NULL,NULL,varnames,varvalues,nvar);
		CHECK(answer>=0 || answer<0); }

  else if((i1=strPbrkBrackets(expression,length-1,"+-","([{",1))>0 && !strchr("^*/",expression[i1-1]) && !(strchr("Ee",expression[i1-1]) && i1>1 && strchr("0123456789",expression[i1-2]))) {  // binary + -
    unarysymbol=0;
    ptr=expression+i1;
    ptrchar=*ptr;
    *ptr='\0';
    answer=strmatheval(expression,varnames,varvalues,nvar);        // first term
    *ptr=ptrchar;
    while((i2=strPbrkBrackets(ptr+1,strlen(ptr+1)-1,"+-","([{",1))>0 && !strchr("^*/",ptr[i2]) && !(strchr("Ee",ptr[i2]) && strchr("0123456789",ptr[i2-1]))) {
      ptr2=ptr+1+i2;
      ptr2char=*ptr2;
      *ptr2='\0';
      term=strmatheval(ptr+1,varnames,varvalues,nvar);          // middle terms
      *ptr2=ptr2char;
      if(ptrchar=='+') answer=answer+term;
      else answer=answer-term;
      ptr=ptr2;
      ptrchar=ptr2char; }
    term=strmatheval(ptr+1,varnames,varvalues,nvar);            // last term
    if(ptrchar=='+') answer=answer+term;
    else answer=answer-term; }

  else if((i1=strPbrkBrackets(expression,length-1,"*/%","([{",1))>0) {  // binary * / %
    unarysymbol=0;
    ptr=expression+i1;
    ptrchar=*ptr;
    *ptr='\0';
    answer=strmatheval(expression,varnames,varvalues,nvar);     // first term
    *ptr=ptrchar;
    while((i2=strPbrkBrackets(ptr+1,strlen(ptr+1)-1,"*/%","([{",1))>0) {
      ptr2=ptr+1+i2;
      ptr2char=*ptr2;
      *ptr2='\0';
      term=strmatheval(ptr+1,varnames,varvalues,nvar);          // middle terms
      *ptr2=ptr2char;
      if(ptrchar=='*') answer=answer*term;
      else if(ptrchar=='/') {
        CHECKS(term!=0,"divide by zero");
        answer=answer/term; }
      else {
        CHECKS(term>0.5,"illegal modulo value");
        answer=(double)((long int)(answer+0.5)%(long int)(term+0.5)); }
      ptr=ptr2;
      ptrchar=ptr2char; }
    term=strmatheval(ptr+1,varnames,varvalues,nvar);            // last term
    if(ptrchar=='*') answer=answer*term;
    else if(ptrchar=='/') {
      CHECKS(term!=0,"divide by zero");
      answer=answer/term; }
    else {
      CHECKS(term>0.5,"illegal modulo value");
      answer=(double)((long int)(answer+0.5)%(long int)(term+0.5)); }}

  else if(expression[0]=='+' || expression[0]=='-') {           // unary + -
    CHECKS(!unarysymbol,"cannot have multiple preceding signs");
    unarysymbol=1;
    term=strmatheval(expression+1,varnames,varvalues,nvar);
    answer=expression[0]=='+'?term:-term; }

	else if((i1=strPbrkBrackets(expression,length-1,"^","([{",1))>0) {	// binary ^
		unarysymbol=0;
		ptr=expression+i1;
		ptrchar=*ptr;
		*ptr='\0';
		answer=strmatheval(expression,varnames,varvalues,nvar);
		*ptr=ptrchar;
		term=strmatheval(ptr+1,varnames,varvalues,nvar);
		CHECKS(answer>0 || (answer==0 && term>0) || (answer<0 && term==round(term)),"exponent error");
		answer=pow(answer,term); }

  else {
    answer=0;
    CHECKS(0,"syntax error"); }

//	printf("\t=%g\n",answer);	// DEBUG

  return answer;
failure:
	MathParseError=1;
  return dblnan(); }


/* strmathevalint */
int strmathevalint(char *expression,char **varnames,const double *varvalues,int nvar) {
	double value;

	value=strmatheval(expression,varnames,varvalues,nvar);
	return (int)round(value); }


/* strmatherror */
int strmatherror(char *string,int clear) {
	int er;

  if(string)
    strcpy(string,MathParseError?StrErrorString:"");
	er=MathParseError;
	if(clear) {
		MathParseError=0;
		StrErrorString[0]='\0'; }
  return er; }


/* strmathsscanf */
int strmathsscanf(const char *str,const char *format,char **varnames,const double *varvalues,int nvar,...) {
	va_list arguments;
	char newformat[STRCHAR],newstr[STRCHAR],expression[STRCHAR];
	const char *fmtpos1,*fmtpos2,*strpos1,*strpos2;
	int word,count,readint,valueint;
	double value;

	if(!str || !format) return 0;

	newformat[0]='\0';
	newstr[0]='\0';
	fmtpos1=format;														// position in original format string
	strpos1=str;															// position in original str string

	fmtpos2=strstr(fmtpos1,"%m");							// location of math operation to deal with

	while(fmtpos2) {
		if(*(fmtpos2+2)=='i') readint=1;
		else if(*(fmtpos2+2)=='l' && *(fmtpos2+3)=='g') readint=0;
		else CHECKS(0,"BUG: illegal string formatting argument");

		word=strwhichword(fmtpos1,fmtpos2);
		strpos2=strnwordc(strpos1,word);				// position in string with math for parsing
		if(!strpos2) break;

		strncat(newformat,fmtpos1,fmtpos2-fmtpos1);
		strncat(newstr,strpos1,strpos2-strpos1);
		sscanf(strpos2,"%s",expression);
		if(readint) {
			valueint=strmathevalint(expression,varnames,varvalues,nvar);
			if(strmatherror(NULL,0)) break;
			strcat(newformat,"%i ");
			snprintf(newstr+strlen(newstr),STRCHAR-strlen(newstr),"%i ",valueint); }
		else {
			value=strmatheval(expression,varnames,varvalues,nvar);
			if(strmatherror(NULL,0)) break;
			strcat(newformat,"%lg ");
			snprintf(newstr+strlen(newstr),STRCHAR-strlen(newstr),"%.17g ",value); }

		fmtpos1=strnwordc(fmtpos2,2);
		strpos1=strnwordc(strpos2,2);
		fmtpos2=fmtpos1?strstr(fmtpos1,"%m"):NULL; }

	if(!strmatherror(NULL,0)) {
		if(fmtpos1) strcat(newformat,fmtpos1);
		if(strpos1) strcat(newstr,strpos1); }

	va_start(arguments,nvar);
	count=vsscanf(newstr,newformat,arguments);
	va_end(arguments);

	return count;

 failure:
	return 0; }





