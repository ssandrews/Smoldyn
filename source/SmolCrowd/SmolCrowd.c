/* Steve Andrews, started 5/2007 */
/* Makes Crowding objects for Smoldyn. */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Geometry.h"
#include "math2.h"
#include "random2.h"
#include "RnSort.h"

#define VERSION 2.0

int Sphereintsct(int i,int j,double *spheres,int dim,double radextra);
double measurephi(double *low,double *high,double *spheres,int n,int dim,double radextra,int itmax,double originradmin,double originradmax);
double calcphi(double *low,double *high,double *spheres,int n,int dim,double originradmin,double originradmax);
int makeradhist(double *spheres,int n,int dim,double factor,double **histptr,double **scaleptr);
void freeradhist(double *hist,double *scale);
void fillradhist(double *spheres,int n,int dim,double *hist,double *scale,int hn);
int showradhist(double *spheres,int n,int dim);

void writespheres(double *spheres,int n,int dim,char *comment,double radextra,double *low,double *high);
int makespheres(int nnew,double rmin,double gamma,double *low,double *high,double *spheres,int n,int dim);
int compactspheres(double *spheres,int n,int dim);
int wraplastsphere(double *low,double *high,double *spheres,int n,int nmax,int dim);
int wrapspheres(int d,double *low,double *high,double *spheres,int n,int nmax,int dim);
int unwrapsphere(int i,double *low,double *high,double *spheres,int n,int dim);
int MakeSph2Phi(double phi,double rmin,double gamma,double *low,double *high,double *spheres,int n,int nmax,int dim,double originradmin,double originradmax);

void write2Dlines(void);


/* Returns 1 if spheres i and j intersect and 0 if not. */
int Sphereintsct(int i,int j,double *spheres,int dim,double radextra) {
	double *centi,*centj,ri,rj,dist2;
	int d;

	if(i==j) return 0;
	ri=spheres[4*i+dim];
	rj=spheres[4*j+dim];
	if(ri<0 || rj<0) return 0;
	ri+=radextra;
	rj+=radextra;
	centi=spheres+4*i;
	centj=spheres+4*j;
	dist2=0;
	for(d=0;d<dim;d++) dist2+=(centj[d]-centi[d])*(centj[d]-centi[d]);
	return dist2<=(ri+rj)*(ri+rj); }


/* Measures actual sphere coverage with random points. */
double measurephi(double *low,double *high,double *spheres,int n,int dim,double radextra,int itmax,double originradmin,double originradmax) {
	double phi,x[3],dist,rad;
	int it,d,hit,i,count,ignore;

	count=0;
	for(it=0;it<itmax;it++) {
		for(d=0;d<dim;d++) x[d]=unirandCOD(low[d],high[d]);
		ignore=0;
		if(originradmin>0 || originradmax>0) {
			rad=0;
			for(d=0;d<dim;d++) rad+=x[d]*x[d];
			rad=sqrt(rad);
			if(originradmin>0 && rad<originradmin) ignore=1;
			if(originradmax>0 && rad>originradmax) ignore=1; }
		if(ignore) it--;
		else {
			hit=0;
			for(i=0;i<n && !hit;i++) {
				dist=0;
				for(d=0;d<dim;d++) dist+=(x[d]-spheres[4*i+d])*(x[d]-spheres[4*i+d]);
				if(dist<=(spheres[4*i+dim]+radextra)*(spheres[4*i+dim]+radextra)) hit=1; }
			count+=hit; }}
	phi=(double)count/(double)it;
	return phi; }


