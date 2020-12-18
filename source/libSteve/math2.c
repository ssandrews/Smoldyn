/* Steven Andrews, 8/4/1992.
See documentation called math2_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <math.h>
#include <stdlib.h>
#include <float.h>
#include "math2.h"


/********* integer stuff ***********/

int iseven(int x)	{
	return(1-abs(x)%2);	}


int is2ton(int x)	{
	return (x&-x)==x; }


int next2ton(int x)	{
	int i;
	
	if(x<0) return 0;
	if(!x) return 1;
	for(i=0;x!=1;i++)	x=x>>1;
	return x<<(i+1);	}


int isintegerD(double x) {
	return(floor(x)==x); }


int isinteger(float x) {
	return(floor(x)==x); }


int signD(double x)	{
	return((x==0)?0:(x>0)?1:-1);	}


int sign(float x)	{
	return((x==0)?0:(x>0)?1:-1);	}


int minus1to(int x)	{
	return((abs(x)%2)?(-1):1);	}


int intpower(int n,int p)	{
	int y=1;

	if(p<0) return 0;
	for(;p;p--) y*=n;
	return y;	}


double factorialD(int n)	{
	double y;
	
	y=1;
	for(;n>1;n--)	y*=n;
	return y;	}


float factorial(int n)	{
	double y;
	
	y=1;
	for(;n>1;n--)	y*=n;
	return (float)y;	}


double chooseD(int n,int m)	{
	double y;
	
	if(m>n/2) m=n-m;
	y=1;
	while(m>0)	y*=n--/m--;
	return y;	}


float choose(int n,int m)	{
	double y;
	
	if(m>n/2) m=n-m;
	y=1;
	while(m>0)	y*=n--/m--;
	return (float)y;	}


int gcomdiv(int m,int n) {
	int temp;
	
	if(m==0||n==0) return 1;
	if(m<0) m=-m;
	if(n<0) n=-n;
	while(m>0) {
		if(m<n) {temp=m;m=n;n=temp;}
		m%=n; }
	return n; }


/********* special functions ***********/

double sincD(double x)	{
	return (x==0)?1:sin(x)/x;	}


float sinc(float x)	{
	return (x==0) ? (float)(1):(float)(sin(x)/x);	}


double boxD(double x)	{
	return (fabs(x)>1)?0:1;	}


float box(float x)	{
	return (fabs(x)>1)?0:1;	}


double bessj0D(double x) { /* Copied exactly from Numerical Recipies */
	double ax,z;
	double xx,y,ans,ans1,ans2;
	
	if((ax=fabs(x))<8.0) {
		y=x*x;
		ans1=57568490574.0+y*(-13362590354.0+y*(651619640.7+y*(-11214424.18+y*(77392.33017+y*(-184.9052456)))));
		ans2=57568490411.0+y*(1029532985.0+y*(9494680.718+y*(59272.64853+y*(267.8532712+y*1.0))));
	ans=ans1/ans2; }
	else {
		z=8.0/ax;
		y=z*z;
		xx=ax-0.785398164;
		ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4+y*(-0.2073370639e-5+y*0.2093887211e-6)));
		ans2=-0.1562499995e-1+y*(0.1430488765e-3+y*(-0.6911147651e-5+y*(0.7621095161e-6-y*0.934935152e-7)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2); }
	return ans; }


float bessj0(float x) { /* Copied exactly from Numerical Recipies */
	double ax,z;
	double xx,y,ans,ans1,ans2;
	
	if((ax=fabs(x))<8.0) {
		y=x*x;
		ans1=57568490574.0+y*(-13362590354.0+y*(651619640.7+y*(-11214424.18+y*(77392.33017+y*(-184.9052456)))));
		ans2=57568490411.0+y*(1029532985.0+y*(9494680.718+y*(59272.64853+y*(267.8532712+y*1.0))));
	ans=ans1/ans2; }
	else {
		z=8.0/ax;
		y=z*z;
		xx=ax-0.785398164;
		ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4+y*(-0.2073370639e-5+y*0.2093887211e-6)));
		ans2=-0.1562499995e-1+y*(0.1430488765e-3+y*(-0.6911147651e-5+y*(0.7621095161e-6-y*0.934935152e-7)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2); }
	return ans; }


