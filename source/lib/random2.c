/* Steven Andrews, 5/12/95.
Several random number generators.
See documentation called random_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include "random2.h"
#include "math2.h"


double unirandsumCCD(int n,double m,double s) {
	double x=0;
	int i;

	for(i=0;i<n-1;i++) x+=randCCD();
	x=(x-0.5*n)/sqrt(n/12.0);
	return(x*s+m); }


float unirandsumCCF(int n,float m,float s) {
	float x=0;
	int i;

	for(i=0;i<n-1;i++) x+=randCCF();
	x=(x-0.5*n)/sqrt(n/12.0);
	return(x*s+m); }


int intrandpD(int n,double *p) {
	double r;
	int jl,ju,jm;

	r=randCOD()*p[n-1];
	jl=-1;
	ju=n-1;
	while(ju-jl>1) {
		jm=(ju+jl)>>1;
		if(p[jm]<=r) jl=jm;
		else ju=jm; }
	return ju; }


int intrandpF(int n,float *p) {
	float r;
	int jl,ju,jm;

	r=randCOF()*p[n-1];
	jl=-1;
	ju=n-1;
	while(ju-jl>1) {
		jm=(ju+jl)>>1;
		if(p[jm]<=r) jl=jm;
		else ju=jm; }
	return ju; }


int poisrandD(double xm) {
	static double sq,alxm,g,oldm=-1.0;
	float em,t,y;

	if(xm<=0) return 0;
	else if(xm<12.0) {
		if(xm!=oldm) {
			oldm=xm;
			g=exp(-xm); }
		em=0;
		for(t=randCCD();t>g;t*=randCCD()) {
			em+=1.0; }}
	else {
		if(xm!=oldm) {
			oldm=xm;
			sq=sqrt(2.0*xm);
			alxm=log(xm);
			g=xm*alxm-gammaln(xm+1.0); }
		do {
			do {
				y=tan(PI*randCCD());
				em=sq*y+xm; } while(em<0);
			em=floor(em);
			t=0.9*(1.0+y*y)*exp(em*alxm-gammaln(em+1.0)-g); } while(randCCD()>t); }
	return (int) em; }


int poisrandF(float xm) {
	static float sq,alxm,g,oldm=-1.0;
	float em,t,y;

	if(xm<=0) return 0;
	else if(xm<12.0) {
		if(xm!=oldm) {
			oldm=xm;
			g=exp(-xm); }
		em=0;
		for(t=randCCF();t>g;t*=randCCF()) {
			em+=1.0; }}
	else {
		if(xm!=oldm) {
			oldm=xm;
			sq=sqrt(2.0*xm);
			alxm=log(xm);
			g=xm*alxm-gammaln(xm+1.0); }
		do {
			do {
				y=tan(PI*randCCF());
				em=sq*y+xm; } while(em<0);
			em=floor(em);
			t=0.9*(1.0+y*y)*exp(em*alxm-gammaln(em+1.0)-g); } while(randCCF()>t); }
	return (int) em; }


float binomialrandF(float p,int n) {
	int swap,j;
	float am,bnl,g,t,sq,angle,y,em;
	static float nold=-1,pold=-1;
	static float en,oldg,pc,plog,pclog;

	if(n<1) return 0;
	if(p>1) return n;
	if(p<0) return 0;
	swap=0;
	if(p>0.5) {
		swap=1;
		p=1.0-p; }
	am=n*p;
	if(n<25) {
		bnl=0;
		for(j=0;j<n;j++)
			if(coinrandF(p)) bnl+=1.0; }
	else if(am<1.0)	{
		g=exp(-am);
		t=1.0;
		for(j=0;j<=n;j++) {
			t*=randCCF();
			if(t<g) break; }
		bnl=j<=n?j:n; }
	else {
		if(n!=nold) {
			en=n;
			oldg=gammaln(en+1.0);
			nold=n; }
		if(p!=pold) {
			pc=1.0-p;
			plog=log(p);
			pclog=log(pc);
			pold=p; }
		sq=sqrt(2.0*am*pc);
		do {
			do {
				angle=PI*randCCF();
				y=tan(angle);
				em=sq*y+am; }
				while(em<0||em>=(en+1.0));
			em=floor(em);
			t=1.2*sq*(1.0+y*y)*exp(oldg-gammaln(em+1.0)-gammaln(en-em+1.0)+em*plog+(en-em)*pclog); }
			while(randCCF()>t);
		bnl=em; }
	if(swap) bnl=n-bnl;
	return bnl; }


double gaussrandD() {
	static int iset=0;
	static double gset;
	double fac,r,v1,v2;

	if(!iset) {
		do {
			v1=2.0*randCOD()-1.0;
			v2=2.0*randCOD()-1.0;
			r=v1*v1+v2*v2; }
			while(r>=1||r==0);
		fac=sqrt(-2.0*log(r)/r);
		gset=v1*fac;
		iset=1;
		return v2*fac; }
	else {
		iset=0;
		return gset; }}


float gaussrandF() {
	static int iset=0;
	static float gset;
	float fac,r,v1,v2;

	if(!iset) {
		do {
			v1=2.0*randCOF()-1.0;
			v2=2.0*randCOF()-1.0;
			r=v1*v1+v2*v2; }
			while(r>=1||r==0);
		fac=sqrt(-2.0*log(r)/r);
		gset=v1*fac;
		iset=1;
		return v2*fac; }
	else {
		iset=0;
		return gset; }}


void circlerandD(double *x,double radius) {
	double th;

	th=unirandCOD(0,2*PI);
	x[0]=radius*cos(th);
	x[1]=radius*sin(th);
	return; }


void sphererandCCD(double *x,double radius1,double radius2) {
	double th,phi;

	th=thetarandCCD();
	phi=unirandCOD(0,2*PI);
	if(radius1==radius2);
	else if(radius1==0) radius1=radrandsphCCD(radius2);
	else radius1=pow(randCCD()*(radius2*radius2*radius2-radius1*radius1*radius1)+radius1*radius1*radius1,1.0/3.0);
	x[0]=radius1*sin(th)*cos(phi);
	x[1]=radius1*sin(th)*sin(phi);
	x[2]=radius1*cos(th);
	return; }


void sphererandCCF(float *x,float radius1,float radius2) {
	float th,phi;

	th=thetarandCCF();
	phi=unirandCOF(0,2.0*PI);
	if(radius1==radius2);
	else if(radius1==0) radius1=radrandsphCCF(radius2);
	else radius1=pow(randCCF()*(radius2*radius2*radius2-radius1*radius1*radius1)+radius1*radius1*radius1,(float)(1.0/3.0));
	x[0]=radius1*sin(th)*cos(phi);
	x[1]=radius1*sin(th)*sin(phi);
	x[2]=radius1*cos(th);
	return; }


void ballrandCCD(double *x,int dim,double radius) {
	double rad,scale;
	int i;

	rad=0;
	for(i=0;i<dim;i++) {
		x[i]=gaussrandD();
		rad+=x[i]*x[i]; }
	rad=sqrt(rad);
	if(rad==0) return;
	scale=radius/rad*pow(randCCD(),1.0/dim);
	for(i=0;i<dim;i++)
		x[i]*=scale;
	return; }


void trianglerandCD(double *pt1,double *pt2,double *pt3,int dim,double *ans) {
	static double x,y;
	static int xd,yd;
	double x0,y0,x1,y1,x2,y2,m01,m02,m12,area,yx1,yy;
	int d,dsmall,zd;
	double range,smrange,swap,coefx,coefy,coefz,coefk;

	if(dim==2) {
		xd=0;
		yd=1;
		if(pt1[0]<=pt2[0]&&pt2[0]<=pt3[0]) {x0=pt1[0];y0=pt1[1];x1=pt2[0];y1=pt2[1];x2=pt3[0];y2=pt3[1];}
		else if(pt1[0]<=pt3[0]&&pt3[0]<=pt2[0]) {x0=pt1[0];y0=pt1[1];x1=pt3[0];y1=pt3[1];x2=pt2[0];y2=pt2[1];}
		else if(pt2[0]<=pt3[0]&&pt3[0]<=pt1[0]) {x0=pt2[0];y0=pt2[1];x1=pt3[0];y1=pt3[1];x2=pt1[0];y2=pt1[1];}
		else if(pt2[0]<=pt1[0]&&pt1[0]<=pt3[0]) {x0=pt2[0];y0=pt2[1];x1=pt1[0];y1=pt1[1];x2=pt3[0];y2=pt3[1];}
		else if(pt3[0]<=pt1[0]&&pt1[0]<=pt2[0]) {x0=pt3[0];y0=pt3[1];x1=pt1[0];y1=pt1[1];x2=pt2[0];y2=pt2[1];}
		else if(pt3[0]<=pt2[0]&&pt2[0]<=pt1[0]) {x0=pt3[0];y0=pt3[1];x1=pt2[0];y1=pt2[1];x2=pt1[0];y2=pt1[1];}
		else {x0=pt1[0];y0=pt1[1];x1=pt2[0];y1=pt2[1];x2=pt3[0];y2=pt3[1];}
		if(x0==x2) {
			ans[0]=x0;
			x0=(y0<y1)?(y0<y2?y0:y2):(y1<y2?y1:y2);
			x2=(y0>y1)?(y0>y2?y0:y2):(y1>y2?y1:y2);
			ans[1]=unirandCCD(x0,x2);
			return; }
		m01=(y1-y0)/(x1-x0);
		m02=(y2-y0)/(x2-x0);
		m12=(y2-y1)/(x2-x1);
		area=0.5*(y1-y0-m02*(x1-x0))*(x2-x0);
		if(x1-x0>x2-x1) yx1=0.5/area*(m01-m02)*(x1-x0)*(x1-x0);
		else yx1=1.0-0.5/area*(m02-m12)*(x1-x2)*(x1-x2);
		yy=unirandCCD(0,1);
		if(yy<yx1) {
			x=x0+sqrt(2.0*area*yy/(m01-m02));
			y=unirandCCD(y0+m02*(x-x0),y0+m01*(x-x0)); }
		else if(yy>yx1) {
			x=x2-sqrt(2.0*area*(1.0-yy)/(m02-m12));
			y=unirandCCD(y0+m02*(x-x0),y1+m12*(x-x1)); }
		else {
			x=x1;
			y=unirandCCD(y0+m02*(x-x0),y1); }
		ans[0]=x;
		ans[1]=y;
		return; }

	smrange=-1;
	dsmall=0;
	for(d=0;d<dim;d++) {									// find coordinate with smallest used range
		range=(pt1[d]>pt2[d])?(pt1[d]>pt3[d]?pt1[d]:pt3[d]):(pt2[d]>pt3[d]?pt2[d]:pt3[d]);
		range-=(pt1[d]<pt2[d])?(pt1[d]<pt3[d]?pt1[d]:pt3[d]):(pt2[d]<pt3[d]?pt2[d]:pt3[d]);
		if(range<smrange||smrange==-1) {
			smrange=range;
			dsmall=d; }}
	swap=pt1[dim-1];pt1[dim-1]=pt1[dsmall];pt1[dsmall]=swap;	// swap dsmall and last
	swap=pt2[dim-1];pt2[dim-1]=pt2[dsmall];pt2[dsmall]=swap;
	swap=pt3[dim-1];pt3[dim-1]=pt3[dsmall];pt3[dsmall]=swap;
	trianglerandCD(pt1,pt2,pt3,dim-1,ans);												// find reduced dimensional answer

	zd=dim-1;
	coefx=pt1[yd]*(pt2[zd]-pt3[zd])+pt2[yd]*(pt3[zd]-pt1[zd])+pt3[yd]*(pt1[zd]-pt2[zd]);
	coefy=pt1[zd]*(pt2[xd]-pt3[xd])+pt2[zd]*(pt3[xd]-pt1[xd])+pt3[zd]*(pt1[xd]-pt2[xd]);
	coefz=pt1[xd]*(pt2[yd]-pt3[yd])+pt2[xd]*(pt3[yd]-pt1[yd])+pt3[xd]*(pt1[yd]-pt2[yd]);
	coefk=-(pt1[xd]*(pt2[yd]*pt3[zd]-pt3[yd]*pt2[zd])+pt2[xd]*(pt3[yd]*pt1[zd]-pt1[yd]*pt3[zd])+pt3[xd]*(pt1[yd]*pt2[zd]-pt2[yd]*pt1[zd]));
	if(coefz!=0) ans[zd]=-(coefk+coefx*x+coefy*y)/coefz;
	else ans[zd]=pt1[zd];

	swap=pt1[dim-1];pt1[dim-1]=pt1[dsmall];pt1[dsmall]=swap;	// swap dsmall and last
	swap=pt2[dim-1];pt2[dim-1]=pt2[dsmall];pt2[dsmall]=swap;
	swap=pt3[dim-1];pt3[dim-1]=pt3[dsmall];pt3[dsmall]=swap;
	swap=ans[dim-1];ans[dim-1]=ans[dsmall];ans[dsmall]=swap;

	if(xd==dsmall) xd=dim-1;
	if(yd==dsmall) yd=dim-1;

	return; }


void randtableD(double *a,int n,int eq) {
	int i;
	double dy;

	if(eq==1) {
		dy=2.0/n;
	  for(i=0;i<n/2;i++)
	    a[i]=SQRT2*inversefnD(erfnD,(i+0.5)*dy-1.0,-20,20,30);
	  for(i=n/2;i<n;i++)
	    a[i]=-a[n-i-1]; }
	else if(eq==2) {
		dy=1.0/SQRTPI/n;
		for(i=0;i<n;i++)
			a[i]=SQRT2*inversefnD(erfcintegralD,(i+0.5)*dy,0,20,30); }
	return; }


void randtableF(float *a,int n,int eq) {
	int i;
	float dy;

	if(eq==1) {
		dy=2.0/n;
	  for(i=0;i<n/2;i++)
	    a[i]=SQRT2*inversefn(erfn,(i+0.5)*dy-1.0,-20,20,30);
	  for(i=n/2;i<n;i++)
	    a[i]=-a[n-i-1]; }
	else if(eq==2) {
		dy=1.0/SQRTPI/n;
		for(i=0;i<n;i++)
			a[i]=SQRT2*inversefn(erfcintegral,(i+0.5)*dy,0,20,30); }
	return; }


void randshuffletableD(double *a,int n) {
	int i,j;
	double x;

	for(i=n-1;i>0;i--) {
		j=intrand(i+1);
		x=a[i];
		a[i]=a[j];
		a[j]=x; }
	return; }


void randshuffletableF(float *a,int n) {
	int i,j;
	float x;

	for(i=n-1;i>0;i--) {
		j=intrand(i+1);
		x=a[i];
		a[i]=a[j];
		a[j]=x; }
	return; }


void randshuffletableI(int *a,int n) {
	int i,j;
	int x;

	for(i=n-1;i>0;i--) {
		j=intrand(i+1);
		x=a[i];
		a[i]=a[j];
		a[j]=x; }
	return; }


void randshuffletableV(void **a,int n) {
	int i,j;
	void *x;

	for(i=n-1;i>0;i--) {
		j=intrand(i+1);
		x=a[i];
		a[i]=a[j];
		a[j]=x; }
	return; }


void showdist(int n,float low,float high,int bin) {
	int i,a[100],uflow=0,oflow=0,b;
	float x,sum=0,sum2=0;
//	float p[5];
//	float gauss[4096];

//	p[0]=0.1;p[1]=0.2;p[2]=0.7;p[3]=0.7;p[4]=1;
//	randtable(gauss,4096,1);
	if(bin>=100) bin=99;
	for(b=0;b<bin;b++)	a[b]=0;
	for(i=1;i<=n;i++)	{

//		x=binomrand(10,0,1);
//		x=thetarand();
//		x=intrandp(5,p);
//		x=poisrand(20);
		x=10*gaussrandF();
//		x=10*gauss[rand()&4095];

		b=(int) floor((x-low)*(bin-1)/(high-low)+0.5);
		if(b<0) uflow++;
		else if(b>=bin) oflow++;
		else a[b]++;
		sum+=x;
		sum2+=x*x; 	}
	printf("<%0.2f\t:",low-(high-low)/(bin-1)/2);
	for(i=1;i<=uflow;i++)	printf("x");
	for(b=0;b<bin;b++)	{
		printf("\n %0.2f\t:",b*(high-low)/(bin-1)+low);
		for(i=1;i<=a[b];i++)	printf("x");	}	
	printf("\n>%0.2f\t:",high+(high-low)/(bin-1)/2);
	for(i=1;i<=oflow;i++)	printf("x");
	printf("\n");
	printf("mean: %f\tstandard deviation: %f\n",sum/n,sqrt(sum2/n-sum/n*sum/n));
	return; }