/* Calculates phi, assuming no spheres overlap each other */
double calcphi(double *low,double *high,double *spheres,int n,int dim,double originradmin,double originradmax) {
	int i,d;
	double vol,voltot;

	vol=0;
	if(dim==1) {
		for(i=0;i<n;i++)
			if(spheres[4*i+dim]>0)
				if(spheres[4*i+0]+spheres[4*i+dim]<high[0])
					vol+=2*spheres[4*i+dim]; }
	else if(dim==2) {
		for(i=0;i<n;i++)
			if(spheres[4*i+dim]>0)
				if(spheres[4*i+0]+spheres[4*i+dim]<high[0] && spheres[4*i+1]+spheres[4*i+dim]<high[1])
					vol+=PI*spheres[4*i+dim]*spheres[4*i+dim]; }
	else if(dim==3) {
		for(i=0;i<n;i++)
			if(spheres[4*i+dim]>0)
				if(spheres[4*i+0]+spheres[4*i+dim]<high[0] && spheres[4*i+1]+spheres[4*i+dim]<high[1] && spheres[4*i+2]+spheres[4*i+dim]<high[2])
					vol+=4.0/3.0*PI*spheres[4*i+dim]*spheres[4*i+dim]*spheres[4*i+dim]; }

	voltot=1;
	for(d=0;d<dim;d++) voltot*=(high[d]-low[d]);
	if(originradmax>0) {
		if(dim==1) voltot=2*originradmax;
		else if(dim==2) voltot=PI*originradmax*originradmax;
		else voltot=4*PI/3*originradmax*originradmax*originradmax; }
	if(originradmin>0) {
		if(dim==1) voltot-=2*originradmin;
		else if(dim==2) voltot-=PI*originradmin*originradmin;
		else voltot-=4*PI/3*originradmin*originradmin*originradmin; }

	return vol/voltot; }


/*************** HISTOGRAM STUFF ********************/

/* Allocates and sets up a histogram for the radius distribution */
int makeradhist(double *spheres,int n,int dim,double factor,double **histptr,double **scaleptr) {
	double rmin,rmax,r,*hist,*scale;
	int i,hn;

	if(n<1) return 0;
	if(factor<=1) return 0;
	hist=scale=NULL;
	rmin=rmax=spheres[0+dim];
	for(i=1;i<n;i++) {
		r=spheres[4*i+dim];
		if(r<rmin) rmin=r;
		if(r>rmax) rmax=r; }
	rmin=floor(log(rmin)/log(factor));
	rmax=ceil(log(rmax)/log(factor));
	hn=(int)(rmax-rmin+0.5)+2;
	rmin*=log(factor);
	rmax*=log(factor);
	hist=(double*)calloc(hn,sizeof(double));
	if(!hist) return 0;
	scale=(double*)calloc(hn,sizeof(double));
	if(!scale) {free(hist);return 0;}
	setuphistdbl(hist,scale,hn,rmin,rmax);
	for(i=0;i<hn-1;i++) scale[i]=exp(scale[i]);
	*histptr=hist;
	*scaleptr=scale;
	return hn; }


/* frees a radius distribtion histogram */
void freeradhist(double *hist,double *scale) {
	if(hist) free(hist);
	if(scale) free(scale);
	return; }


/* Fills in a previously allocated histogram for the radius distribution */
void fillradhist(double *spheres,int n,int dim,double *hist,double *scale,int hn) {
	int i;

	for(i=0;i<hn;i++) hist[i]=0;
	for(i=0;i<n;i++)
		hist[histbindbl(spheres[4*i+dim],scale,hn)]+=1.0;
	return; }


/* Creates, fills in, displays, and frees a histogram for the radius distribution */
int showradhist(double *spheres,int n,int dim) {
	double *hist,*scale,factor;
	int i,hn;

	printf("Enter histogram bin width factor (>1, e.g. 1.1) or 0 to stop: ");
	scanf("%lf",&factor);
	if(factor==0) return 1;
	if(factor<=1) return 0;
	hn=makeradhist(spheres,n,dim,factor,&hist,&scale);
	if(!hn) {
		printf("Failure in makeradhist\n");
		return 0; }
	fillradhist(spheres,n,dim,hist,scale,hn);
	printf("r_max  number\n");
	for(i=0;i<hn-1;i++)
		printf("%f %f\n",scale[i],hist[i]);
	printf("\n");
	freeradhist(hist,scale);
	return 0; }



/********************* END OF HISTROGRAM STUFF *****************/

/******************** cluster analysis **********************/