double bessj1D(double x)	{	/* Copied exactly from Numerical Recipies */
	double ax,z;
	double xx,y,ans,ans1,ans2;

	if((ax=fabs(x))<8.0) {
		y=x*x;
		ans1=x*(72362614232.0+y*(-7895059235.0+y*(242396853.1+y*(-2972611.439+y*(15704.48260+y*(-30.16036606))))));
		ans2=144725228442.0+y*(2300535178.0+y*(18583304.74+y*(99447.43394+y*(376.9991397+y*1.0))));
		ans=ans1/ans2;	}
	else	{
		z=8.0/ax;
		y=z*z;
		xx=ax-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3+y*(0.8449199096e-5+y*(-0.88228987e-6+y*0.105787412e-6)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
		if(x<0.0) ans=-ans;	}
	return ans;	}


float bessj1(float x)	{	/* Copied exactly from Numerical Recipies */
	double ax,z;
	double xx,y,ans,ans1,ans2;

	if((ax=fabs(x))<8.0) {
		y=x*x;
		ans1=x*(72362614232.0+y*(-7895059235.0+y*(242396853.1+y*(-2972611.439+y*(15704.48260+y*(-30.16036606))))));
		ans2=144725228442.0+y*(2300535178.0+y*(18583304.74+y*(99447.43394+y*(376.9991397+y*1.0))));
		ans=ans1/ans2;	}
	else	{
		z=8.0/ax;
		y=z*z;
		xx=ax-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3+y*(0.8449199096e-5+y*(-0.88228987e-6+y*0.105787412e-6)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
		if(x<0.0) ans=-ans;	}
	return ans;	}


double bessy0D(double x) { /* Copied exactly from Numerical Recipies */
  double z;
	double xx,y,ans,ans1,ans2;

	if(x<8.0) {
		y=x*x;
		ans1=-2957821389.0+y*(7062834065.0+y*(-512359803.6+y*(10879881.29+y*(-86327.92757+y*228.4622733))));
		ans2=40076544269.0+y*(745249964.8+y*(7189466.438+y*(47447.26470+y*(226.1030244+y*1.0))));
    ans=(ans1/ans2)+0.636619772*bessj0D(x)*log(x); }
	else {
		z=8.0/x;
		y=z*z;
		xx=x-0.785398164;
		ans1=1.0+y*(-0.1098628627e-2+y*(0.2734510407e-4+y*(-0.2073370639e-5+y*0.2093887211e-6)));
		ans2=-0.1562499995e-1+y*(0.1430488765e-3+y*(-0.6911147651e-5+y*(0.7621095161e-6+y*(-0.934945152e-7))));
		ans=sqrt(0.636619772/x)*(sin(xx)*ans1+z*cos(xx)*ans2); }
	return ans; }


double bessy1D(double x)	{	/* Copied exactly from Numerical Recipies */
	double z;
	double xx,y,ans,ans1,ans2;
  
	if(x<8.0) {
		y=x*x;
		ans1=x*(-0.4900604943e13+y*(0.1275274390e13+y*(-0.5153438139e11+y*(0.7349264551e9+y*(-0.4237922726e7+y*0.8511937935e4)))));
		ans2=0.2499580570e14+y*(0.4244419664e12+y*(0.3733650367e10+y*(0.2245904002e8+y*(0.1020426050e6+y*(0.3549632885e3+y)))));
		ans=(ans1/ans2)+0.636619772*(bessj1D(x)*log(x)-1.0/x); }
	else	{
		z=8.0/x;
		y=z*z;
		xx=x-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3+y*(0.8449199096e-5+y*(-0.88228987e-6+y*0.105787412e-6)));
		ans=sqrt(0.636619772/x)*(sin(xx)*ans1+z*cos(xx)*ans2); }
	return ans;	}


double bessi0D(double x) {	/* Copied exactly from Numerical Recipes */
	double ax,ans;
	double y;

	if((ax=fabs(x))<3.75) {
		y=x/3.75;
		y*=y;
		ans=1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492+y*(0.2659732+y*(0.360768e-1+y*0.45813e-2))))); }
	else {
		y=3.75/ax;
		ans=(exp(ax)/sqrt(ax))*(0.39894228+y*(0.1328592e-1+y*(0.225319e-2+y*(-0.157565e-2+y*(0.916281e-2+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1+y*0.392377e-2)))))))); }
	return ans; }


