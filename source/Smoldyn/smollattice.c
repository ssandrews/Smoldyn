/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */
/* This file, smollattice.c, was written by Martin Robinson and Steve Andrews
 during 2013-2016. */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "random2.h"
#include "string2.h"
#include "Zn.h"
#include <string>

#include "smoldynconfigure.h"
#ifdef OPTION_NSV
  #include "nsvc.h"
#endif

#include "smoldyn.h"
#include "smoldynfuncs.h"


/******************************************************************************/
/********************************** lattices **********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// memory management
latticeptr latticealloc(int dim);
void latticefree(latticeptr lattice);
int latticeexpandreactions(latticeptr lattice,int maxrxns);
int latticeexpandsurfaces(latticeptr lattice,int maxsurfs);

int latticeexpandspecies(latticeptr lattice,int maxspecies);
int latticeexpandmols(latticeptr lattice,int species,int maxmols,int dim);
latticessptr latticessalloc(latticessptr latticess,int maxlattice,int dim);



latticeptr latticereadstring(simptr sim,ParseFilePtr pfp,latticeptr lattice,const char *word,char *line2);
int latticesupdateparams(simptr sim);
int latticesupdatelists(simptr sim);

// core simulation functions


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/


/* latticealloc */
latticeptr latticealloc(int dim) {
  int d;
	latticeptr lattice;

	lattice=(latticeptr) malloc(sizeof(struct latticestruct));
	CHECKMEM(lattice);
	lattice->latticename=NULL;
	lattice->type = LATTICEnone;
	for(d=0;d<dim;d++) {
		lattice->min[d]=0;
		lattice->max[d]=1;
		lattice->dx[d]=1;
		lattice->btype[d]='u'; }
	lattice->port=NULL;
	lattice->convert=NULL;
	lattice->maxreactions=0;
	lattice->nreactions=0;
	lattice->reactionlist=NULL;
	lattice->reactionmove=NULL;
	lattice->maxsurfaces=0;
	lattice->nsurfaces=0;
	lattice->surfacelist=NULL;
	lattice->maxspecies=0;
	lattice->nspecies=0;
	lattice->species_index=NULL;
	lattice->maxmols=NULL;
	lattice->nmols=NULL;
	lattice->mol_positions=NULL;
	lattice->nsv=NULL;
	lattice->pde=NULL;
	return lattice;
	failure:
	simLog(NULL,10,"Unable to allocate memory in latticealloc");
	return NULL; }


/* latticefree */
void latticefree(latticeptr lattice) {
	int i,m;

	if(!lattice) return;
	NSV_CALL(nsv_delete(lattice->nsv));
	if(lattice->mol_positions) {
		for(i=0;i<lattice->maxspecies;i++)
			if(lattice->mol_positions[i]) {
				for(m=0;m<lattice->maxmols[i];m++)
					free(lattice->mol_positions[i][m]);
				free(lattice->mol_positions[i]); }
		free(lattice->mol_positions); }

	if(lattice->convert) {
		for(i=0;i<lattice->maxspecies;i++)
			free(lattice->convert[i]);
		free(lattice->convert); }

	free(lattice->nmols);
	free(lattice->maxmols);
	free(lattice->species_index);
	free(lattice->reactionmove);
	free(lattice->reactionlist);
	free(lattice);
	return; }


/* latticeexpandreactions */
int latticeexpandreactions(latticeptr lattice,int maxrxns) {
	int r;
	rxnptr *newreactionlist;
	int *newreactionmove;

	if(maxrxns<=lattice->maxreactions) return 0;

	CHECKMEM(newreactionlist=(rxnptr*) calloc(maxrxns,sizeof(rxnptr)));
	CHECKMEM(newreactionmove=(int*) calloc(maxrxns,sizeof(int)));
	for(r=0;r<lattice->maxreactions;r++) {
		newreactionlist[r]=lattice->reactionlist[r];
		newreactionmove[r]=lattice->reactionmove[r]; }
	for(;r<maxrxns;r++) {
		newreactionlist[r]=NULL;
		newreactionmove[r]=0; }
	free(lattice->reactionlist);
	free(lattice->reactionmove);
	lattice->reactionlist=newreactionlist;
	lattice->reactionmove=newreactionmove;
	lattice->maxreactions=maxrxns;
	return 0;
	failure:
	return 1; }


/* latticeexpandsurfaces */
int latticeexpandsurfaces(latticeptr lattice,int maxsurfs) {
	int r;
	surfaceptr *newsurfacelist;

	if(maxsurfs<=lattice->maxsurfaces) return 0;

	CHECKMEM(newsurfacelist=(surfaceptr*) calloc(maxsurfs,sizeof(surfaceptr)));
	for(r=0;r<lattice->maxsurfaces;r++) {
		newsurfacelist[r]=lattice->surfacelist[r]; }
	for(;r<maxsurfs;r++) {
		newsurfacelist[r]=NULL; }
	free(lattice->surfacelist);
	lattice->surfacelist=newsurfacelist;
	lattice->maxsurfaces=maxsurfs;
	return 0;
	failure:
	return 1; }


