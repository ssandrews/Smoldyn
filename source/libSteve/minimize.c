/* Steve Andrews, 4/07 */

#include <float.h>
#include <string.h>
#include "math2.h"
#include "minimize.h"
#include "random2.h"

#define CHECK(A) if(!(A)) goto failure;

double smplxmove(minimizeptr mnmz,int ihi,double fac);



minimizeptr mnmz_alloc(int maxparam,void* systemptr,double (*minfn)(void*,double)) {
	minimizeptr mnmz;
	int i;

	mnmz=(minimizeptr) malloc(sizeof(struct minimizestruct));
	if(!mnmz) return NULL;
	mnmz->maxparam=maxparam;
	mnmz->nparam=0;
	mnmz->param=NULL;
	mnmz->priorparam=NULL;
	mnmz->scale=NULL;
	mnmz->lo=NULL;
	mnmz->hi=NULL;
	mnmz->fix=NULL;
	mnmz->systemptr=systemptr;
	mnmz->minfn=minfn;
	mnmz->distance=DBL_MAX;
	mnmz->f1=mnmz->f2=mnmz->f3=0;
	mnmz->v1=mnmz->v2=mnmz->v3=NULL;
	mnmz->m1=NULL;

	CHECK(mnmz->param=(double**)calloc(maxparam,sizeof(double*)));
	for(i=0;i<maxparam;i++) mnmz->param[i]=NULL;
	CHECK(mnmz->priorparam=(double*)calloc(maxparam,sizeof(double)));
	for(i=0;i<maxparam;i++) mnmz->priorparam[i]=0;
	CHECK(mnmz->scale=(double*)calloc(maxparam,sizeof(double)));
	for(i=0;i<maxparam;i++) mnmz->scale[i]=1;
	CHECK(mnmz->lo=(double*)calloc(maxparam,sizeof(double)));
	for(i=0;i<maxparam;i++) mnmz->lo[i]=DBL_MIN;
	CHECK(mnmz->hi=(double*)calloc(maxparam,sizeof(double)));
	for(i=0;i<maxparam;i++) mnmz->hi[i]=DBL_MAX;
	CHECK(mnmz->fix=(int*)calloc(maxparam,sizeof(int)));
	for(i=0;i<maxparam;i++) mnmz->fix[i]=1;
	CHECK(mnmz->v1=(double*)calloc(maxparam+1,sizeof(double)));
	CHECK(mnmz->v2=(double*)calloc(maxparam+1,sizeof(double)));
	CHECK(mnmz->v3=(double*)calloc(maxparam+1,sizeof(double)));
	for(i=0;i<maxparam+1;i++) mnmz->v1[i]=mnmz->v2[i]=mnmz->v3[i]=0;
	CHECK(mnmz->m1=(double*)calloc((maxparam+1)*(maxparam+1),sizeof(double)));
	for(i=0;i<(maxparam+1)*(maxparam+1);i++) mnmz->m1[i]=0;
	return mnmz;
 failure:
 	mnmz_free(mnmz);
 	return NULL; }



void mnmz_free(minimizeptr mnmz) {
	if(!mnmz) return;
	free(mnmz->m1);
	free(mnmz->v3);
	free(mnmz->v2);
	free(mnmz->v1);
	free(mnmz->fix);
	free(mnmz->hi);
	free(mnmz->lo);
	free(mnmz->scale);
	free(mnmz->priorparam);
	free(mnmz->param);
	free(mnmz);
	return; }