double gaussD(double x,double mean,double sd)	{
	return 1.0/(sd*SQRT2PI)*exp(-1.0/2*((x-mean)/sd*(x-mean)/sd));	}


float gauss(float x,float mean,float sd)	{
	return 1.0/(sd*SQRT2PI)*exp(-1.0/2*((x-mean)/sd*(x-mean)/sd));	}


double gammalnD(double x)	{	/* Returns natural log of gamma function, partly from Numerical Recipies and part from Math CRC */
	double sum,t;
	int j;
	static double c[6]={76.18009173,-86.50532033,24.01409822,-1.231739516,0.120858003e-2,-0.536382e-5};

	if(x==floor(x)&&x<=0) sum=DBL_MAX;					// 0 or negative integer
	else if(x==floor(x))	{											// positive integer
		sum=0;
		for(t=2;t<x-0.1;t+=1.0)	sum+=log(t);	}
	else if(x==0.5)	sum=0.572364942;						// 1/2
	else if(2*x==floor(2*x)&&x>0)	{							// other positive half integer
		sum=0.572364942;
		for(t=0.5;t<x-0.1;t+=1)	sum+=log(t);	}
	else if(2*x==floor(2*x))	{									// negative half integer
		sum=0.572364942;
		for(t=0.5;t<-x+0.1;t+=1)	sum-=log(t);	}
	else if(x<0)																// other negative
		sum=gammalnD(x+1)-log(-x);
	else	{																			// other positive
		x-=1.0;
		t=x+5.5;
		t-=(x+0.5)*log(t);
		sum=1.0;
		for(j=0;j<=5;j++)	{
			x+=1.0;
			sum+=c[j]/x;	}
		sum=-t+log(2.50662827465*sum);	}
	return(sum);	}


float gammaln(float x)	{	/* Returns natural log of gamma function, partly from Numerical Recipies and part from Math CRC */
	double sum,t;
	int j;
	static double c[6]={76.18009173,-86.50532033,24.01409822,-1.231739516,0.120858003e-2,-0.536382e-5};
	
	if(x==floor(x)&&x<=0) sum=DBL_MAX;					// 0 or negative integer
	else if(x==floor(x))	{											// positive integer
		sum=0;
		for(t=2;t<x-0.1;t+=1.0)	sum+=log(t);	}
	else if(x==0.5)	sum=0.572364942;						// 1/2
	else if(2*x==floor(2*x)&&x>0)	{							// other positive half integer
		sum=0.572364942;
		for(t=0.5;t<x-0.1;t+=1)	sum+=log(t);	}
	else if(2*x==floor(2*x))	{									// negative half integer
		sum=0.572364942;
		for(t=0.5;t<-x+0.1;t+=1)	sum-=log(t);	}
	else if(x<0)																// other negative
		sum=gammaln(x+1)-log(-x);
	else	{																			// other positive
		x-=1.0;
		t=x+5.5;
		t-=(x+0.5)*log(t);
		sum=1.0;
		for(j=0;j<=5;j++)	{
			x+=1.0;
			sum+=c[j]/x;	}
		sum=-t+log(2.50662827465*sum);	}
	return(sum);	}


double gammpD(double a,double x)	{ // Returns incomplete gamma function, partly from Numerical Recipes
	double sum,del,ap,eps;
	double gold=0,g=1,fac=1,b1=1,b0=0,anf,ana,an=0,a1,a0=1;

	eps=3e-7;
	if(x<0||a<=0) return -1;			// out of bounds
	else if(x==0) return 0;
	else if(x<a+1)	{
		ap=a;
		del=sum=1.0/a;
		while(fabs(del)>fabs(sum)*eps&&ap-a<100)	{
			ap+=1;
			del*=x/ap;
			sum+=del;	}
		return sum*exp(-x+a*log(x)-gammalnD(a));	}
	else {
		a1=x;
		for(an=1;an<100;an++)	{
			ana=an-a;
			a0=(a1+a0*ana)*fac;
			b0=(b1+b0*ana)*fac;
			anf=an*fac;
			a1=x*a0+anf*a1;
			b1=x*b0+anf*b1;
			if(a1)	{
				fac=1.0/a1;
				g=b1*fac;
				if(fabs((g-gold)/g)<eps)
					return 1.0-exp(-x+a*log(x)-gammalnD(a))*g;
				gold=g;	}}}
	return -1;	}							// shouldn't ever get here


