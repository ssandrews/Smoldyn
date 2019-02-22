/* Steven Andrews, 12/98.
See documentation called RnSort_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <math.h>
#include <float.h>
#include <string.h>
#include "math2.h"
#include "Rn.h"
#include "RnSort.h"

#define	DEBUG 0


/****************************************************************************/
/************************   sorting functions   *****************************/
/****************************************************************************/

/* sortV */
void sortV(float *a,float *b,int n) {
	int i,j,ir,l;
	float az,bz;

	if(!n) return;
	if(!b) b=a;
	for(i=0;i<n-1&&a[i]<a[i+1];i++)
		;
	if(i==n-1) return;
	for(i=0;i<n-1&&a[i]>a[i+1];i++)
		;
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			bz=b[i];
			a[i]=a[j=n-i-1];
			b[i]=b[j];
			a[j]=az;
			b[j]=bz; }
		return; }
	l=(n>>1)+1;
	ir=n;
	for(;;)	{
		if(l>1)	{
			az=a[--l-1];
			bz=b[l-1]; }
		else	{
			az=a[ir-1];
			bz=b[ir-1];
			a[ir-1]=a[0];
			b[ir-1]=b[0];
			if(--ir==1)	{
				a[0]=az;
				b[0]=bz;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir&&a[j-1]<a[j])	++j;
			if(az<a[j-1])	{
				a[i-1]=a[j-1];
				b[i-1]=b[j-1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az;
		b[i-1]=bz; }}


/* sortVii */
void sortVii(int *a,int *b,int n) {
	int i,j,ir,l;
	int az,bz;

	if(!n) return;
	if(!b) b=a;
	for(i=0;i<n-1&&a[i]<a[i+1];i++)
		;
	if(i==n-1) return;
	for(i=0;i<n-1&&a[i]>a[i+1];i++)
		;
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			bz=b[i];
			a[i]=a[j=n-i-1];
			b[i]=b[j];
			a[j]=az;
			b[j]=bz; }
		return; }
	l=(n>>1)+1;
	ir=n;
	for(;;)	{
		if(l>1)	{
			az=a[--l-1];
			bz=b[l-1]; }
		else	{
			az=a[ir-1];
			bz=b[ir-1];
			a[ir-1]=a[0];
			b[ir-1]=b[0];
			if(--ir==1)	{
				a[0]=az;
				b[0]=bz;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir&&a[j-1]<a[j])	++j;
			if(az<a[j-1])	{
				a[i-1]=a[j-1];
				b[i-1]=b[j-1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az;
		b[i-1]=bz; }}


/* sortVdbl */
void sortVdbl(double *a,double *b,int n) {
	int i,j,ir,l;
	double az,bz;

	if(!n) return;
	if(!b) b=a;
	for(i=0;i<n-1&&a[i]<a[i+1];i++)
		;
	if(i==n-1) return;
	for(i=0;i<n-1&&a[i]>a[i+1];i++)
		;
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			bz=b[i];
			a[i]=a[j=n-i-1];
			b[i]=b[j];
			a[j]=az;
			b[j]=bz; }
		return; }
	l=(n>>1)+1;
	ir=n;
	for(;;)	{
		if(l>1)	{
			az=a[--l-1];
			bz=b[l-1]; }
		else	{
			az=a[ir-1];
			bz=b[ir-1];
			a[ir-1]=a[0];
			b[ir-1]=b[0];
			if(--ir==1)	{
				a[0]=az;
				b[0]=bz;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir&&a[j-1]<a[j])	++j;
			if(az<a[j-1])	{
				a[i-1]=a[j-1];
				b[i-1]=b[j-1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az;
		b[i-1]=bz; }}


/* sortCV */
void sortCV(float *a,float *bc,int n) {
	int i,j,ir,l;
	float az,bzr,bzi;

	if(!n) return;
	for(i=0;i<n-1&&a[i]<a[i+1];i++)
		;
	if(i==n-1) return;
	for(i=0;i<n-1&&a[i]>a[i+1];i++)
		;
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			bzr=bc[2*i];
			bzi=bc[2*i+1];
			a[i]=a[j=n-i-1];
			bc[2*i]=bc[2*j];
			bc[2*i+1]=bc[2*j+1];
			a[j]=az;
			bc[2*j]=bzr;
			bc[2*j+1]=bzi; }
		return; }
	l=(n>>1)+1;
	ir=n;
	for(;;)	{
		if(l>1)	{
			az=a[--l-1];
			bzr=bc[2*(l-1)];
			bzi=bc[2*(l-1)+1]; }
		else	{
			az=a[ir-1];
			bzr=bc[2*(ir-1)];
			bzi=bc[2*(ir-1)+1];
			a[ir-1]=a[0];
			bc[2*(ir-1)]=bc[0];
			bc[2*(ir-1)+1]=bc[1];
			if(--ir==1)	{
				a[0]=az;
				bc[0]=bzr;
				bc[1]=bzi;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir&&a[j-1]<a[j])	++j;
			if(az<a[j-1])	{
				a[i-1]=a[j-1];
				bc[2*(i-1)]=bc[2*(j-1)];
				bc[2*(i-1)+1]=bc[2*(j-1)+1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az;
		bc[2*(i-1)]=bzr;
		bc[2*(i-1)+1]=bzi; }}


/* sortVliv */
void sortVliv(long int *a,void **b,int n) {
	int i,j,ir,l;
	long int az;
	void *bz;
	
	if(!n) return;
	for(i=0;i<n-1&&a[i]<a[i+1];i++)
		;
	if(i==n-1) return;
	for(i=0;i<n-1&&a[i]>a[i+1];i++)
		;
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			bz=b[i];
			a[i]=a[j=n-i-1];
			b[i]=b[j];
			a[j]=az;
			b[j]=bz; }
		return; }
	l=(n>>1)+1;
	ir=n;
	for(;;)	{
		if(l>1)	{
			az=a[--l-1];
			bz=b[l-1]; }
		else	{
			az=a[ir-1];
			bz=b[ir-1];
			a[ir-1]=a[0];
			b[ir-1]=b[0];
			if(--ir==1)	{
				a[0]=az;
				b[0]=bz;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir&&a[j-1]<a[j])	++j;
			if(az<a[j-1])	{
				a[i-1]=a[j-1];
				b[i-1]=b[j-1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az;
		b[i-1]=bz; }}


/* sortVoid */
void sortVoid(void **a,int n,int (*compare)(void*,void*)) {
	int i,j,ir,l;
	void *az;
	
	if(!n) return;
	for(i=0;i<n-1 && compare(a[i],a[i+1])<=0;i++);		// check for pre-sorted forward
	if(i==n-1) return;

	for(i=0;i<n-1 && compare(a[i],a[i+1])>=0;i++);		// check for pre-sorted reverse
	if(i==n-1)	{
		for(i=0;i<n/2;i++)	{
			az=a[i];
			a[i]=a[j=n-i-1];
			a[j]=az; }
		return; }

	l=(n>>1)+1;													// sort
	ir=n;
	for(;;)	{
		if(l>1)
			az=a[--l-1];
		else	{
			az=a[ir-1];
			a[ir-1]=a[0];
			if(--ir==1)	{
				a[0]=az;
				return; }}
		i=l;
		j=l<<1;
		while(j<=ir)	{
			if(j<ir && compare(a[j-1],a[j])<0) ++j;
			if(compare(az,a[j-1])<0) {
				a[i-1]=a[j-1];
				j+=(i=j); }
			else
				j=ir+1; }
		a[i-1]=az; }

	return; }


/****************************************************************************/
/***********************   locating functions   *****************************/
/****************************************************************************/

int locateV(float *a,float x,int n) {
	int jl,jm,ju,ascnd;

	jl=-1;
	ju=n;
	ascnd=(a[n-1]>=a[0]);
	while(ju-jl>1)	{
		jm=(ju+jl)>>1;
		if(ascnd==(x>=a[jm]))	jl=jm;
		else	ju=jm; }
	return jl; }


int locateVdbl(double *a,double x,int n) {
	int jl,jm,ju,ascnd;

	jl=-1;
	ju=n;
	ascnd=(a[n-1]>=a[0]);
	while(ju-jl>1)	{
		jm=(ju+jl)>>1;
		if(ascnd==(x>=a[jm]))	jl=jm;
		else	ju=jm; }
	return jl; }


int locateVli(long int *a,long int x,int n) {
	int jl,jm,ju,ascnd;

	jl=-1;
	ju=n;
	ascnd=(a[n-1]>=a[0]);
	while(ju-jl>1)	{
		jm=(ju+jl)>>1;
		if(ascnd==(x>=a[jm]))	jl=jm;
		else	ju=jm; }
	if(jl>=0 && a[jl]==x) return jl;
	else return -1; }


int locateVi(int *a,int x,int n,int mode) {
	int jl,jm,ju,ascnd;

	jl=-1;
	ju=n;
	ascnd=(a[n-1]>=a[0]);
	while(ju-jl>1)	{
		jm=(ju+jl)>>1;
		if(ascnd==(x>=a[jm]))	jl=jm;
		else	ju=jm; }
	if(mode==1 || (jl>=0 && a[jl]==x)) return jl;
	return -1; }


int locateVstr(char **a,const char *x,int n,int mode) {
	int jl,jm,ju,ascnd;

	jl=-1;
	ju=n;
	ascnd=(strcmp(a[n-1],a[0])>=0);
	while(ju-jl>1)	{
		jm=(ju+jl)>>1;
		if(ascnd==(strcmp(x,a[jm])>=0))	jl=jm;
		else	ju=jm; }
	if(mode==1 || (jl>=0 && !strcmp(a[jl],x))) return jl;
	return -1; }


/****************************************************************************/
/*********************   interpolation functions   **************************/
/****************************************************************************/

float interpolate1(float *ax,float *ay,int n,int *j,float x) {
	int i;

	i=*j;
	if(i<-1) i=locateV(ax,x,n);
	else while(i<n-1&&ax[i+1]<=x) i++;
	*j=i;
	if(i>n-2) i=n-2;
	if(i<0) i=0;
	if(n==1||ax[i+1]==ax[i]) return ay[i];
	return (ay[i]*(ax[i+1]-x)+ay[i+1]*(x-ax[i]))/(ax[i+1]-ax[i]); }


double interpolate1dbl(double *ax,double *ay,int n,int *j,double x) {
	int i;

	i=*j;
	if(i<-1) i=locateVdbl(ax,x,n);
	else while(i<n-1&&ax[i+1]<=x) i++;
	*j=i;
	if(i>n-2) i=n-2;
	if(i<0) i=0;
	if(n==1||ax[i+1]==ax[i]) return ay[i];
	return (ay[i]*(ax[i+1]-x)+ay[i+1]*(x-ax[i]))/(ax[i+1]-ax[i]); }


float interpolate1Cr(float *ax,float *ayc,int n,int *j,float x) {
	int i;
	
	i=*j;
	if(i<-1) i=locateV(ax,x,n);
	else while(i<n-1&&ax[i+1]<=x) i++;
	*j=i;
	if(i>n-2) i=n-2;
	if(i<0) i=0;
	if(n==1||ax[i+1]==ax[i]) return ayc[2*i];
	return (ayc[2*i]*(ax[i+1]-x)+ayc[2*(i+1)]*(x-ax[i]))/(ax[i+1]-ax[i]); }


float interpolate1Ci(float *ax,float *ayc,int n,int *j,float x) {
	int i;
	
	i=*j;
	if(i<-1) i=locateV(ax,x,n);
	else while(i<n-1&&ax[i+1]<=x) i++;
	*j=i;
	if(i>n-2) i=n-2;
	if(i<0) i=0;
	if(n==1||ax[i+1]==ax[i]) return ayc[2*i+1];
	return (ayc[2*i+1]*(ax[i+1]-x)+ayc[2*(i+1)+1]*(x-ax[i]))/(ax[i+1]-ax[i]); }


/* cubicinterpolate1D */
double cubicinterpolate1D(double *xdata,double *ydata,int n,double x) {
	int i;
	double *xd,ans;
	
	if(n<4) return -1;
	for(i=0;i<n && xdata[i]<x;i++);
	if(i<2) i=2;
	else if(i>n-2) i=n-2;
	
	xd=xdata+(i-2);
	ans=ydata[i-2]* (x-xd[1])*(x-xd[2])*(x-xd[3])/((xd[0]-xd[1])*(xd[0]-xd[2])*(xd[0]-xd[3]));
	ans+=ydata[i-1]*(x-xd[0])*(x-xd[2])*(x-xd[3])/((xd[1]-xd[0])*(xd[1]-xd[2])*(xd[1]-xd[3]));
	ans+=ydata[i]*  (x-xd[0])*(x-xd[1])*(x-xd[3])/((xd[2]-xd[0])*(xd[2]-xd[1])*(xd[2]-xd[3]));
	ans+=ydata[i+1]*(x-xd[0])*(x-xd[1])*(x-xd[2])/((xd[3]-xd[0])*(xd[3]-xd[1])*(xd[3]-xd[2]));
	return ans; }


/* cubicinterpolate2D */
/* y is fast-changing index of zdata */
double cubicinterpolate2D(double *xdata,double *ydata,double *zdata,int nx,int ny,double x,double y) {
	int i,j;
	double ans,*xd,*yd,xcoeff[4],zval[4];
	
	if(nx<4 || ny<4) return -1;
	for(i=0;i<nx && xdata[i]<x;i++);		// i is index for x
	for(j=0;j<ny && ydata[j]<y;j++);		// j is index for y
	
	if(i<2) i=2;
	else if(i>nx-2) i=nx-2;
	if(j<2) j=2;
	else if(j>ny-2) j=ny-2;
	
	xd=xdata+(i-2);
	xcoeff[0]=(x-xd[1])*(x-xd[2])*(x-xd[3])/((xd[0]-xd[1])*(xd[0]-xd[2])*(xd[0]-xd[3]));
	xcoeff[1]=(x-xd[0])*(x-xd[2])*(x-xd[3])/((xd[1]-xd[0])*(xd[1]-xd[2])*(xd[1]-xd[3]));
	xcoeff[2]=(x-xd[0])*(x-xd[1])*(x-xd[3])/((xd[2]-xd[0])*(xd[2]-xd[1])*(xd[2]-xd[3]));
	xcoeff[3]=(x-xd[0])*(x-xd[1])*(x-xd[2])/((xd[3]-xd[0])*(xd[3]-xd[1])*(xd[3]-xd[2]));
	
	zval[0]=xcoeff[0]*zdata[ny*(i-2)+(j-2)]+xcoeff[1]*zdata[ny*(i-1)+(j-2)]+xcoeff[2]*zdata[ny*(i)+(j-2)]+xcoeff[3]*zdata[ny*(i+1)+(j-2)];
	zval[1]=xcoeff[0]*zdata[ny*(i-2)+(j-1)]+xcoeff[1]*zdata[ny*(i-1)+(j-1)]+xcoeff[2]*zdata[ny*(i)+(j-1)]+xcoeff[3]*zdata[ny*(i+1)+(j-1)];
	zval[2]=xcoeff[0]*zdata[ny*(i-2)+(j-0)]+xcoeff[1]*zdata[ny*(i-1)+(j-0)]+xcoeff[2]*zdata[ny*(i)+(j-0)]+xcoeff[3]*zdata[ny*(i+1)+(j-0)];
	zval[3]=xcoeff[0]*zdata[ny*(i-2)+(j+1)]+xcoeff[1]*zdata[ny*(i-1)+(j+1)]+xcoeff[2]*zdata[ny*(i)+(j+1)]+xcoeff[3]*zdata[ny*(i+1)+(j+1)];
	
	yd=ydata+(j-2);
	ans=zval[0]* (y-yd[1])*(y-yd[2])*(y-yd[3])/((yd[0]-yd[1])*(yd[0]-yd[2])*(yd[0]-yd[3]));
	ans+=zval[1]*(y-yd[0])*(y-yd[2])*(y-yd[3])/((yd[1]-yd[0])*(yd[1]-yd[2])*(yd[1]-yd[3]));
	ans+=zval[2]*(y-yd[0])*(y-yd[1])*(y-yd[3])/((yd[2]-yd[0])*(yd[2]-yd[1])*(yd[2]-yd[3]));
	ans+=zval[3]*(y-yd[0])*(y-yd[1])*(y-yd[2])/((yd[3]-yd[0])*(yd[3]-yd[1])*(yd[3]-yd[2]));
	
	return ans; }


/****************************************************************************/
/***********************   resampling functions   ***************************/
/****************************************************************************/

void convertxV(float *ax,float *ay,float *cx,float *cy,int na,int nc) {
	int ia,ic;
	float x,dx;

	if(na==nc) {
		for(ia=0;ia<na&&cx[ia]==ax[ia];ia++)
			;
		if(ia==na) {
			for(ia=0;ia<na;ia++) cy[ia]=ay[ia];
			return; }}
	ia=locateV(ax,cx[0],na);
	if(ia>na-2) ia=na-2;
	if(ia<0) ia=0;
	for(ic=0;ic<nc;ic++) {
		x=cx[ic];
		while(ia<na-2&&ax[ia+1]<=x) ia++;
		dx=ax[ia+1]-ax[ia];
		cy[ic]=dx==0?ay[ia]:(ay[ia]*(ax[ia+1]-x)+ay[ia+1]*(x-ax[ia]))/dx; }
	return; }

	
void convertxCV(float *ax,float *ayc,float *cx,float *cyc,int na,int nc) {
	int ia,ic;
	float x,dx;

	if(na==nc) {
		for(ia=0;ia<na&&cx[ia]==ax[ia];ia++)
			;
		if(ia==na) {
			for(ia=0;ia<2*na;ia++) cyc[ia]=ayc[ia];
			return; }}
	ia=locateV(ax,cx[0],na);
	if(ia>na-2) ia=na-2;
	if(ia<0) ia=0;
	for(ic=0;ic<nc;ic++) {
		x=cx[ic];
		while(ia<na-2&&ax[ia+1]<=x) ia++;
		dx=ax[ia+1]-ax[ia];
		cyc[2*ic]=dx==0?ayc[2*ia]:(ayc[2*ia]*(ax[ia+1]-x)+ayc[2*ia+2]*(x-ax[ia]))/dx;
		cyc[2*ic+1]=dx==0?ayc[2*ia+1]:(ayc[2*ia+1]*(ax[ia+1]-x)+ayc[2*ia+3]*(x-ax[ia]))/dx; }
	return; }


/****************************************************************************/
/************************   fitting functions   *****************************/
/****************************************************************************/

double fitoneparam(double *xdata,double *ydata,int nlo,int nhi,int function,double *constant) {
	double a0,alpha,beta,c[4];
	int i;

	if(constant) c[0]=constant[0];
	else c[0]=0;
	alpha=0;
	beta=0;

	if(function==1) {									// a+c0
		alpha=(double)(nhi-nlo);
		for(i=nlo;i<nhi;i++) beta+=ydata[i]-c[0]; }

	else if(function==2) {						// ax+c0
		for(i=nlo;i<nhi;i++) {
			alpha+=xdata[i]*xdata[i];
			beta+=(ydata[i]-c[0])*xdata[i]; }}

	else if(function==3) {						// a/x+c0
		for(i=nlo;i<nhi;i++) {
			alpha+=1.0/(xdata[i]*xdata[i]);
			beta+=(ydata[i]-c[0])/xdata[i]; }}

	a0=beta/alpha;
	if(DEBUG)
		printf("fitoneparam: alpha=%g, beta=%g, a0=%g\n",alpha,beta,a0);
	return a0; }


/****************************************************************************/
/***********************   histogram functions   ****************************/
/****************************************************************************/

void setuphist(float *hist,float *scale,int hn,float low,float high) {
	int i;
	float delta;

	for(i=0;i<hn;i++) hist[i]=0;
	delta=(high-low)/(hn-2.0);
	for(i=0;i<hn-1;i++) scale[i]=low+i*delta;
	scale[hn-1]=FLT_MAX;
	return; }


void setuphistdbl(double *hist,double *scale,int hn,double low,double high) {
	int i;
	double delta;

	for(i=0;i<hn;i++) hist[i]=0;
	delta=(high-low)/(hn-2.0);
	for(i=0;i<hn-1;i++) scale[i]=low+i*delta;
	scale[hn-1]=DBL_MAX;
	return; }


int histbin(float value,float *scale,int hn) {
	return 1+locateV(scale,value,hn); }


int histbindbl(double value,double *scale,int hn) {
	return 1+locateVdbl(scale,value,hn); }


void data2hist(float *data,int dn,char op,float *hist,float *scale,int hn) {
	int i,j;
	float delta;

	if(op=='-') delta=-1;
	else delta=1;
	if(op=='=') for(j=0;j<hn;j++) hist[j]=0;
	for(i=0;i<dn;i++) {
		j=1+locateV(scale,data[i],hn);
		hist[j]+=delta; }
	return; }


/****************************************************************************/
/********************   event analysis functions   **************************/
/****************************************************************************/

double maxeventrateVD(double *event,double *weight,int n,double sigma,double *tptr) {
	double xlo,xhi,x,rate,ratemax,xmax,step;
	int i,scan,nstep;

	xlo=minVD(event,n,NULL);
	xhi=maxVD(event,n,NULL);
	ratemax=0;
	xmax=xlo;
	step=sigma;
	nstep=(int)((xhi-xlo)/step);
	for(scan=0;scan<3;scan++) {
		for(x=xlo;x<=xhi;x+=step) {
			rate=0;
			if(weight) for(i=0;i<n;i++) rate+=weight[i]*exp(-(x-event[i])*(x-event[i])/(2*sigma));
			else for(i=0;i<n;i++) rate+=exp(-(x-event[i])*(x-event[i])/(2*sigma));
			if(rate>ratemax) {
				ratemax=rate;
				xmax=x; }}
		if(xmax>xlo) xlo=xmax-step;
		if(xmax<xhi) xhi=xmax+step;
		step=(xhi-xlo)/nstep; }
	if(tptr) *tptr=xmax;
	return ratemax/(sigma*sqrt(2*3.1415926535)); }


/****************************************************************************/
/*****************   concentration profile functions   **********************/
/****************************************************************************/

/* cpxinitializer */
double *cpxinitializer(int n,double *r,double rlow,double rhigh,double rjump) {
	double dr;
	int i,ijump;
	
	if(!r) {
		r=(double*)calloc(n,sizeof(double));
		if(!r) return NULL; }
	
	if(rjump>=rhigh)
		dr=(rhigh-rlow)/(double)(n-1);
	else {
		dr=(rhigh-rlow)/(double)(n-2);
		dr=(rjump-rlow)/(floor((rjump-rlow)/dr)+0.5); }
	
	r[0]=rlow;																		// r min is set to 0
	for(i=1;r[i-1]<rjump && i-1<n;i++) r[i]=r[i-1]+dr;
	ijump=i-1;
	r[i-1]=rjump-dr/100;													// r point just below rjump
	r[i]=rjump+dr/100;														// r point just above rjump
	i++;
	r[i]=r[i-1]+dr/2.0;
	i++;
	for(;i<n;i++) r[i]=r[i-1]+dr;
	if(rjump>=rhigh)
		r[n-1]=rhigh;
	
	if(DEBUG) {
		printf("cpxinitializer: r= [%g %g %g ... ",r[0],r[1],r[2]);
		if(rjump<rhigh) printf("%g %g %g %g ... ",r[ijump-1],r[ijump],r[ijump+1],r[ijump+2]);
		printf("%g %g]\n",r[n-2],r[n-1]); }
	
	return r; }


/* cpxinitializec */
double *cpxinitializec(double *r,double *c,int n,double *values,int code) {
	int i;
	
	if(!c) {
		c=(double*)calloc(n,sizeof(double));
		if(!c) return NULL; }
	
	if(code==0)
		for(i=0;i<n;i++) c[i]=0;
	else if(code==1)
		for(i=0;i<n;i++) c[i]=values[0];
	else
		for(i=0;i<n;i++) c[i]=0;
	
	if(DEBUG) {
		printf("cpxinitializec: c= [%g %g %g ... ",c[0],c[1],c[2]);
		printf("%g %g]\n",c[n-2],c[n-1]); }
	
	return c; }


/* cp1integrate */
double cp1integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft) {
	int j,reverse;
	double sum,r0,r1,f0,f1;
	
	if(rmin>rmax) {
		r0=rmin;
		rmin=rmax;
		rmax=r0;
		reverse=1; }
	else reverse=0;
	
	sum=0;
	for(j=0;j<n && r[j]<=rmin;j++);						// j becomes first site above rmin
	
	r0=rmin;
	if(j==0) f0=rdf[0];
	else if(j==n) f0=rdf[n-1];
	else f0=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmin-r[j-1])/(r[j]-r[j-1]);
	if(j<n) {
		r1=r[j];
		f1=rdf[j]; }
	else {
		r1=rmax;
		f1=rdf[n-1]; }
	if(r1<rmax) sum+=0.5*(f0+f1)*(r1-r0);			// first trapezoid, from rmin to j
	else {																		// rmin and rmax are in same trapezoid
		j--;
		r1=r0;
		f1=f0; }
	
	for(j++;j<n && r[j]<rmax;j++) {
		r0=r1;
		f0=f1;
		r1=r[j];
		f1=rdf[j];
		sum+=0.5*(f0+f1)*(r1-r0); }							// middle trapezoids, from j-1 to j
	
	r0=r1;
	f0=f1;
	if(upperleft && j<n) {
		r1=r[j];
		f1=rdf[j];
		sum+=0.5*f0*(r1-r0); }
	else {
		r1=rmax;
		if(j==0) f1=rdf[0];
		else if(j==n) f1=rdf[n-1];
		else f1=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmax-r[j-1])/(r[j]-r[j-1]);
		sum+=0.5*(f0+f1)*(r1-r0); }							// last trapezoid, from j-1 to rmax

	if(reverse) sum*=-1;
	
	return sum; }


/* cp2integrate */
double cp2integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft) {
	int j,reverse;
	double sum,r0,r1,f0,f1;
	
	if(rmin>rmax) {
		r0=rmin;
		rmin=rmax;
		rmax=r0;
		reverse=1; }
	else reverse=0;
	
	sum=0;
	for(j=0;j<n && r[j]<=rmin;j++);						// j becomes first site above rmin
	
	r0=rmin;
	if(j==0) f0=rdf[0];
	else if(j==n) f0=rdf[n-1];
	else f0=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmin-r[j-1])/(r[j]-r[j-1]);
	if(j<n) {
		r1=r[j];
		f1=rdf[j]; }
	else {
		r1=rmax;
		f1=rdf[n-1]; }
	if(r1<rmax) sum+=PI/3.0*(r1-r0)*(f0*(2*r0+r1)+f1*(2*r1+r0));			// first trapezoid, from rmin to j
	else {																		// rmin and rmax are in same trapezoid
		j--;
		r1=r0;
		f1=f0; }
	
	for(j++;j<n && r[j]<rmax;j++) {
		r0=r1;
		f0=f1;
		r1=r[j];
		f1=rdf[j];
		sum+=PI/3*(r1-r0)*(f0*(2*r0+r1)+f1*(2*r1+r0)); }	// middle trapezoids, from j-1 to j

	r0=r1;
	f0=f1;
	if(upperleft && j<n) {
		r1=r[j];
		f1=rdf[j];
		sum+=PI/3*(r1-r0)*f0*(2*r0+r1); }
	else {
		r1=rmax;
		if(j==0) f1=rdf[0];
		else if(j==n) f1=rdf[n-1];
		else f1=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmax-r[j-1])/(r[j]-r[j-1]);
		sum+=PI/3*(r1-r0)*(f0*(2*r0+r1)+f1*(2*r1+r0)); }		// last trapezoid, from j-1 to rmax
		
	if(reverse) sum*=-1;
	
	return sum; }


