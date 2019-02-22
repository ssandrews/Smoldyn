/* Steve Andrews, started 5/2007 */
/* Makes Crowding objects for Smoldyn. */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Geometry.h"
#include "gnuPipe.h"
#include "math2.h"
#include "random2.h"
#include "rxnparam.h"
#include "rxn2Dparam.h"
#include "RnSort.h"
#include "string2.h"
#include "SurfaceParam.h"

#define VERSION 1.0

#define CHECKS(A,...)		if(!(A)) {ErrorType=2;sprintf(ErrorString,__VA_ARGS__); goto failure;} else (void)0

char ErrorString[256]="";
int ErrorType=0;
FILE* gnu=NULL;


typedef struct emustruct {
	int dim;
	int npts;
	int reversible;
	char algorithm;
	double bindrad;
	double unbindrad;
	double rxnprob;
	double epsilon;
	double difc;
	double dt;
	double rmsstep;
	char initialrdf;
	double initialrdfval;
	double dr;
	double *r;
	double *rdfa;
	double *rdfd;
	} *emuptr;


/******************************************************************************/
/******************************* Smoldyn emulator *****************************/
/******************************************************************************/


/******************************************************************************/
/******************************* Memory management ****************************/
/******************************************************************************/


/* emualloc */
emuptr emualloc(emuptr emu,int npts) {
	int i;

	if(!emu) {
		emu=(emuptr) malloc(sizeof(struct emustruct));
		if(!emu) return NULL;
		emu->dim=-1;
		emu->npts=0;
		emu->reversible=-1;
		emu->algorithm='a';
		emu->bindrad=-1;
		emu->unbindrad=-1;
		emu->rxnprob=-1;
		emu->epsilon=-1;
		emu->difc=-1;
		emu->dt=-1;
		emu->rmsstep=-1;
		emu->initialrdf='w';
		emu->initialrdfval=-1;
		emu->dr=-1;
		emu->r=NULL;
		emu->rdfa=NULL;
		emu->rdfd=NULL; }

	if(npts>0 && emu->npts!=npts) {
		if(emu->r) free(emu->r);
		if(emu->rdfa) free(emu->rdfa);
		if(emu->rdfd) free(emu->rdfd);
		emu->npts=0;

		emu->r=(double *) calloc(npts,sizeof(double));
		emu->rdfa=(double *) calloc(npts,sizeof(double));
		emu->rdfd=(double *) calloc(npts,sizeof(double));
		if(!emu->r || !emu->rdfa || !emu->rdfd) return NULL;
		for(i=0;i<npts;i++)
			emu->r[i]=emu->rdfa[i]=emu->rdfd[i]=0;
		emu->npts=npts; }

	return emu; }


/* emufree */
void emufree(emuptr emu) {
	if(!emu) return;
	free(emu->r);
	free(emu->rdfa);
	free(emu->rdfd);
	free(emu);
	return; }


/******************************************************************************/
/***************************** Data structure output **************************/
/******************************************************************************/


/* emuoutput */
void emuoutput(emuptr emu) {
	if(!emu) {
		printf("No emulation structure defined.\n");
		return; }
	printf("Emulation structure:\n");
	printf(" dimensions: %i\n",emu->dim);
	printf(" reversible: %s\n",emu->reversible?"yes":"no");
	printf(" algorithm: %c\n",emu->algorithm);
	printf(" difc, dt, rms step: %g %g %g\n",emu->difc,emu->dt,emu->rmsstep);
	if(emu->reversible)
		printf(" bind/unbind radius: %g %g\n",emu->bindrad,emu->unbindrad);
	else
		printf(" binding radius: %g\n",emu->bindrad);
	printf(" reaction probability: %g\n",emu->rxnprob);
	printf(" epsilon value: %g\n",emu->epsilon);
	printf(" initial rdf shape: %c",emu->initialrdf);
	if(emu->initialrdf=='h')
		printf(", value: %g",emu->initialrdfval);
	printf("\n points in rdf, dr: %i %g\n",emu->npts,emu->dr);
	return; }


