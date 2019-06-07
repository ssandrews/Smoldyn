/* Steven Andrews, 6/08
See documentation called parse_doc.doc
Copyright 2008 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "string2.h"

#define CHECK(A) if(!(A)) {goto failure;} else (void)0
#define CHECKS(A,...)		if(!(A)) {snprintf(erstr,STRCHAR,__VA_ARGS__); goto failure;} else (void)0


/******************************************************************************/
/************************** Functions for internal use ************************/
/******************************************************************************/


/* Parse_AllocFilePtr. */
ParseFilePtr Parse_AllocFilePtr(const char *fileroot,const char *filename) {
	ParseFilePtr pfp;
	char str[STRCHAR];

	pfp=(ParseFilePtr)malloc(sizeof(struct ParseFileStruct));
	if(!pfp) return NULL;
	pfp->froot=NULL;
	pfp->fname=NULL;
	pfp->fptr=NULL;
	pfp->lctr=0;
	pfp->line=NULL;
	pfp->linecopy=NULL;
	pfp->incomment=0;
	pfp->prevfile=NULL;
	pfp->maxdef=0;
	pfp->ndef=0;
	pfp->defkey=NULL;
	pfp->defreplace=NULL;
	pfp->defgbl=NULL;
	pfp->inifdef=0;

	CHECK(pfp->froot=EmptyString());
	CHECK(pfp->fname=EmptyString());
	CHECK(pfp->line=EmptyString());
	CHECK(pfp->linecopy=EmptyString());

	if(fileroot) strncpy(pfp->froot,fileroot,STRCHAR-1);
	if(fileroot) strncpy(pfp->fname,fileroot,STRCHAR-1);
	if(filename) strncat(pfp->fname,filename,STRCHAR-1-strlen(pfp->fname));

	if(filename) {
		strncpy(str,filename,STRCHAR-1);
		str[STRCHAR-1]='\0';
		strchrreplace(str,'.','\0');
		CHECK(!Parse_AddDefine(pfp,"FILEROOT",str,0)); }

	return pfp;

 failure:
 	Parse_FreeFilePtr(pfp);
 	return NULL; }


/* Parse_FreeFilePtr. */
void Parse_FreeFilePtr(ParseFilePtr pfp) {
	int d;

	if(!pfp) return;
	if(pfp->maxdef) {
		if(pfp->defreplace) for(d=0;d<pfp->maxdef;d++) free(pfp->defreplace[d]);
		if(pfp->defkey) for(d=0;d<pfp->maxdef;d++) free(pfp->defkey[d]); }
	free(pfp->defgbl);
	free(pfp->defreplace);
	free(pfp->defkey);
	free(pfp->linecopy);
	free(pfp->line);
	free(pfp->fname);
	free(pfp->froot);
	free(pfp);
	return; }


/* Parse_ExpandDefine. */
int Parse_ExpandDefine(ParseFilePtr pfp,int maxdef) {
	char **newdefkey,**newdefrep;
	int *newdefgbl,d;

	if(!pfp || maxdef<1) return 2;

	newdefkey=NULL;
	newdefrep=NULL;
	newdefgbl=NULL;
	CHECK(newdefkey=(char**) calloc(maxdef,sizeof(char*)));
	for(d=0;d<maxdef;d++) newdefkey[d]=NULL;
	CHECK(newdefrep=(char**) calloc(maxdef,sizeof(char*)));
	for(d=0;d<maxdef;d++) newdefrep[d]=NULL;
	CHECK(newdefgbl=(int*) calloc(maxdef,sizeof(int)));
	for(d=0;d<maxdef;d++) newdefgbl[d]=0;

	for(d=0;d<pfp->ndef && d<maxdef;d++) {
		newdefkey[d]=pfp->defkey[d];
		newdefrep[d]=pfp->defreplace[d];
		newdefgbl[d]=pfp->defgbl[d]; }
	for(;d<maxdef;d++) {
		CHECK(newdefkey[d]=EmptyString());
		CHECK(newdefrep[d]=EmptyString()); }

	free(pfp->defkey);
	free(pfp->defreplace);
	free(pfp->defgbl);
	pfp->defkey=newdefkey;
	pfp->defreplace=newdefrep;
	pfp->defgbl=newdefgbl;
	pfp->ndef=(maxdef>pfp->ndef)?pfp->ndef:maxdef;
	pfp->maxdef=maxdef;
	return 0;

 failure:
	if(newdefrep) for(d=pfp->ndef;d<maxdef;d++) free(newdefrep[d]);
	if(newdefkey) for(d=pfp->ndef;d<maxdef;d++) free(newdefkey[d]);
	free(newdefgbl);
	free(newdefrep);
	free(newdefkey);
	return 1; }