/* cp3integrate */
double cp3integrate(double *r,double *rdf,int n,double rmin,double rmax,int upperleft) {
	int j,reverse;
	double sum,r0,r1,f0,f1;
	
	if(rmin>rmax) {
		r0=rmin;
		rmin=rmax;
		rmax=r0;
		reverse=1; }
	else reverse=0;
	
	sum=0;
	for(j=0;j<n && r[j]<=rmin;j++);						// j becomes first site above rmin
	
	r0=rmin;
	if(j==0) f0=rdf[0];
	else if(j==n) f0=rdf[n-1];
	else f0=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmin-r[j-1])/(r[j]-r[j-1]);
	if(j<n) {
		r1=r[j];
		f1=rdf[j]; }
	else {
		r1=rmax;
		f1=rdf[n-1]; }
	if(r1<rmax) sum+=PI*(f1-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0-r0*f1)*(r1*r1+r1*r0+r0*r0);	// first trapezoid, from rmin to j
	else {																		// rmin and rmax are in same trapezoid
		j--;
		r1=r0;
		f1=f0; }
	
	for(j++;j<n && r[j]<rmax;j++) {
		r0=r1;
		f0=f1;
		r1=r[j];
		f1=rdf[j];
		sum+=PI*(f1-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0-r0*f1)*(r1*r1+r1*r0+r0*r0); }	// middle trapezoids, from j-1 to j
	
	r0=r1;
	f0=f1;
	if(upperleft && j<n) {
		r1=r[j];
		f1=rdf[j];
		sum+=PI*(-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0)*(r1*r1+r1*r0+r0*r0); }
	else {
		r1=rmax;
		if(j==0) f1=rdf[0];
		else if(j==n) f1=rdf[n-1];
		else f1=rdf[j-1]+(rdf[j]-rdf[j-1])*(rmax-r[j-1])/(r[j]-r[j-1]);
		sum+=PI*(f1-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0-r0*f1)*(r1*r1+r1*r0+r0*r0); }	// last trapezoid, from j-1 to rmax
	
	if(reverse) sum*=-1;
	
	return sum; }