int countclusters(double *spheres,int n,int dim,double radextra) {
	int i,j,k,ncluster;
	int *sphereid;

	sphereid=(int*) calloc(n,sizeof(int));
	if(!sphereid) return -1;
	for(i=0;i<n;i++) sphereid[i]=i;		// sphereid is equal to sphere number

	for(i=0;i<n;i++)				// for each, sphereid is equal to lowest sphereid of spheres in cluster
		for(j=i+1;j<n;j++)
			if(sphereid[i]!=sphereid[j] && Sphereintsct(i,j,spheres,dim,radextra))
				for(k=0;k<n;k++)
					if(sphereid[k]==sphereid[j])
						sphereid[k]=sphereid[i];

	for(i=0;i<n;i++)
		printf("%i  %i (%lf,%lf) \n",i,sphereid[i],spheres[4*i],spheres[4*i+1]);

	for(i=0;i<n;i++)					// replace all duplicate sphereid values with -1
		for(j=0;j<i;j++)
			if(sphereid[j]==sphereid[i]) {
				sphereid[i]=-1;
				j=i; }

	ncluster=0;					// count number of clusters
	for(i=0;i<n;i++)
		if(sphereid[i]>=0) ncluster++;

	free(sphereid);
	return ncluster; }



/* Writes spheres to a Smoldyn readable file.  The file name is gotten from the
user.  An optional comment line is added to the file. */
void writespheres(double*spheres,int n,int dim,char *comment,double radextra,double *low,double *high) {
	int d,i,skip;
	double r,radextra2;
	FILE *fptr;
	char fname[256],yn[256],spname[256];

	printf("Enter output file name: ");
	scanf("%s",fname);
	fptr=fopen(fname,"w");
	if(!fptr) {printf("Failed to open for writing.\n");return; }

	fprintf(fptr,"# Smoldyn surface of spheres created by program SmolCrowd\n");
	fprintf(fptr,"# File name: %s\n",fname);
	if(comment) fprintf(fptr,"# %s\n",comment);
	fprintf(fptr,"\n");
	fprintf(fptr,"start_surface smolcrowd\n");
	fprintf(fptr,"action all(all) both reflect\n");
	fprintf(fptr,"color both magenta 0.4\n");
	fprintf(fptr,"polygon both edge\n");
	fprintf(fptr,"max_panels sphere %i\n",n);
	for(i=0;i<n;i++) {
		if(spheres[4*i+dim]>=0) {
			fprintf(fptr,"panel sphere ");
			for(d=0;d<dim;d++) fprintf(fptr,"%lf ",spheres[4*i+d]);
			r=spheres[4*i+d]+radextra;
			fprintf(fptr,"%lf ",r);
			if(dim==2) fprintf(fptr,"%i\n",(int)(20.0*sqrt(r)));
			else if(dim==3) fprintf(fptr,"%i %i\n",(int)(10.0*sqrt(r)),(int)(10.0*sqrt(r))); }}
	fprintf(fptr,"end_surface\n\n");

	printf("Do you want to define a compartment for the crowders? ");
	scanf("%s",yn);
	if(yn[0]=='Y' || yn[0]=='y') {
		fprintf(fptr,"start_compartment smolcrowd\n");
		fprintf(fptr,"surface smolcrowd\n");
		for(i=0;i<n;i++) {
			if(spheres[4*i+dim]>=0) {
				fprintf(fptr,"point");
				for(d=0;d<dim;d++) fprintf(fptr," %lf",spheres[4*i+d]);
				fprintf(fptr,"\n"); }}
		fprintf(fptr,"end_compartment\n\n"); }

	printf("Do you want a molecule in the middle of each sphere? ");
	scanf("%s",yn);
	if(yn[0]=='Y' || yn[0]=='y') {
		printf("Do you want to include locations that are outside of the system volume? ");
		scanf("%s",yn);
		printf("Enter molecule species name: ");
		scanf("%s",spname);
		fprintf(fptr,"species %s\n\n",spname);
		for(i=0;i<n;i++) {
			if(spheres[4*i+dim]>=0) {
				skip=0;
				if(!(yn[0]=='Y' || yn[0]=='y')) {
					for(d=0;d<dim;d++)
						if(spheres[4*i+d]<low[d] || spheres[4*i+d]>high[d]) skip=1; }
				if(!skip) {
					fprintf(fptr,"mol 1 %s",spname);
					for(d=0;d<dim;d++) fprintf(fptr," %lf",spheres[4*i+d]);
					fprintf(fptr,"\n"); }}}
		fprintf(fptr,"\n"); }

	printf("Do you want double spheres that are concentric with the originals? ");
	scanf("%s",yn);
	if(yn[0]=='y' || yn[0]=='Y') {
		printf("Enter extra radius of double spheres: ");
		scanf("%lf",&radextra2);
		fprintf(fptr,"\n");
		fprintf(fptr,"# double spheres, extra radius is %lf\n",radextra2);
		fprintf(fptr,"start_surface smolcrowd2\n");
		fprintf(fptr,"action all(all) both reflect\n");
		fprintf(fptr,"color both green 0.4\n");
		fprintf(fptr,"polygon both edge\n");
		fprintf(fptr,"max_panels sphere %i\n",n);
		for(i=0;i<n;i++) {
			if(spheres[4*i+dim]>=0) {
				fprintf(fptr,"panel sphere ");
				for(d=0;d<dim;d++) fprintf(fptr,"%lf ",spheres[4*i+d]);
				r=spheres[4*i+d]+radextra2;
				fprintf(fptr,"%lf ",r);
				if(dim==2) fprintf(fptr,"%i\n",(int)(20.0*sqrt(r)));
				else if(dim==3) fprintf(fptr,"%i %i\n",(int)(10.0*sqrt(r)),(int)(10.0*sqrt(r))); }}
		fprintf(fptr,"end_surface\n\n"); }

	fprintf(fptr,"end_file\n");
	fclose(fptr);
	return; }