float gammp(float a,float x)	{ // Returns incomplete gamma function, partly from Numerical Recipes
	double sum,del,ap,eps;
	double gold=0,g=1,fac=1,b1=1,b0=0,anf,ana,an=0,a1,a0=1;

	eps=3e-7;
	if(x<0||a<=0) return -1;			// out of bounds
	else if(x==0) return 0;
	else if(x<a+1)	{
		ap=a;
		del=sum=1.0/a;
		while(fabs(del)>fabs(sum)*eps&&ap-a<100)	{
			ap+=1;
			del*=x/ap;
			sum+=del;	}
		return sum*exp(-x+a*log(x)-gammaln(a));	}
	else {
		a1=x;
		for(an=1;an<100;an++)	{
			ana=an-a;
			a0=(a1+a0*ana)*fac;
			b0=(b1+b0*ana)*fac;
			anf=an*fac;
			a1=x*a0+anf*a1;
			b1=x*b0+anf*b1;
			if(a1)	{
				fac=1.0/a1;
				g=b1*fac;
				if(fabs((g-gold)/g)<eps)
					return 1.0-exp(-x+a*log(x)-gammaln(a))*g;
				gold=g;	}}}
	return -1;	}							// shouldn't ever get here


double erfnD(double x)	{				// Numerical Recipies
	return (x<0?-gammpD(0.5,x*x):gammpD(0.5,x*x));	}


float erfn(float x)	{				// Numerical Recipies
	return (x<0?-gammp(0.5,x*x):gammp(0.5,x*x));	}


double erfncD(double x)	{			// Numerical Recipies
	return 1.0-(x<0?-gammpD(0.5,x*x):gammpD(0.5,x*x));	}


float erfnc(float x)	{			// Numerical Recipies
	return 1.0-(x<0?-gammp(0.5,x*x):gammp(0.5,x*x));	}


/* erfcc returns the complementary error function, calculated with a method
copied verbatim from Numerical Recipies in C, by Press et al., Cambridge
University Press, Cambridge, 1988.  It works for all x and has fractional error
everywhere less than 1.2e-7.  */

double erfccD(double x) {
	double t,z,ans;

	z=fabs(x);
	t=1.0/(1.0+0.5*z);
	ans=t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+t*(-0.82215223+t*0.17087277)))))))));
	return x>=0.0?ans:2.0-ans; }


float erfcc(float x) {
	double t,z,ans;

	z=fabs(x);
	t=1.0/(1.0+0.5*z);
	ans=t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+t*(0.09678418+t*(-0.18628806+t*(0.27886807+t*(-1.13520398+t*(1.48851587+t*(-0.82215223+t*0.17087277)))))))));
	return x>=0.0?ans:2.0-ans; }


double erfD(double x) {
	return 1.0-erfccD(x); }


double experfcD(double x) {
	double ans,xxinv;

	if(fabs(x)<20) ans=exp(x*x)*erfccD(x);
	else {
		xxinv=1.0/(x*x);
		ans=1.0+xxinv*(-1.0/2.0+xxinv*(3.0/4.0+xxinv*(-15.0/8.0+xxinv*(105.0/16.0+xxinv*(-945.0/32.0)))));
		ans/=x*SQRTPI;
		if(x<0) ans+=2.0*exp(x*x); }
	return ans; }


double erfcintegralD(double x) {
	return (1.0-exp(-x*x))/SQRTPI+x*erfccD(x); }


float erfcintegral(float x) {
	return (1.0-exp(-x*x))/SQRTPI+x*erfcc(x); }