/* Parse_AddDefine. */
int Parse_AddDefine(ParseFilePtr pfp,const char *key,const char *replace,int global) {
	int d,d2,er;
	unsigned int len;
	ParseFilePtr pfp1;

	d=stringfind(pfp->defkey,pfp->ndef,key);
	if(d>=0) return 2;

	if(pfp->ndef==pfp->maxdef)
		if(Parse_ExpandDefine(pfp,1+2*pfp->ndef)) return 1;
	pfp->ndef++;

	len=strlen(key);
	for(d=0;d<pfp->ndef-1;d++) {				// find location
		if(len>strlen(pfp->defkey[d])) break; }
	for(d2=pfp->ndef-2;d2>=d;d2--) {		// move others up one
		strcpy(pfp->defkey[d2+1],pfp->defkey[d2]);
		strcpy(pfp->defreplace[d2+1],pfp->defreplace[d2]);
		pfp->defgbl[d2+1]=pfp->defgbl[d2]; }

	strncpy(pfp->defkey[d],key,STRCHAR-1);	// add new item
	pfp->defkey[d][STRCHAR-1]='\0';
	if(replace) strncpy(pfp->defreplace[d],replace,STRCHAR-1);
	else pfp->defreplace[d][0]='\0';
	pfp->defreplace[d][STRCHAR-1]='\0';
	pfp->defgbl[d]=global;

	if(global) {												// add global defines to upstream files
		pfp1=pfp->prevfile;
		while(pfp1) {
			er=Parse_AddDefine(pfp1,key,replace,global);
			if(er!=2) return er;
			pfp1=pfp1->prevfile; }}

	return 0; }


/* Parse_RemoveDefine. */
int Parse_RemoveDefine(ParseFilePtr pfp,char *key) {
	int d,d2;

	if(!key) {
		pfp->ndef=0;
		return 0; }
	d=stringfind(pfp->defkey,pfp->ndef,key);
	if(d<0) return 1;

	for(d2=d;d2<pfp->ndef-1;d2++) {
		strcpy(pfp->defkey[d2],pfp->defkey[d2+1]);
		strcpy(pfp->defreplace[d2],pfp->defreplace[d2+1]);
		pfp->defgbl[d2]=pfp->defgbl[d2+1]; }
	pfp->defkey[d2][0]='\0';
	pfp->defreplace[d2][0]='\0';
	pfp->defgbl[d2]=0;
	pfp->ndef--;
	return 0; }


/* Parse_DisplayDefine */
void Parse_DisplayDefine(ParseFilePtr pfp) {
	int d;

	printf("Definitions in %s file:\n",pfp->fname);
	for(d=0;d<pfp->ndef;d++)
		printf("%s\t%s\n",pfp->defkey[d],pfp->defreplace[d]);
	return; }