/* latticeexpandspecies */
int latticeexpandspecies(latticeptr lattice,int maxspecies) {
	int i;
	int *newspecies_index,*newmaxmols,*newnmols,**newconvert;
	double ***newmolpositions;

	if(maxspecies<=lattice->maxspecies) return 0;
	CHECKMEM(newspecies_index=(int*) calloc(maxspecies,sizeof(int)));
	CHECKMEM(newmaxmols=(int*) calloc(maxspecies,sizeof(int)));
	CHECKMEM(newnmols=(int*) calloc(maxspecies,sizeof(int)));
	CHECKMEM(newmolpositions=(double ***) calloc(maxspecies,sizeof(double **)));
	CHECKMEM(newconvert=(int**) calloc(maxspecies,sizeof(int*)));

	for(i=0;i<lattice->maxspecies;i++) {
		newspecies_index[i]=lattice->species_index[i];
		newmaxmols[i]=lattice->maxmols[i];
		newnmols[i]=lattice->nmols[i];
		newmolpositions[i]=lattice->mol_positions[i];
		newconvert[i]=lattice->convert[i]; }
	for(;i<maxspecies;i++) {
		newspecies_index[i]=0;
		newmaxmols[i]=0;
		newnmols[i]=0;
		newmolpositions[i]=NULL;
		CHECKMEM(newconvert[i]=(int*) calloc(2,sizeof(int)));
		newconvert[i][0]=newconvert[i][1]=0; }

	free(lattice->species_index);
	free(lattice->maxmols);
	free(lattice->nmols);
	free(lattice->mol_positions);
	free(lattice->convert);

	lattice->species_index=newspecies_index;
	lattice->maxmols=newmaxmols;
	lattice->nmols=newnmols;
	lattice->mol_positions=newmolpositions;
	lattice->convert=newconvert;
	lattice->maxspecies=maxspecies;
	return 0;

 failure:
	return 1; }


/* latticeexpandmols */
int latticeexpandmols(latticeptr lattice,int species,int maxmols,int dim) {
	int m,d;
	double **newmolpositions;

	if(maxmols<=lattice->maxmols[species]) return 0;
	CHECKMEM(newmolpositions=(double **) calloc(maxmols,sizeof(double*)));
	for(m=0;m<lattice->maxmols[species];m++)
		newmolpositions[m]=lattice->mol_positions[species][m];
	for(;m<maxmols;m++) {
		CHECKMEM(newmolpositions[m]=(double*) calloc(dim,sizeof(double)));
		for(d=0;d<dim;d++)
			newmolpositions[m][d]=0; }

	free(lattice->mol_positions[species]);
	lattice->mol_positions[species]=newmolpositions;
	lattice->maxmols[species]=maxmols;
	return 0;
failure:
	return 1; }


/* latticessalloc */
latticessptr latticessalloc(latticessptr latticess,int maxlattice,int dim) {
	int lat;
	char **newnames;
	latticeptr *newlatticelist;

	if(maxlattice<1) return NULL;

	newnames=NULL;
	newlatticelist=NULL;

	if(!latticess) {// new allocation
		CHECKMEM(latticess=(latticessptr) malloc(sizeof(struct latticesuperstruct)));
		latticess->condition=SCinit;
		latticess->sim=NULL;
		latticess->maxlattice=0;
		latticess->nlattice=0;
		latticess->latticenames=NULL;
		latticess->latticelist=NULL; }

	if(maxlattice>latticess->maxlattice) {											// allocate new lattice names and lattices

		CHECKMEM(newnames=(char**) calloc(maxlattice,sizeof(char*)));
		for(lat=0;lat<maxlattice;lat++) newnames[lat]=NULL;
		for(lat=0;lat<latticess->maxlattice;lat++) newnames[lat]=latticess->latticenames[lat];
		for(;lat<maxlattice;lat++)
			CHECKMEM(newnames[lat]=EmptyString());

		CHECKMEM(newlatticelist=(latticeptr*) calloc(maxlattice,sizeof(latticeptr)));	// lattice list
		for(lat=0;lat<maxlattice;lat++) newlatticelist[lat]=NULL;
		for(lat=0;lat<latticess->maxlattice;lat++) newlatticelist[lat]=latticess->latticelist[lat];
		for(;lat<maxlattice;lat++) {
			CHECKMEM(newlatticelist[lat]=latticealloc(dim));
			newlatticelist[lat]->latticess=latticess;
			newlatticelist[lat]->latticename=newnames[lat]; }
  
    free(latticess->latticenames);
    latticess->latticenames=newnames;
    free(latticess->latticelist);
    latticess->latticelist=newlatticelist;
    latticess->maxlattice=maxlattice; }

	return latticess;

 failure:
 	latticessfree(latticess);
	simLog(NULL,10,"Unable to allocate memory in latticessalloc");
 	return NULL; }