/* Adds nnew dim dimensional random spheres to a list of n existing spheres.
Their centers are uniformly randomly distributed between low and high, which are
dim dimensional vectors.  Set gamma to zero for all spheres to have radius rmin;
otherwise set gamma to a value less than -1 for a power law distribution of
radii with power gamma and minimum radius rmin.  The function returns the total
number of spheres in the list, which is n+nnew. */
int makespheres(int nnew,double rmin,double gamma,double *low,double *high,double *spheres,int n,int dim) {
	double r,cent[3];
	int i,j,d;

	for(i=0;i<nnew;i++) {
		j=i+n;
		for(d=0;d<dim;d++) cent[d]=unirandCOD(low[d],high[d]);
		r=gamma==0?rmin:powrandCOD(rmin,gamma);
		for(d=0;d<dim;d++) spheres[4*j+d]=cent[d];
		spheres[4*j+d]=r; }
	return n+nnew; }


/* Compacts a list of spheres, pushing all negative radius ones to the high
index end of the list.  Their center positions are not preserved.  Returns the
number of remaining spheres. */
int compactspheres(double *spheres,int n,int dim) {
	int i,j,d;

	i=j=0;
	while(j<n) {
		if(spheres[4*j+dim]<=0) j++;
		else if(i==j && spheres[4*i+dim]>0) {i++;j++;}
		else if(spheres[4*i+dim]<=0) {
			for(d=0;d<dim+1;d++) spheres[4*i+d]=spheres[4*j+d];
			spheres[4*j+dim]=-1;
			j++; }
		else i++; }
	for(i=0;i<n && spheres[4*i+dim]>0;i++)
		;
	return i; }


/* wraps just the last sphere, number n-1, on all dimensions */
int wraplastsphere(double *low,double *high,double *spheres,int n,int nmax,int dim) {
	double *cent,r;
	int j,d,i,d2,n2;

	cent=&spheres[4*(n-1)];
	r=spheres[4*(n-1)+dim];
	if(r<0) return n;
	j=n-1;
	for(d=0;d<dim;d++) {
		n2=n;
		for(i=j;i<n2 && n<nmax;i++) {
			if(cent[d]-r<low[d]) {
				for(d2=0;d2<dim+1;d2++) spheres[4*n+d2]=spheres[4*j+d2];
				spheres[4*n+d]+=high[d]-low[d];
				n++; }
			if(cent[d]+r>high[d]-low[d] && n<nmax) {
				for(d2=0;d2<dim+1;d2++) spheres[4*n+d2]=spheres[4*j+d2];
				spheres[4*n+d]-=high[d]-low[d];
				n++; }}}
	return n; }