/* Parse_DoDefine. */
int Parse_DoDefine(ParseFilePtr pfp) {
	int d,val,ans,offset,total;
	char *line2;
	static int recurs=0;

	line2=strnword(pfp->line,1);
	offset=line2-pfp->line;
	if(!line2) return 0;
	if(!strncmp(line2,"define",6)) return 0;
	if(!strncmp(line2,"undefine",8)) return 0;
	if(!strncmp(line2,"ifdefine",8)) return 0;
	if(!strncmp(line2,"ifundefine",10)) return 0;

	ans=0;
	total=0;
	for(d=0;d<pfp->ndef;d++) {
		val=strstrreplace(line2,pfp->defkey[d],pfp->defreplace[d],STRCHAR-offset);
		if(val<0) ans=2;
		else total+=val; }
	if(total && recurs<10) {
		recurs++;
		Parse_DoDefine(pfp); }
	else
		recurs=0;

	return ans; }


/******************************************************************************/
/************************** Functions for external use ************************/
/******************************************************************************/


/* Parse_CmdLineArg */
int Parse_CmdLineArg(int *argcptr,char **argv,ParseFilePtr pfp) {
	static int ndefine=0,maxdefine=0;
	static char **keylist=NULL,**replist=NULL;

	int argc,argc2,i,j,newmax,er;
	char **newkeylist,**newreplist,*eqchar,*str1,*str2;

	if(pfp) {																	// write any current data to pfp
		for(i=0;i<ndefine;i++) {
			er=Parse_AddDefine(pfp,keylist[i],replist[i],1);
			if(er==1) return 1; }

		for(i=0;i<maxdefine;i++) {
			free(keylist[i]);
			free(replist[i]); }
		free(keylist);
		free(replist);
		keylist=NULL;
		replist=NULL;
		ndefine=0;
		maxdefine=0; }
	
	if(argcptr && *argcptr>0 && argv) {				// there are arguments for reading
		argc=*argcptr;
		argc2=argc/2;
		if(!pfp && maxdefine-ndefine<argc2) {		// allocate space
			newmax=ndefine+argc2;
	
			CHECK(newkeylist=(char **) calloc(newmax,sizeof(char*)));
			for(i=0;i<newmax;i++) newkeylist[i]=NULL;
			for(i=0;i<maxdefine;i++) newkeylist[i]=keylist[i];
			for(;i<newmax;i++) CHECK(newkeylist[i]=EmptyString());

			CHECK(newreplist=(char **) calloc(newmax,sizeof(char *)));
			for(i=0;i<newmax;i++) newreplist[i]=NULL;
			for(i=0;i<maxdefine;i++) newreplist[i]=replist[i];
			for(;i<newmax;i++) CHECK(newreplist[i]=EmptyString());

			maxdefine=newmax;
			free(keylist);
			keylist=newkeylist;
			free(replist);
			replist=newreplist; }

		for(i=0;i<argc;i++)
			if(!strcmp(argv[i],"--define")) {
				if(argc-i<2 || !strchr(argv[i+1],'=')) return 2;
				eqchar=strchr(argv[i+1],'=');
				eqchar[0]='\0';
				eqchar++;
				if(!pfp) {
					strncpy(keylist[ndefine],argv[i+1],STRCHAR);
					strncpy(replist[ndefine],eqchar,STRCHAR);
					ndefine++; }
				else {
					er=Parse_AddDefine(pfp,argv[i+1],eqchar,1);
					CHECK(er!=1); }
				str1=argv[i];
				str2=argv[i+1];
				for(j=i;j<argc-2;j++) argv[j]=argv[j+2];
				argv[j]=str1;
				argv[j+1]=str2;
				argc-=2;
				i--; }
		*argcptr=argc; }
	
	return 0;
	
failure:
	return 1; }


/* Parse_Start. */
ParseFilePtr Parse_Start(const char *fileroot,const char *filename,char *erstr) {
	ParseFilePtr pfp;
	char string[STRCHAR];

	pfp=Parse_AllocFilePtr(fileroot,filename);
	CHECKS(pfp,"Unable to allocate memory for reading configuration file");
	pfp->fptr=fopen(pfp->fname,"r");
	if(!pfp->fptr) {
		snprintf(string,STRCHAR,"File '%s' not found\n",pfp->fname);
		Parse_FreeFilePtr(pfp);
		CHECKS(0,"%s",string); }
	return pfp;

 failure:
	return NULL; }