/* latticessfree */
void latticessfree(latticessptr latticess) {
	int lat;

	if(!latticess) return;
	if(latticess->latticelist)
		for(lat=0;lat<latticess->maxlattice;lat++)
      latticefree(latticess->latticelist[lat]);
	free(latticess->latticelist);
  
	if(latticess->latticenames)
		for(lat=0;lat<latticess->maxlattice;lat++)
      free(latticess->latticenames[lat]);
	free(latticess->latticenames);

	free(latticess);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/


/* latticeoutput */
void latticeoutput(simptr sim) {
	latticessptr latticess;
	latticeptr lattice;
	int lat,r,d,i;
	char *buffer;

	latticess=sim->latticess;
	if(!latticess) return;
	buffer=NULL;

	simLog(sim,2,"LATTICE PARAMETERS\n");
	simLog(sim,2," Lattices allocated: %i, lattices defined: %i\n",latticess->maxlattice,latticess->nlattice);
	for(lat=0;lat<latticess->nlattice;lat++) {
		lattice=latticess->latticelist[lat];
		simLog(sim,2," Lattice: %s\n",lattice->latticename);
		if(lattice->type==LATTICEnsv) simLog(sim,2,"  Type: NSV\n");
		if(lattice->type==LATTICEpde) simLog(sim,2,"  Type: PDE\n");
		if(lattice->type==LATTICEnone) simLog(sim,2,"  Type: NONE\n");
		for(d=0;d<sim->dim;d++)
			simLog(sim,2,"  Boundaries on axis %i: from %lg to %lg, step %lg, type %s\n",d,lattice->min[d],lattice->max[d],lattice->dx[d],lattice->btype[d]=='r'?"reflect":(lattice->btype[d]=='p'?"periodic":"undefined"));
		simLog(sim,2,"  Interface port: %s\n",lattice->port?lattice->port->portname:"none");
    
		simLog(sim,2,"  Reactions (%i allocated, %i defined):\n",lattice->maxreactions,lattice->nreactions);
		for(r=0;r<lattice->nreactions;r++) {
			simLog(sim,2,"   %s%s\n",lattice->reactionlist[r]->rname,lattice->reactionmove[r]?" (moved)":""); }

		simLog(sim,2,"  Surfaces (%i allocated, %i defined):\n",lattice->maxsurfaces,lattice->nsurfaces);
		for(r=0;r<lattice->nsurfaces;r++) {
			simLog(sim,2,"   %s\n",lattice->surfacelist[r]->sname); }

		simLog(sim,2,"  Species (%i allocated, %i defined):\n",lattice->maxspecies,lattice->nspecies);
		for(i=0;i<lattice->nspecies;++i) {
			simLog(sim,2,"   %s has %i molecules",sim->mols->spname[lattice->species_index[i]],lattice->nmols[i]);
			simLog(sim,2,", %s at port front",lattice->convert[i][PFfront]?"becomes particle":"stays on lattice");
			simLog(sim,2,", %s at port back\n",lattice->convert[i][PFback]?"becomes particle":"stays on lattice"); }

		if(lattice->nsv) {
			NSV_CALL(nsv_print(lattice->nsv,&buffer));
			simLog(sim,2,"  External nsv class description: %s",buffer?buffer:"Error");
			free(buffer);
			buffer=NULL; }
		if(lattice->pde) {
      simLog(sim,2,"  ERROR: lattice is setup for PDE but this hasn't been implemented yet\n"); }}

	simLog(sim,2,"\n");
	return; }


/* writelattices */
void writelattices(simptr sim,FILE *fptr) {
	latticessptr latticess;
	latticeptr lattice;
	int lat,i,d,r,m,n;
	const int* copy_numbers;
	const double* positions;

	latticess=sim->latticess;
	if(!latticess) return;
	n=0;
	positions=NULL;
	copy_numbers=NULL;
	fprintf(fptr,"# lattice parameters\n");
	for(lat=0;lat<latticess->nlattice;lat++) {
		lattice=latticess->latticelist[lat];
		fprintf(fptr,"start_lattice %s\n",lattice->latticename);
		if(lattice->type==LATTICEnsv) fprintf(fptr,"type nsv\n");
		else if(lattice->type==LATTICEpde) fprintf(fptr,"type pde\n");
		else if(lattice->type==LATTICEnone) fprintf(fptr,"type none\n");

		for(d=0;d<sim->dim;d++) {
			fprintf(fptr,"boundaries %i %lg %lg %c\n",d,lattice->min[d],lattice->max[d],lattice->btype[d]); }

		fprintf(fptr,"lengthscale");
		for(d=0;d<sim->dim;d++) {
			fprintf(fptr," %lg",lattice->dx[d]); }
		fprintf(fptr,"\n");

    if(lattice->port)
      fprintf(fptr,"port %s\n",lattice->port->portname);

		for(r=0;r<lattice->nreactions;r++)
			fprintf(fptr,"reactions %s%s\n",lattice->reactionmove[r]?"move ":"",lattice->reactionlist[r]->rname);

		for(i=0;i<lattice->nsurfaces;i++)
			fprintf(fptr,"surfaces %s\n",lattice->surfacelist[i]->sname);

		for(i=0;i<lattice->nspecies;i++)
			fprintf(fptr,"species %s\n",sim->mols->spname[lattice->species_index[i]]);

		for(i=0;i<lattice->nspecies;i++)
			for(m=0;m<lattice->nmols[i];m++) {
				fprintf(fptr,"mol 1 %s",sim->mols->spname[lattice->species_index[i]]);
				for(d=0;d<sim->dim;d++)
					fprintf(fptr," %lg",lattice->mol_positions[i][m][d]);
				fprintf(fptr,"\n"); }

		NSV_CALL(n=nsv_get_species_copy_numbers(lattice->nsv,lattice->species_index[i], &copy_numbers, &positions));
		for (m = 0; m < n; ++m) {
			fprintf(fptr,"mol %d %s",copy_numbers[m],sim->mols->spname[lattice->species_index[i]]);
			for(d=0;d<sim->dim;d++)
				fprintf(fptr," %lg",positions[3*m+d]);
			fprintf(fptr,"\n"); }

//		free(copy_numbers);		//?? This can't be freed, but it seems that it should be
//		free(positions);
//		copy_numbers=NULL;
//		positions=NULL;

		fprintf(fptr,"end_lattice\n\n"); }
	return; }


/* checklatticeparams */
int checklatticeparams(simptr sim,int *warnptr) {
	int error,warn,lat,d,i,interact,s,p;
	double volume,comparts,pos;
	latticessptr latticess;
	latticeptr lattice;
	char string[STRCHAR];
	surfaceptr srf;

	error=warn=0;
	latticess=sim->latticess;
	if(!latticess) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(latticess->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: lattice structure %s\n",simsc2string(latticess->condition,string)); }

	for(lat=0;lat<latticess->nlattice;lat++) {
		lattice=latticess->latticelist[lat];

		volume=1.0;
		for(d=0;d<sim->dim;d++)
			volume=volume*(lattice->max[d]-lattice->min[d]);
		if(volume<=0) {
      error++;simLog(sim,10," ERROR: volume of lattice %s is less than or equal to zero\n",lattice->latticename); }

		for(d=0;d<sim->dim;d++) {		// check for integer number of compartments on each axis
			comparts=(lattice->max[d]-lattice->min[d])/lattice->dx[d];
			if(comparts!=floor(comparts)) {error++;simLog(sim,10," ERROR: lattice lengthscale is not an integer divisor of the boundaries on dimension %i\n",d); } }

		if(!lattice->port) {		// check port
      warn++;simLog(sim,7," WARNING: there is no interface port assigned to lattice %s\n",lattice->latticename); }
		else if (lattice->port->srf) {
			//if(lattice->port->srf->npanel[PSrect]==0) {warn++;simLog(sim,5,"WARNING: porting surface contains no rectangle panels, which lattice porting requires"); }
			if(lattice->port->srf->npanel[PStri]>0) {warn++;simLog(sim,5," WARNING: porting surface contains triangle panels which will be ignored by the lattice\n"); }
			if(lattice->port->srf->npanel[PSsph]>0) {warn++;simLog(sim,5," WARNING: porting surface contains sphere panels which will be ignored by the lattice\n"); }
			if(lattice->port->srf->npanel[PScyl]>0) {warn++;simLog(sim,5," WARNING: porting surface contains cylinder panels which will be ignored by the lattice\n"); }
			if(lattice->port->srf->npanel[PShemi]>0) {warn++;simLog(sim,5," WARNING: porting surface contains hemisphere panels which will be ignored by the lattice\n"); }
			if(lattice->port->srf->npanel[PSdisk]>0) {warn++;simLog(sim,5," WARNING: porting surface contains disk panels which will be ignored by the lattice\n"); }}

		if(lattice->port && lattice->port->srf) {		// check that port is at compartment boundary and at least 1 compartment in on all sides
			srf=lattice->port->srf;
			for(p=0;p<srf->npanel[PSrect];p++) {
				d=(int)srf->panels[PSrect][p]->front[1];
				pos=srf->panels[PSrect][p]->point[0][d];
				comparts=(pos-lattice->min[d])/lattice->dx[d];
				if(comparts!=floor(comparts)) {error++;simLog(sim,10," ERROR: port panel on dimension %i is not at a compartment boundary\n",d);}
				if(comparts<1) {error++;simLog(sim,10," ERROR: port panel on dimension %i needs to be at least one compartment in from the boundary on the low side\n");}
				comparts=(lattice->max[d]-pos)/lattice->dx[d];
				if(comparts!=floor(comparts)) {error++;simLog(sim,10," ERROR: port panel on dimension %i is not at a compartment boundary\n",d);}
				if(comparts<1) {error++;simLog(sim,10," ERROR: port panel on dimension %i needs to be at least one compartment in from the boundary on the high side\n");} }}

		if((!lattice->pde) && (!lattice->nsv)) {
			error++;simLog(sim,10," ERROR: no method has been assigned to lattice %s\n",lattice->latticename); }

		if(lattice->nreactions==0) {
      warn++;simLog(sim,5," WARNING: there are no reactions assigned to lattice %s\n",lattice->latticename); }

	for(i=0;i<lattice->nsurfaces;i++) {
		interact = 0;
		for(s=0;s<lattice->nspecies;s++) {
			if ((lattice->surfacelist[i]->action[lattice->species_index[s]][0][PFfront]==SAreflect) ||
					(lattice->surfacelist[i]->action[lattice->species_index[s]][0][PFback]==SAreflect) ||
					(lattice->surfacelist[i]->action[lattice->species_index[s]][0][PFfront]==SAabsorb) ||
					(lattice->surfacelist[i]->action[lattice->species_index[s]][0][PFback]==SAabsorb)) interact++;
		}
		if (!interact) {
			warn++;simLog(sim,5," WARNING: no species interact with surface %s for lattice %s. Note that only reflect and absorb surface actions are supported for lattices\n",lattice->surfacelist[i]->sname,lattice->latticename); }

	}

    if(lattice->nspecies==0) {
			warn++;simLog(sim,5," WARNING: there are no species assigned to lattice %s\n",lattice->latticename); }}

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/******************************** structure set up ****************************/
/******************************************************************************/


/* latticesetcondition */
void latticesetcondition(latticessptr latticess,enum StructCond cond,int upgrade) {
	if(!latticess) return;
	if(upgrade==0 && latticess->condition>cond) latticess->condition=cond;
	else if(upgrade==1 && latticess->condition<cond) latticess->condition=cond;
	else if(upgrade==2) latticess->condition=cond;
	if(latticess->sim && latticess->condition<latticess->sim->condition) {
		cond=latticess->condition;
		simsetcondition(latticess->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* latticeenablelattices */
int latticeenablelattices(simptr sim) {
	latticessptr latticess;

	if(sim->latticess) return 0;							// check for redundant function call

	latticess=latticessalloc(sim->latticess,1,sim->dim);
	if(!latticess) return 1;
	sim->latticess=latticess;
	latticess->sim=sim;

  NSV_CALL(nsv_init());

	latticesetcondition(sim->latticess,SClists,0);
	return 0; }


/* latticeaddlattice */
int latticeaddlattice(simptr sim,latticeptr *latptr,const char *latticename,const double *min,const double *max,const double *dx,const char *btype,enum LatticeType type) {
	int er,lat,d,dim;
	latticessptr latticess;
	latticeptr lattice;

  dim=sim->dim;

	if(!sim->latticess) {
    er=latticeenablelattices(sim);
    if(er) return 1; }
	latticess=sim->latticess;

  if(*latptr) lattice=*latptr;
  else {
	  if(latticess->nlattice==latticess->maxlattice) {
		  latticess=latticessalloc(latticess,latticess->nlattice*2+1,dim);
		  if(!latticess) return 1; }

	  lat=latticess->nlattice++;
	  lattice=latticess->latticelist[lat];
	  *latptr=lattice; }

  if(latticename) {
    strncpy(lattice->latticename,latticename,STRCHAR-1);
		lattice->latticename[STRCHAR-1]='\0'; }
	if(type!=LATTICEnone)
    lattice->type = type;
  if(min)
    for(d=0;d<dim;d++) lattice->min[d]=min[d];
  if(max)
    for(d=0;d<dim;d++) lattice->max[d]=max[d];
  if(dx)
    for(d=0;d<dim;d++) lattice->dx[d]=dx[d];
  if(btype)
    for(d=0;d<dim;d++) lattice->btype[d]=btype[d];

	latticesetcondition(latticess,SClists,0);
	return 0; }


/* latticeaddspecies */
int latticeaddspecies(latticeptr lattice,int ident,int *index) {
	int i,j,er;

	if(index) {
		for(j=0;j<index[PDnresults];j++) {
			er=latticeaddspecies(lattice,index[PDMAX+j],NULL);
			if(er==1) return 1; }
		return 0; }

	for(i=0;i<lattice->nspecies;++i)
		if(lattice->species_index[i]==ident)
			return 2;
	if(lattice->nspecies==lattice->maxspecies) {
		er=latticeexpandspecies(lattice,2*lattice->maxspecies+1);
		if(er) return 1; }

	i=lattice->nspecies;
	lattice->species_index[i]=ident;
	lattice->nmols[i]=0;
	lattice->nspecies++;
	latticesetcondition(lattice->latticess,SCparams,0);
	return 0; }


/* latticeaddrxn */
int latticeaddrxn(latticeptr lattice,rxnptr reaction,int move) {
	int i,er;

	for(i=0;i<lattice->nreactions;++i) {
		if(lattice->reactionlist[i]==reaction)
      return 2; }
	if(lattice->nreactions==lattice->maxreactions) {
		er=latticeexpandreactions(lattice,2*lattice->maxreactions+1);
		if(er) return 1; }

	i=lattice->nreactions;
	lattice->reactionlist[i]=reaction;
	lattice->reactionmove[i]=move;
	lattice->nreactions++;
	latticesetcondition(lattice->latticess,SCparams,0);
	return 0; }


/* latticeaddsurface */
int latticeaddsurface(latticeptr lattice,surfaceptr surface) {
	int i,er;

	for(i=0;i<lattice->nsurfaces;++i) {
		if(lattice->surfacelist[i]==surface)
      return 2; }
	if(lattice->nsurfaces==lattice->maxsurfaces) {
		er=latticeexpandsurfaces(lattice,2*lattice->maxsurfaces+1);
		if(er) return 1; }

	i=lattice->nsurfaces;
	lattice->surfacelist[i]=surface;
	lattice->nsurfaces++;
	latticesetcondition(lattice->latticess,SCparams,0);
	return 0; }


/* latticeaddmols */
int latticeaddmols(latticeptr lattice,int nmol,int i,double *poslo,double *poshi,int dim) {
	int j,er,m,d;

	for(j=0;j<lattice->nspecies;j++)
		if(lattice->species_index[j]==i) break;
	if(j==lattice->nspecies) {
		er=latticeaddspecies(lattice,i,NULL);
		if(er) return 1; }

	if(lattice->nmols[j]+nmol>=lattice->maxmols[j]) {
		er=latticeexpandmols(lattice,j,lattice->nmols[j]+nmol+1,dim);
		if(er) return 1; }

	for(m=lattice->nmols[j];m<lattice->nmols[j]+nmol;m++) {
		for(d=0;d<dim;d++)
			if(poslo[d]==poshi[d])
				lattice->mol_positions[j][m][d]=poslo[d];
			else
				lattice->mol_positions[j][m][d]=unirandOOD(poslo[d],poshi[d]); }
	lattice->nmols[j]+=nmol;

	if(lattice->latticess->sim->mols)
		lattice->latticess->sim->mols->touch++;
	return 0; }


/* latticeaddport */
void latticeaddport(latticeptr lattice,portptr port) {
	lattice->port = port;
	latticesetcondition(lattice->latticess,SCparams,0);
	return; }


/* latticeaddconvert */
int latticeaddconvert(latticeptr lattice,int ident,int *index,enum PanelFace face,int convert) {
	int j,latident,er;

	if(index) {
		for(j=0;j<index[PDnresults];j++) {
			er=latticeaddconvert(lattice,index[PDMAX+j],NULL,face,convert);
			if(er) return er; }
		return 0; }

	for(latident=0;latident<lattice->nspecies;latident++)
		if(lattice->species_index[latident]==ident) break;
	if(latident==lattice->nspecies) return ident;

	if(face==PFfront || face==PFboth)
		lattice->convert[latident][PFfront]=convert;
	if(face==PFback || face==PFboth)
		lattice->convert[latident][PFback]=convert;
	return 0; }


/* latticereadstring */
latticeptr latticereadstring(simptr sim,ParseFilePtr pfp,latticeptr lattice,const char *word,char *line2) {
	char nm[STRCHAR],ch;
	int axis,s,itct,i,j,nmol,d,dim,move,lat,er,*index;
	double min,max,dx,flt1,flt2;
	double poslo[DIMMAX],poshi[DIMMAX];
	enum MolecState ms;
	enum PanelFace face;

	char **varnames;
	double *varvalues;
	int nvar;

	varnames=sim->varnames;
	varvalues=sim->varvalues;
	nvar=sim->nvar;

	dim=sim->dim;

	if(!strcmp(word,"name")) {                        // name
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading lattice name");
		lattice=NULL;
		lat=-1;
		if(sim->latticess)
      lat=stringfind(sim->latticess->latticenames,sim->latticess->nlattice,nm);
		if(lat>=0) lattice=sim->latticess->latticelist[lat];
		else {
			er=latticeaddlattice(sim,&lattice,nm,NULL,NULL,NULL,NULL,LATTICEnone);
			CHECKS(!er,"out of memory adding lattice"); }
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"type")) {                   // type
		CHECKS(lattice,"lattice name has to be entered before type");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading lattice type");
		if(!strcmp(nm,"nsv"))
			er=latticeaddlattice(sim,&lattice,NULL,NULL,NULL,NULL,NULL,LATTICEnsv);
		else if(!strcmp(nm,"pde"))
			er=latticeaddlattice(sim,&lattice,NULL,NULL,NULL,NULL,NULL,LATTICEpde);
		else {
			CHECKS(0,"lattice type not recognised"); }
		CHECKS(!strnword(line2,2),"unexpected text following type"); }

	else if(!strcmp(word,"boundaries")) {             // boundaries

		CHECKS(lattice,"lattice name has to be entered before lattice boundaries");
		CHECKS(dim>0,"need to enter dim before boundaries");
		itct=strmathsscanf(line2,"%s %mlg %mlg",varnames,varvalues,nvar,nm,&min,&max);
		CHECKS(itct==3,"boundaries format: dimension position position [type]");
		if(!strcmp(nm,"0") || !strcmp(nm,"x")) axis=0;
		else if(!strcmp(nm,"1") || !strcmp(nm,"y")) axis=1;
		else if(!strcmp(nm,"2") || !strcmp(nm,"z")) axis=2;
		else axis=3;
		CHECKS(axis>=0 && axis<dim,"boundaries dimension needs to be between 0 and dim-1");
		CHECKS(min<max,"the first boundary position needs to be smaller than the second one");
		line2=strnword(line2,4);
		ch='r';
		if(line2) {
			itct=sscanf(line2,"%c",&ch);
			CHECKS(itct==1,"boundary type character could not be read");
			CHECKS(ch=='r' || ch=='p',"boundaries type needs to be r or p.");
			line2=strnword(line2,2); }
		lattice->min[axis]=min;
		lattice->max[axis]=max;
		lattice->btype[axis]=ch;
		CHECKS(!line2,"unexpected text following boundaries"); }

	else if(!strcmp(word,"lengthscale")) {            // lengthscale

		CHECKS(lattice,"lattice name has to be entered before lengthscale");
		CHECKS(dim>0,"need to enter dim before lengthscale");
		for(d=0;d<dim;d++) {
      CHECKS(line2,"missing values for lengthscale");
			itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&dx);
			CHECKS(itct==1,"error reading lengthscale");
			lattice->dx[d]=dx;
      line2=strnword(line2,2); }
    CHECKS(!line2,"unexpected text following lengthscale"); }

	else if(!strcmp(word,"port")) {                   // port

		CHECKS(lattice,"lattice name has to be entered before import");
    CHECKS(sim->portss,"ports need to be defined before being assigned");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading port name");
		s=stringfind(sim->portss->portnames,sim->portss->nport,nm);
		CHECKS(s>=0,"port '%s' not recognized",nm);
		//CHECKS(sim->portss->portlist[s]->srf->npanel[PSrect]>=1,"porting surface needs to contain a rectangle panel");
		latticeaddport(lattice,sim->portss->portlist[s]);
		CHECKS(!strnword(line2,2),"unexpected text following export"); }

	else if(!strcmp(word,"make_particle")) {                // make_particle

		CHECKS(lattice,"lattice name has to be entered before make_particle");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"make_particle format: front/back/both species");
		face=surfstring2face(nm);
		CHECKS(face!=PFnone,"unrecognized surface face name");
		line2=strnword(line2,2);
		CHECKS(line2,"make_particle format: front/back/both species");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"make_particle format: front/back/both species");
		i=molstring2index1(sim,nm,&ms,&index);
		CHECKS(i!=-1,"make_particle format: face species");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-6,"cannot read wildcard logic");
		CHECKS(i!=-7,"error allocating memory");
		CHECKS(ms==MSsoln,"lattices only work with solution phase molecules");
		er=latticeaddconvert(lattice,i,index,face,1);
		CHECKS(!er,"species %s needs to be imported to the lattice with the species statement",sim->mols->spname[er]);
		CHECKS(!strnword(line2,2),"unexpected text following make_particle"); }

	else if(!strcmp(word,"reactions")) {                // reactions

		CHECKS(lattice,"lattice name has to be entered before reactions");
		move=0;
		while(line2) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"failed to read reaction name");
			if(!strcmp(nm,"move")) move=1;
			else if(!strcmp(nm,"all")) {
				for(j=0;j<MAXORDER;j++) {
					if(sim->rxnss[j]) {
						for(i=0;i<sim->rxnss[j]->totrxn;++i) {
							er=latticeaddrxn(lattice,sim->rxnss[j]->rxn[i],move);
							CHECKS(er!=1,"out of memory adding reaction to lattice"); }}}}
			else {
				s=-1;
				for(j=0;j<MAXORDER && s<0;++j)
					if(sim->rxnss[j]) {
						s=stringfind(sim->rxnss[j]->rname,sim->rxnss[j]->totrxn,nm);
						if (s >= 0) break;
					}
				CHECKS(s>=0,"reaction '%s' not recognized",nm);
				er=latticeaddrxn(lattice,sim->rxnss[j]->rxn[s],move);
				CHECKS(er!=1,"out of memory adding reaction to lattice"); }
			line2=strnword(line2,2); }
		CHECKS(!line2,"unexpected text following reactions"); }

	else if(!strcmp(word,"surfaces")) {                   // surfaces

			CHECKS(lattice,"lattice name has to be entered before surfaces");
			while(line2) {
				itct=sscanf(line2,"%s",nm);
				CHECKS(itct==1,"failed to read surface name");
				if(!strcmp(nm,"all")) {
					for(i=0;i<sim->srfss->nsrf;++i) {
						er=latticeaddsurface(lattice,sim->srfss->srflist[i]);
						CHECKS(er!=1,"out of memory adding surface to lattice"); }}
				else {
					s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
					CHECKS(s>=0,"surface '%s' not recognized",nm);
					er=latticeaddsurface(lattice,sim->srfss->srflist[s]);
					CHECKS(er!=1,"out of memory adding surface to lattice"); }
				line2=strnword(line2,2); }
			CHECKS(!line2,"unexpected text following surfaces"); }

	else if(!strcmp(word,"species")) {                   // species

		CHECKS(lattice,"lattice name has to be entered before species");
		while(line2) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"failed to read species name");
			er=molstring2index1(sim,nm,&ms,&index);
			CHECKS(er!=-1,"species format: species_name");
			CHECKS(er!=-2,"mismatched or improper parentheses around species state");
			CHECKS(er!=-3,"cannot read molecule state value");
			CHECKS(er!=-4,"species name not recognized");
			CHECKS(er!=-6,"cannot read wildcard logic");
			CHECKS(er!=-7,"error allocating memory");
			CHECKS(ms==MSsoln,"molecule state needs to be solution");
			er=latticeaddspecies(lattice,er,index);
			CHECKS(er!=1,"out of memory adding species to lattice");
			line2=strnword(line2,2); }
		CHECKS(!line2,"unexpected text following species"); }

	else if(!strcmp(word,"mol")) {                        // mol
    CHECKS(lattice,"lattice name has to be entered before mol");
		CHECKS(sim->mols,"need to enter species before mol");
    for(d=0;d<dim;d++)
      if(lattice->btype[d]=='u') {
        CHECKS(0,"need to define all lattice boundaries before entering mol"); }
		//CHECKS(lattice->port,"need to set port before mol");

		itct=strmathsscanf(line2,"%mi",varnames,varvalues,nvar,&nmol);
		CHECKS(itct==1,"mol format: number name position_vector");
		CHECKS(nmol>=0,"number of molecules added needs to be >=0");
		CHECKS(line2=strnword(line2,2),"mol format: number name position_vector");
		i=molstring2index1(sim,line2,&ms,NULL);
		CHECKS(i!=0,"wildcards and species groups are not permitted");
		CHECKS(i!=-1,"error reading molecule name");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"'all' species is not permitted");
		CHECKS(i!=-7,"out of memory");
		CHECKS(ms==MSsoln,"state needs to be solution");
		for(d=0;d<dim;d++) {poslo[d]=lattice->min[d];poshi[d]=lattice->max[d];}
		for(d=0;d<dim;d++) {
			CHECKS(line2=strnword(line2,2),"insufficient position data for mol");
			if(line2[0]=='u') {
				CHECKS(sim->wlist,"need to enter boundaries before using 'u' in a mol statement"); }
			else {
				itct=sscanf(line2,"%lg-%lg",&flt1,&flt2);
				if(itct==2) {
					poslo[d]=flt1;
					poshi[d]=flt2; }
				else {
					itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&flt1);
					CHECKS(itct==1,"cannot read position value for mol");
					poslo[d]=poshi[d]=flt1; }}}
		er=latticeaddmols(lattice,nmol,i,poslo,poshi,sim->dim);
		CHECKS(!er,"out of memory adding molecules to lattice");
		CHECKS(!strnword(line2,2),"unexpected text following mol"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within lattice block: statement not recognized"); }

	return lattice;

 failure:
	simParseError(sim,pfp);
	return NULL; }


