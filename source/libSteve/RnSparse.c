/* Steven Andrews, 3/26/2025.
See documentation called RnSparse_doc.tex.
Copyright 2025 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <string.h>
#include "RnSparse.h"


/*********************** Memory allocation and freeing *******************/

/* sparseAllocBandM */
sparsematrix sparseAllocBandM(sparsematrix sprs, int maxrows, int bandsize, int BiCGSTAB) {
	int i,j,*newcol0,*newcol1,ccols;
	double *newmatrix;

	if(!sprs) {
		sprs=(sparsematrix)malloc(sizeof(struct sparsematrixstruct));
		if(!sprs) return NULL;
		sprs->type=band;
		sprs->maxrows=0;
		sprs->nrows=0;
		sprs->fcols=0;
		sprs->ccols=0;
		sprs->col0=NULL;
		sprs->col1=NULL;
		sprs->matrix=NULL;
		sprs->BiCGSTAB=0;
		sprs->BiC=NULL; }

	if(maxrows>=0 && (sprs->maxrows!=maxrows || sprs->ccols!=2*bandsize+1)) {
		ccols=2*bandsize+1;
		newcol0=(int*) calloc(maxrows,sizeof(int));
		newcol1=(int*) calloc(maxrows,sizeof(int));
		newmatrix=(double*)calloc(maxrows*ccols,sizeof(double));
		if(!newcol0 || !newcol1 || !newmatrix) return NULL;
		for(i=0;i<maxrows;i++) {
			newcol0[i]=i-bandsize;
			newcol1[i]=i+bandsize+1;
			if(newcol1[i]>maxrows) newcol1[i]=maxrows;
			for(j=0;j<ccols;j++)
				newmatrix[i*ccols+j]=0; }
		free(sprs->col0);
		free(sprs->col1);
		free(sprs->matrix);
		sprs->maxrows=maxrows;
		sprs->nrows=maxrows;
		sprs->fcols=maxrows;
		sprs->ccols=ccols;
		sprs->col0=newcol0;
		sprs->col1=newcol1;
		sprs->matrix=newmatrix; }

	if(BiCGSTAB) {
		if(!sprs->BiC) {
			sprs->BiC=(double**) calloc(8,sizeof(double*));
			if(!sprs->BiC) return NULL;
			for(i=0;i<8;i++)
				sprs->BiC[i]=NULL; }
		for(i=0;i<8;i++) {
			free(sprs->BiC[i]);
			sprs->BiC[i]=(double*) calloc(sprs->maxrows,sizeof(double));
			if(!sprs->BiC[i]) return NULL;
			for(j=0;j<sprs->maxrows;j++)
				sprs->BiC[i][j]=0; }
		sprs->BiCGSTAB=1; }

	return sprs; }


/* sparseFreeM */
void sparseFreeM(sparsematrix sprs) {
	int i;

	if(!sprs) return;
	if(sprs->BiC) {
		for(i=0;i<8;i++)
			free(sprs->BiC[i]);
		free(sprs->BiC); }
	free(sprs->col0);
	free(sprs->col1);
	free(sprs->matrix);
	free(sprs);
	return; }


/*********************** Matrix output *******************/

/* sparsePrintM */
void sparsePrintM(const sparsematrix sprs,int full) {
	int i,j,nrows,fcols,ccols,*col0,*col1;

	nrows=sprs->nrows;
	fcols=sprs->fcols;
	ccols=sprs->ccols;
	col0=sprs->col0;
	col1=sprs->col1;
	printf("maxrows=%i nrows=%i fcols=%i ccols=%i BiCGSTAB=%i\n",sprs->maxrows,nrows,fcols,ccols,sprs->BiCGSTAB);
	for(i=0;i<nrows;i++) {
		printf("%i -> %i |",col0[i],col1[i]);
		if(full) {
			for(j=0;j<col0[i];j++)
				printf(" 0 ");
			if(j==col0[i]) printf(" :");
			for(;j<col1[i] && j<fcols;j++)
				printf(" % 1.3g",sparseReadM(sprs,i,j));
			if(j-col0[i]==sprs->ccols) printf(" :");
			for(;j<fcols;j++)
				printf(" 0 "); }
		else {
			for(j=col0[i];j<col0[i]+ccols;j++) {
				if(j==0 || j==col1[i]) printf(" :");
				printf(" % 1.3g",sparseReadM(sprs,i,j)); }}
		printf("\n"); }
	return; }


/*********************** Matrix manipulation *******************/

/* sparseSetSizeM */
void sparseSetSizeM(sparsematrix sprs,int nrows,int fcols) {
	sprs->nrows=nrows;
	sprs->fcols=fcols;
	return; }