/* Parse_fgets */
char* Parse_fgets(char *str,int num,FILE *stream) {
	int i,i1,done;
	char ch;

	done=0;
	i1=0;
	for(i=0;i<num-1 && !done;i++) {
		i1=fgetc(stream);
		ch=(char) i1;
		if(i1==EOF || ch=='\r' || ch=='\0') {
			str[i]='\0';
			done=1; }
		else if(i1=='\n') {
			str[i]='\n';
			str[i+1]='\0';
			done=1; }
		else
			str[i]=ch; }

	if(i==1 && i1==EOF) return NULL;
	return str; }


/* Parse_ReadLine. */
int Parse_ReadLine(ParseFilePtr *pfpptr,char *word,char **line2ptr,char *erstr) {
	int itct,ans,er,skip,toolong;
	char *linetest,*line,*line2,ch;
	ParseFilePtr pfp,pfp1;

	int d;
	char str1[STRCHAR];

	CHECKS(pfpptr && word && line2ptr && erstr,"BUG: Parse_ReadLine missing parameters");
	pfp=*pfpptr;
	if(!pfp) return 2;

	ans=er=0;
	line=pfp->line;
	linetest=Parse_fgets(line,STRCHAR,pfp->fptr);
	skip=0;
	if(linetest) {																// pre-process line
		pfp->lctr++;
		strcpy(pfp->linecopy,line);
		toolong=(!strchr(line,'\n') && !strchr(line,'\0'));

		if(strchr(line,'#')) {											// single-line comment
			*strchr(line,'#')='\0';
			if(toolong) {
				while((ch=fgetc(pfp->fptr))!=EOF && ch!='\n' && ch!='\r');
				toolong=0; }}
		if(pfp->incomment) {
			skip=1;
			toolong=0;
			if(!strncmp(line,"*/",2))
				pfp->incomment=0; }
		if(!skip && pfp->inifdef) {
			skip=1;
			if(!strncmp(line,"ifdefine",8)) pfp->inifdef++;
			else if(!strncmp(line,"endif",5)) pfp->inifdef--;
			else if(pfp->inifdef==1 && !strncmp(line,"else",4)) pfp->inifdef=0; }
		if(!skip) {
			CHECKS(!toolong,"Line exceeds maximum allowable length of %i characters",STRCHAR);
			er=Parse_DoDefine(pfp);
			CHECKS(er!=2,"overflow in line due to macro substitution");
			itct=sscanf(line,"%s",word);
			line2=strnword(line,2);
			*line2ptr=line2;
			if(line2) strchrreplace(line2,'\n','\0'); }}

	if(skip);

	else if(!linetest || (itct>0 && !strcmp(word,"end_file"))) {// end_file
		pfp1=pfp->prevfile;
		fclose(pfp->fptr);
		Parse_FreeFilePtr(pfp);
		pfp=pfp1;
		*pfpptr=pfp;
		if(!pfp) return 2; }

	else if(itct<=0);															// blank line

	else if(!strcmp(word,"/*")) {									// start comment with /*
		pfp->incomment=1; }

	else if(!strcmp(word,"*/")) {									// extra */
		CHECKS(0,"*/ without a corresponding /*"); }

	else if(!strcmp(word,"read_file")) {					// read_file
		CHECKS(line2,"file name missing");
		strcutwhite(line2,2);
		pfp1=Parse_AllocFilePtr(pfp->froot,line2);
		CHECKS(pfp1,"Unable to allocate memory for reading new file");
		pfp1->prevfile=pfp;
		for(d=0;d<pfp->ndef;d++)
			if(pfp->defgbl[d]) {
				CHECKS(!Parse_AddDefine(pfp1,pfp->defkey[d],pfp->defreplace[d],1),"unable to allocate memory for global defines"); }
		pfp=pfp1;
		*pfpptr=pfp;
		pfp->fptr=fopen(pfp->fname,"r");
		CHECKS(pfp->fptr,"file %s not found in read_file",pfp->fname); }

	else if(!strcmp(word,"define")) {							// define
		CHECKS(line2,"missing define text");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"unable to read define key");
		line2=strnword(line2,2);
		if(line2)
			strcutwhite(line2,2);
		er=Parse_AddDefine(pfp,str1,line2,0);
		if(er==2) printf("Warning: 'define %s %s' is ignored because it is a re-definition\n",str1,line2);
		CHECKS(er!=1,"Out of memory adding new define"); }

	else if(!strcmp(word,"define_global")) {			// define_global
		CHECKS(line2,"missing define_global text");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"unable to read define_global key");
		line2=strnword(line2,2);
		er=Parse_AddDefine(pfp,str1,line2,1);
		if(er==2) printf("Warning: 'define %s %s' is ignored because it is a re-definition\n",str1,line2);
		CHECKS(er!=1,"Out of memory adding new define"); }

	else if(!strcmp(word,"undefine")) {						// undefine
		CHECKS(line2,"missing undefine key");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"unable to read undefine key");
		if(strcmp(str1,"all")) Parse_RemoveDefine(pfp,str1);
		else Parse_RemoveDefine(pfp,NULL); }

	else if(!strcmp(word,"ifdefine")) {						// ifdefine
		CHECKS(line2,"missing ifdefine key");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"unable to read undefine key");
		if(stringfind(pfp->defkey,pfp->ndef,str1)==-1)
			pfp->inifdef=1; }

	else if(!strcmp(word,"ifundefine")) {					// ifundefine
		CHECKS(line2,"missing ifundefine key");
		itct=sscanf(line2,"%s",str1);
		CHECKS(itct==1,"unable to read undefine key");
		if(stringfind(pfp->defkey,pfp->ndef,str1)!=-1)
			pfp->inifdef=1; }
	
	else if(!strcmp(word,"display_define")) {			// display_define
		Parse_DisplayDefine(pfp); }
		
	else if(!strcmp(word,"else")) {								// else
		pfp->inifdef=1; }

	else if(!strcmp(word,"endif"));								// endif

	else ans=1;

	return ans;

 failure:
	return 3; }