/* emucheckparams */
int emucheckparams(emuptr emu) {
	int warn,error;

	warn=error=0;
	if(!emu) return 0;
	if(emu->dim<1 || emu->dim>3) {error++;printf("ERROR: dim must be between 1 and 3\n");}
	if(emu->npts<2) {error++;printf("ERROR: npts too small\n");}
	if(emu->bindrad<0) {error++;printf("ERROR: bindrad must be >= 0\n");}
	if(emu->reversible && emu->unbindrad<0) {error++;printf("ERROR: unbindrad must be >=0\n");}
	if(emu->dim<1 || emu->dim>3) {error++;printf("ERROR: dim out of range\n");}
	if(emu->rxnprob<0 || emu->rxnprob>1) {error++;printf("ERROR: rxnprob must be between 0 and 1\n");}
	if(emu->epsilon<=0) {error++;printf("ERROR: epsilon must be >0\n");}
	if(emu->rmsstep<=0) {error++;printf("ERROR: rmsstep must be >0\n");}
	if(emu->dr<=0) {error++;printf("ERROR: dr must be >0\n");}
	if(emu->r==NULL) {error++;printf("ERROR: r vector not allocated\n");}
	if(emu->rdfa==NULL) {error++;printf("ERROR: r vector not allocated\n");}
	if(emu->rdfd==NULL) {error++;printf("ERROR: r vector not allocated\n");}
	if(emu->difc>0 && emu->dt>0 && emu->rmsstep!=sqrt(2*emu->difc*emu->dt)) {warn++;printf("WARNING: rms step doesn't agree with difc and dt.\n");}
	printf("%i warnings, %i errors\n",warn,error);
	return error; }


/******************************************************************************/
/******************************* Structure set up *****************************/
/******************************************************************************/


/* emusetparam */
int emusetparam(emuptr emu,char *param,double value) {
	int errcode;

	errcode=0;
	if(!strcmp(param,"dim"))
		emu->dim=(int) value;
	else if(!strcmp(param,"algorithm"))
		emu->algorithm=(char) value;
	else if(!strcmp(param,"bindrad"))
		emu->bindrad=value;
	else if(!strcmp(param,"unbindrad"))
		emu->unbindrad=value;
	else if(!strcmp(param,"reversible"))
		emu->reversible=(int) value;
	else if(!strcmp(param,"rxnprob"))
		emu->rxnprob=value;
	else if(!strcmp(param,"dt"))
		emu->dt=value;
	else if(!strcmp(param,"difc"))
		emu->difc=value;
	else if(!strcmp(param,"rmsstep"))
		emu->rmsstep=value;
	else if(!strcmp(param,"initialrdf"))
		emu->initialrdf=(char) value;
	else if(!strcmp(param,"initialrdfvalue"))
		emu->initialrdfval=value;
	else if(!strcmp(param,"dr"))
		emu->dr=value;
	else if(!strcmp(param,"npts")) {
		if(emualloc(emu,(int)value)==NULL)
			errcode=-1; }
	else if(!strcmp(param,"epsilon"))
		emu->epsilon=value;
	else
		errcode=-2;

	if(errcode==-1) fprintf(stderr,"BUG: out of memory in emusetparam\n");
	else if(errcode==-2) fprintf(stderr,"BUG: unknown parameter in emusetparam\n");

	return errcode; }


/* emuhardcodeparams */
int emuhardcodeparams(emuptr emu) {
	emusetparam(emu,"dim",3);
	emusetparam(emu,"algorithm",(double)'l');
	emusetparam(emu,"reversible",0);
//	emusetparam(emu,"bindrad",0.432834);
	emusetparam(emu,"bindrad",0.386114);
	emusetparam(emu,"unbindrad",-1);
	emusetparam(emu,"rxnprob",0.5);
	emusetparam(emu,"difc",0.5);
	emusetparam(emu,"dt",0.1);
	emusetparam(emu,"npts",200);
	emusetparam(emu,"epsilon",0.0001);
	emusetparam(emu,"initialrdf",(double)'w');
	return 0; }