/* loadlattice */
int loadlattice(simptr sim,ParseFilePtr *pfpptr,char* line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	latticeptr lattice;

	pfp=*pfpptr;
	done=0;
	lattice=NULL;
	firstline2=line2?1:0;

	while(!done) {
		if(pfp->lctr==0)
			simLog(sim,2," Reading file: '%s'\n",pfp->fname);
		if(firstline2) {
			strcpy(word,"name");
			pfpcode=1;
			firstline2=0; }
		else
			pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
		*pfpptr=pfp;
		CHECKS(pfpcode!=3,"%s",errstring);

		if(pfpcode==0);																// already taken care of
		else if(pfpcode==2) {													// end reading
			done=1; }
		else if(!strcmp(word,"end_lattice")) {						// end_lattice
			CHECKS(!line2,"unexpected text following end_lattice");
			return 0; }
		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }
		else {
			lattice=latticereadstring(sim,pfp,lattice,word,line2);
			CHECK(lattice); }}															// failed but error has already been reported

	CHECKS(0,"end of file encountered before end_lattice statement");	// end of file

 failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* latticesupdateparams */
int latticesupdateparams(simptr sim) {
	int i,j,k,order,r,si,latcount,er,s;
	double start[3];
	double end[3];
	double norm_front[3];
	double norm_back[3];
	panelptr panel;
	portptr port;
	latticeptr lattice;
	rxnptr reaction;
	surfaceptr surface;

	for(i=0;i<sim->latticess->nlattice;++i) {
		lattice=sim->latticess->latticelist[i];
		port=lattice->port;

		if ((port) && (port->srf)) {											// send port information to NSV code
            for(j=0;j<port->srf->npanel[PSrect];++j) {
                for(k=0;k<3;++k) {
                    start[k]=0.0;							// start and end are opposite port rectangle corners
                    end[k]=1.0;								// norm is port rectangle normal
                    norm_front[k]=0.0; 
                    norm_back[k]=0.0; 
                }

                panel=port->srf->panels[PSrect][j];
                switch(sim->dim) {
                case 1:											// 1D system
                    start[0]=panel->point[0][0];
                    end[0]=panel->point[0][0];
                    norm_front[0]=panel->front[0];
                    norm_back[0]=-panel->front[0];
                    break;
                case 2:											// 2D system
                    start[0]=panel->point[0][0];
                    start[1]=panel->point[0][1];
                    end[0]=panel->point[1][0];
                    end[1]=panel->point[1][1];
                    norm_front[(int)(panel->front[1])]=panel->front[0];
                    norm_back[(int)(panel->front[1])]=-panel->front[0];
                    break;
                case 3:											// 3D system
                    start[0]=panel->point[0][0];
                    start[1]=panel->point[0][1];
                    start[2]=panel->point[0][2];
                    end[0]=panel->point[2][0];
                    end[1]=panel->point[2][1];
                    end[2]=panel->point[2][2];
                    norm_front[(int)(panel->front[1])]=panel->front[0];
                    norm_back[(int)(panel->front[1])]=-panel->front[0];
                    break; }
                for(s=0;s<lattice->nspecies;++s) {			
                    si=lattice->species_index[s];
                    if(lattice->type==LATTICEnsv) {
                        if(lattice->convert[s][PFback])
                            NSV_CALL(nsv_add_interface(lattice->nsv,si,sim->dt,start,end,norm_front,sim->dim)); 
                        if(lattice->convert[s][PFfront]) 
                            NSV_CALL(nsv_add_interface(lattice->nsv,si,sim->dt,start,end,norm_back,sim->dim)); 
                    }
                    else if (lattice->type==LATTICEpde) {
                        //not implemented
                    }
                }
            }
        }
    

		for(order=0;order<MAXORDER;order++) {
			if(sim->rxnss[order])
				for(r=0;r<sim->rxnss[order]->totrxn;r++) {
					reaction=sim->rxnss[order]->rxn[r];
					if(reaction->rctrep) {
						latcount=0;
						for(j=0;j<order;j++)
							if(reaction->rctrep[j]==SRlattice) latcount++;
						if(latcount==order) {
							er=latticeaddrxn(lattice,reaction,0);
							if(er==1) return 1; }}}}

        for(j=0;j<lattice->nreactions;++j) {    // send reaction information to NSV code
            reaction=lattice->reactionlist[j];
            if(lattice->type==LATTICEnsv) {
                NSV_CALL(nsv_add_reaction(lattice->nsv,reaction)); }
            else if(lattice->type==LATTICEpde) {
                //not implemented
            RxnSetValue(sim,"disable",reaction,lattice->reactionmove[j]?1:0); } // disable reactions in particle space if needed
        }


        /* only loop through reaction list
        for(order=0;order<MAXORDER;++order) {
            for(j=0;j<sim->rxnss[order]->totrxn;++j) {
                reaction=sim->rxnss[order]->rxn[j];
                latticerxn=true;
                for(s=0;s<MAXORDER;++s) {
                    enum SpeciesRepresentation rep = reaction->rctrep[s];
                    if ((rep == SRparticle) || (rep == SRnone)) latticerxn &= false;
                }
                if ((lattice->type==LATTICEnsv) && latticerxn) {
                    NSV_CALL(nsv_add_reaction(lattice->nsv,reaction)); }
                else if(lattice->type==LATTICEpde) {
                    //not implemented
                }

            }
        }
        */


		for(j=0;j<lattice->nsurfaces;++j) {	// send surface information to NSV code
			surface=lattice->surfacelist[j];
			if(lattice->type==LATTICEnsv) {
				NSV_CALL(nsv_add_surface(lattice->nsv,surface)); 
            } 
            else if(lattice->type==LATTICEpde) {
				//not implemented
			}
        }
    }
	return 0; 
}


