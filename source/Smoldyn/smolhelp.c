/* Steven Andrews, started 10/5/2026.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2026 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "string2.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"

/******************************************************************************/
/************************************* Help ***********************************/
/******************************************************************************/

/* Command line help, as in "smoldyn help difc". The help text is in the table
 SmolHelpData, which is in smolhelp_data.h. That file is generated from the
 Statements and Runtime commands chapters of SmoldynManual.tex by the script
 scripts/make_smolhelp.py, so edit the manual and rerun the script rather than
 editing smolhelp_data.h. */

typedef struct smolhelpentry {
	const char *category;		// "statement" or "command"
	const char *section;		// manual section title
	int block;							// 1 for statements entered within a block
	const char *keys;				// space-separated keywords
	const char *syntax;			// syntax lines
	const char *text;				// description
	} smolhelpentry;

#include "smolhelp_data.h"

#define SMOLHELP_MAXLIST 1000
#define SMOLHELP_KEYLEN 64
#define SMOLHELP_WIDTH 79

enum SmolHelpMatch {SHMexact,SHMprefix,SHMsubstring,SHMall};


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

int smolhelpkeymatch(const char *key,int keylen,const char *topic,enum SmolHelpMatch mode);
int smolhelpentrymatch(const smolhelpentry *entry,const char *topic,enum SmolHelpMatch mode,char *keylist,int *nlist);
void smolhelpprintentry(const smolhelpentry *entry);
void smolhelpprintlist(const char *keylist,int nlist,const char *indent);
void smolhelpprintall(void);


/******************************************************************************/
/******************************* Local functions ******************************/
/******************************************************************************/

/* smolhelpkeymatch */
int smolhelpkeymatch(const char *key,int keylen,const char *topic,enum SmolHelpMatch mode) {
	int i,j,toplen;

	if(mode==SHMall) return 1;
	toplen=(int)strlen(topic);
	if(toplen==0 || toplen>keylen) return 0;
	if(mode==SHMexact && toplen!=keylen) return 0;
	for(i=0;i<=(mode==SHMsubstring?keylen-toplen:0);i++) {
		for(j=0;j<toplen && tolower((unsigned char)key[i+j])==tolower((unsigned char)topic[j]);j++);
		if(j==toplen) return 1; }
	return 0; }


/* smolhelpentrymatch */
int smolhelpentrymatch(const smolhelpentry *entry,const char *topic,enum SmolHelpMatch mode,char *keylist,int *nlist) {
	const char *key;
	int keylen,match,i;
	char name[SMOLHELP_KEYLEN];

	match=0;
	for(key=entry->keys;*key;key+=keylen) {
		while(*key==' ') key++;
		keylen=(int)strcspn(key," ");
		if(keylen==0 || !smolhelpkeymatch(key,keylen,topic,mode)) continue;
		match=1;
		if(mode==SHMprefix && keylen==(int)strlen(topic)) continue;		// list omits exact matches
		if(keylist && *nlist<SMOLHELP_MAXLIST) {
			snprintf(name,SMOLHELP_KEYLEN,"%s%.*s",entry->block?"*":"",keylen,key);
			for(i=0;i<*nlist && strcmp(keylist+i*SMOLHELP_KEYLEN,name);i++);
			if(i==*nlist) {
				strcpy(keylist+i*SMOLHELP_KEYLEN,name);
				(*nlist)++; }}}
	return match; }


/* smolhelpprintentry */
void smolhelpprintentry(const smolhelpentry *entry) {
	printf("%s%.*s  [%s%s; %s]\n",entry->block?"* ":"",(int)strcspn(entry->keys," "),entry->keys,entry->block?"block ":"",entry->category,entry->section);
	printf("%s\n\n",entry->syntax);
	if(entry->text[0]) printf("%s\n\n",entry->text);
	return; }


