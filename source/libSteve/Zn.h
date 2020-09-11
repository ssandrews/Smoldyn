/* Steven Andrews, 10/17/01
Simple routines for manipulating vectors of integers.
See documentation called Zn_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __Zn_h
#define __Zn_h

#include <stdlib.h>
#include <stdio.h>
#define allocZV(n) ((int *) calloc(n,sizeof(int)))
#define freeZV(a) free(a)

int *setstdZV(int *c,int n,int k);
int *printZV(int *c,int n);
char *Zn_vect2csvstring(int *c,int n,char *string);
int *fprintZV(FILE *stream,int *c,int n);
int minZV(int *a,int n);
int maxZV(int *a,int n);
int *copyZV(int *a,int *c,int n);
int *sumZV(float ax,int *a,float bx,int *b,int *c,int n);
int *deriv1ZV(int *a,int *c,int n);
int *deriv2ZV(int *a,int *c,int n);
int productZV(int *a,int n);
int intfindZV(int *a,int n,int i);
int indx2addZV(int *indx,int *dim,int rank);
int *add2indxZV(int add,int *indx,int *dim,int rank);
int nextaddZV(int add,int *indx1,int *indx2,int *dim,int rank);
int indx2add3ZV(int *indx,int rank);
int *add2indx3ZV(int add,int *indx,int rank);
int neighborZV(int *indx,int *c,int *dim,int rank,int type,int *wrap,int *mid);
int Zn_sameset(const int *a,const int *b,int *work,int n);
void Zn_sort(int *a,int *b,int n);
int Zn_issort(int *a,int n);

/************ combinatorics ***********/

int Zn_incrementcounter(int *a,int digits,int base);
int Zn_permute(int *a,int *b,int n,int k);
int Zn_permutelex(int *seq,int n);


#endif
