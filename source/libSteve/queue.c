/* Steven Andrews, 4/16/95.
Operates a queue with a circular array.
See documentation called queue_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdlib.h>
#include "queue.h"

#define CHECK(A) if(!(A)) {goto failure;} else (void)0

queue q_alloc(int n,enum Q_types type,int (*keycmp)(void *,void *)) {
	queue q;
	int i;

	if(n<0) return NULL;
	q=(queue) malloc(sizeof(struct qstruct));
	if(!q) return NULL;

	q->type=type;
	q->kv=NULL;
	q->ki=NULL;
	q->kd=NULL;
	q->kl=NULL;
	q->keycmp=keycmp;
	q->x=NULL;
	q->n=++n;
	q->f=0;
	q->b=0;

	CHECK(type==Qusort || type==Qvoid || type==Qint || type==Qdouble || type==Qlong);
	if(type==Qvoid) {
		CHECK(q->kv=(void**) calloc(n,sizeof(void*)));
		for(i=0;i<n;i++) q->kv[i]=NULL; }
	else if(type==Qint) {
		CHECK(q->ki=(int*) calloc(n,sizeof(int)));
		for(i=0;i<n;i++) q->ki[i]=0; }
	else if(type==Qdouble) {
		CHECK(q->kd=(double*) calloc(n,sizeof(double)));
		for(i=0;i<n;i++) q->kd[i]=0; }
	else if(type==Qlong) {
		CHECK(q->kl=(Q_LONGLONG*) calloc(n,sizeof(Q_LONGLONG)));
		for(i=0;i<n;i++) q->kl[i]=0; }

	CHECK(q->x=(void**) calloc(n,sizeof(void*)));
	for(i=0;i<n;i++) q->x[i]=NULL;
	return q;

 failure:
	q_free(q,0,0);
	return NULL; }



int q_expand(queue q,int addspace) {
	int i,j,num,nnew;
	void **xnew,**kvnew;
	int *kinew;
	double *kdnew;
	Q_LONGLONG *klnew;

	nnew=q->n+addspace;
	if(nnew<1) return 2;

	xnew=(void**)calloc(nnew,sizeof(void*));
	if(!xnew) return 1;
	for(i=q->f,j=0;i!=q->b && j<nnew;i=(i+1)%q->n) xnew[j++]=q->x[i];
	num=j;
	for(;j<nnew;j++) xnew[j]=NULL;

	kvnew=NULL;
	kinew=NULL;
	kdnew=NULL;
	klnew=NULL;
	if(q->type==Qusort);
	else if(q->type==Qvoid) {
		kvnew=(void**)calloc(nnew,sizeof(void*));
		if(!kvnew) {free(xnew);return 1;}
		for(i=q->f,j=0;i!=q->b && j<nnew;i=(i+1)%q->n) kvnew[j++]=q->kv[i];
		for(;j<nnew;j++) kvnew[j]=NULL; }
	else if(q->type==Qint) {
		kinew=(int*)calloc(nnew,sizeof(int));
		if(!kinew) {free(xnew);return 1;}
		for(i=q->f,j=0;i!=q->b && j<nnew;i=(i+1)%q->n) kinew[j++]=q->ki[i];
		for(;j<nnew;j++) kinew[j]=0; }
	else if(q->type==Qdouble) {
		kdnew=(double*)calloc(nnew,sizeof(double));
		if(!kdnew) {free(xnew);return 1;}
		for(i=q->f,j=0;i!=q->b && j<nnew;i=(i+1)%q->n) kdnew[j++]=q->kd[i];
		for(;j<nnew;j++) kdnew[j]=0; }
	else if(q->type==Qlong) {
		klnew=(Q_LONGLONG*)calloc(nnew,sizeof(Q_LONGLONG));
		if(!klnew) {free(xnew);return 1;}
		for(i=q->f,j=0;i!=q->b && j<nnew;i=(i+1)%q->n) klnew[j++]=q->kl[i];
		for(;j<nnew;j++) klnew[j]=0; }

	free(q->x);
	free(q->kv);
	free(q->ki);
	free(q->kd);
	free(q->kl);
	q->x=xnew;
	q->kv=kvnew;
	q->ki=kinew;
	q->kd=kdnew;
	q->kl=klnew;
	q->n=nnew;
	q->f=0;
	q->b=num%nnew;
	return 0; }



void q_free(queue q,int freek,int freex) {
	int i;

	if(!q) return;
	if(freek || freex) {
		for(i=q->f;i!=q->b;i=(i+1)%q->n) {
			if(freex) free(q->x[i]);
			if(freek && q->type==Qvoid) free(q->kv[i]); }}
	free(q->x);
	free(q->kl);
	free(q->kd);
	free(q->ki);
	free(q->kv);
	free(q); }



void q_null(queue q) {
	q->f=q->b=0; }



int q_enqueue(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,queue q) {
	int sp;

	if(q->type==Qvoid) q->kv[q->b]=kv;
	else if(q->type==Qint) q->ki[q->b]=ki;
	else if(q->type==Qdouble) q->kd[q->b]=kd;
	else if(q->type==Qlong) q->kl[q->b]=kl;
	q->x[q->b]=x;
	q->b=(q->b+1)%q->n;
	sp=(q->n+q->f-q->b)%q->n-1;
	if(q->b==q->f) q->f=(q->f+1)%q->n;
	return sp; }



int q_push(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,queue q) {
	int sp;

	q->f=(q->n+q->f-1)%q->n;
	if(q->type==Qvoid) q->kv[q->f]=kv;
	else if(q->type==Qint) q->ki[q->f]=ki;
	else if(q->type==Qdouble) q->kd[q->f]=kd;
	else if(q->type==Qlong) q->kl[q->f]=kl;
	q->x[q->f]=x;
	sp=(q->n+q->f-q->b)%q->n-1;
	if(q->b==q->f) q->b=(q->n+q->b-1)%q->n;
	return sp; }



int q_insert(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,queue q) {
	int i,n,sp,im1;

	n=q->n;
	i=0;
	if(q->type==Qvoid) {
		for(im1=(n+(i=q->b)-1)%n;i!=q->f && (q->keycmp)(kv,q->kv[im1])<0;im1=(n+(i=im1)-1)%n) {
			q->kv[i]=q->kv[im1];
			q->x[i]=q->x[im1]; }
		q->kv[i]=kv; }
	else if(q->type==Qint) {
		for(im1=(n+(i=q->b)-1)%n;i!=q->f && ki<q->ki[im1];im1=(n+(i=im1)-1)%n) {
			q->ki[i]=q->ki[im1];
			q->x[i]=q->x[im1]; }
		q->ki[i]=ki; }
	if(q->type==Qdouble) {
		for(im1=(n+(i=q->b)-1)%n;i!=q->f && kd<q->kd[im1];im1=(n+(i=im1)-1)%n) {
			q->kd[i]=q->kd[im1];
			q->x[i]=q->x[im1]; }
		q->kd[i]=kd; }
	else if(q->type==Qlong) {
		for(im1=(n+(i=q->b)-1)%n;i!=q->f && kl<q->kl[im1];im1=(n+(i=im1)-1)%n) {
			q->kl[i]=q->kl[im1];
			q->x[i]=q->x[im1]; }
		q->kl[i]=kl; }

	q->x[i]=x;
	q->b=(q->b+1)%n;
	sp=(n+q->f-q->b)%n-1;
	if(q->b==q->f) q->b=(n+q->b-1)%n;
	return sp; }



void q_front(queue q,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr) {
	if(q->f==q->b) {
		if(kvptr) *kvptr=NULL;
		if(kiptr) *kiptr=0;
		if(kdptr) *kdptr=0;
		if(klptr) *klptr=0;
		if(xptr) *xptr=NULL; }

	if(q->type==Qvoid && kvptr) *kvptr=q->kv[q->f];
	else if(q->type==Qint && kiptr) *kiptr=q->ki[q->f];
	else if(q->type==Qdouble && kdptr) *kdptr=q->kd[q->f];
	else if(q->type==Qlong && klptr) *klptr=q->kl[q->f];
	if(xptr) *xptr=q->x[q->f];
	return; }



int q_pop(queue q,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr) {
	if(q->f==q->b) {
		if(kvptr) *kvptr=NULL;
		if(kiptr) *kiptr=0;
		if(kdptr) *kdptr=0;
		if(klptr) *klptr=0;
		if(xptr) *xptr=NULL;
		return -1; }

	if(q->type==Qvoid && kvptr) *kvptr=q->kv[q->f];
	else if(q->type==Qint && kiptr) *kiptr=q->ki[q->f];
	else if(q->type==Qdouble && kdptr) *kdptr=q->kd[q->f];
	else if(q->type==Qlong && klptr) *klptr=q->kl[q->f];
	if(xptr) *xptr=q->x[q->f];
	q->f=(q->f+1)%q->n;
	return (q->n+q->b-q->f)%q->n; }



int q_length(queue q) {
	return (q->n+q->b-q->f)%q->n; }



int q_maxlength(queue q) {
	return q->n-1; }



int q_next(int i,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr,queue q) {
	int f,b;

	f=q->f;
	b=q->b;
	if(i<0) i=f;
	else if(i>=q->n || (b<f && i<f && i>=b) || i<f || i>=b) return -1;
	else i=(i+1)%q->n;
	if((b<f && i<f && i>=b) || i<f || i>=b) return -1;
	if(q->type==Qvoid && kvptr) *kvptr=q->kv[i];
	else if(q->type==Qint && kiptr) *kiptr=q->ki[i];
	else if(q->type==Qdouble && kdptr) *kdptr=q->kd[i];
	else if(q->type==Qlong && klptr) *klptr=q->kl[i];
	if(xptr) *xptr=q->x[i];
	return i; }