/* Parse_ReadFailure. */
int Parse_ReadFailure(ParseFilePtr pfp,char *erstr) {
	int i1;
	ParseFilePtr pfp1;

	if(!pfp) i1=0;
	else {
		i1=pfp->lctr;
		snprintf(erstr,STRCHAR,"Error reading file in line %i",i1);
		if(pfp->linecopy[0]) {
			strncat(erstr,"\nline: ",STRCHAR-1-strlen(erstr));
			if(strchr(pfp->linecopy,'\n')) *(strchr(pfp->linecopy,'\n'))='\0';
			strncat(erstr,pfp->linecopy,STRCHAR-1-strlen(erstr));
			strcpy(pfp->line,pfp->linecopy);
			Parse_DoDefine(pfp);
			if(strcmp(pfp->line,pfp->linecopy)) {
				strncat(erstr,"\nsubstituted line: ",STRCHAR-1-strlen(erstr));
				strncat(erstr,pfp->line,STRCHAR-1-strlen(erstr)); }}
		strncat(erstr,"\nfile: ",STRCHAR-1-strlen(erstr));
		strncat(erstr,pfp->fname,STRCHAR-1-strlen(erstr));

		while(pfp) {
			if(pfp->fptr) fclose(pfp->fptr);
			pfp1=pfp->prevfile;
			Parse_FreeFilePtr(pfp);
			pfp=pfp1; }}

	return i1; }
