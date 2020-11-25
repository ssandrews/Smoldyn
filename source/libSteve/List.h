/* Steven Andrews, 10/6/2015
See documentation called list_doc.doc
Copyright 2015 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __list_h
#define __list_h


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

// Memory management
listptrli ListAllocLI(int max);
listptrv ListAllocV(int max);
void ListFreeLI(listptrli list);
void ListFreeV(listptrv list);

// Reading lists
int ListMemberLI(const listptrli list,long int x);

// Adding elements to lists
listptrli ListReadStringLI(char *string);
listptrli ListAppendItemLI(listptrli list,long int newitem);
listptrv ListAppendItemV(listptrv list,void *newitem);

// Combining lists
listptrli ListAppendListLI(listptrli list,const listptrli newstuff);
int ListRemoveListLI(listptrli list,const listptrli remove);


#endif