void mnmz_clear(minimizeptr mnmz) {
	int i,maxparam;

	maxparam=mnmz->maxparam;
	mnmz->nparam=0;
	for(i=0;i<maxparam;i++) mnmz->param[i]=NULL;
	for(i=0;i<maxparam;i++) mnmz->priorparam[i]=0;
	for(i=0;i<maxparam;i++) mnmz->scale[i]=1;
	for(i=0;i<maxparam;i++) mnmz->lo[i]=DBL_MIN;
	for(i=0;i<maxparam;i++) mnmz->hi[i]=DBL_MAX;
	for(i=0;i<maxparam;i++) mnmz->fix[i]=1;
	mnmz->distance=DBL_MAX;
	mnmz->f1=mnmz->f2=mnmz->f3=0;
	for(i=0;i<maxparam+1;i++) mnmz->v1[i]=mnmz->v2[i]=mnmz->v3[i]=0;
	for(i=0;i<(maxparam+1)*(maxparam+1);i++) mnmz->m1[i]=0;
	return; }



int mnmz_setparamptr(minimizeptr mnmz,char *param,void *value) {
	int er;

	er=0;
	if(!strcmp(param,"systemptr"))
		mnmz->systemptr=value;
	else if(!strcmp(param,"minfn"))
		mnmz->minfn=value;
	else er=1;
	return er; }



int mnmz_setparam(minimizeptr mnmz,double *paramptr,double scale,double lo,double hi,int fix) {
	int i;

	for(i=0;i<mnmz->nparam;i++)
		if(mnmz->param[i]==paramptr) break;
	if(i==mnmz->nparam) {
		if(mnmz->nparam==mnmz->maxparam) return 0;
		mnmz->nparam++;
		mnmz->param[i]=paramptr; }
	mnmz->scale[i]=scale;
	mnmz->lo[i]=lo;
	mnmz->hi[i]=hi;
	mnmz->fix[i]=fix;
	return 1; }



int mnmz_step1(minimizeptr mnmz,int rptstep) {
	int nparam,*fix,i,better;
	double **param,*priorparam,range,*scale,*lo,*hi,dist;

	nparam=mnmz->nparam;
	if(!nparam) return 0;
	param=mnmz->param;
	priorparam=mnmz->priorparam;
	scale=mnmz->scale;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	range=mnmz->f1;
	if(!rptstep) {
		range=1;
		mnmz->distance=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX); }

	for(i=0;i<nparam;i++)
		if(!fix[i]) {
			priorparam[i]=*param[i];
			*param[i]=reflect(unirandsumCCD(3,priorparam[i],range*scale[i]),lo[i],hi[i]); }
	dist=(*mnmz->minfn)(mnmz->systemptr,mnmz->distance);
	if(dist<mnmz->distance) {
		better=1;
		range*=1.1;
		mnmz->distance=dist; }
	else {
		better=0;
		range*=0.999;
		for(i=0;i<nparam;i++)
			if(!fix[i]) *param[i]=priorparam[i]; }
	mnmz->f1=range;
	return better; }



int mnmz_step2(minimizeptr mnmz,int rptstep) {
	int nparam,*fix,i,better;
	double **param,*priorparam,*sigma,*lo,*hi,dist;

	nparam=mnmz->nparam;
	if(!nparam) return 0;
	param=mnmz->param;
	priorparam=mnmz->priorparam;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	sigma=mnmz->v1;
	if(!rptstep) {
		mnmz->distance=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX);
		for(i=0;i<nparam;i++) sigma[i]=mnmz->scale[i]; }

	while(fix[i=intrand(nparam)]);
	priorparam[i]=*param[i];
	*param[i]=reflect(unirandsumCCD(3,priorparam[i],sigma[i]),lo[i],hi[i]);
	dist=(*mnmz->minfn)(mnmz->systemptr,mnmz->distance);
	if(dist<mnmz->distance) {
		better=1;
		sigma[i]*=1.2;
		mnmz->distance=dist; }
	else {
		better=0;
		sigma[i]*=0.99;
		*param[i]=priorparam[i]; }
	return better; }