double betalnD(double x1,double x2)	{				// Math CRC
	return(gammalnD(x1)+gammalnD(x2)-gammalnD(x1+x2));	}


float betaln(float x1,float x2)	{				// Math CRC
	return(gammaln(x1)+gammaln(x2)-gammaln(x1+x2));	}


double hermiteD(double x,int n)	{
	if(n==0)	return 1;
	else if(n==1)	return 2*x;
	else if(n>1)	return(2*x*hermiteD(x,n-1)-2*(n-1)*hermiteD(x,n-2));
	else return 0;	}


float hermite(float x,int n)	{
	if(n==0)	return 1;
	else if(n==1)	return 2*x;
	else if(n>1)	return(2*x*hermite(x,n-1)-2*(n-1)*hermite(x,n-2));
	else return 0;	}


/********** Green's functions **********/


double diffgreen2D(double r1,double r2) {
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


/********* interval stuff ***********/

float constrain(float x,float lo,float hi) {
	if(x<lo) return lo;
	if(x>hi) return hi;
	return x; }


double reflectD(double x,double lo,double hi) {
	if(x>=lo&&x<=hi) return x;
	if(x<lo) return reflectD(2*lo-x,lo,hi);
	return reflectD(2*hi-x,lo,hi); }


float reflect(float x,float lo,float hi) {
	if(x>=lo&&x<=hi) return x;
	if(x<lo) return reflect(2*lo-x,lo,hi);
	return reflect(2*hi-x,lo,hi); }


/********* inverse functions ***********/

double inversefnD(double (*fn)(double),double y,double x1,double x2,int n) {
	double dx,y2;

	if((*fn)(x1)<(*fn)(x2))	dx=x2-x1;
	else {
		dx=x1-x2;
		x1=x2; }
	for(;n>0;n--) {
		dx*=0.5;
		x2=x1+dx;
		y2=(*fn)(x2);
		if(y2<y) x1=x2; }
	return x1+0.5*dx; }


float inversefn(float (*fn)(float),float y,float x1,float x2,int n) {
	float dx,y2;

	if((*fn)(x1)<(*fn)(x2))	dx=x2-x1;
	else {
		dx=x1-x2;
		x1=x2; }
	for(;n>0;n--) {
		dx*=0.5;
		x2=x1+dx;
		y2=(*fn)(x2);
		if(y2<y) x1=x2; }
	return x1+0.5*dx; }


/********* various utilities ***********/

double quadpts2paramsD(double *x,double *y,double *abc) {
	double det;

	det=x[0]*x[0]*x[1]-x[0]*x[0]*x[2]-x[0]*x[1]*x[1]+x[0]*x[2]*x[2]+x[1]*x[1]*x[2]-x[1]*x[2]*x[2];
	if(det>0||det<0) {
		abc[0]=((x[1]-x[2])*y[0]+(-x[0]+x[2])*y[1]+(x[0]-x[1])*y[2])/det;
		abc[1]=((-x[1]*x[1]+x[2]*x[2])*y[0]+(x[0]*x[0]-x[2]*x[2])*y[1]+(-x[0]*x[0]+x[1]*x[1])*y[2])/det;
		abc[2]=((x[1]*x[1]*x[2]-x[1]*x[2]*x[2])*y[0]+(-x[0]*x[0]*x[2]+x[0]*x[2]*x[2])*y[1]+(x[0]*x[0]*x[1]-x[0]*x[1]*x[1])*y[2])/det; }
	return det; }


double fouriersumD(double *a,double *b,int n,double l,double x) {
	double sum;
	int j;

	sum=a[0]/2;
	for(j=1;j<n;j++)
		sum+=a[j]*cos(j*PI*x/l)+b[j]*sin(j*PI*x/l);
	return sum; }


void radialftD(double *r,double *a,double *k,double *c,int nr,int nk) {
  int i,j;
  double f1,sum;
  const double c1=0.797884560803;  /* sqrt(2/PI) */

  for(i=0;i<nk;i++) {
    f1=r[0]*sin(k[i]*r[0])*a[0];
    sum=f1*r[0];
    for(j=1;j<nr;j++) {
      sum+=f1*(r[j]-r[j-1]);
      f1=r[j]*sin(k[i]*r[j])*a[j];
      sum+=f1*(r[j]-r[j-1]); }
    c[i]=c1*sum/2.0/k[i]; }
  return; }


void linefitD(double *x,double *y,int n,double *m,double *b) {
	int i;
	double sx,sy,sxx,sxy,denom;

	sx=sy=sxx=sxy=0;
	for(i=0;i<n;i++) {
		sx+=x[i];
		sy+=y[i];
		sxx+=x[i]*x[i];
		sxy+=x[i]*y[i]; }
	denom=n*sxx-sx*sx;
	if(b) *b=(sxx*sy-sx*sxy)/denom;
	if(m) *m=(n*sxy-sx*sy)/denom;
	return; }


/********* Hill functions ***********/

/* Sets vector of Hill function parameters called hp to a, e, n, and b.  No math
or checking is done, just assignment of a, e, n, and b to the 4-element hp
vector, which needs to be pre-allocated.  Note that prior versions of this
function did not include the b term and they also used a 3-element hp vector;
all 4 elements are required now. */
void SetHillParamD(double *hp,double a,double e,double n,double b) {
	hp[0]=a;
	hp[1]=e;
	hp[2]=n;
	hp[3]=b;
	return; }


/* Hill function.  hp is 4-element vector of Hill function parameters with values
A, E, N, and B respectively, and x is the variable.  Note that prior versions of
this function used a 3-element vector, although all 4 elements are required now.
Function: H(x)=A*x^n/(E^n+x^n) */
double HillFnD(double *hp,double x) {
	return hp[3]+hp[0]*pow(x,hp[2])/(pow(hp[1],hp[2])+pow(x,hp[2])); }


/* Composition of Hill functions 1 and 2: H_tot(x)=H2(H1(x)).  The composition
is not exactly a Hill function but is sufficiently close that H_tot is a "close"
Hill function.  Parameters for H1, H2, and H_tot are the same as for HillFn. */
void HillFnComposeD(double *hp1,double *hp2,double *hp12) {
	double a1,a2,a12,e1,e2,e12,n1,n2,n12,e2p,a12p,e12p;

	a1=hp1[0];
	a2=hp2[0];
	e1=hp1[1];
	e2=hp2[1];
	n1=hp1[2];
	n2=hp2[2];
	e2p=e2/a1;
	a12=a2/(1.0+pow(e2p,n2));
	e12=e1/pow(1/e2p*pow(1.0+2*pow(e2p,n2),1.0/n2)-1.0,1.0/n1);
	a12p=a12/a2;
	e12p=e12/e1;
	n12=4.0*n1*n2*pow(e2p,n2)/(a12p*pow(e12p,n1))/pow(1.0+pow(e12p,-n1),n2+1)/pow(pow(1.0+pow(e12p,-n1),-n2)+pow(e2p,n2),2);
	hp12[0]=a12;
	hp12[1]=e12;
	hp12[2]=n12;
	return; }


/* Composition of Hill functions 1 and 2, including feedback from 2 to 1.  The
composition is not exactly a Hill function but is a "close" fit to one.  The
reaction mechanism is that the activated product of function 2 is a reactant for
the inactivation of reaction 1.  Either, both, or neither hpf1 and hpf2 are
returned; for one to be returned send in a non-NULL address.  It is assumed, but
not checked, that both input n values (hp1[2], hp2[2]) are equal to 1. */
void HillFnComposeNF1D(double *hp1,double *hp2,double *hpf1,double *hpf12) {
	double a1,e1,a2,e2;

	a1=hp1[0];
	e1=hp1[1];
	a2=hp2[0];
	e2=hp2[1];
	if(hpf1) {
		hpf1[0]=a1;
		hpf1[1]=e1*a1/(2*e2+a1);
		hpf1[2]=(2*e2+a1)/(3*e2+a1); }
	if(hpf12) {
		hpf12[0]=a1*a2/(a1+e2);
		hpf12[1]=e1*e2*a1/(2.0*(e2+a1)*(e2+a1));
		hpf12[2]=2.0/3.0; }
	return; }


