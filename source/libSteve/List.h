/* Steven Andrews, 10/6/2015
See documentation called list_doc.doc
Copyright 2015 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __list_h
#define __list_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

typedef struct liststructli{
	int max;
	int n;
	long int *xs;
	} *listptrli;

typedef struct liststructv{
	int max;
	int n;
	void **xs;
	} *listptrv;

typedef struct liststructdd{
	int maxrow;
	int nrow;
	int maxcol;
	int ncol;
	int nextcol;
	double *data;
	} *listptrdd;

typedef struct liststructULVD4{
	int max;
	int n;
	unsigned long long *dataul;
	void **datav;
	double **datad4;
	} *listptrULVD4;


// Memory management
listptrli ListAllocLI(int max);
listptrv ListAllocV(int max);
listptrdd ListAllocDD(int maxrow,int maxcol);
listptrULVD4 ListAllocULVD4(int max);
void ListFreeLI(listptrli list);
void ListFreeV(listptrv list);
void ListFreeDD(listptrdd list);
void ListFreeULVD4(listptrULVD4 list);

// Reading lists
int ListMemberLI(const listptrli list,long int x);

// Adding elements to lists
listptrli ListReadStringLI(char *string);
listptrli ListAppendItemLI(listptrli list,long int newitem);
listptrv ListAppendItemV(listptrv list,void *newitem);
listptrdd ListAppendItemsDDv(listptrdd list, int newrow, int narg, va_list items);
listptrdd ListAppendItemsDD(listptrdd list, int newrow, int narg, ...);
int ListInsertItemULVD4(listptrULVD4 list,unsigned long long xdataul,void *xdatav,const double *xdatad4,int mode);

// Removing elements from lists
void List_CleanULVD4(listptrULVD4 list);

// Combining lists
listptrli ListAppendListLI(listptrli list,const listptrli newstuff);
int ListRemoveListLI(listptrli list,const listptrli remove);
void ListClearDD(listptrdd list);

// List output
void ListPrintDD(listptrdd list);

#ifdef __cplusplus
}
#endif


#endif

