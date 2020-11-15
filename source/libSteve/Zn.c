/* Steven Andrews, 10/17/01.
Simple routines for manipulating vectors of integers.
See documentation called Zn_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <string.h>
#include "Zn.h"
#include "math2.h"
#include "random2.h"

int *setstdZV(int *c,int n,int k) {
	int i;

	if(k==0) for(i=0;i<n;i++) c[i]=0;
	else if(k==1) for(i=0;i<n;i++) c[i]=1;
	else if(k<0) {
		for(i=0;i<n;i++) c[i]=0;
		c[-k]=1; }
	else if(k==2) for(i=0;i<n;i++) c[i]=i;
	else if(k==3) for(i=0;i<n;i++) c[i]=intrand(2);
	return c; }


int *printZV(int *c,int n) {
  int i;

  for(i=0;i<n;i++) printf("%i ",c[i]);
  printf("\n");
  return c; }


char *Zn_vect2csvstring(int *c,int n,char *string) {
  int i;

	if(n>0) snprintf(string, n*sizeof(string),"%i",c[0]);
  for(i=1;i<n;i++) snprintf(string+strlen(string),sizeof(string)-strlen(string),",%i",c[i]);
  return string; }


int *fprintZV(FILE *stream,int *c,int n) {
  int i;

	if(n) fprintf(stream,"%i",c[0]);
  for(i=1;i<n;i++) fprintf(stream," %i",c[i]);
  fprintf(stream,"\n");
  return c; }


int minZV(int *a,int n) {
  int min,i;

  min=a[0];
  for(i=1;i<n;i++) if(a[i]<min) min=a[i];
  return min; }


int maxZV(int *a,int n) {
  int max,i;

  max=a[0];
  for(i=1;i<n;i++) if(a[i]>max) max=a[i];
  return max; }


int *copyZV(int *a,int *c,int n) {
  for(n--;n>=0;n--) c[n]=a[n];
  return c; }


int *sumZV(float ax,int *a,float bx,int *b,int *c,int n) {
  int i;

  for(i=0;i<n;i++) c[i]=(int)(ax*a[i]+bx*b[i]);
  return c; }


int *deriv1ZV(int *a,int *c,int n) {
  int i;

  c[0]=(-3*a[0]+4*a[1]-a[2])/2;
  for(i=1;i<n-1;i++) c[i]=(a[i+1]-a[i-1])/2;
  c[n-1]=(a[n-3]-4*a[n-2]+3*a[n-1])/2;
  return c; }


int *deriv2ZV(int *a,int *c,int n) {
  int i;

  c[0]=a[0]+a[2]-2*a[1];
  for(i=1;i<n-1;i++) c[i]=a[i-1]+a[i+1]-2*a[i];
  c[n-1]=a[n-3]+a[n-1]-2*a[n-2];
  return c; }


int productZV(int *a,int n) {
	int i,p;
	
	p=1;
	for(i=0;i<n;i++) p*=a[i];
	return p; }


int intfindZV(int *a,int n,int i) {
	int j;
	
	for(j=0;j<n&&a[j]!=i;j++);
	return j<n?j:-1; }


int indx2addZV(int *indx,int *dim,int rank) {
	int i,add;
	
	add=indx[0];
	for(i=1;i<rank;i++)	add=dim[i]*add+indx[i];
	return add; }


int *add2indxZV(int add,int *indx,int *dim,int rank) {
	int i;
	
	for(i=rank-1;i>0;i--) {
		indx[i]=add%dim[i];
		add/=dim[i]; }
	indx[0]=add;
	return indx; }


int indx2add3ZV(int *indx,int rank) {
	int i,add;

	add=indx[0];
	for(i=1;i<rank;i++)	add=3*add+indx[i];
	return add; }


int nextaddZV(int add,int *indx1,int *indx2,int *dim,int rank) {
	int i,done;

	done=0;
	for(i=rank-1;i>=0&&!done;i--) {
		if(add%dim[i]<indx2[i]) done=1;
		else add/=dim[i]; }
	if(!done) return 1+indx2addZV(indx2,dim,rank);
	add++;
	for(i+=2;i<rank;i++) add=dim[i]*add+indx1[i];
	return add; }


int *add2indx3ZV(int add,int *indx,int rank) {
	int i;
	
	for(i=rank-1;i>0;i--) {
		indx[i]=add%3;
		add/=3; }
	indx[0]=add;
	return indx; }


int neighborZV(int *indx,int *c,int *dim,int rank,int type,int *wrap,int *mid) {
	int i,n,p,ad,k,j;
	static int *ofst=NULL,*wp=NULL,rankold=0;

	n=0;
	if(type==-1||rank!=rankold) {
		if(ofst) freeZV(ofst);
		if(wp) freeZV(wp);
		ofst=wp=NULL;
		rankold=rank; }

	if(type==0) {
		p=productZV(dim,rank);
		ad=indx2addZV(indx,dim,rank);
		for(i=0;i<rank;i++) {
			p/=dim[i];
			if(indx[i]>0) c[n++]=ad-p; }
		if(mid) *mid=n;
		for(i=rank-1;i>=0;i--) {
			if(indx[i]<dim[i]-1) c[n++]=ad+p;
			p*=dim[i]; }}
	else if(type==1) {
		p=productZV(dim,rank);
		ad=indx2addZV(indx,dim,rank);
		for(i=0;i<rank;i++) {
			p/=dim[i];
			if(indx[i]>0) c[n++]=ad-p;
			else c[n++]=ad+(dim[i]-1)*p;
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }
		if(mid) *mid=n;
		for(i=rank-1;i>=0;i--) {
			if(indx[i]<dim[i]-1) c[n++]=ad+p;
			else c[n++]=ad-(dim[i]-1)*p;
			p*=dim[i];
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }}
	else if(type==2) {
		if(!ofst) ofst=allocZV(rank);
		if(!ofst) return -1;
		p=intpower(3,rank);
		for(i=0;i<p;i++) {
			if(i==p/2) {if(mid) *mid=n;i++;}
			add2indx3ZV(i,ofst,rank);
			for(j=0;j<rank;j++) {
				ofst[j]+=indx[j]-1;
				if(ofst[j]<0||ofst[j]>=dim[j]) j=rank+1; }
			if(j==rank) c[n++]=indx2addZV(ofst,dim,rank); }}
	else if(type==3) {
		if(!ofst) ofst=allocZV(rank);
		if(!ofst) return -1;
		p=intpower(3,rank);
		for(i=0;i<p;i++) {
			if(i==p/2) {if(mid) *mid=n;i++;}
			add2indx3ZV(i,ofst,rank);
			for(j=0;j<rank;j++) {
				ofst[j]+=indx[j]-1;
				if(ofst[j]<0) ofst[j]=dim[j]-1;
				else if(ofst[j]>=dim[j]) ofst[j]=0; }
			c[n++]=indx2addZV(ofst,dim,rank);
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }}
	else if(type==4) {
		p=productZV(dim,rank);
		ad=indx2addZV(indx,dim,rank);
		for(i=0;i<rank;i++) {
			p/=dim[i];
			if(indx[i]>0) c[n++]=ad-p;
			else if(wrap[i]) c[n++]=ad+(dim[i]-1)*p;
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }
		if(mid) *mid=n;
		for(i=rank-1;i>=0;i--) {
			if(indx[i]<dim[i]-1) c[n++]=ad+p;
			else if(wrap[i]) c[n++]=ad-(dim[i]-1)*p;
			p*=dim[i];
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }}
	else if(type==5) {
		if(!ofst) ofst=allocZV(rank);
		if(!ofst) return -1;
		p=intpower(3,rank);
		for(i=0;i<p;i++) {
			if(i==p/2) {if(mid) *mid=n;i++;}
			add2indx3ZV(i,ofst,rank);
			for(j=0;j<rank;j++) {
				ofst[j]+=indx[j]-1;
				if(ofst[j]<0&&wrap[j]) ofst[j]=dim[j]-1;
				else if(ofst[j]>=dim[j]&&wrap[j]) ofst[j]=0;
				else if(ofst[j]<0||ofst[j]>=dim[j]) j=rank+1; }
			if(j==rank) c[n++]=indx2addZV(ofst,dim,rank);
			for(k=0;k<n-1;k++) if(c[k]==c[n-1]) n--; }}
	else if(type==6) {
		p=productZV(dim,rank);
		ad=indx2addZV(indx,dim,rank);
		if(!wp) wp=allocZV(rank);
		if(!wp) return -1;
		copyZV(wrap,wp,rank);
		setstdZV(wrap,2*rank,0);
		for(i=0;i<rank;i++) {
			p/=dim[i];
			if(indx[i]>0) c[n++]=ad-p;
			else if(wp[i]) {
				wrap[n]=1<<2*i;
				c[n++]=ad+(dim[i]-1)*p; }}
		if(mid) *mid=n;
		for(i=rank-1;i>=0;i--) {
			if(indx[i]<dim[i]-1) c[n++]=ad+p;
			else if(wp[i]) {
				wrap[n]=2<<2*i;
				c[n++]=ad-(dim[i]-1)*p; }
			p*=dim[i]; }}
	else if(type==7) {
		if(!wp) wp=allocZV(rank);
		if(!ofst) ofst=allocZV(rank);
		if(!ofst||!wp) return -1;
		p=intpower(3,rank);
		copyZV(wrap,wp,rank);
		for(i=0;i<p;i++) {
			wrap[n]=0;
			if(i==p/2) {if(mid) *mid=n;i++;}
			add2indx3ZV(i,ofst,rank);
			for(j=0;j<rank;j++) {
				ofst[j]+=indx[j]-1;
				if(ofst[j]<0&&wp[j]) {
					ofst[j]=dim[j]-1;wrap[n]+=1<<2*j; }
				else if(ofst[j]>=dim[j]&&wp[j]) {
					ofst[j]=0;wrap[n]+=2<<2*j; }
				else if(ofst[j]<0||ofst[j]>=dim[j]) j=rank+1; }
			if(j==rank) c[n++]=indx2addZV(ofst,dim,rank); }}
	return n; }


int Zn_sameset(const int *a,const int *b,int *work,int n) {
	int i,j,sum;

	for(j=0;j<n;j++) work[j]=0;
	sum=0;
	for(i=0;i<n;i++)
		for(j=0;j<n;j++)
			if(b[j]==a[i] && work[j]==0) {
				work[j]=1;
				sum++;
				j=n; }
	return (sum==n); }


void Zn_sort(int *a,int *b,int n) {
	int i,j,ir,l;
	int az,bz;

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



int Zn_issort(int *a,int n) {
	int i;

	if(n<=1) return 1;
	for(i=1;i<n&&a[i-1]==a[i];i++);
	if(i==n) return 1;
	for(i=1;i<n&&a[i-1]<a[i];i++);
	if(i==n) return 3;
	for(;i<n&&a[i-1]<=a[i];i++);
	if(i==n) return 2;
	for(i=1;i<n&&a[i-1]>a[i];i++);
	if(i==n) return -3;
	for(;i<n&&a[i-1]>=a[i];i++);
	if(i==n) return -2;
	return 0; }


int Zn_incrementcounter(int *a,int digits,int base) {
	int i;

	i=0;
	a[0]++;
	while(a[i]==base) {
		a[i]=0;
		i++;
		if(i==digits) return 1;
		a[i]++; }
	return 0; }


int Zn_permute(int *a,int *b,int n,int k) {
	int ans;

	if(n==0) ans=0;

	else if(n==1) {b[0]=a[0];ans=0; }

	else if(n==2) {
		if(k==0) {b[0]=a[0];b[1]=a[1];ans=1; }
		else     {b[0]=a[1];b[1]=a[0];ans=0; }
		if(a[0]==a[1]) ans=0; }

	else if(n==3) {
		if(k==0)      {b[0]=a[0];b[1]=a[1];b[2]=a[2];ans=1;}
		else if(k==1) {b[0]=a[0];b[1]=a[2];b[2]=a[1];ans=2;}
		else if(k==2) {b[0]=a[1];b[1]=a[0];b[2]=a[2];ans=3;}
		else if(k==3) {b[0]=a[1];b[1]=a[2];b[2]=a[0];ans=4;}
		else if(k==4) {b[0]=a[2];b[1]=a[0];b[2]=a[1];ans=5;}
		else          {b[0]=a[2];b[1]=a[1];b[2]=a[0];ans=0;}
		if(a[1]==a[2]&&ans==1) ans=2;
		else if(a[1]==a[2]&&(ans==4||ans==5)) ans=0;
		if(a[0]==a[1]&&(ans==2||ans==3)) ans=4;
		else if(a[0]==a[1]&&ans==5) ans=0;
		if(a[0]==a[2]&&(ans==3||ans==4||ans==5)) ans=0; }

	else ans=-1;

	return ans; }


/******** combinatorics *********/


int Zn_permutelex(int *seq,int n) {
	int i,j;
	int temp;
	
	i=n-1;
	while(i>0 && seq[i-1]>=seq[i]) i--;
	if(i==0) {						// input was final sequence
		i=0;
		j=n-1;
		while(i<j) {
			temp=seq[i];			// swap values at positions i and j
			seq[i]=seq[j];
			seq[j]=temp;
			i++;
			j--; }
		return 2; }
	
	j=n;
	while(seq[j-1]<=seq[i-1]) j--;
	
  temp=seq[i-1];			// swap values at positions (i-1) and (j-1)
	seq[i-1]=seq[j-1];
	seq[j-1]=temp;
	
	i++;
	j=n;
	while(i<j) {
		temp=seq[i-1];			// swap values at positions (i-1) and (j-1)
		seq[i-1]=seq[j-1];
		seq[j-1]=temp;
		i++;
		j--; }
	
	i=n-1;
	while(i>0 && seq[i-1]>=seq[i]) i--;
	if(i==0) return 1;	// at final sequence
	
	return 0; }