/* Adds additional spheres to the list to account for spheres that overlap the
edges of periodic boundaries, only on dimension d.  n is the number of spheres
in the list, nmax is the total allocated space, and d is the dimension to be
wrapped.  low and high are dim dimensionsional vectors for the low and high
corners of space.  Returns the new total list length. */
int wrapspheres(int d,double *low,double *high,double *spheres,int n,int nmax,int dim) {
	int i,j,d2;
	double r;

	j=n;
	for(i=0;i<n && j<nmax-1;i++) {
		r=spheres[4*i+dim];
		if(r>0) {
			if(spheres[4*i+d]-r<low[d]) {
				for(d2=0;d2<dim+1;d2++) spheres[4*j+d2]=spheres[4*i+d2];
				spheres[4*j+d]+=high[d]-low[d];
				j++; }
			if(spheres[4*i+d]+r>high[d]) {
				for(d2=0;d2<dim+1;d2++) spheres[4*j+d2]=spheres[4*i+d2];
				spheres[4*j+d]-=high[d]-low[d];
				j++; }}}
	return j; }


/* Sphere i is being killed.  This also kills off all wrapped images of it and
returns the number that were killed. */
int unwrapsphere(int i,double *low,double *high,double *spheres,int n,int dim) {
	int d,d1,d2,d3,j,wrap[3],kill,same;
	double ri,*centi,centwp[3];

	centi=spheres+4*i;
	ri=spheres[4*i+dim];
	if(ri<0) return 0;
	wrap[0]=wrap[1]=wrap[2]=0;
	for(d=0;d<dim;d++) {
		if(centi[d]-ri<low[d]) wrap[d]=-1;
		else if(centi[d]+ri>high[d]) wrap[d]=1; }
	if(wrap[0]==0 && wrap[1]==0 && wrap[2]==0) return 0;

	kill=0;
	for(d1=0;d1<dim;d1++) {				// all singly wrapped image spheres
		if(wrap[d1]!=0) {
			for(d=0;d<dim;d++) centwp[d]=centi[d];
			if(wrap[d1]<0) centwp[d1]+=high[d1]-low[d1];
			else centwp[d1]-=high[d1]-low[d1];
			same=0;
			for(j=0;j<n && !same;j++) {
				same=1;
				for(d=0;d<dim && same;d++) if(centwp[d]!=spheres[4*j+d]) same=0; }
			if(same) {
				j--;
				spheres[4*j+dim]=-1;
				kill++; }}}

	for(d2=0;d2<dim;d2++)					// all doubly wrapped image spheres
		for(d1=0;d1<dim;d1++) {
			if(wrap[d1]!=0 && wrap[d2]!=0) {
				for(d=0;d<dim;d++) centwp[d]=centi[d];
				if(wrap[d1]<0) centwp[d1]+=high[d1]-low[d1];
				else centwp[d1]-=high[d1]-low[d1];
				if(wrap[d2]<0) centwp[d2]+=high[d2]-low[d2];
				else centwp[d2]-=high[d2]-low[d2];
				same=0;
				for(j=0;j<n && !same;j++) {
					same=1;
					for(d=0;d<dim && same;d++) if(centwp[d]!=spheres[4*j+d]) same=0; }
				if(same) {
					j--;
					spheres[4*j+dim]=-1;
					kill++; }}}

	for(d3=0;d3<dim;d3++)					// all triply wrapped image spheres
		for(d2=0;d2<dim;d2++)
			for(d1=0;d1<dim;d1++) {
				if(wrap[d1]!=0 && wrap[d2]!=0 && wrap[d3]!=0) {
					for(d=0;d<dim;d++) centwp[d]=centi[d];
					if(wrap[d1]<0) centwp[d1]+=high[d1]-low[d1];
					else centwp[d1]-=high[d1]-low[d1];
					if(wrap[d2]<0) centwp[d2]+=high[d2]-low[d2];
					else centwp[d2]-=high[d2]-low[d2];
					if(wrap[d3]<0) centwp[d3]+=high[d3]-low[d3];
					else centwp[d3]-=high[d3]-low[d3];
					same=0;
					for(j=0;j<n && !same;j++) {
						same=1;
						for(d=0;d<dim && same;d++) if(centwp[d]!=spheres[4*j+d]) same=0; }
					if(same) {
						j--;
						spheres[4*j+dim]=-1;
						kill++; }}}

	return kill; }