/* cp1diffuse */
void cp1diffuse(double *r,double *rdfa,double *rdfd,int n,double rmsstep,double cminfinity,double cinfinity) {
	int i,j;
	double r0,r1,f0,f1,sum,grn;

	for(i=0;i<n;i++) {																// i is counter for rdfd
		sum=cminfinity/2.0*(erfccD((r[i]-r[0])/(SQRT2*rmsstep)));		// from -infinity to r[0]
		r1=r[0];
		grn=1.0/(rmsstep*SQRT2PI)*exp(-(r[i]-r1)*(r[i]-r1)/(2.0*rmsstep*rmsstep));
		f1=rdfa[0]*grn;
		for(j=1;j<n;j++) {															// j is counter for the integral
			r0=r1;
			f0=f1;
			r1=r[j];
			grn=1.0/(rmsstep*SQRT2PI)*exp(-(r[i]-r1)*(r[i]-r1)/(2.0*rmsstep*rmsstep));
			f1=rdfa[j]*grn;
			sum+=0.5*(f0+f1)*(r1-r0); }										// from r[j-1] to r[j]
		sum+=cinfinity/2.0*(1.0+erfnD((r[i]-r[n-1])/(SQRT2*rmsstep)));	// from r[n-1] to infinity
		rdfd[i]=sum; }
	return; }


