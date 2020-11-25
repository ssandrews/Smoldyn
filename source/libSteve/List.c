/* Steven Andrews, 10/6/2015
See documentation called list_doc.doc
Copyright 2015 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdlib.h>
#include "List.h"
#include "string2.h"


/****************************************************************************/
/***************************** Internal functions ***************************/
/****************************************************************************/


/* ListExpandLI */
int ListExpandLI(listptrli list,int spaces) {
	long int *newxs;
	int newmax,i,newn;

	newmax=list->max+spaces;
	if(newmax==0) {
		newxs=NULL;
		newmax=0;
		newn=0; }
	else {
		newxs=(long int*) calloc(newmax,sizeof (long int));
		if(!newxs) return 1;
		for(i=0;i<list->n && i<newmax;i++) newxs[i]=list->xs[i];
		newn=i;
		for(i=newn;i<newmax;i++) newxs[i]=0; }

	free(list->xs);
	list->xs=newxs;
	list->max=newmax;
	list->n=newn;
	return 0; }


/* ListExpandV */
int ListExpandV(listptrv list,int spaces) {
	void **newxs;
	int newmax,i,newn;

	newmax=list->max+spaces;
	if(newmax==0) {
		newxs=NULL;
		newmax=0;
		newn=0; }
	else {
		newxs=(void**) calloc(newmax,sizeof (void*));
		if(!newxs) return 1;
		for(i=0;i<list->n && i<newmax;i++) newxs[i]=list->xs[i];
		newn=i;
		for(i=newn;i<newmax;i++) newxs[i]=NULL; }

	free(list->xs);
	list->xs=newxs;
	list->max=newmax;
	list->n=newn;
	return 0; }


/****************************************************************************/
/***************************** Memory management ****************************/
/****************************************************************************/


/* ListAllocLI */
listptrli ListAllocLI(int max) {
	listptrli list;
	int er;

	list=(listptrli) malloc(sizeof(struct liststructli));
	if(!list) return NULL;
	list->max=0;
	list->n=0;
	list->xs=NULL;
	er=ListExpandLI(list,max);
	if(er) {
		ListFreeLI(list);
		return NULL; }
	return list; }


/* ListAllocV */
listptrv ListAllocV(int max) {
	listptrv list;
	int er;

	list=(listptrv) malloc(sizeof(struct liststructv));
	if(!list) return NULL;
	list->max=0;
	list->n=0;
	list->xs=NULL;
	er=ListExpandV(list,max);
	if(er) {
		ListFreeV(list);
		return NULL; }
	return list; }


/* ListFreeLI */
void ListFreeLI(listptrli list) {
	if(list) {
		free(list->xs);
		free(list); }
	return; }


/* ListFreeV */
void ListFreeV(listptrv list) {
	if(list) {
		free(list->xs);
		free(list); }
	return; }


/****************************************************************************/
/******************************* Reading lists ******************************/
/****************************************************************************/


/* ListMemberLI */
int ListMemberLI(const listptrli list,long int x) {
	int i;

	for(i=0;i<list->n;i++)
		if(list->xs[i]==x) return 1;
	return 0; }


/****************************************************************************/
/************************** Adding elements to lists ************************/
/****************************************************************************/


/* ListReadStringLI */
listptrli ListReadStringLI(char *string) {
	listptrli list;
	int n,nread;

	n=wordcount(string);
	list=ListAllocLI(n);
	if(!list) return NULL;
	nread=strreadnli(string,n,list->xs,NULL);
	if(nread!=n) {
		ListFreeLI(list);
		return NULL; }
	list->n=n;

	return list; }


/* ListAppendItemLI */
listptrli ListAppendItemLI(listptrli list,long int newitem) {
	int er;

	if(!list) {
		list=ListAllocLI(2);
		if(!list) return NULL; }
	else if(list->n==list->max) {
		er=ListExpandLI(list,list->n+1);
		if(er) return NULL; }

	list->xs[list->n]=newitem;
	list->n++;

	return list; }


/* ListAppendItemV */
listptrv ListAppendItemV(listptrv list,void *newitem) {
	int er;

	if(!newitem) return list;

	if(!list) {
		list=ListAllocV(2);
		if(!list) return NULL; }
	else if(list->n==list->max) {
		er=ListExpandV(list,list->n+1);
		if(er) return NULL; }

	list->xs[list->n]=newitem;
	list->n++;

	return list; }


/****************************************************************************/
/******************************* Combining lists ****************************/
/****************************************************************************/


/* ListAppendListLI */
listptrli ListAppendListLI(listptrli list,const listptrli newstuff) {
	int spaces,i,newn,er;

	if(!newstuff || newstuff->n==0) return list;

	newn=newstuff->n;
	if(!list) {
		list=ListAllocLI(newn);
		if(!list) return NULL; }
	else {
		spaces=newn-(list->max-list->n);		// number of spaces to allocate
		if(spaces>0) {
			er=ListExpandLI(list,spaces);
			if(er) return NULL; }}

	for(i=0;i<newn;i++)
		list->xs[list->n+i]=newstuff->xs[i];
	list->n+=newn;

	return list; }


/* ListRemoveListLI */
int ListRemoveListLI(listptrli list,const listptrli remove) {
	int i,i2,j,count;

	if(!remove || remove->n==0) return 0;

	count=0;
	for(j=0;j<remove->n;j++)
		for(i=list->n-1;i>=0;i--)
			if(list->xs[i]==remove->xs[j]) {
				for(i2=i;i2<list->n-1;i2++)
					list->xs[i2]=list->xs[i2+1];
				list->n--;
				count++;
				break; }
	return count; }