/* emuinputparams*/
int emuinputparams(emuptr emu) {
	int i1;
	double d1;
	char string[STRCHAR];
	emuptr emutemp;

	printf("Enter system dimensionality (1, 2, or 3): ");
	scanf("%i",&i1);
	if(i1<1 || i1>3) {printf("Illegal input.\n");return 1;}
	emusetparam(emu,"dim",(double)i1);

	printf("Enter reaction algorithm: (A)ndrews-Bray, (L)ambda-rho: ");
	scanf("%s",string);
	emusetparam(emu,"algorithm",(double)tolower(string[0]));

	printf("Is reaction reversible, 'y' or 'n': ");
	scanf("%s",string);
	if(string[0]=='y' || string[0]=='Y')
		emusetparam(emu,"reversible",(double)1);
	else
		emusetparam(emu,"reversible",(double)0);

	printf("Enter binding radius (e.g. 1): ");
	scanf("%lg",&d1);
	if(d1<=0) {printf("Illegal input.\n");return 1;}
	emusetparam(emu,"bindrad",d1);

	if(emu->reversible) {
		printf("Enter unbinding radius (e.g. 2): ");
		scanf("%lg",&d1);
		if(d1<0) {printf("Illegal input.\n");return 1;}}
	else d1=-1;
	emusetparam(emu,"unbindrad",d1);

	if(emu->algorithm=='l') {
		printf("Enter reaction probability (e.g. 0.5): ");
		scanf("%lg",&d1);
		if(d1<0 || d1>1) {printf("Illegal input.\n");return 1; }}
	else d1=1;
	emusetparam(emu,"rxnprob",d1);

	printf("Enter rms step length (e.g. 0.1): ");
	scanf("%lg",&d1);
	emusetparam(emu,"rmsstep",d1);
	emusetparam(emu,"dt",-1);
	emusetparam(emu,"difc",-1);

	printf("Enter the number of radial points in the rdf (e.g. 200): ");
	scanf("%i",&i1);
	if(i1<10) {
		printf("Value is too low.  Function stopped.\n");return 1; }
	emusetparam(emu,"npts",(double)i1);

	printf("Enter the shape for the initial rdf:\n (w)ell-mixed, (H)eaviside function, (S)moluchowski: ");
	scanf("%s",string);
	string[0]=tolower(string[0]);
	emusetparam(emu,"initialrdf",(double)string[0]);
	if(string[0]=='h') {
		printf("Enter radius of step in initial rdf: ");
		scanf("%lg",&d1);
		emusetparam(emu,"initialrdfvalue",d1); }

	printf("Enter level of precision (e.g. 1e-4): ");
	scanf("%lf",&d1);
	if(d1<=0) {
		printf("Impossible precision.  Function stopped.\n");return 1; }
	emusetparam(emu,"epsilon",d1);

	return 0; }


/******************************************************************************/
/****************************** Structure set up ******************************/
/******************************************************************************/


/* emusetrmsstep */
int emusetrmsstep(emuptr emu) {
	double s;

	s=emu->rmsstep;
	if(emu->difc>0 && emu->dt>0)
		emusetparam(emu,"rmsstep",sqrt(2*emu->difc*emu->dt));
	else if(s>0 && emu->difc>0)
		emusetparam(emu,"dt",s*s/2/emu->difc);
	else if(s>0 && emu->dt>0)
		emusetparam(emu,"difc",s*s/2/emu->dt);
	else if(s>0) {
		emusetparam(emu,"dt",1);
		emusetparam(emu,"difc",s*s/2/emu->dt); }
	else
		return -1;

	return 0; }


/* emumakervect */
int emumakervect(emuptr emu) {
	int i,reversible,npts;
	double dr,*r,bindrad,unbindrad;

	bindrad=emu->bindrad;
	unbindrad=emu->unbindrad;
	reversible=emu->reversible;
	npts=emu->npts;
	r=emu->r;

	// set dr based on value for r_max
	if(!reversible) dr=10.0*bindrad/npts;
	else if(reversible && unbindrad>bindrad)
		dr=(unbindrad+4.0*bindrad)/npts;
	else dr=5.0*bindrad/npts;
	emu->dr=dr;

	r[0]=0;																			// r min is set to 0
	for(i=1;r[i-1]<bindrad && i-1<npts;i++)
		r[i]=r[i-1]+dr;
	r[i-1]=0.9999*bindrad;											// r point below r is set to 0.9999r
	r[i++]=1.0001*bindrad;											// r point above 1 is set to 1.0001r
	for(;i<npts;i++)
		r[i]=r[i-1]+dr;

	return 0; }