/* sparseClearM */
void sparseClearM(sparsematrix sprs) {
	int i,j,nrows,ccols;
	double *matrix;

	nrows=sprs->nrows;
	ccols=sprs->ccols;
	matrix=sprs->matrix;
	for(i=0;i<nrows;i++) {
		for(j=0;j<ccols;j++)
			matrix[i*ccols+j]=0; }
	return; }


/* sparseMultiplyMV */
void sparseMultiplyMV(const sparsematrix sprs,const double *vect,double *ans) {
	int i,j,nrows,*col0,*col1;

	nrows=sprs->nrows;
	col0=sprs->col0;
	col1=sprs->col1;
	for(i=0;i<nrows;i++) {
		ans[i]=0;
		for(j=col0[i];j<col1[i];j++)
			ans[i]+=sparseReadM(sprs,i,j)*vect[j]; }
	return; }


/* sparseAddIdentityM */
void sparseAddIdentityM(sparsematrix sprs,double factor) {
	int i,nrows;

	nrows=sprs->nrows;
	for(i=0;i<nrows;i++)
		sparseAddToM(sprs,i,i,factor);
	return; }


/* sparseMultiplyScalarM */
void sparseMultiplyScalarM(sparsematrix sprs,double scalar) {
	int i,j,nrows,*col0,*col1;

	nrows=sprs->nrows;
	col0=sprs->col0;
	col1=sprs->col1;
	for(i=0;i<nrows;i++)
		for(j=col0[i];j<col1[i];j++)
			sparseMultiplyBy(sprs,i,j,scalar);
	return; }


void sparseBiCGSTAB(const sparsematrix sprs,const double *b,double *x,double tolerance) {
	double *ax,*r,*rhat,rho,*p,*nu,alpha,*h,*s,*t,omega,rhop,beta,numer,denom,size;
	int i,iter,nrows;

	ax=sprs->BiC[0];
	r=sprs->BiC[1];
	rhat=sprs->BiC[2];
	p=sprs->BiC[3];
	nu=sprs->BiC[4];
	h=sprs->BiC[5];
	s=sprs->BiC[6];
	t=sprs->BiC[7];

	nrows=sprs->nrows;

	sparseMultiplyMV(sprs,x,ax);
	rho=0;
	for(i=0;i<nrows;i++) {
		r[i]=b[i]-ax[i];								// 1. r = b-Ax
		rhat[i]=r[i];										// 2. rhat = r
		rho+=rhat[i]*r[i]; 							// 3. rho = (rhat,r)
		p[i]=r[i]; }										// 4. p = r

	for(iter=0;iter<5;iter++) {				// 5. iterate
		sparseMultiplyMV(sprs,p,nu);		// 5.1 nu = A.p
		denom=0;
		for(i=0;i<nrows;i++)
			denom+=rhat[i]*nu[i];
		alpha=rho/denom; 								// 5.2 alpha = rho/(rhat,nu)
		size=0;
		for(i=0;i<nrows;i++) {
			h[i]=x[i]+alpha*p[i];					// 5.3 h = x+alpha*p
			s[i]=r[i]-alpha*nu[i];				// 5.4 s = r-alpha*nu
			size+=s[i]*s[i]; }
		if(iter>2 && size<tolerance) {	// 5.5 quit if s is small
			for(i=0;i<nrows;i++)
				x[i]=h[i];
			break; }
		sparseMultiplyMV(sprs,s,t);			// 5.6 t = A.s
		numer=denom=0;
		for(i=0;i<nrows;i++) {
			numer+=t[i]*s[i];
			denom+=t[i]*t[i]; }
		omega=numer/denom;							// 5.7 omega = (t,s)/(t,t)
		rhop=0;
		size=0;
		for(i=0;i<nrows;i++) {
			x[i]=h[i]+omega*s[i];					// 5.8 x = h+omega*s
			r[i]=s[i]-omega*t[i];					// 5.9 r = s-omega*t
			size+=r[i]*r[i];
			rhop+=rhat[i]*r[i]; }					// 5.11 rho' = (rhat,r)
		if(iter>2 && size<tolerance)		// 5.10 quit if r is small
			break;
		beta=(rhop/rho)*(alpha/omega);	// 5.12 beta = (rho'/rho)*(alpha/omega)
		rho=rhop;												// rho = rho'
		for(i=0;i<nrows;i++)
			p[i]=r[i]+beta*(p[i]-omega*nu[i]); }	// 5.13 p = r+beta*(p-omega*nu)

	return; }


