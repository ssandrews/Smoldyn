/* Steven Andrews 12/14/94.
See documentation called Rn_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */


#ifndef __Rn_h
#define __Rn_h

#include <stdlib.h>
#include <stdio.h>
#define allocV(n) ((float *) calloc(n,sizeof(float)))
#define allocM(m,n) ((float *) calloc((m)*(n),sizeof(float)))
#define freeV(a) free(a)
#define freeM(a) free(a)
#define itemV(a,i) (a[i])
#define itemM(a,n,i,j) (a[(n)*(i)+(j)])
#define setitemV(a,i,x) ((a[i])=(x))
#define setitemM(a,n,i,j,x) ((a[(n)*(i)+(j)])=(x))

int makeV(float *c,int n,char *s);
int makeM(float *c,int m,int n,char *s);
float *setstdV(float *c,int n,int k);
float *setstdM(float *c,int m,int n,int k);
float *DirCosM(float *c,float theta,float phi,float chi);
double *DirCosMD(double *c,double theta,double phi,double chi);
float *DirCosM2(float *c,float theta);
double *DirCosM2D(double *c,double theta);
float *printV(float *a,int n);
double *printVD(double *a,int n);
float *fprintV(FILE *stream,float *a,int n);
double *fprintVD(FILE *stream,double *a,int n);
float *printM(float *a,int m,int n,char s[]);
float *sprintM(float *a,int m,int n,char s[],char *t,int tn);
float minV(float *a,int n);
float maxV(float *a,int n);
double maxVD(double *a,int n,int *indx);
double minVD(double *a,int n,int *indx);
int equalV(float *a,float *b,int n);
int isevenspV(float *a,int n,float tol);
float detM(float *a,int n);
float traceM(float *a,int n);
double traceMD(double *a,int n);
int issymmetricMD(double *a,int n);
float *columnM(float *a,float *c,int m,int n,int j);
float *copyV(float *a,float *c,int n);
double *copyVD(double *a,double *c,int n);
float *copyM(float *a,float *c,int m,int n);
float *leftrotV(float *a,float *c,int n,int k);
float *sumV(float ax,float *a,float bx,float *b,float *c,int n);
double *sumVD(double ax,double *a,double bx,double *b,double *c,int n);
float *sumM(float ax,float *a,float bx,float *b,float *c,int m,int n);
float *addKV(float k,float *a,float *c,int n);
float *multKV(float k,float *a,float *c,int n);
float *divKV(float k,float *a,float *c,int n);
float *multV(float *a,float *b,float *c,int n);
float *multM(float *a,float *b,float *c,int m,int n);
float *divV(float *a,float *b,float *c,int n);
float *divM(float *a,float *b,float *c,int m,int n);
float *transM(float *a,float *c,int m,int n);
float dotVV(float *a,float *b,int n);
double dotVVD(double *a,double *b,int n);
void crossVV(float *a,float *b,float *c);
float distanceVV(float *a,float *b,int n);
double distanceVVD(double *a,double *b,int n);
float normalizeV(float *a,int n);
float normalizeVD(double *a,int n);
float averageV(float *a,int n,int p);
float *dotVM(float *a,float *b,float *c,int m,int n);
float *dotMV(float *a,float *b,float *c,int m,int n);
double *dotMVD(double *a,double *b,double *c,int m,int n);
float *dotMM(float *a,float *b,float *c,int ma,int na,int nb);
double *dotMMD(double *a,double *b,double *c,int ma,int na,int nb);
float minorM(float *a,int n,char *row,char *col);
float invM(float *a,float *c,int n);
float *deriv1V(float *a,float *c,int n);
float *deriv2V(float *a,float *c,int n);
float *integV(float *a,float *c,int n);
float *smoothV(float *a,float *c,int n,int k);
float *convolveV(float *a,float *b,float *c,int na,int nb,int nc,int bz,float left,float right);
float *correlateV(float *a,float *b,float *c,int na,int nb,int nc,int bz,float left,float right);
int histogramV(float *a,float *h,float lo,float hi,int na,int nh);
int histogramVdbl(double *a,double *h,double lo,double hi,int na,int nh);

#endif