/* emumakerdfvect */
int emumakerdfvect(emuptr emu,char type) {
	int i,npts,reversible;
	double *r,*rdfa,*rdfd,bindrad,unbindrad,initialrdfval;
	char initialrdf;
	double f1;

	npts=emu->npts;
	r=emu->r;
	rdfa=emu->rdfa;
	rdfd=emu->rdfd;
	bindrad=emu->bindrad;
	unbindrad=emu->unbindrad;
	reversible=emu->reversible;
	initialrdf=(type=='d') ? emu->initialrdf : type;	// d = default
	initialrdfval=emu->initialrdfval;

	if(initialrdf=='h') {													// h = Heaviside function
		for(i=0;i<npts && r[i]<initialrdfval;i++)
			rdfa[i]=rdfd[i]=0;
		for(;i<npts;i++)
			rdfa[i]=rdfd[i]=1; }

	else if(initialrdf=='s') {										// s = Smoluchowski, here in 3D
		for(i=0;i<npts && r[i]<bindrad;i++) rdfa[i]=rdfd[i]=0;
		if(!reversible) for(;i<npts;i++) rdfa[i]=rdfd[i]=1.0-bindrad/r[i];
		else if(reversible && unbindrad>bindrad)
			for(;i<npts && r[i]<unbindrad;i++)
				rdfa[i]=rdfd[i]=1.0-(unbindrad-r[i])/r[i]/(unbindrad-bindrad);
		for(;i<npts;i++) rdfa[i]=rdfd[i]=1.0; }

	else {																				// w = well-mixed
		for(i=0;i<npts;i++)
			rdfa[i]=rdfd[i]=1; }

	return 0; }


/******************************************************************************/
/****************************** Top level functions ***************************/
/******************************************************************************/


/* emuactivationlimit */
int emuactivationlimit(emuptr emu) {
	double flux,theoryflux,bindrad,*r,*rdfa,*rdfd,rxnprob,rmsstep,dt;
	int npts,dim;

	dim=emu->dim;
	bindrad=emu->bindrad;
	r=emu->r;
	rdfa=emu->rdfa;
	rdfd=emu->rdfd;
	npts=emu->npts;
	rxnprob=emu->rxnprob;
	rmsstep=emu->rmsstep;
	dt=emu->dt;

	emumakerdfvect(emu,'w');
	if(dim==3) {
		rdfabsorb(r,rdfa,npts,bindrad,1);
		rdfdiffuse(r,rdfa,rdfd,npts,rmsstep);
		flux=rdfabsorb(r,rdfd,npts,bindrad,rxnprob);
		theoryflux=(double) actrxnrate(rmsstep,bindrad,rxnprob); }
	else if(dim==2) {
		rdf2Dabsorbprob(r,rdfa,npts,1);
		rdf2Ddiffuse(r,rdfa,rdfd,npts,rmsstep);
		flux=rdf2Dabsorbprob(r,rdfd,npts,rxnprob);
		theoryflux=rxn2Dactrate(rmsstep,bindrad); }

	printf("emulator flux during first step from well-mixed state: %g\n",flux);
	printf("emulator activation-limited rate constant, defined from initial flux: %g\n",flux/dt);
	printf("theoretical activation-limited rate constant, defined from initial flux: %g\n",theoryflux/dt);
	return 0; }


