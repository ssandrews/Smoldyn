/* Steven Andrews, 10/6/2015
See documentation called list_doc.doc
Copyright 2015 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __list_h
#define __list_h

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

// Memory management
listptrli ListAllocLI(int max);
listptrv ListAllocV(int max);
listptrdd ListAllocDD(int maxrow,int maxcol);
void ListFreeLI(listptrli list);
void ListFreeV(listptrv list);
void ListFreeDD(listptrdd list);

// Reading lists
int ListMemberLI(const listptrli list,long int x);

// Adding elements to lists
listptrli ListReadStringLI(char *string);
listptrli ListAppendItemLI(listptrli list,long int newitem);
listptrv ListAppendItemV(listptrv list,void *newitem);
listptrdd ListAppendItemsDDv(listptrdd list, int newrow, int narg, va_list items);
listptrdd ListAppendItemsDD(listptrdd list, int newrow, int narg, ...);

// Combining lists
listptrli ListAppendListLI(listptrli list,const listptrli newstuff);
int ListRemoveListLI(listptrli list,const listptrli remove);
void ListClearDD(listptrdd list);

// List output
void ListPrintDD(listptrdd list);


#endif

