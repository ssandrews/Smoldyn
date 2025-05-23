/* Steven Andrews 3/26/25.
See documentation called RnSparse_doc.tex.
Copyright 2025 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */


#ifndef __RnSparse_h
#define __RnSparse_h

#ifdef __cplusplus
extern "C" {
#endif

enum sparse_type {band};

typedef struct sparsematrixstruct{
  enum sparse_type type;     // matrix shape
  int maxrows;               // allocated matrix rows
  int nrows;                 // used matrix rows
  int fcols;                 // columns in full matrix
  int ccols;                 // columns in compressed matrix
  int *col0;                 // first column of compressed matrix
  int *col1;                 // 1+ last column of compressed matrix
  double *matrix;            // actual matrix data
  int BiCGSTAB;              // 1 if allocatad for BiCGSTAB
  double **BiC;	             // BiCSTAB vectors
  } *sparsematrix;


#include <stdlib.h>
#include <stdio.h>

#define sparseReadM(a,i,j) (a->matrix[(a->ccols)*(i)+(j)-(a->col0[i])])
#define sparseWriteM(a,i,j,x) ((a->matrix[(a->ccols)*(i)+(j)-(a->col0[i])])=(x))
#define sparseAddToM(a,i,j,x) ((a->matrix[(a->ccols)*(i)+(j)-(a->col0[i])])+=(x))
#define sparseMultiplyBy(a,i,j,x) ((a->matrix[(a->ccols)*(i)+(j)-(a->col0[i])])*=(x))

// Memory allocation and freeing
sparsematrix sparseAllocBandM(sparsematrix sprs, int maxrows, int bandsize, int BiCGSTAB);
void sparseFreeM(sparsematrix sprs);

// Matrix output
void sparsePrintM(const sparsematrix sprs,int full);

// Matrix manipulation
void sparseSetSizeM(sparsematrix sprs,int nrows,int fcols);
void sparseClearM(sparsematrix sprs);
void sparseMultiplyMV(const sparsematrix sprs,const double *vect,double *ans);
void sparseAddIdentityM(sparsematrix sprs,double factor);
void sparseMultiplyScalarM(sparsematrix sprs,double scalar);
void sparseBiCGSTAB(const sparsematrix sprs,const double *b,double *x,double tolerance);

#ifdef __cplusplus
}
#endif

#endif