/* cp3diffuse */
void cp3diffuse(double *r,double *rdfa,double *rdfd,int n,double rmsstep,double cinfinity) {
	int i,j;
	double grn,sum,f0,f1,rr,r0,r1,a2,erfcdif,erfcsum;

	a2=fitoneparam(r,rdfa,(int)(0.9*n),n,3,&cinfinity);
	a2=a2/(cinfinity*rmsstep);

	grn=0;
	if(r[i=0]==0) {
		rr=0;
		r1=0;
		f1=0;
		sum=0;
		for(j=1;j<n;j++) {
			r0=r1;
			f0=f1;
			r1=r[j]/rmsstep;
			grn=exp(-r1*r1/2.0)/(2.0*PI*SQRT2PI);
			f1=(rdfa[j]-rdfa[0])/cinfinity*grn;
			sum+=PI*(f1-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0-r0*f1)*(r1*r1+r1*r0+r0*r0); }
		sum+=4.0*PI*((1.0-rdfa[0]/cinfinity)*r1+a2)*grn+(1.0-rdfa[0]/cinfinity)*erfccD(r1/SQRT2);
		rdfd[i++]=sum*cinfinity+rdfa[0]; }

	for(;i<n;i++) {
		rr=r[i]/rmsstep;
		r1=0;
		grn=exp(-rr*rr/2.0)/(2.0*PI*SQRT2PI);
		f1=(rdfa[0]-rdfa[i])/cinfinity*grn;
		sum=0;
		for(j=(r[0]==0?1:0);j<n;j++) {
			r0=r1;
			f0=f1;
			r1=r[j]/rmsstep;
			grn=1.0/rr/r1*(exp(-(rr-r1)*(rr-r1)/2.0)-exp(-(rr+r1)*(rr+r1)/2.0))/(4.0*PI*SQRT2PI);
			f1=(rdfa[j]-rdfa[i])/cinfinity*grn;
			sum+=PI*(f1-f0)*(r1+r0)*(r1*r1+r0*r0)+4.0*PI/3.0*(r1*f0-r0*f1)*(r1*r1+r1*r0+r0*r0); }
		erfcdif=erfccD((r1-rr)/SQRT2);
		erfcsum=erfccD((r1+rr)/SQRT2);
		sum+=(1.0-rdfa[i]/cinfinity)*(4.0*PI*r1*grn+0.5*(erfcdif+erfcsum))+a2/2.0/rr*(erfcdif-erfcsum);
		rdfd[i]=sum*cinfinity+rdfa[i]; }
	return; }