/* Adds spheres that do not overlap prior spheres until density is phi */
int MakeSph2Phi(double phi,double rmin,double gamma,double *low,double *high,double *spheres,int n,int nmax,int dim,double originradmin,double originradmax) {
	int block,maxblock,rmvmax,rmv,j,d,intsct,i2,i,kill;
	double vol,originrad;

	vol=1;
	maxblock=0;
	for(d=0;d<dim;d++) vol*=(high[d]-low[d]);
	if(dim==1) maxblock=(int) (vol/(2*rmin));			// maxblock is number of trials for adding before removing
	else if(dim==2) maxblock=(int) (vol/(PI*rmin*rmin));
	else if(dim==3) maxblock=(int) (vol/(4.0/3.0*PI*rmin*rmin*rmin));
	maxblock*=2;
	rmvmax=maxblock;

	n=compactspheres(spheres,n,dim);

	rmv=0;
	while(calcphi(low,high,spheres,n,dim,originradmin,originradmax)<phi && n<nmax && rmv<rmvmax) {

		for(block=0;block>=0 && block<maxblock;block++) {		// try adding until either success or maxblock times
			j=n;
			for(d=0;d<dim;d++) spheres[4*j+d]=unirandCOD(low[d],high[d]);
			spheres[4*j+dim]=gamma==0?rmin:powrandCOD(rmin,gamma);
			n++;
			n=wraplastsphere(low,high,spheres,n,nmax,dim);
			intsct=0;
			if(originradmin>0 || originradmax>0) {
				originrad=0;
				for(d=0;d<dim;d++) originrad+=spheres[4*j+d]*spheres[4*j+d];
				originrad=sqrt(originrad);
				if(originradmin>0 && originrad-spheres[4*j+dim]<originradmin) intsct=1;
				if(originradmax>0 && originrad+spheres[4*j+dim]>originradmax) intsct=1; }
			for(i2=j;i2<n && !intsct;i2++)
				for(i=0;i<n && !intsct;i++)
					intsct=Sphereintsct(i,i2,spheres,dim,0);
			if(intsct) n=j;
			else block=-5; }

		if(block==maxblock) {					// kill a random sphere
			if(rmv<rmvmax-1) {
				kill=0;
				while(!kill) {
					i=intrand(n);
					kill=gamma==0?1:coinrandD(pow(spheres[4*i+dim]/rmin,2*gamma)); }
				unwrapsphere(i,low,high,spheres,n,dim);
				spheres[4*i+dim]=-1;
				n=compactspheres(spheres,n,dim); }
			rmv++; }}

	return n; }


/*****************************/


void write2Dlines(void) {
	int dim,d,n,i;
	double low[3],high[3],len,x1[3],x2[3],theta;
	char fname[256];
	FILE *fptr;

	dim=2;
	for(d=0;d<dim;d++) {
		printf("Enter low and high coordinate on dimension %i: ",d);
		scanf("%lf %lf",&low[d],&high[d]); }
	printf("Enter number of lines and line length: ");
	scanf("%i %lf",&n,&len);
	printf("Enter output file name: ");
	scanf("%s",fname);
	fptr=fopen(fname,"w");
	if(!fptr) {printf("Failed to open for writing.\n");return; }
	fprintf(fptr,"# Smoldyn surface of lines (2-D triangles) created by program SmolCrowd\n");
	fprintf(fptr,"# File name: %s\n",fname);
	fprintf(fptr,"\n");
	fprintf(fptr,"max_panels t %i\n",n);
	for(i=0;i<n;i++) {
		x1[0]=unirandCOD(low[0],high[0]);
		x1[1]=unirandCOD(low[1],high[1]);
		theta=unirandCOD(0,2*PI);
		x2[0]=x1[0]+len*cos(theta);
		x2[1]=x1[1]+len*sin(theta);
		fprintf(fptr,"panel t %lf %lf %lf %lf\n",x1[0],x1[1],x2[0],x2[1]); }
	fprintf(fptr,"end_surface\n\n");
	fprintf(fptr,"end_file\n");
	fclose(fptr);
	printf("Done.\n");
	return; }