/* emustepbystep */
int emustepbystep(emuptr emu) {
	int npts,it,i,dim;
	double flux,fluxold,dt,numflux,numfluxprob,bindrad,unbindrad,rmsstep,rxnprob,difc;
	double numbindrad,numbindradprob,numprob,chi,rxnrate,numchi,numbindradchi,modelrate;
	char string[STRCHAR];
	double *r,*rdfa,*rdfd;

	dim=emu->dim;
	dt=emu->dt;
	r=emu->r;
	rdfa=emu->rdfa;
	rdfd=emu->rdfd;
	npts=emu->npts;
	bindrad=emu->bindrad;
	unbindrad=emu->unbindrad;
	rmsstep=emu->rmsstep;
	rxnprob=emu->rxnprob;
	difc=emu->difc;

	if(!gnu) gnu=gnuWindow();
	gnuSetScales(gnu,0,emu->dr*npts,0,1.1);
	gnuData(gnu,r,rdfa,npts,1);
//	printf("Graph shows starting RDF. Enter any letter to continue.\n");
//	scanf("%s",string);

	flux=fluxold=0;
	if(dim==3) rdfabsorb(r,rdfa,npts,bindrad,1);
	else if(dim==2) flux=rdf2Dabsorbprob(r,rdfa,npts,1);

	gnuData(gnu,r,rdfa,npts,1);
//	printf("Graph shows RDF after first step. Enter any letter to continue.\n");
//	scanf("%s",string);

	printf("... running ...\n");
	for(it=0;it<20 || fabs(flux/fluxold-1.0)>emu->epsilon;it++) {
		fluxold=flux;
		if(dim==3) rdfdiffuse(r,rdfa,rdfd,npts,rmsstep);
		else if(dim==2) rdf2Ddiffuse(r,rdfa,rdfd,npts,rmsstep);

		if(emu->reversible) {
			if(dim==3) rdfreverserxn(r,rdfd,npts,rmsstep,unbindrad,flux);
			else if(dim==2) rdf2Dreverserxn(r,rdfd,npts,rmsstep,unbindrad,flux); }
		for(i=0;i<npts;i++) rdfa[i]=rdfd[i];
		if(dim==3) flux=rdfabsorb(r,rdfa,npts,bindrad,rxnprob);
		else if(dim==2) flux=rdf2Dabsorbprob(r,rdfa,npts,rxnprob);

		if(it%10==0)
			gnuData(gnu,r,rdfa,npts,1); }

	printf("emulator final flux value in one time step: %g\n",flux);
	numflux=numrxnrate(rmsstep,bindrad,unbindrad);
	printf("computed flux from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux);
	numfluxprob=numrxnrateprob(rmsstep,bindrad,unbindrad,rxnprob);
	printf("computed flux from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob);

	rxnrate=flux/dt;
	printf("emulator steady-state reaction rate constant: %g\n",rxnrate);
	printf("computed reaction rate from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux/dt);
	printf("computed reaction rate from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob/dt);

	modelrate=modelrxnrate(bindrad,unbindrad,difc,1);
	chi=(modelrate>0) ? rxnrate/modelrate : 0;
	printf("emulator chi value: %g\n",chi);

	printf("emulator binding radius from definition: %g\n",bindrad);
	numbindrad=bindingradius(rxnrate,dt,difc,unbindrad,0);
	printf("computed binding radius from bindingradius(%g,%g,%g,%g,0): %g\n",rxnrate,dt,difc,unbindrad,numbindrad);
	numprob=rxnprob;
	numbindradprob=bindingradiusprob(rxnrate,dt,difc,unbindrad,0,-1,&numprob);
	printf("computed binding radius from bindingradiusprob(%g,%g,%g,%g,0,-1,%g): %g\n",rxnrate,dt,difc,unbindrad,numprob,numbindradprob);

	numbindradchi=bindingradiusprob(rxnrate,dt,difc,unbindrad,0,chi,&numprob);
	printf("computed binding radius and prob from bindingradiusprob(%g,%g,%g,%g,0,%g,&prob): %g %g\n",rxnrate,dt,difc,unbindrad,chi,numbindradchi,numprob);


	return 0; }


/* emusteadystate */
int emusteadystate(emuptr emu) {
	int npts,it,i,dim;
	double flux,fluxold,dt,numflux,numfluxprob,bindrad,unbindrad,rmsstep,rxnprob,difc,rxnrate,chi;
	double numbindrad,numbindradprob,numprob,numbindradchi,modelrate;
	char string[STRCHAR];
	double *r,*rdfa,*rdfd;

	dim=emu->dim;
	dt=emu->dt;
	r=emu->r;
	rdfa=emu->rdfa;
	rdfd=emu->rdfd;
	npts=emu->npts;
	bindrad=emu->bindrad;
	unbindrad=emu->unbindrad;
	rmsstep=emu->rmsstep;
	rxnprob=emu->rxnprob;
	difc=emu->difc;

	printf("... running ...\n");
	flux=rdfsteadystate(r,rdfa,rdfd,npts,rmsstep,bindrad,unbindrad,emu->epsilon,rxnprob);

	printf("emulator final flux value in one time step: %g\n",flux);
	numflux=numrxnrate(rmsstep,bindrad,unbindrad);
	printf("computed flux from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux);
	numfluxprob=numrxnrateprob(rmsstep,bindrad,unbindrad,rxnprob);
	printf("computed flux from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob);

	rxnrate=flux/dt;
	printf("emulator steady-state reaction rate constant: %g\n",rxnrate);
	printf("computed reaction rate from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux/dt);
	printf("computed reaction rate from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob/dt);

	modelrate=modelrxnrate(bindrad,unbindrad,difc,1);
	chi=(modelrate>0) ? rxnrate/modelrate : 0;
	printf("emulator chi value: %g\n",chi);

	printf("emulator binding radius from definition: %g\n",bindrad);
	numbindrad=bindingradius(rxnrate,dt,difc,unbindrad,0);
	printf("computed binding radius from bindingradius(%g,%g,%g,%g,0): %g\n",rxnrate,dt,difc,unbindrad,numbindrad);
	numprob=rxnprob;
	numbindradprob=bindingradiusprob(rxnrate,dt,difc,unbindrad,0,-1,&numprob);
	printf("computed binding radius from bindingradiusprob(%g,%g,%g,%g,0,-1,%g): %g\n",rxnrate,dt,difc,unbindrad,numprob,numbindradprob);

	numbindradchi=bindingradiusprob(rxnrate,dt,difc,unbindrad,0,chi,&numprob);
	printf("computed binding radius and prob from bindingradiusprob(%g,%g,%g,%g,0,%g,&prob): %g %g\n",rxnrate,dt,difc,unbindrad,chi,numbindradchi,numprob);

	return 0; }


/* emurateconstant */
int emurateconstant(emuptr emu) {
	int npts,i,dim,nrecord,irec;
	double flux,fluxold,numflux,numfluxprob,tmax,dtrecord,t,fluxmax,dt,*r,*rdfa,*rdfd;
	double *tvect,*kvect,bindrad,unbindrad,rmsstep,rxnprob;
	char string[STRCHAR];

	dim=emu->dim;
	dt=emu->dt;
	r=emu->r;
	rdfa=emu->rdfa;
	rdfd=emu->rdfd;
	rmsstep=emu->rmsstep;
	npts=emu->npts;
	rxnprob=emu->rxnprob;
	bindrad=emu->bindrad;
	unbindrad=emu->unbindrad;

	printf("Enter maximum time and recording interval: ");
	scanf("%lf %lf",&tmax,&dtrecord);
	nrecord=(int)floor(tmax/dtrecord);
	tvect=(double*) calloc(nrecord,sizeof(double));
	kvect=(double*) calloc(nrecord,sizeof(double));
	if(!tvect || !kvect) return 1;
	for(irec=0;irec<nrecord;irec++) kvect[irec]=0;

	if(!gnu) gnu=gnuWindow();

	flux=fluxold=fluxmax=0;
	if(dim==3) rdfabsorb(r,rdfa,npts,bindrad,rxnprob);
	else if(dim==2) rdf2Dabsorbprob(r,rdfa,npts,rxnprob);

	irec=-1;
	for(t=0;t<tmax;t+=emu->dt) {
		fluxold=flux;
		if(dim==3) rdfdiffuse(r,rdfa,rdfd,npts,rmsstep);
		else if(dim==2) rdf2Ddiffuse(r,rdfa,rdfd,npts,rmsstep);

		if(emu->reversible) {
			if(dim==3) rdfreverserxn(r,rdfd,npts,rmsstep,unbindrad,flux);
			else if(dim==2) rdf2Dreverserxn(r,rdfd,npts,rmsstep,unbindrad,flux); }
		for(i=0;i<emu->npts;i++) rdfa[i]=rdfd[i];
		if(dim==3) flux=rdfabsorb(r,rdfa,npts,bindrad,rxnprob);
		else if(dim==2) flux=rdf2Dabsorbprob(r,rdfa,npts,rxnprob);

		if(flux>fluxmax) {
			fluxmax=flux;
			gnuSetScales(gnu,0,tmax,0,fluxmax); }
		if((int)floor(t/dtrecord)>irec) {
			irec++;
			tvect[irec]=t;
			kvect[irec]=flux;
			gnuData(gnu,tvect,kvect,irec+1,1);
			printf("%g,%g\n",t,flux);
		}}

	printf("emulator final flux value: %g\n",flux);
	numflux=numrxnrate(rmsstep,bindrad,unbindrad);
	printf("flux from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux);
	numfluxprob=numrxnrateprob(rmsstep,bindrad,unbindrad,rxnprob);
	printf("flux from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob);
	if(emu->difc>0 && dt>0) {
		printf("emulator steady-state reaction rate: %g\n",flux/dt);
		printf("reaction rate from numrxnrate(%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,numflux/dt);
		printf("reaction rate from numrxnrateprob(%g,%g,%g,%g): %g\n",rmsstep,bindrad,unbindrad,rxnprob,numfluxprob/dt); }

	return 0; }


/* emugraphnumrxnrates */
int emugraphnumrxnrates(emuptr emu) {
	const int npts=100;
	const int cols=11;
	int i;
	double x[npts],y[cols*npts];
	double step,b,prob,a,xhi,xstart;
	double xlo,xstep;

	xstep=1.04;
	a=1;
	printf("Enter unbinding radius (or -1 for irreversible): ");
	scanf("%lf",&b);
	printf("Enter low and high rms step lengths (e.g. 0 2): ");
	scanf("%lf %lf",&xlo,&xhi);
	xstart=(xlo<=0) ? exp(-4) : xlo;
	xstep=exp(log(xhi/xstart)/npts);

	for(i=0;i<npts;i++) {
		x[i]=(i==0)? xstart : x[i-1]*xstep;
		y[cols*i+0]=numrxnrateprob(x[i],a,b,0.1);
		y[cols*i+1]=numrxnrateprob(x[i],a,b,0.2);
		y[cols*i+2]=numrxnrateprob(x[i],a,b,0.3);
		y[cols*i+3]=numrxnrateprob(x[i],a,b,0.4);
		y[cols*i+4]=numrxnrateprob(x[i],a,b,0.5);
		y[cols*i+5]=numrxnrateprob(x[i],a,b,0.6);
		y[cols*i+6]=numrxnrateprob(x[i],a,b,0.7);
		y[cols*i+7]=numrxnrateprob(x[i],a,b,0.8);
		y[cols*i+8]=numrxnrateprob(x[i],a,b,0.9);
		y[cols*i+9]=numrxnrateprob(x[i],a,b,0.9999);
		y[cols*i+10]=numrxnrateprob(x[i],a,b,1); }

	if(!gnu) gnu=gnuWindow();
	gnuSetScales(gnu,xlo,x[npts-1],y[0],y[0]);
	gnuData(gnu,x,y,npts,cols);

	return 0; }


/* emugraphnumrxnrateb */
int emugraphnumrxnrateb(emuptr emu) {
	const int npts=100;
	const int cols=11;
	int i;
	double b[npts],y[cols*npts];
	double a,s,blo,bhi,bstep;

	a=1;
	printf("Enter reduced rms step length: ");
	scanf("%lf",&s);
	printf("Enter low and high b values (e.g. 0 2): ");
	scanf("%lf %lf",&blo,&bhi);
	bstep=(bhi-blo)/npts;

	for(i=0;i<npts;i++) {
		b[i]=blo+i*bstep;
		y[cols*i+0]=numrxnrateprob(s,a,b[i],0.1);
		y[cols*i+1]=numrxnrateprob(s,a,b[i],0.2);
		y[cols*i+2]=numrxnrateprob(s,a,b[i],0.3);
		y[cols*i+3]=numrxnrateprob(s,a,b[i],0.4);
		y[cols*i+4]=numrxnrateprob(s,a,b[i],0.5);
		y[cols*i+5]=numrxnrateprob(s,a,b[i],0.6);
		y[cols*i+6]=numrxnrateprob(s,a,b[i],0.7);
		y[cols*i+7]=numrxnrateprob(s,a,b[i],0.8);
		y[cols*i+8]=numrxnrateprob(s,a,b[i],0.9);
		y[cols*i+9]=numrxnrateprob(s,a,b[i],0.9999);
		y[cols*i+10]=numrxnrateprob(s,a,b[i],1); }

	if(!gnu) gnu=gnuWindow();
	gnuSetScales(gnu,blo,bhi,y[0],y[0]);
	gnuData(gnu,b,y,npts,cols);

	return 0; }


/******************************************************************************/
/********************************* Main function ******************************/
/******************************************************************************/


int main(void) {
	emuptr emu;
	int selection;

	printf("Runing SmolEmulate version %g\n",VERSION);

	emu=emualloc(NULL,0);

	selection=0;
	while(selection!=9) {
		printf("\n----------------------------\n");
		printf("What do you want to do:\n");
		printf(" (1) Compute simulated reaction rate from parameters with graphics\n");
		printf(" (2) Compute simulated reaction rate from pre-selected parameters\n");
		printf(" (3) Run Andrews-Bray calibration\n");
		printf(" (4) Run Lambda-rho calibration\n");
		printf(" (5) Compute time-dependent reaction rate\n");
		printf(" (6) Graph numerical reaction rate as function of s\n");
		printf(" (7) Graph numerical reaction rate as function of b\n");
		printf(" (8) Compute steady state flux without graphics\n");
		printf(" (9) Quit\n");
		printf("> ");
		scanf("%i",&selection);

		if(selection==1) {
			CHECKS(!emuinputparams(emu),"error with parameter input\n");
			CHECKS(!emusetrmsstep(emu),"error with setting rms step\n");
			CHECKS(!emumakervect(emu),"error with making r vector\n");
			emuoutput(emu);
			CHECKS(!emucheckparams(emu),"error during parameter checking\n");
			CHECKS(!emuactivationlimit(emu),"error computing activation limit\n");
			CHECKS(!emumakerdfvect(emu,'d'),"error making rdf vectors\n");
			CHECKS(!emustepbystep(emu),"error running step-by-step\n"); }
		else if(selection==2) {
			CHECKS(!emuhardcodeparams(emu),"error with hard-codeed parameter input\n");
			CHECKS(!emusetrmsstep(emu),"error with setting rms step\n");
			CHECKS(!emumakervect(emu),"error with making r vector\n");
			CHECKS(!emumakerdfvect(emu,'d'),"error making rdf vecgtors\n");
			emuoutput(emu);
			CHECKS(!emucheckparams(emu),"error during parameter checking\n");
			CHECKS(!emuactivationlimit(emu),"error computing activation limit\n");
			CHECKS(!emustepbystep(emu),"error running step-by-step\n"); }
		else if(selection==3) {
			rdfmaketable(); }
		else if(selection==4) {
			rdfmaketableprob(); }
		else if(selection==5) {
			CHECKS(!emuinputparams(emu),"error with parameter input\n");
			CHECKS(!emusetrmsstep(emu),"error with setting rms step\n");
			CHECKS(!emumakervect(emu),"error with making r vector\n");
			emuoutput(emu);
			CHECKS(!emucheckparams(emu),"error during parameter checking\n");
			CHECKS(!emuactivationlimit(emu),"error computing activation limit\n");
			CHECKS(!emumakerdfvect(emu,'d'),"error making rdf vectors\n");
			CHECKS(!emurateconstant(emu),"error running emurateconstant"); }
		else if(selection==6) {
			emugraphnumrxnrates(emu); }
		else if(selection==7) {
			emugraphnumrxnrateb(emu); }
		else if(selection==8) {
			CHECKS(!emuinputparams(emu),"error with parameter input\n");
			CHECKS(!emusetrmsstep(emu),"error with setting rms step\n");
			CHECKS(!emumakervect(emu),"error with making r vector\n");
			emuoutput(emu);
			CHECKS(!emucheckparams(emu),"error during parameter checking\n");
			CHECKS(!emumakerdfvect(emu,'d'),"error making rdf vectors\n");
			CHECKS(!emusteadystate(emu),"error running steady state\n"); }
		}

	if(gnu) gnuClose(gnu);
	emufree(emu);
	return 0;

 failure:
	if(gnu) gnuClose(gnu);
 	printf("Terminated due to: %s\n",ErrorString);
 	return 0; }

