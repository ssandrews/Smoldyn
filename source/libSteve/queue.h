/* Steven Andrews, 4/16/95.
See documentation called queue_doc.doc.
Copyright 2003-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __queue_h
#define __queue_h

#include <limits.h>
#if !defined(LLONG_MAX) || defined(WIN32)
	typedef long int Q_LONGLONG;
	#define Q_LLONG_MAX LONG_MAX
	#define Q_LLI "%li"
#else
	typedef long long int Q_LONGLONG;
	#define Q_LLONG_MAX LLONG_MAX
	#define Q_LLI "%lli"
#endif


enum Q_types {Qusort,Qvoid,Qint,Qdouble,Qlong};

typedef struct qstruct{
	enum Q_types type;
	void **kv;
	int *ki;
	double *kd;
	Q_LONGLONG *kl;
	int (*keycmp)(void *,void *);
	void **x;
	int n;
	int f;
	int b; } *q_queue;

#define q_frontkeyV(q) ((q)->b==(q)->f?NULL:(q)->kv[(q)->f])
#define q_frontkeyI(q) ((q)->b==(q)->f?0:(q)->ki[(q)->f])
#define q_frontkeyD(q) ((q)->b==(q)->f?0:(q)->kd[(q)->f])
#define q_frontkeyL(q) ((q)->b==(q)->f?0:(q)->kl[(q)->f])

q_queue q_alloc(int n,enum Q_types type,int (*keycmp)(void *,void *));
int q_expand(q_queue q,int addspace);
void q_free(q_queue q,int freek,int freex);
void q_null(q_queue q);
int q_enqueue(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,q_queue q);
int q_push(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,q_queue q);
int q_insert(void *kv,int ki,double kd,Q_LONGLONG kl,void *x,q_queue q);
void q_front(q_queue q,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr);
int q_pop(q_queue q,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr);
int q_length(q_queue q);
int q_maxlength(q_queue q);
int q_next(int i,void **kvptr,int *kiptr,double *kdptr,Q_LONGLONG *klptr,void **xptr,q_queue q);

#endif

