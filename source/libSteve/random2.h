/* Steven Andrews, 5/12/95.
See documentation called random_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __random2_h
#define __random2_h

// Comment out the following lines if the Mersenne Twister is unavailable
#include "SFMT/SFMT.h"

#include <time.h>
#include <stdlib.h>
#include <math.h>

/* Definitions of basic random number generators */
#ifdef SFMT_H

	#define RAND_BITS 32
	#define RAND2_MAX 4294967295UL
	#define RAND_MAX_30 1073741823UL
	#define rand30() ((unsigned long int)gen_rand32()&RAND_MAX_30)

	inline static double randCCD(void) {
		return genrand_real1(); }

	inline static double randCOD(void) {
		return genrand_real2(); }

	inline static double randOCD(void) {
		return 1.0-genrand_real2(); }

	inline static double randOOD(void) {
		return genrand_real3(); }

	inline static float randCCF(void) {
		return (float)genrand_real1(); }

	inline static float randCOF(void) {
		return (float)genrand_real2(); }

	inline static float randOCF(void) {
		return (float)(1.0)-(float)genrand_real2(); }

	inline static float randOOF(void) {
		return (float)genrand_real3(); }

	inline static unsigned long int randULI(void) {
		return (unsigned long int) gen_rand32(); }

	inline static long int randomize(long int seed) {
		if(seed<0) seed=(long int) time(NULL);
		init_gen_rand((uint32_t)seed);
		return seed; }

#else

	#define RAND_BITS 30
	#define RAND_MAX_30 1073741823UL
	#define RAND2_MAX 1073741823UL
	#if RAND_MAX==32767
		#define rand30() ((unsigned long int)rand()<<15|(unsigned long int)rand())
	#elif RAND_MAX<RAND_MAX_30
		#define rand30() (((unsigned long int)rand()&32767UL)<<15|((unsigned long int)rand()&32767UL))
	#else
		#define rand30() ((unsigned long int)rand()&RAND_MAX_30)
	#endif

	inline static double randCCD(void) {
		return (double)rand30()*(1.0/RAND_MAX_30); }

	inline static double randCOD(void) {
		return (double)rand30()*(1.0/(RAND_MAX_30+1.0)); }

	inline static double randOCD(void) {
		return (1.0+(double)rand30())*(1.0/(RAND_MAX_30+1.0)); }

	inline static double randOOD(void) {
		return (1.0+(double)rand30())*(1.0/(RAND_MAX_30+2.0)); }

	inline static float randCCF(void) {
		return (float)rand()*(1.0/RAND_MAX); }

	inline static float randCOF(void) {
		return (float)rand()*(1.0/(RAND_MAX+1.0)); }

	inline static float randOCF(void) {
		return (1.0+(float)rand())*(1.0/(RAND_MAX+1.0)); }

	inline static float randOOF(void) {
		return (1.0+(float)rand())*(1.0/(RAND_MAX+2.0)); }

	inline static unsigned long int randULI(void) {
		return rand30(); }

	inline static long int randomize(long int seed) {
		if(seed<0) seed=(unsigned int) time(NULL);
		srand((unsigned int)seed);
		return seed; }

#endif


/* Higher level random functions */

inline static double unirandCCD(double lo,double hi) {
	return randCCD()*(hi-lo)+lo; }

inline static double unirandCOD(double lo,double hi) {
	return randCOD()*(hi-lo)+lo; }

inline static double unirandOCD(double lo,double hi) {
	return randOCD()*(hi-lo)+lo; }

inline static double unirandOOD(double lo,double hi) {
	return randOOD()*(hi-lo)+lo; }

inline static float unirandCCF(float lo,float hi) {
	return randCCF()*(hi-lo)+lo; }

inline static float unirandCOF(float lo,float hi) {
	return randCOF()*(hi-lo)+lo; }

inline static float unirandOCF(float lo,float hi) {
	return randOCF()*(hi-lo)+lo; }

inline static float unirandOOF(float lo,float hi) {
	return randOOF()*(hi-lo)+lo; }


inline static int signrand(void) {
	return randULI()&1?1:-1; }

inline static int coinrandD(double p) {
	return (int)(randCOD()<p); }

inline static int coinrandF(float p) {
	return (int)(randCOF()<p); }

inline static int intrand(int n) {
	return (int)(randULI()%n); }


inline static double exprandCOD(double a) {
	return -log(randOCD())*a; }

inline static double logscalerandCCD(double min,double max) {
	double loglo;
	loglo=log(min);
	return exp(randCCD()*(log(max)-loglo)+loglo); }

inline static double powrandCOD(double xmin,double power) {
	return xmin*pow(randOCD(),1.0/(1.0+power)); }

inline static double radrandcircCCD(double r) {
	return r*sqrt(randCCD()); }

inline static double radrandsphCCD(double r) {
	return r*pow(randCCD(),1.0/3.0); }

inline static float radrandsphCCF(float r) {
	return (float)r*pow(randCCF(),(float)(1.0/3.0)); }

inline static double thetarandCCD(void) {
	return acos(1.0-2.0*randCCD()); }

inline static float thetarandCCF(void) {
	return (float) acos(1.0-2.0*randCCF()); }


double unirandsumCCD(int n,double m,double s);
float unirandsumCCF(int n,float m,float s);
int intrandpD(int n,double *p);
int intrandpF(int n,float *p);
int poisrandD(double xm);
int poisrandF(float xm);
float binomialrandF(float p,int n);
double gaussrandD();
float gaussrandF();
void circlerandD(double *x,double radius);
void sphererandCCD(double *x,double radius1,double radius2);
void sphererandCCF(float *x,float radius1,float radius2);
void ballrandCCD(double *x,int dim,double radius);
void trianglerandCD(double *pt1,double *pt2,double *pt3,int dim,double *ans);
void randtableD(double *a,int n,int eq);
void randtableF(float *a,int n,int eq);
void randshuffletableD(double *a,int n);
void randshuffletableF(float *a,int n);
void randshuffletableI(int *a,int n);
void randshuffletableV(void **a,int n);
void showdist(int n,float low,float high,int bin);

#endif
