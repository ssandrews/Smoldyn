/* Steven Andrews, 8/7/96.
See documentation called math2_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __math2_h
#define __math2_h

/******** useful constants and macros ***********/

#define PI 3.14159265358979323846
#define SQRT2 1.41421356237
#define SQRTPI 1.7724538509
#define SQRT2PI 2.50662827462
#define SQUARE(X) ((X)*(X))
#define QUADPLUS(A,B,C) ((-(B)+sqrt((B)*(B)-4*(A)*(C)))/(2*(A)))
#define QUADMINUS(A,B,C) ((-(B)-sqrt((B)*(B)-4*(A)*(C)))/(2*(A)))

/******** integer stuff ***********/

int iseven(int x);
int is2ton(int x);
int next2ton(int x);
int isintegerD(double x);
int isinteger(float x);
int signD(double x);
int sign(float x);
int minus1to(int x);
int intpower(int n,int p);
double factorialD(int n);
float factorial(int n);
double chooseD(int n,int m);
float choose(int n,int m);
int gcomdiv(int m,int n);

/********* special functions ***********/

double sincD(double x);
float sinc(float x);
double boxD(double x);
float box(float x);
double bessj0D(double x);
float bessj0(float x);
double bessj1D(double x);
float bessj1(float x);
double bessy0D(double x);
double bessy1D(double x);
double bessi0D(double x);
double gaussD(double x,double mean,double sd);
float gauss(float x,float mean,float sd);
double gammalnD(double x);
float gammaln(float x);
double gammpD(double a,double x);
float gammp(float a,float x);
double erfnD(double x);
float erfn(float x);
double erfncD(double x);
float erfnc(float x);
double erfccD(double x);
float erfcc(float x);
double erfD(double x);
double experfcD(double x);
double erfcintegralD(double x);
float erfcintegral(float x);
double betalnD(double x1,double x2);
float betaln(float x1,float x2);
double hermiteD(double x,int n);
float hermite(float x,int n);

/******** Green's functions *********/

double diffgreen2D(double r1,double r2);

/********* interval stuff ***********/

float constrain(float x,float lo,float hi);
double reflectD(double x,double lo,double hi);
float reflect(float x,float lo,float hi);

/********* inverse functions ***********/

double inversefnD(double (*fn)(double),double y,double x1,double x2,int n);
float inversefn(float (*fn)(float),float y,float x1,float x2,int n);

/********* various utilities ***********/

double quadpts2paramsD(double *x,double *y,double *abc);
double fouriersumD(double *a,double *b,int n,double l,double x);
void radialftD(double *r,double *a,double *k,double *c,int nr,int nk);
void linefitD(double *x,double *y,int n,double *m,double *b);

/********* Hill functions ***********/

void SetHillParamD(double *hp,double a,double e,double n,double b);
double HillFnD(double *hp,double x);
void HillFnComposeD(double *hp1,double *hp2,double *hp12);
void HillFnComposeNF1D(double *hp1,double *hp2,double *hpf1,double *hpf12);

#endif
