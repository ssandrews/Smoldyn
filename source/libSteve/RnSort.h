/* Steven Andrews, 12/98 and afterwards.
See documentation called RnSort_doc.doc.
Copyright 2003-2011 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __RnSort_h
#define __RnSort_h

/************************   sorting functions   *****************************/

void sortV(float *a,float *b,int n);
void sortVii(int *a,int *b,int n);
void sortVdbl(double *a,double *b,int n);
void sortCV(float *a,float *bc,int n);
void sortVliv(long int *a,void **b,int n);
void sortVoid(void **a,int n,int (*compare)(void*,void*));

/***********************   locating functions   *****************************/

int locateV(float *a,float x,int n);
int locateVdbl(double *a,double x,int n);
int locateVli(long int *a,long int x,int n);
int locateVi(int *a,int x,int n,int mode);
int locateVstr(char **a,const char *x,int n,int mode);

/*********************   interpolation functions   **************************/

float interpolate1(float *ax,float *ay,int n,int *j,float x);
double interpolate1dbl(double *ax,double *ay,int n,int *j,double x);
float interpolate1Cr(float *ax,float *ayc,int n,int *j,float x);
float interpolate1Ci(float *ax,float *ayc,int n,int *j,float x);
double cubicinterpolate1D(double *xdata,double *ydata,int n,double x);
double cubicinterpolate2D(double *xdata,double *ydata,double *zdata,int nx,int ny,double x,double y);

/***********************   resampling functions   ***************************/

void convertxV(float *ax,float *ay,float *cx,float *cy,int na,int nc);
void convertxCV(float *ax,float *ayc,float *cx,float *cyc,int na,int nc);

/************************   fitting functions   *****************************/

double fitoneparam(double *xdata,double *ydata,int nlo,int nhi,int function,double *constant);

/***********************   histogram functions   ****************************/

void setuphist(float *hist,float *scale,int hn,float low,float high);
void setuphistdbl(double *hist,double *scale,int hn,double low,double high);
int histbin(float value,float *scale,int hn);
int histbindbl(double value,double *scale,int hn);
void data2hist(float *data,int dn,char op,float *hist,float *scale,int hn);

/********************   event analysis functions   **************************/

double maxeventrateVD(double *event,double *weight,int n,double sigma,double *tptr);

/*****************   concentration profile functions   **********************/

double *cpxinitializer(int n,double *r,double rlow,double rhigh,double rjump);
double *cpxinitializec(double *r,double *c,int n,double *values,int code);
double cp1integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft);
double cp2integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft);
double cp3integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft);
void cp1diffuse(double *r,double *rdfa,double *rdfd,int n,double rmsstep,double cminfinity,double cinfinity);
void cp3diffuse(double *r,double *rdfa,double *rdfd,int n,double rmsstep,double cinfinity);
double cp1absorb(double *r,double *rdf,int n,double rabsorb);
double cp2absorb(double *r,double *rdf,int n,double rabsorb);
double cp3absorb(double *r,double *rdf,int n,double rabsorb);
void cpxaddconc(double *r,double *rdf,int n,double amount,int profile,double r1,double r2);
void cpxmassactionreact(double *rdfa,double *rdfb,int n,double rate);


#endif
