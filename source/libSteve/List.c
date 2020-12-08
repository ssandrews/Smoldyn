/* Steven Andrews, 10/6/2015
See documentation called list_doc.doc
Copyright 2015 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdlib.h>
#include <stdio.h>
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


/* ListExpandDD */
int ListExpandDD(listptrdd list,int addrows,int addcols) {
	double *newdata;
	int newmaxrow,newmaxcol,newnrow,newncol,i,j;

	newmaxrow=list->maxrow+addrows;
	newmaxcol=list->maxcol+addcols;
	if(newmaxrow==0 || newmaxcol==0) {
		newdata=NULL;
		newmaxrow=0;
		newmaxcol=0;
		newnrow=0;
		newncol=0; }
	else {
		newdata=(double*) calloc(newmaxrow*newmaxcol,sizeof (double));
		if(!newdata) return 1;
		for(i=0;i<newmaxrow;i++)
			for(j=0;j<newmaxcol;j++) {
				if(i<list->nrow && j<list->ncol)
					newdata[i*newmaxcol+j]=list->data[i*list->maxcol+j];
				else
					newdata[i*newmaxcol+j]=0; }
		newnrow=(list->nrow<=newmaxrow) ? list->nrow:newmaxrow;
		newncol=(list->ncol<=newmaxcol) ? list->ncol:newmaxcol; }

	free(list->data);
	list->data=newdata;
	list->maxrow=newmaxrow;
	list->maxcol=newmaxcol;
	list->nrow=newnrow;
	list->ncol=newncol;
	if(list->nextcol>=list->maxcol)
		list->nextcol=list->maxcol-1;
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


/* ListAllocDD */
listptrdd ListAllocDD(int maxrow,int maxcol) {
	listptrdd list;
	int er;

	list=(listptrdd) malloc(sizeof(struct liststructdd));
	if(!list) return NULL;
	list->maxrow=0;
	list->maxcol=0;
	list->nrow=0;
	list->ncol=0;
	list->nextcol=0;
	list->data=NULL;
	er=ListExpandDD(list,maxrow,maxcol);
	if(er) {
		ListFreeDD(list);
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


/* ListFreeDD */
void ListFreeDD(listptrdd list) {
	if(list) {
		free(list->data);
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


/* ListAppendItemsDDv */
listptrdd ListAppendItemsDDv(listptrdd list, int newrow, int narg, va_list items) {
	int er,i,j,k;

	if(!narg) return list;

	er=0;
	if(!list) {
		list=ListAllocDD(1,narg);
		if(!list) return NULL; }
	else if(newrow && list->nrow==list->maxrow) // add rows, and columns if needed
		er=ListExpandDD(list,list->nrow+1,(narg>list->maxcol) ? narg-list->maxcol:0);
	else if(newrow && narg>list->maxcol)				// add columns
		er=ListExpandDD(list,0,narg-list->maxcol);
	else if(!newrow && narg+list->nextcol>list->maxcol)	// add columns
		er=ListExpandDD(list,0,narg+list->nextcol-list->maxcol);
	if(er) return NULL;

	if(newrow || list->nrow==0) {
		i=list->nrow;
		j=0;
		list->nrow++;
		if(narg>list->ncol) list->ncol=narg;
		list->nextcol=narg; }
	else {
		i=list->nrow-1;
		j=list->nextcol;
		if(j+narg>list->ncol) list->ncol=j+narg;
		list->nextcol+=narg; }

	for(k=0;k<narg;k++)
		list->data[i*list->maxcol+j+k]=va_arg(items,double);

	return list; }


/* ListAppendItemsDD */
listptrdd ListAppendItemsDD(listptrdd list, int newrow, int narg, ...) {
	va_list items;

	va_start(items,narg);
	list=ListAppendItemsDDv(list,newrow,narg,items);
	va_end(items);
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


/* ListClearDD */
void ListClearDD(listptrdd list) {
	list->nrow=0;
	list->ncol=0;
	list->nextcol=0;
	return; }


/**************************************************/
/****** List output ***************************/
/************************************************/

/* ListPrintDD */
void ListPrintDD(listptrdd list) {
	int i,j;

	printf("\n---- List -----\n");
	if(!list) {
		printf("No list\n");
		return; }
	printf("List type: dd\n");
	printf("Allocated for %i rows and %i columns\n",list->maxrow,list->maxcol);
	printf("Currently %i rows and %i columns\n",list->nrow,list->ncol);
	printf("Next column for adding to: %i\n",list->nextcol);
	printf("Data:\n");
	if(list->data)
		for(i=0;i<list->nrow;i++) {
			for(j=0;j<list->ncol;j++)
				printf(" %.3g",list->data[i*list->maxcol+j]);
			printf("\n"); }
	else
		printf("Data element is NULL\n");
	printf("--------------\n");
	return; }
