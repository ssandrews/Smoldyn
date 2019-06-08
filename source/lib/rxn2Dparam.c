/* File rxn2Dparam.c, written by Steve Andrews, 2017.
This code is in the public domain.  It is not copyrighted and may not be
copyrighted. */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rxn2Dparam.h"

#define PI 3.14159265358979323846
#define SQRT2PI 2.50662827462
#define SQRT2 1.41421356237310

// Function declarations for internal use
double rxn2Dparam_bessi0D(double x);
double rxn2Dparam_erfccD(double x);
double rxn2Dparam_erfD(double x);



/* rxn2Dactrate */
double rxn2Dactrate(double step,double sigmab) {
	double ans;

	step/=sigmab;
	ans=PI;
	ans+=(2.0-exp(-1/(2.0*step*step)))*step*SQRT2PI;
	ans-=PI*(step*step+1)*rxn2Dparam_erfD(1.0/SQRT2/step);
	ans*=0.5;
	ans*=sigmab*sigmab;
	return ans; }


/******************************************************************************/
/*************    UTILITY FUNCTIONS, COPIED FROM MY MATH2.C FILE    ***********/
/******************************************************************************/


double rxn2Dparam_diffgreen2D(double r1,double r2) {
	double ax,ans;
	double y;

	if(r1==0)
		return 1.0/(2*PI)*exp(-r2*r2/2.0);
	else if(r2==0)
		return 1.0/(2*PI)*exp(-r1*r1/2.0);

	if((ax=fabs(r1*r2))<3.75) {
		y=ax/3.75;
		y*=y;
		ans=1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492+y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
		ans*=1.0/(2*PI)*exp(-(r1*r1+r2*r2)/2.0); }
	else {
		y=3.75/ax;
		ans=0.39894228+y*(0.1328592e-1+y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1+y*0.392377e-2)))))));
		ans*=1.0/(2*PI)*exp(ax-(r1*r1+r2*r2)/2.0)/sqrt(ax); }
	return ans; }


double rxn2Dparam_erfccD(double x) {
	double t,z,ans;

	z=fabs(x);
	t=1.0/(1.0+0.5*z);
	ans=t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+t*(-0.82215223+t*0.17087277)))))))));
	return x>=0.0?ans:2.0-ans; }


double rxn2Dparam_erfD(double x) {
	return 1.0-rxn2Dparam_erfccD(x); }



/******************************************************************************/
/************    FUNCTIONS FOR INVESTIGATING AN ABSORBING DISK      ***********/
/******************************************************************************/


/* rdf2Dabsorbprob */
double rdf2Dabsorbprob(double *r,double *rdf,int n,double prob) {
	int j;
	double sum,r0,r1,f0,f1;

	r0=r1=0;
	f1=rdf[0];
	sum=0;
	for(j=(r[0]==0?1:0);r1<1 && j<n;j++) {
		r0=r1;
		f0=f1;
		r1=r[j];
		f1=rdf[j];
		sum+=PI*(r1-r0)/3*(f0*(2*r0+r1)+f1*(r0+2*r1)); }
	f0=0;
	sum-=PI*(r1-r0)/3*(f0*(2*r0+r1)+f1*(r0+2*r1));
	sum*=prob;
	for(j-=2;j>=0;j--) rdf[j]*=(1.0-prob);
	return sum; }


/* rdf2Ddiffuse  */
void rdf2Ddiffuse(double *r,double *rdfa,double *rdfd,int n,double step) {
	int i,j;
	double grn,sum,f0,f1,rr,r0,r1;

	for(i=0;i<n;i++) {											// loop over r values
		rr=r[i]/step;
		r1=0;
		grn=rxn2Dparam_diffgreen2D(rr,r1);
		f1=(rdfa[0]-rdfa[i])*grn;
		sum=0;
		for(j=(r[0]==0?1:0);j<n;j++) {		// loop over r' values
			r0=r1;
			f0=f1;
			r1=r[j]/step;
			grn=rxn2Dparam_diffgreen2D(rr,r1);
			f1=(rdfa[j]-rdfa[i])*grn;
			sum+=PI*(r1-r0)/3*(f0*(2*r0+r1)+f1*(r0+2*r1)); }
		sum+=(1.0-rdfa[i])*(0.5+rxn2Dparam_erfD((rr-r[n-1]/step)/SQRT2)/2.0);
		rdfd[i]=sum+rdfa[i]; }
	return; }


/* rdf2Dreverserxn */
void rdf2Dreverserxn(double *r,double *rdf,int n,double step,double b,double flux) {
	int i;
	double rr;

	flux/=step*step;
	b/=step;
	for(i=0;i<n;i++) {
		rr=r[i]/step;
		rdf[i]+=flux*rxn2Dparam_diffgreen2D(rr,b); }
	return; }


/* rdf2Dsteadystate */
// not updated yet
double rdf2Dsteadystate(double *r,double *rdfa,double *rdfd,int n,double step,double b,double eps) {
	const int maxit=100000;
	const double maxflux=1e7;
	int i,it;
	double flux,fluxold;

	rdf2Ddiffuse(r,rdfa,rdfd,n,step);
	flux=fluxold=rdf2Dabsorbprob(r,rdfd,n,1);
	for(it=0;it<30||(it<maxit && flux<maxflux && fabs((flux-fluxold)/(fluxold+1e-20))>eps);it++) {
		fluxold=flux;
		rdf2Ddiffuse(r,rdfa,rdfd,n,step);
		if(b>=0) rdf2Dreverserxn(r,rdfd,n,step,b,flux);
		for(i=0;i<n;i++) rdfa[i]=rdfd[i];
		flux=rdf2Dabsorbprob(r,rdfa,n,1); }
	if(it>=maxit||flux>=maxflux) flux=-1;
	return flux; }