/*****************************/


int main(void) {
	int dim,d,nmax,n,i,done;
//	int ncluster;
	double low[3],high[3],vol,*spheres;
	double rmin,gamma,phi,mphi,radextra,originradmin,originradmax;
	char str[256];

	printf("----------------------------\n");
	printf("Runing SmolCrowd version %g\n",VERSION);
	randomize(-1);
	printf("Enter system dimensionality: ");
	scanf("%i",&dim);
	if(dim<1 || dim>3) printf("Illegal input.  Quit now, or let the program crash.\n");
	vol=1;
	for(d=0;d<dim;d++) {
		printf("Enter low and high coordinate on dimension %i: ",d);
		scanf("%lf %lf",&low[d],&high[d]);
		vol*=high[d]-low[d]; }
	printf("Enter desired crowding fraction (phi): ");
	scanf("%lf",&phi);
	printf("Enter crowder radius, or minimum radius if a range is desired: ");
	scanf("%lf",&rmin);
	printf("Enter 0 for all crowders the same size, or power law slope (-2 to -4) for a range: ");
	scanf("%lf",&gamma);
	printf("Enter crowder extra radius, which can overlap and is not part of phi: ");
	scanf("%lf",&radextra);
	printf("Enter smallest permitted crowder edge distance from the origin or 0 if none: ");
	scanf("%lf",&originradmin);
	printf("Enter largest permitted crowder edge distance from the origin or 0 if none: ");
	scanf("%lf",&originradmax);

	nmax=0;
	if(dim==1) nmax=(int) (vol/(2*rmin));
	else if(dim==2) nmax=(int) (vol/(PI*rmin*rmin));
	else if(dim==3) nmax=(int) (vol/(4.0/3.0*PI*rmin*rmin*rmin));
	nmax*=2;
	spheres=(double*)calloc(4*nmax,sizeof(double));
	if(!spheres) {printf("memory allocation error\n");return(0);}
	for(i=0;i<4*nmax;i++) spheres[i]=0;
	n=0;
	printf("Generating crowders...\n");
	n=MakeSph2Phi(phi,rmin,gamma,low,high,spheres,n,nmax,dim,originradmin,originradmax);

	printf("Calulating statistics...\n");
	phi=calcphi(low,high,spheres,n,dim,originradmin,originradmax);
	mphi=measurephi(low,high,spheres,n,dim,radextra,100000,originradmin,originradmax);
//	ncluster=countclusters(spheres,n,dim,radextra);
	printf("crowders: %i\n",n);
	printf("phi calculated from volume ratio: %lf\n",phi);
	printf("phi measured with random sampling: %lf\n",mphi);
//	printf("number of unconnected crowding clusters: %i\n",ncluster);
	sprintf(str,"dim=%i, rmin=%lf, gamma=%lf, calc. phi=%lf, measured phi=%lf\n",dim,rmin,gamma,phi,mphi);
//	sprintf(str+strlen(str),"# unconnected clusters=%i\n",ncluster);
	if(originradmin>0) sprintf(str+strlen(str),"# minimum crowder edge radius=%lf\n",originradmin);
	if(originradmax>0) sprintf(str+strlen(str),"# maximum crowder edge radius=%lf\n",originradmax);
	for(d=0;d<dim;d++) sprintf(str+strlen(str),"# boundaries %i %lf %lf\n",d,low[d],high[d]);
	writespheres(spheres,n,dim,str,radextra,low,high);

	if(gamma!=0) {
		printf("Ready to calculate actual crowder size ratio\n");
		done=0;
		while(!done)
			done=showradhist(spheres,n,dim); }

	printf("Done.\n");
	return 0; }