int mnmz_step3(minimizeptr mnmz,int rptstep) {
	int nparam,*fix,i,better,j;
	double **param,*priorparam,range,*scale,*lo,*hi,dist,*step,factor,oldfactor,range2;

	nparam=mnmz->nparam;
	if(!nparam) return 0;
	param=mnmz->param;
	priorparam=mnmz->priorparam;
	scale=mnmz->scale;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	range=mnmz->f1;
	if(!rptstep) {
		range=1;
		mnmz->distance=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX); }

	for(i=0;i<nparam;i++)
		if(!fix[i]) {
			priorparam[i]=*param[i];
			*param[i]=reflect(unirandsumCCD(3,priorparam[i],range*scale[i]),lo[i],hi[i]); }
	dist=(*mnmz->minfn)(mnmz->systemptr,mnmz->distance);

	if(dist<mnmz->distance) {
		mnmz->distance=dist;
		step=mnmz->v1;
		for(i=0;i<nparam;i++) step[i]=fix[i]?0:*param[i]-priorparam[i];
		factor=1;		// now, optimize factor
		oldfactor=1;
		range2=0.5;
		for(j=0;j<100;j++) {
			factor+=unirandsumCCD(3,0,range2);
			for(i=0;i<nparam;i++) *param[i]=priorparam[i]+factor*step[i];
			dist=(*mnmz->minfn)(mnmz->systemptr,mnmz->distance);
			if(dist<mnmz->distance) {
				mnmz->distance=dist;
				oldfactor=factor;
				range2*=1.2; }
			else {
				factor=oldfactor;
				range2*=0.9; }}
		factor=oldfactor;
		for(i=0;i<nparam;i++) *param[i]=priorparam[i]+factor*step[i];
		if(factor>1) range*=factor;
		better=1; }
	else {
		better=0;
		range*=0.999;
		for(i=0;i<nparam;i++)
			if(!fix[i]) *param[i]=priorparam[i]; }
	mnmz->f1=range;
	return better; }



int mnmz_annealstep(minimizeptr mnmz,int rptstep) {
	int nparam,*fix,i,better;
	double **param,*priorparam,*scale,*lo,*hi,dist,kt,range;

	nparam=mnmz->nparam;
	if(!nparam) return 0;
	param=mnmz->param;
	priorparam=mnmz->priorparam;
	scale=mnmz->scale;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	range=mnmz->f1;
	kt=mnmz->f2;
	if(!rptstep) {
		mnmz->distance=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX);
		kt=fabs(mnmz->distance);
		if(kt==0) kt=1;
		range=1; }

	for(i=0;i<nparam;i++)
		if(!fix[i]) {
			priorparam[i]=*param[i];
			*param[i]=reflect(unirandsumCCD(3,priorparam[i],range*scale[i]),lo[i],hi[i]); }
	dist=(*mnmz->minfn)(mnmz->systemptr,mnmz->distance);
	if(dist<mnmz->distance||coinrandD(exp(-(dist-mnmz->distance)/kt))) {
		better=1;
		range*=1.1;
		mnmz->distance=dist; }
	else {
		better=0;
		range*=0.999;
		for(i=0;i<nparam;i++)
			if(!fix[i]) *param[i]=priorparam[i]; }
	mnmz->f1=range;
	mnmz->f2=kt*0.99;
	return better; }



void mnmz_randstep(minimizeptr mnmz,double change) {
	int nparam,*fix,i;
	double **param,*scale,*lo,*hi;

	nparam=mnmz->nparam;
	if(!nparam) return;
	fix=mnmz->fix;
	param=mnmz->param;
	scale=mnmz->scale;
	lo=mnmz->lo;
	hi=mnmz->hi;

	for(i=0;i<nparam;i++)
		if(!fix[i])
			*param[i]=reflect(unirandsumCCD(3,*param[i],change*scale[i]),lo[i],hi[i]);
	return; }



