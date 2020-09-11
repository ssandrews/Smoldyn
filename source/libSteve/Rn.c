/* Steven Andrews, 11/10/90.
See documentation called Rn_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include "Rn.h"
#include "random2.h"
#include <math.h>
#include <string.h>
#include "math2.h"

float detpart(float *a,int n,char s[],int i);


int makeV(float *c,int n,char *s) {
	int i,as;

	as=0;
	for(i=0;i<n;i++)	{
		if(sscanf(s,"%f",&c[i])) as++;
		else c[i]=0;
		s=strchr(s,' ');
		if(s) s++;	}
	return as;	}


int makeM(float *c,int m,int n,char *s) {
	return makeV(c,m*n,s);	}


float *setstdV(float *c,int n,int k)	{
	int i;
	
	if(k==0)	for(i=0;i<n;i++)	c[i]=0;
	else if(k==1)	for(i=0;i<n;i++)	c[i]=1;
	else if(k<0)	{
		for(i=0;i<n;i++)	c[i]=0;
		c[-k]=1;	}
	else if(k==2)	for(i=0;i<n;i++)	c[i]=i;
	else if(k==3)	for(i=0;i<n;i++)	c[i]=randCCF();
	return c; }


float *setstdM(float *c,int m,int n,int k)	{
	int i,j;
	
	if(k==0)
		for(i=0;i<m;i++)
			for(j=0;j<n;j++)	c[n*i+j]=0;
	else if(k==1)
		for(i=0;i<m;i++)
			for(j=0;j<n;j++)	c[n*i+j]=1.0*(i==j);
	else if(k==2)
		for(i=0;i<m;i++)
			for(j=0;j<n;j++)	c[n*i+j]=1;
	else if(k==3)
		for(i=0;i<m;i++)
			for(j=0;j<n;j++)	c[n*i+j]=randCCF();
	return c;	}


float *DirCosM(float *c,float theta,float phi,float chi)	{
	float cp,ct,cc,sp,st,sc;

	cp=cos(phi);
	ct=cos(theta);
	cc=cos(chi);
	sp=sin(phi);
	st=sin(theta);
	sc=sin(chi);
	c[0]=cp*ct*cc-sp*sc;
	c[1]=sp*ct*cc+cp*sc;
	c[2]=-st*cc;
	c[3]=-cp*ct*sc-sp*cc;
	c[4]=-sp*ct*sc+cp*cc;
	c[5]=st*sc;
	c[6]=cp*st;
	c[7]=sp*st;
	c[8]=ct;
	return c;	}


double *DirCosMD(double *c,double theta,double phi,double chi)	{
	double cp,ct,cc,sp,st,sc;

	cp=cos(phi);
	ct=cos(theta);
	cc=cos(chi);
	sp=sin(phi);
	st=sin(theta);
	sc=sin(chi);
	c[0]=cp*ct*cc-sp*sc;
	c[1]=sp*ct*cc+cp*sc;
	c[2]=-st*cc;
	c[3]=-cp*ct*sc-sp*cc;
	c[4]=-sp*ct*sc+cp*cc;
	c[5]=st*sc;
	c[6]=cp*st;
	c[7]=sp*st;
	c[8]=ct;
	return c;	}


float *DirCosM2(float *c,float theta) {
  c[0]=c[3]=cos(theta);
  c[2]=-(c[1]=sin(theta));
  return c; }


double *DirCosM2D(double *c,double theta) {
  c[0]=c[3]=cos(theta);
  c[2]=-(c[1]=sin(theta));
  return c; }


float *printV(float *a,int n)	{
	int i,er=0;

	if(!a) return NULL;
	if(n) if(printf("%f",a[0])<0) er=1;
	for(i=1;i<n;i++) if(printf(" %f",a[i])<0) er=1;
	if(printf("\n")<0) er=1;
	return er?NULL:a;	}


double *printVD(double *a,int n)	{
	int i,er=0;

	if(!a) return NULL;
	if(n) if(printf("%g",a[0])<0) er=1;
	for(i=1;i<n;i++) if(printf(" %g",a[i])<0) er=1;
	if(printf("\n")<0) er=1;
	return er?NULL:a;	}


float *fprintV(FILE *stream,float *a,int n)	{
	int i,er=0;

	if(!a) return NULL;
	for(i=0;i<n;i++) if(fprintf(stream,"%f ",a[i])<0) er=1;
	if(fprintf(stream,"\n")<0) er=1;
	return er?NULL:a;	}


double *fprintVD(FILE *stream,double *a,int n)	{
	int i,er=0;

	if(!a) return NULL;
	for(i=0;i<n;i++) if(fprintf(stream,"%g ",a[i])<0) er=1;
	if(fprintf(stream,"\n")<0) er=1;
	return er?NULL:a;	}


float *printM(float *a,int m,int n,char s[])	{
	int i,j,er=0;
	char dft[]="%f ";

	if(!a) return NULL;
	if(!s) s=dft;
	else if(s[0]=='\0')	s=dft;
	for(i=0;i<m;i++) {
		for(j=0;j<n;j++)
			if(printf(s,a[n*i+j])<0) er=1;
		if(printf("\n")<0) er=1; }
	return er?NULL:a;	}


float *sprintM(float *a,int m,int n,char s[],char *t,int tn)	{
	int i,j,ti;
	char dft[]="%f ";
	char s2[255];

	if(!a) return NULL;
	if(!s) s=dft;
	else if(s[0]=='\0') s=dft;
	ti=0;
	for(i=0;i<m;i++) {
		for(j=0;j<n;j++)
			if(snprintf(s2,255,s,a[n*i+j])<tn-ti-1)
				ti+=sprintf(t+ti,s,a[n*i+j]);
		if(tn-ti>1) ti+=sprintf(t+ti,"\n"); }
	return a;	}


float minV(float *a,int n) {
	float min;
	int i;
	
	min=a[0];
	for(i=1;i<n;i++) if(a[i]<min) min=a[i];
	return min;	}


float maxV(float *a,int n) {
	float max;
	int i;
	
	max=a[0];
	for(i=1;i<n;i++) if(a[i]>max) max=a[i];
	return max;	}


double maxVD(double *a,int n,int *indx) {
	double max;
	int i,imax;

	max=a[imax=0];
	for(i=1;i<n;i++) if(a[i]>max) max=a[imax=i];
	if(indx) *indx=imax;
	return max;	}


double minVD(double *a,int n,int *indx) {
	double min;
	int i,imin;

	min=a[imin=0];
	for(i=1;i<n;i++) if(a[i]<min) min=a[imin=i];
	if(indx) *indx=imin;
	return min;	}


int equalV(float *a,float *b,int n)	{
	int i;
	
	for(i=0;i<n;i++) if(a[i]!=b[i]) return 0;
	return 1;	}


int isevenspV(float *a,int n,float tol) {
	int i;
	float sp;

	if(n<2) return 0;
	sp=(a[n-1]-a[0])/(n-1);
	tol*=fabs(sp);
	for(i=1;i<n;i++)
		if(fabs(a[i]-a[i-1]-sp)>tol) return 0;
	return 1; }


float detpart(float *a,int n,char s[],int i) {
	int j,p;
	float sum;
	
	if(i==n-1)	{
		for(j=0;s[j];j++)
			;
		return a[n*i+j]; }
	else	{
		sum=0;
		p=1;
		for(j=0;j<n;j++)
			if(!s[j])	{
				s[j]=1;
				sum+=p*a[n*i+j]*detpart(a,n,s,i+1);
				p=-p;
				s[j]=0; }
		return sum; }}


float detM(float *a,int n)	{
	int j;
	char *s;
	float det;
	
	s=(char *) calloc(n,sizeof(char));
	if(!s) return 0;
	for(j=0;j<n;j++)	s[j]=0;
	det=detpart(a,n,s,0);
	free(s);
	return det;	}


float traceM(float *a,int n)	{
	int j;
	float ans;

	ans=0;
	for(j=0;j<n;j++)	ans+=a[j*n+j];
	return ans;	}


double traceMD(double *a,int n)	{
	int j;
	double ans;

	ans=0;
	for(j=0;j<n;j++)	ans+=a[j*n+j];
	return ans;	}


int issymmetricMD(double *a,int n) {
	int i,j,ans;

	ans=1;
	for(i=1;i<n&&ans;i++)
		for(j=0;j<i&&ans;j++)
			if(a[n*i+j]!=a[n*j+i]) ans=0;
	return ans; }


float *columnM(float *a,float *c,int m,int n,int j)	{
	int i;

	for(i=0;i<m;i++)
		c[i]=a[n*i+j];
	return c;	}


float *copyV(float *a,float *c,int n)	{
	int i;

	for(i=0;i<n;i++)	c[i]=a[i];
	return c;	}


double *copyVD(double *a,double *c,int n)	{
	int i;

	for(i=0;i<n;i++)	c[i]=a[i];
	return c;	}


float *copyM(float *a,float *c,int m,int n)	{
	int i;
	
	n*=m;
	for(i=0;i<n;i++)	c[i]=a[i];
	return c;	}


float *leftrotV(float *a,float *c,int n,int k) {
	int i,i2,gcd;
	float temp;
	
	if(k<0) k+=((-k)/n+1)*n;
	else k-=(k/n)*n;
	if(k==0) return copyV(a,c,n);
	gcd=gcomdiv(n,k);
	for(i2=0;i2<gcd;i2++) {
		temp=a[i2];
		for(i=i2;(i+k)%n!=i2;i=(i+k)%n)
			c[i]=a[(i+k)%n];
		c[i]=temp; }
	return c; }


float *sumV(float ax,float *a,float bx,float *b,float *c,int n)	{
	int i;
	
	for(i=0;i<n;i++)	c[i]=ax*a[i]+bx*b[i];
	return c;	}


double *sumVD(double ax,double *a,double bx,double *b,double *c,int n)	{
	int i;

	for(i=0;i<n;i++) c[i]=ax*a[i]+bx*b[i];
	return c;	}


float *sumM(float ax,float *a,float bx,float *b,float *c,int m,int n)	{
	int i;
	
	n*=m;
	for(i=0;i<n;i++)	c[i]=ax*a[i]+bx*b[i];
	return c;	}

	
float *addKV(float k,float *a,float *c,int n)	{
	int i;

	for(i=0;i<n;i++)	c[i]=k+a[i];
	return c;	}


float *multKV(float k,float *a,float *c,int n)	{
	int i;

	for(i=0;i<n;i++)	c[i]=k*a[i];
	return c;	}


float *divKV(float k,float *a,float *c,int n)	{
	int i;

	for(i=0;i<n;i++)	c[i]=k/a[i];
	return c;	}


float *multV(float *a,float *b,float *c,int n)	{
	int i;
	
	for(i=0;i<n;i++)	c[i]=a[i]*b[i];
	return c;	}

	
float *multM(float *a,float *b,float *c,int m,int n)	{
	int i;
	
	n*=m;
	for(i=0;i<n;i++)	c[i]=a[i]*b[i];
	return c;	}


float *divV(float *a,float *b,float *c,int n)	{
	int i;
	
	for(i=0;i<n;i++)	c[i]=a[i]/b[i];
	return c;	}

	
float *divM(float *a,float *b,float *c,int m,int n)	{
	int i;
	
	n*=m;
	for(i=0;i<n;i++)	c[i]=a[i]/b[i];
	return c;	}


float *transM(float *a,float *c,int m,int n)	{
	int i,j;
	
	for(i=0;i<m;i++)	for(j=0;j<n;j++)	c[m*j+i]=a[n*i+j];
	return c;	}


float dotVV(float *a,float *b,int n)	{
	float x=0;
	int i;
	
	for(i=0;i<n;i++)	x+=a[i]*b[i];
	return x;	}


double dotVVD(double *a,double *b,int n)	{
	double x=0;
	int i;
	
	for(i=0;i<n;i++)	x+=a[i]*b[i];
	return x;	}


void crossVV(float *a,float *b,float *c) {
	c[0]=a[2]*b[3]-a[3]*b[2];
	c[1]=a[3]*b[1]-a[1]*b[3];
	c[2]=a[1]*b[2]-a[2]*b[1];
	return; }


float distanceVV(float *a,float *b,int n) {
  int i;
  double x=0;

  for(i=0;i<n;i++) x+=(a[i]-b[i])*(a[i]-b[i]);
  return sqrt(x); }


double distanceVVD(double *a,double *b,int n) {
  int i;
  double x=0;

  for(i=0;i<n;i++) x+=(a[i]-b[i])*(a[i]-b[i]);
  return sqrt(x); }


float normalizeV(float *a,int n) {
	int i;
	float x=0;

	for(i=0;i<n;i++) x+=a[i]*a[i];
	if(!x) return 0;
	x=sqrt(x);
	for(i=0;i<n;i++) a[i]/=x;
	return x; }


float normalizeVD(double *a,int n) {
	int i;
	double x=0;

	for(i=0;i<n;i++) x+=a[i]*a[i];
	if(!x) return 0;
	x=sqrt(x);
	for(i=0;i<n;i++) a[i]/=x;
	return x; }


float averageV(float *a,int n,int p) {
	int i;
	double x=0;
	
	if(p==1)
		for(i=0;i<n;i++) x+=a[i];
	else if(p==2)
		for(i=0;i<n;i++) x+=a[i]*a[i];
	else if(p==-1)
		for(i=0;i<n;i++) x+=1.0/a[i];
	else if(p==0) x=n;
	else for(i=0;i<n;i++) x+=pow(a[i],p);
	return x/n; }


float *dotVM(float *a,float *b,float *c,int m,int n)	{
	int i,j;
	
	for(j=0;j<n;j++)	{
		c[j]=0;
		for(i=0;i<m;i++)	c[j]+=a[i]*b[n*i+j];	}
	return c;	}


float *dotMV(float *a,float *b,float *c,int m,int n)	{
	int i,j;

	for(i=0;i<m;i++)	{
		c[i]=0;
		for(j=0;j<n;j++)	c[i]+=a[n*i+j]*b[j];	}
	return c;	}


double *dotMVD(double *a,double *b,double *c,int m,int n)	{
	int i,j;

	for(i=0;i<m;i++)	{
		c[i]=0;
		for(j=0;j<n;j++)	c[i]+=a[n*i+j]*b[j];	}
	return c;	}


float *dotMM(float *a,float *b,float *c,int ma,int na,int nb)	{
	int i,j,k;
	
	for(i=0;i<ma;i++)
		for(k=0;k<nb;k++)	{
			c[nb*i+k]=0;
			for(j=0;j<na;j++)	c[nb*i+k]+=a[na*i+j]*b[nb*j+k];	}
	return c;	}


double *dotMMD(double *a,double *b,double *c,int ma,int na,int nb)	{
	int i,j,k;
	
	for(i=0;i<ma;i++)
		for(k=0;k<nb;k++)	{
			c[nb*i+k]=0;
			for(j=0;j<na;j++)	c[nb*i+k]+=a[na*i+j]*b[nb*j+k];	}
	return c;	}


float minorM(float *a,int n,char *row,char *col)	{
	int i,j,sgn;
	float sum;
	
	for(i=0;row[i]&&i<n;i++)
		;
	if(i==n) return 1;
	row[i]=1;
	sum=0;
	sgn=1;
	for(j=0;j<n;j++)
		if(!col[j])	{
			col[j]=1;
			sum+=sgn*a[n*i+j]*minorM(a,n,row,col);
			sgn=-sgn;
			col[j]=0; }
	row[i]=0;
	return sum;	}


float invM(float *a,float *c,int n)	{
	int i,j;
	char *row,*col;
	float det;
	
	det=detM(a,n);
	if(det)	{
		row=(char *) calloc(n,sizeof(char));
		if(!row) return 0;
		col=(char *) calloc(n,sizeof(char));
		if(!col) return 0;
		for(j=0;j<n;j++)	row[j]=col[j]=0;
		for(i=0;i<n;i++)	{
			row[i]=1;
			for(j=0;j<n;j++)	{
				col[j]=1;
				c[n*j+i]=minus1to(i+j)*minorM(a,n,row,col)/det;
				col[j]=0; }
			row[i]=0; }}
	return det;	}


float *deriv1V(float *a,float *c,int n)	{
	int i;

	if(n==1) c[0]=0;
	else if(n==2) c[0]=c[1]=a[1]-a[0];
	else {
		c[0]=-1.5*a[0]+2.0*a[1]-0.5*a[2];
		for(i=1;i<n-1;i++)
			c[i]=(a[i+1]-a[i-1])/2.0;
		c[n-1]=0.5*a[n-3]-2.0*a[n-2]+1.5*a[n-1]; }
	return c;	}


float *deriv2V(float *a,float *c,int n)	{
	int i;

	if(n==1) c[0]=0;
	else if(n==2) c[0]=c[1]=0;
	else {
		c[0]=a[0]+a[2]-2.0*a[1];
		for(i=1;i<n-1;i++)
			c[i]=a[i-1]+a[i+1]-2.0*a[i];
		c[n-1]=a[n-3]+a[n-1]-2.0*a[n-2]; }
	return c;	}


float *integV(float *a,float *c,int n)	{
	int i;

	c[0]=a[0]/2;
	for(i=1;i<n;i++)
		c[i]=c[i-1]+(a[i-1]+a[i])/2;
	return c;	}


float *smoothV(float *a,float *c,int n,int k)	{
	int i,j;
	float *smo,norm;

	if(k<0) return NULL;
	smo=allocV(2*k+1);
	if(!smo) return NULL;
	smo+=k;
	for(j=-k;j<=k;j++) smo[j]=choose(2*k,j+k);
	setstdV(c,n,0);

	for(i=0;i<n;i++) {
		norm=0;
		for(j=-k;j<=k;j++)
			if(i+j>=0&&i+j<n) {
				norm+=smo[j];
				c[i]+=smo[j]*a[i+j]; }
		c[i]/=norm; }
	freeV(smo-k);
	return c;	}


float *convolveV(float *a,float *b,float *c,int na,int nb,int nc,int bz,float left,float right) {
	int i,j,blo,bhi;
	double sum;

	blo=-bz;
	bhi=nb-bz;
	b+=bz;
	for(i=0;i<nc;i++) {
		sum=0;
		for(j=blo;i-j>=na&&j<bhi;j++) sum+=right*b[j];
		for(;i-j>=0&&j<bhi;j++) sum+=a[i-j]*b[j];
		for(;j<bhi;j++) sum+=left*b[j];
		c[i]=sum; }
	return c; }


float *correlateV(float *a,float *b,float *c,int na,int nb,int nc,int bz,float left,float right) {
	int i,j,blo,bhi;
	double sum;

	blo=-bz;
	bhi=nb-bz;
	b+=bz;
	for(i=0;i<nc;i++) {
		sum=0;
		for(j=blo;i+j<0&&j<bhi;j++) sum+=left*b[j];
		for(;i+j<na&&j<bhi;j++) sum+=a[i+j]*b[j];
		for(;j<bhi;j++) sum+=right*b[j];
		c[i]=sum; }
	return c; }


int histogramV(float *a,float *h,float lo,float hi,int na,int nh) {
	int asgn,i,j;
	float delta;

	delta=(hi-lo)/(nh-1);
	asgn=na;
	for(j=0;j<nh;j++) h[j]=0;
	for(i=0;i<na;i++) {
		j=(int)floor((a[i]-lo)/delta)+1;
		if(j<=0) h[0]+=1.0;
		else if(j<nh) h[j]+=1.0;
		else asgn--; }
	return asgn; }


int histogramVdbl(double *a,double *h,double lo,double hi,int na,int nh) {
	int asgn,i,j;
	double delta;

	delta=(hi-lo)/(nh-1);
	asgn=na;
	for(j=0;j<nh;j++) h[j]=0;
	for(i=0;i<na;i++) {
		j=(int)floor((a[i]-lo)/delta)+1;
		if(j<=0) h[0]+=1.0;
		else if(j<nh) h[j]+=1.0;
		else asgn--; }
	return asgn; }