/* smolhelpprintlist */
void smolhelpprintlist(const char *keylist,int nlist,const char *indent) {
	int i,col,len;

	col=0;
	for(i=0;i<nlist;i++) {
		len=(int)strlen(keylist+i*SMOLHELP_KEYLEN);
		if(col>0 && col+1+len>SMOLHELP_WIDTH) {
			printf("\n");
			col=0; }
		if(col==0) col=printf("%s%s",indent,keylist+i*SMOLHELP_KEYLEN);
		else col+=printf(" %s",keylist+i*SMOLHELP_KEYLEN); }
	if(col>0) printf("\n");
	return; }


/* smolhelpprintall */
void smolhelpprintall(void) {
	static char keylist[SMOLHELP_MAXLIST*SMOLHELP_KEYLEN];
	int e,nlist;
	const char *section;

	printf("Usage: smoldyn help topic [topic ...]\n");
	printf("  where topic is a configuration file statement or runtime command.\n");
	printf("  A partial name lists all matching topics. Statements marked with * are\n");
	printf("  entered within a block, such as between start_surface and end_surface.\n");
	printf("  See SmoldynManual.pdf for complete documentation.\n");

	section=NULL;
	nlist=0;
	for(e=0;e<SMOLHELP_NENTRY;e++) {
		if(!section || strcmp(section,SmolHelpData[e].section)) {
			smolhelpprintlist(keylist,nlist,"    ");
			nlist=0;
			section=SmolHelpData[e].section;
			if(e==0 || strcmp(SmolHelpData[e].category,SmolHelpData[e-1].category))
				printf("\n%s:\n",strcmp(SmolHelpData[e].category,"command")?"Configuration file statements":"Runtime commands");
			printf("  %s\n",section); }
		smolhelpentrymatch(&SmolHelpData[e],"",SHMall,keylist,&nlist); }
	smolhelpprintlist(keylist,nlist,"    ");
	printf("\n");
	return; }


/******************************************************************************/
/******************************* Public function ******************************/
/******************************************************************************/

/* smolhelp */
int smolhelp(const char *topic) {
	static char keylist[SMOLHELP_MAXLIST*SMOLHELP_KEYLEN];
	int e,nexact,nlist,eprefix;

	if(!topic || !topic[0]) {
		smolhelpprintall();
		return 0; }
	while(*topic=='*' || *topic==' ') topic++;

	nexact=0;																	// exact matches
	for(e=0;e<SMOLHELP_NENTRY;e++)
		if(smolhelpentrymatch(&SmolHelpData[e],topic,SHMexact,NULL,NULL)) {
			smolhelpprintentry(&SmolHelpData[e]);
			nexact++; }

	nlist=0;																	// prefix matches
	eprefix=-1;
	for(e=0;e<SMOLHELP_NENTRY;e++)
		if(smolhelpentrymatch(&SmolHelpData[e],topic,SHMprefix,keylist,&nlist))
			eprefix=(eprefix==-1)?e:-2;

	if(nexact>0) {
		if(nlist>0) {
			printf("See also:\n");
			smolhelpprintlist(keylist,nlist,"  ");
			printf("\n"); }
		return 0; }

	if(eprefix>=0) {													// prefix matches one entry
		smolhelpprintentry(&SmolHelpData[eprefix]);
		return 0; }

	if(nlist>0) {
		printf("Topics that begin with '%s':\n",topic);
		smolhelpprintlist(keylist,nlist,"  ");
		printf("Enter 'smoldyn help topic' for any of these.\n\n");
		return 0; }

	for(e=0;e<SMOLHELP_NENTRY;e++)						// substring matches
		smolhelpentrymatch(&SmolHelpData[e],topic,SHMsubstring,keylist,&nlist);
	if(nlist>0) {
		printf("No help for '%s'. Topics that contain '%s':\n",topic,topic);
		smolhelpprintlist(keylist,nlist,"  ");
		printf("Enter 'smoldyn help topic' for any of these.\n\n");
		return 0; }

	printf("No help for '%s'. Enter 'smoldyn help' for a list of topics.\n\n",topic);
	return 1; }