/* cp1absorb */
double cp1absorb(double *r,double *rdf,int n,double rabsorb) {
	double area;
	int i;
	
	area=cp1integrate(r,rdf,n,r[0],rabsorb,1);
	for(i=0;i<n && r[i]<rabsorb;i++)
		rdf[i]=0;
	return area; }


/* cp2absorb */
double cp2absorb(double *r,double *rdf,int n,double rabsorb) {
	double area;
	int i;
	
	area=cp2integrate(r,rdf,n,0,rabsorb,1);
	for(i=0;i<n && r[i]<rabsorb;i++)
		rdf[i]=0;
	return area; }


/* cp3absorb */
double cp3absorb(double *r,double *rdf,int n,double rabsorb) {
	double area;
	int i;
	
	area=cp3integrate(r,rdf,n,0,rabsorb,1);
	for(i=0;i<n && r[i]<rabsorb;i++)
		rdf[i]=0;
	return area; }


/* cpxaddconc */
void cpxaddconc(double *r,double *rdf,int n,double amount,int profile,double r1,double r2) {
	int i;

	if(profile==0) {
		for(i=0;i<n;i++)
			rdf[i]+=amount; }
	else if(profile==1) {
		for(i=0;i<n && r[i]<r1;i++)
			rdf[i]+=amount; }
	else if(profile==2) {
		for(i=0;i<n && r[i]<r1;i++);
		for(;i<n;i++)
			rdf[i]+=amount; }
	else if(profile==3) {
		for(i=0;i<n && r[i]<r1;i++);
		for(;i<n && r[i]<r2;i++)
			rdf[i]+=amount; }
	
	return; }


/* cpxmassactionreact */
void cpxmassactionreact(double *rdfa,double *rdfb,int n,double rate) {
	int i;
	double x;

	for(i=0;i<n;i++) {
		x=rate*rdfa[i]*rdfb[i];
		rdfa[i]-=x;
		rdfb[i]-=x; }
	return; }





