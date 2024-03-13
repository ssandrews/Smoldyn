/* Steve Andrews, 4/07 */

#ifndef __minimize_h
#define __minimize_h

typedef struct minimizestruct {
	int maxparam;								// maximum number of parameters
	int nparam;									// actual number of parameters
	double **param;							// pointers to parameter variables
	double *priorparam;					// values of parameters prior to fitting
	double *scale;							// characteristic scale for each parameter
	double *lo;									// minimum value for each parameter
	double *hi;									// maximum value for each parameter
	int *fix;										// 1 if parameter is fixed, 0 if not
	void *systemptr;						// pointer to system, which is passed to minfn
	double (*minfn)(void *,double);	// function that is to be minimized
	double distance;						// the most recently achieved best distance
	double f1,f2,f3;						// floats for minimization functions
	double *v1,*v2,*v3;					// vectors for minimization functions
	double *m1;									// matrix for minimization function
	} *minimizeptr;


minimizeptr mnmz_alloc(int maxparam,void* systemptr,double (*minfn)(void*,double));
void mnmz_free(minimizeptr mnmz);
void mnmz_clear(minimizeptr mnmz);
int mnmz_setparamptr(minimizeptr mnmz,char *param,void *value);
int mnmz_setparam(minimizeptr mnmz,double *paramptr,double scale,double lo,double hi,int fix);
int mnmz_step1(minimizeptr mnmz,int rptstep);
int mnmz_step2(minimizeptr mnmz,int rptstep);
int mnmz_step3(minimizeptr mnmz,int rptstep);
int mnmz_annealstep(minimizeptr mnmz,int rptstep);
void mnmz_randstep(minimizeptr mnmz,double change);
int mnmz_simplex(minimizeptr mnmz,int rptstep);

# endif

