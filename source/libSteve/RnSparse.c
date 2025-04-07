/* Steven Andrews, 3/26/2025.
See documentation called RnSparse_doc.tex.
Copyright 2025 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <string.h>
#include "RnSparse.h"


/*********************** Memory allocation and freeing *******************/

/* sparseAllocBandM */
sparsematrix sparseAllocBandM(sparsematrix sprs,int maxrows,int bandsize) {
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
		sprs->matrix=NULL; }

	if(sprs->maxrows!=maxrows || sprs->ccols!=2*bandsize+1) {
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

	return sprs; }


/* sparseFreeM */
void sparseFreeM(sparsematrix sprs) {
	if(!sprs) return;
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
	printf("maxrows=%i nrows=%i fcols=%i ccols=%i\n",sprs->maxrows,nrows,fcols,ccols);
	for(i=0;i<nrows;i++) {
		printf("%i -> %i |",col0[i],col1[i]);
		if(full) {
			for(j=0;j<col0[i];j++)
				printf(" 0 ");
			if(j==col0[i]) printf(" :");
			for(;j<col1[i];j++)
				printf(" % 1.3g",sparseReadM(sprs,i,j));
			if(col1[i]-col0[i]==sprs->ccols) printf(" :");
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

