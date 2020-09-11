/* File SurfaceParam.h, written by Steven Andrews, 2008.
This code is in the public domain.  It is not copyrighted and may not be
copyrighted.
This is a header file for SurfaceParam.c. */

#ifndef __srfparam_h
#define __srfparam_h

/***********************  FUNCTIONS FOR EXTERNAL USE  ***********************/

enum SurfParamAlgo {SPAirrTrans,SPAirrTransT,SPAirrTransQ,SPArevTrans,SPAirrAds,SPAirrAdsT,SPAirrAdsQ,SPAirrAdsEC,SPArevAds,SPArevAdsND,SPAirrDes,SPArevDes,SPAirrFlip,SPArevFlip,SPAirrDesC,SPArevAdsC};

double surfaceprob(double k1,double k2,double dt,double difc,double *p2ptr,enum SurfParamAlgo algo);
double desorbdist(double step,enum SurfParamAlgo algo);
double surfacerate(double p1,double p2,double dt,double difc,double *k2ptr,enum SurfParamAlgo algo);

/*********************  PARAMETER CALCULATION FUNCTIONS  ********************/

double lookupirrevadsorb(double value,int pfromk);
double lookuprevadsorbnd(double probon,double proboff);
double lookuprevads(double value1,double value2,int pfromk,double *ans2ptr);
double lookuprevtrans(double pf,double pb,double *kbptr);

/*******   FUNCTIONS FOR INVESTIGATING A PARTIALLY ADSORBING SURFACE  *******/

void xdfdiffuse(double *x,double *xdfa,double *xdfd,int n);
double xdfadsorb(double *x,double *xdf,int n,double probon);
void xdfdesorb(double *x,double *xdf,int n,double b,double flux);
void xdfdesorbdelta(double *x,double *xdf,int n,double b,double flux);
double xdfsteadystate(double *x,double *xdfa,double *xdfd,int n,double cs,double b,double probon,double proboff,double eps);
void xdfmaketableirrev(void);
void xdfmaketable(void);

#endif