double smplxmove(minimizeptr mnmz,int ihi,double fac) {
	int nparam,*fix,j,ndim;
	double **param,*y,*psum,*p,fac1,fac2,ytry,*lo,*hi;

	nparam=mnmz->nparam;
	param=mnmz->param;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	y=mnmz->v1;
	psum=mnmz->v2;
	p=mnmz->m1;
	ndim=(int)mnmz->f1;

	fac1=(1.0-fac)/ndim;
	fac2=fac1-fac;
	for(j=0;j<nparam;j++)
		if(!fix[j]) *param[j]=constrain(psum[j]*fac1-p[nparam*ihi+j]*fac2,lo[j],hi[j]);
	ytry=(*mnmz->minfn)(mnmz->systemptr,y[ihi]);
	if(ytry<y[ihi]) {
		y[ihi]=ytry;
		for(j=0;j<nparam;j++)
			if(!fix[j]) {
				psum[j]+=*param[j]-p[nparam*ihi+j];
				p[nparam*ihi+j]=*param[j]; }}
	return ytry; }



int mnmz_simplex(minimizeptr mnmz,int rptstep) {
	int i,nparam,*fix,j,ihi,ilo,inhi,ndim,better;
	double *p,*scale,*lo,*hi,**param,*y,*psum,ytry,ysave;

	nparam=mnmz->nparam;
	if(!nparam) return 0;
	param=mnmz->param;
	scale=mnmz->scale;
	lo=mnmz->lo;
	hi=mnmz->hi;
	fix=mnmz->fix;
	y=mnmz->v1;
	psum=mnmz->v2;
	p=mnmz->m1;

	if(!rptstep) {
		ndim=0;										// calc. ndim
		for(j=0;j<nparam;j++)
			if(!fix[j]) ndim++;
		mnmz->f1=(double) ndim;

		for(i=0;i<nparam+1;i++)		// calc. p
			if(i==nparam||!fix[i])
				for(j=0;j<nparam;j++)
					if(!fix[j]) p[nparam*i+j]=*param[j];
		for(i=0;i<nparam;i++)
			if(!fix[i])
				p[nparam*i+i]=reflect(p[nparam*i+i]+scale[i],lo[i],hi[i]);

		for(i=0;i<nparam+1;i++)		// calc. y
			if(i==nparam||!fix[i]) {
				for(j=0;j<nparam;j++)
					if(!fix[j]) *param[j]=p[nparam*i+j];
				y[i]=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX); }}

	for(j=0;j<nparam;j++)		// calc. psum
		if(!fix[j]) {
			psum[j]=0;
			for(i=0;i<nparam+1;i++)
				if(i==nparam||!fix[i]) psum[j]+=p[nparam*i+j]; }

	ilo=ihi=inhi=-1;				// find ilo,ihi,inhi
	for(i=0;i<nparam+1;i++)
		if(i==nparam||!fix[i]) {
			if(ilo==-1||y[i]<y[ilo]) ilo=i;
			if(ihi==-1||y[i]>y[ihi]) {
				inhi=ihi;
				ihi=i; }
			else if(inhi==-1||y[i]>y[inhi])
				inhi=i; }

	ytry=smplxmove(mnmz,ihi,-1);		// optimize with amoeba
	if(ytry<=y[ilo]) ytry=smplxmove(mnmz,ihi,2);
	else if(ytry>=y[inhi]) {
		ysave=y[ihi];
		ytry=smplxmove(mnmz,ihi,0.5);
		if(ytry>=ysave) {
			for(i=0;i<nparam+1;i++)
				if((i==nparam||!fix[i])&&i!=ilo) {
					for(j=0;j<nparam;j++)
						if(!fix[j]) *param[j]=p[nparam*i+j]=constrain(0.5*(p[nparam*i+j]+p[nparam*ilo+j]),lo[j],hi[j]);
					y[i]=(*mnmz->minfn)(mnmz->systemptr,DBL_MAX); }}}

	better=(ytry<y[inhi]);					// stuff for output
	if(y[ihi]<y[ilo]) ilo=ihi;
	for(j=0;j<nparam;j++) *param[j]=p[nparam*ilo+j];
	mnmz->distance=y[ilo];

	return better; }