/* latticesupdatelists */
int latticesupdatelists(simptr sim) {
	latticeptr lattice;
	int i,si,m,s;

	for(i=0;i<sim->latticess->nlattice;++i) {
		lattice=sim->latticess->latticelist[i];
    if(lattice->type==LATTICEnone)
			latticeaddlattice(sim,&lattice,NULL,NULL,NULL,NULL,NULL,LATTICEnsv);  // default is NSV, if not defined up to now
		if(lattice->type==LATTICEnsv) {						// delete any old nsv data and replace with new data
			if(lattice->nsv)
				NSV_CALL(nsv_delete(lattice->nsv));
			NSV_CALL(lattice->nsv=nsv_new(lattice->min,lattice->max,lattice->dx,sim->dim));

			for(s=0;s<lattice->nspecies;++s) {			// send species and molecules to nsv code
				si=lattice->species_index[s];
				NSV_CALL(nsv_add_species(lattice->nsv,si,sim->mols->difc[si][MSsoln],lattice->btype,sim->dim));

				for(m=0;m<lattice->nmols[s];++m) {		// add molecules from mol_positions to new lattice
					NSV_CALL(nsv_add_mol(lattice->nsv,si,lattice->mol_positions[s][m],sim->dim)); }
				lattice->nmols[s]=0; }}

		else if(lattice->type==LATTICEpde) {
			//not implemented
		}}

	return 0; }


/* latticesupdate */
int latticesupdate(simptr sim) {
	int er;
	latticessptr latticess;
	
	latticess=sim->latticess;
	if(latticess) {
		if(latticess->condition<=SClists) {
			er=latticesupdatelists(sim);
			if(er) return er;
			latticesetcondition(latticess,SCparams,1); }
		if(latticess->condition==SCparams) {
			er=latticesupdateparams(sim);
			if(er) return er;
			latticesetcondition(latticess,SCok,1); }}
	return 0; }


/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* latticeruntimestep */
int latticeruntimestep(simptr sim) {
	int i;
	latticeptr lattice;

	for(i=0;i<sim->latticess->nlattice;++i) {
		lattice=sim->latticess->latticelist[i];
		if(lattice->type==LATTICEnsv) {
			NSV_CALL(nsv_integrate(lattice->nsv,sim->dt,lattice->port,lattice)); }
		else if(lattice->type==LATTICEpde) {
			//not implemented
		}}

	if(sim->mols) sim->mols->touch++;
	return 0; }


