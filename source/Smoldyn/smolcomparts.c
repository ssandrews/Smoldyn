/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "random2.h"
#include "RnSort.h"
#include "string2.h"
#include "Zn.h"

#include "smoldyn.h"
#include "smoldynfuncs.h"

#ifdef OPTION_VCELL
	#include <algorithm>
	using std::max;
	using std::min;
	#include <fstream>
	#include <ostream>
	#include <sstream>
	#include <iostream>

	extern "C" {
		#include "zlib.h" }
#endif


int compartsupdateparams_original(simptr sim);
int compartsupdateparams_volumeSample(simptr sim);


/******************************************************************************/
/********************************* Compartments *******************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types
enum CmptLogic compartstring2cl(char *string);
char *compartcl2string(enum CmptLogic cls,char *string);

// low level utilities

// memory management
compartptr compartalloc(void);
void compartfree(compartptr cmpt);
compartssptr compartssalloc(compartssptr cmptss,int maxcmpt);

// data structure output

// structure set up
int compartupdatebox(simptr sim,compartptr cmpt,boxptr bptr,double volfrac);
int compartsupdateparams(simptr sim);
int compartsupdatelists(simptr sim);

/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/


/* compartstring2cl */
enum CmptLogic compartstring2cl(char *string) {
	enum CmptLogic ans;

	if(!strcmp(string,"equal")) ans=CLequal;
	else if(!strcmp(string,"equalnot")) ans=CLequalnot;
	else if(!strcmp(string,"and")) ans=CLand;
	else if(!strcmp(string,"or")) ans=CLor;
	else if(!strcmp(string,"xor")) ans=CLxor;
	else if(!strcmp(string,"andnot")) ans=CLandnot;
	else if(!strcmp(string,"ornot")) ans=CLornot;
	else ans=CLnone;
	return ans; }


/* compartcl2string */
char *compartcl2string(enum CmptLogic cls,char *string) {
	if(cls==CLequal) strcpy(string,"equal");
	else if(cls==CLequalnot) strcpy(string,"equalnot");
	else if(cls==CLand) strcpy(string,"and");
	else if(cls==CLor) strcpy(string,"or");
	else if(cls==CLxor) strcpy(string,"xor");
	else if(cls==CLandnot) strcpy(string,"andnot");
	else if(cls==CLornot) strcpy(string,"ornot");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/****************************** low level utilities ***************************/
/******************************************************************************/

#ifdef OPTION_VCELL
//called by posincompart and compartsupdateparams_volumeSample
unsigned char getCompartmentID(char* cmptName, VolumeSamplesPtr vSamplesPtr)
{
	int c;
	for(c=0;c<vSamplesPtr->nCmptIDPair;c++) {
		if(!strcmp(cmptName, vSamplesPtr->compartmentIDPairPtr[c].name))
		{
			return vSamplesPtr->compartmentIDPairPtr[c].pixel;
		}
	}
	return 0; //TODO: if can't find the compartment ID, what should we do? ??
}
#endif


/* posincompart */
int posincompart(simptr sim,double *pos,compartptr cmpt,int useoldpos) {
	int s,p,k,incmpt,pcross,cl,incmptl;
	enum PanelShape ps;
	surfaceptr srf;
	double crsspt[DIMMAX];
	enum CmptLogic sym;
	
#ifdef OPTION_VCELL
	int dim;
	VolumeSamples* volumeSamplePtr;
	const int numNeighbors =3;
	int sampleIdxNeighbors[DIMMAX][numNeighbors];
	
	incmpt = 0;
	int sameResultCount = 0;

	//for smoldyn simulations with volumeSample
	if(sim->volumeSamplesPtr != NULL) {
		volumeSamplePtr = sim->volumeSamplesPtr;
		dim = sim->dim;
		unsigned char cmptID = getCompartmentID(cmpt->cname, sim->volumeSamplesPtr);
		
		for (int idim = 0; idim < dim; idim ++) {
			double h = volumeSamplePtr->size[idim]/volumeSamplePtr->num[idim];
			int idx = (int)floor((pos[idim] - volumeSamplePtr->origin[idim])/h);
			idx = max(0,idx);//clamp index lower bound to 0
			idx = min((volumeSamplePtr->num[idim]-1),idx);//clamp index upper bound to number of volume sample at specific dimension
			sampleIdxNeighbors[idim][0] = std::max<int>(0,(idx-1));
			sampleIdxNeighbors[idim][1] = idx;
			sampleIdxNeighbors[idim][2] = std::min<int>((volumeSamplePtr->num[idim]-1),(idx+1)); }

		//int numNeighbors =3;
		for (int kk = 0; kk < (dim < 3 ?  1 : numNeighbors); kk ++) {
			for (int jj = 0; jj < (dim < 2 ?  1 : numNeighbors); jj ++) {
				for (int ii = 0; ii < numNeighbors; ii ++) {
					int i = sampleIdxNeighbors[0][ii];
					int j = dim < 2 ?  0 : sampleIdxNeighbors[1][jj];
					int k = dim < 3 ?  0 : sampleIdxNeighbors[2][kk];
					int posInSample = i + j*volumeSamplePtr->num[0] + k*volumeSamplePtr->num[0]*volumeSamplePtr->num[1];
					unsigned char cmptIDFoundInSample = volumeSamplePtr->volsamples[posInSample];
					if(cmptID == cmptIDFoundInSample){
						sameResultCount ++; }}}}
		if (sameResultCount == (int)pow((double)numNeighbors, dim)) {
			incmpt = 1; }}

	if((incmpt == 0 && sameResultCount > 0) || sim->volumeSamplesPtr == NULL) //for original smoldyn simulations(without volumeSample)
#endif

	{
		incmpt=0;
		for(k=0;k<cmpt->npts && incmpt==0;k++) {
			pcross=0;
			for(s=0;s<cmpt->nsrf && !pcross;s++) {
				srf=cmpt->surflist[s];
				for(ps=(enum PanelShape)0;ps<PSMAX&&!pcross;ps=(enum PanelShape)(ps+1))
					for(p=0;p<srf->npanel[ps]&&!pcross;p++)
						if(lineXpanel(pos,cmpt->points[k],srf->panels[ps][p],sim->dim,crsspt,NULL,NULL,NULL,NULL,NULL,useoldpos))
							pcross=1; }
			if(pcross==0) incmpt=1; }

		for(cl=0;cl<cmpt->ncmptl;cl++) {
			incmptl=posincompart(sim,pos,cmpt->cmptl[cl],0);
			sym=cmpt->clsym[cl];
			if(sym==CLequal) incmpt=incmptl;
			else if(sym==CLequalnot) incmpt=!incmptl;
			else if(sym==CLand) incmpt=incmpt&&incmptl;
			else if(sym==CLor) incmpt=incmpt||incmptl;
			else if(sym==CLxor) incmpt=(incmpt!=incmptl);
			else if(sym==CLandnot) incmpt=incmpt&&!incmptl;
			else if(sym==CLornot) incmpt=incmpt||!incmptl; }
	}
	return incmpt; }


/* compartrandpos */
int compartrandpos(simptr sim,double *pos,compartptr cmpt) {
	static int ptmax=10000;
	int d,dim,i,done,k,bc;
	boxptr bptr;

	if(cmpt->npts==0&&cmpt->ncmptl==0) return 1;
	dim=sim->dim;

	done=0;
	if(cmpt->nbox) {
		bc=intrandpD(cmpt->nbox,cmpt->cumboxvol);
		bptr=cmpt->boxlist[bc];
		for(i=0;i<ptmax&&!done;i++) {
			boxrandpos(sim,pos,bptr);
			if(posincompart(sim,pos,cmpt,0)) done=1; }}
	else {
		for(i=0;i<ptmax&&!done;i++) {
			for(d=0;d<dim;d++) pos[d]=unirandCCD(sim->wlist[2*d]->pos,sim->wlist[2*d+1]->pos);
			if(posincompart(sim,pos,cmpt,0)) done=1; }}
	if(!done&&cmpt->npts>0) {
		k=intrand(cmpt->npts);
		for(d=0;d<sim->dim;d++) pos[d]=cmpt->points[k][d];
		done=1; }
	if(!done) return 1;
	return 0; }


/* fromHex */
unsigned char fromHex(const char* src) {
	char chs[5];
	chs[0] = '0';
	chs[1] = 'x';
	chs[2] = src[0];
	chs[3] = src[1];
	chs[4] = 0;
	int v;
	sscanf(chs, "%x", &v);
	return (unsigned char)v;
}


#ifdef OPTION_VCELL
	using std::istringstream;
/* loadHighResVolumeSamples */
int loadHighResVolumeSamples(simptr sim,ParseFilePtr *pfpptr,char *line2) {		//?? needs to be added to user manual
	char errstring[STRCHAR];
	
	char word[STRCHAR];
	ParseFilePtr pfp = *pfpptr;
	int pfpcode;
	*pfpptr=pfp;

	VolumeSamplesPtr volumeSamplesPtr = new VolumeSamples;
	sim->volumeSamplesPtr = volumeSamplesPtr;
	volumeSamplesPtr->num[0] = volumeSamplesPtr->num[1] = volumeSamplesPtr->num[2] = 1;

	int done = 0;
	string pixelLine;
	while(!done) {
		pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
		
		*pfpptr=pfp;
		CHECKS(pfpcode!=3,"%s",errstring);

		if(pfpcode==0);	// already taken care of

		else if(pfpcode==2) { // end reading
			done = 1;
		} else if(pfpcode==3) {	// error
			CHECKS(0,"SMOLDYN BUG: parsing error");
		} else if(!strcmp(word,"end_highResVolumeSamples")) {  // end_jms
			CHECKS(!line2,"unexpected text following end_highResVolumeSamplesFile");
			break;
		} else {
			if (!strcmp(word, "Size")) {
				istringstream lineInput(line2);
				lineInput >> volumeSamplesPtr->size[0] >> volumeSamplesPtr->size[1] >> volumeSamplesPtr->size[2];
			} else if (!strcmp(word, "Origin")) {
				istringstream lineInput(line2);
				lineInput >> volumeSamplesPtr->origin[0] >> volumeSamplesPtr->origin[1] >> volumeSamplesPtr->origin[2];
			} else if (!strcmp(word, "CompartmentHighResPixelMap")) {
				istringstream lineInput(line2);
				lineInput >> volumeSamplesPtr->nCmptIDPair;
				volumeSamplesPtr->compartmentIDPairPtr = new CompartmentIdentifierPair[volumeSamplesPtr->nCmptIDPair];
				for (int i = 0; i < volumeSamplesPtr->nCmptIDPair; i ++) {
					pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
					strcpy(volumeSamplesPtr->compartmentIDPairPtr[i].name, word);
					istringstream lineInput1(line2);
					int pixel;
					lineInput1 >> pixel;
					volumeSamplesPtr->compartmentIDPairPtr[i].pixel = pixel;
				}
			} else if (!strcmp(word, "VolumeSamples")) {
				istringstream lineInput(line2);
				lineInput >> volumeSamplesPtr->num[0] >> volumeSamplesPtr->num[1] >> volumeSamplesPtr->num[2];				
			} else {//volume sample data  with no token in front
				pixelLine += word;
			}
		}
	}

#ifdef HAVE_ZLIB
	unsigned long inflated_len;
	const char* compressed_hex;
	unsigned char* bytes_from_compressed;
	int compressed_len, returnVal;
	long numVolumeSamples;

	numVolumeSamples = volumeSamplesPtr->num[0] * volumeSamplesPtr->num[1] * volumeSamplesPtr->num[2];
	//cout<<pixelLine<<endl;
	compressed_len = pixelLine.size();
	if (compressed_len <= 1) {
		throw "CartesianMesh::readGeometryFile() : invalid compressed volume";
	}

	compressed_hex = pixelLine.c_str();
	//volumeSamples compressed, changed from byte to short
	bytes_from_compressed = new unsigned char[compressed_len+1];
	memset(bytes_from_compressed, 0, (compressed_len+1) * sizeof(unsigned char));
	for (int i = 0, j = 0; i < compressed_len; i += 2, j ++) {
		bytes_from_compressed[j] = fromHex(compressed_hex + i);
	}

	volumeSamplesPtr->volsamples = new unsigned char[numVolumeSamples];
	memset(volumeSamplesPtr->volsamples, 0, numVolumeSamples * sizeof(unsigned char));

	inflated_len = numVolumeSamples;
	returnVal = uncompress(volumeSamplesPtr->volsamples, &inflated_len, bytes_from_compressed, compressed_len);
	//check if the data can be properly uncompressed, Z_OK should be returned if successfully uncompressed.
	if(returnVal == Z_MEM_ERROR)
	{
		throw "There is not enough memory to uncompress volume samples.";
	}
	else if(returnVal == Z_BUF_ERROR)
	{
		throw "There is not enough room in the output buffer for volume samples.";
	}
	else if(returnVal == Z_DATA_ERROR)
	{
		throw "The volume sample input data was corrupted.";
	}
	//check if we get the expected number of volume samples after uncompression
	if ((long)inflated_len != numVolumeSamples) {
		throw "loadHighResVolumeSamples : unexpected number of volume samples";
	}
	//for debug purpose
	/*for (unsigned long i = 0; i < inflated_len; i ++) {		
		if (volumeSamplesPtr->volsamples[i] == 6) {
			cout << "volume sample  at " << i<< " is " << ((int)volumeSamplesPtr->volsamples[i])<< endl;
		}
		else if (volumeSamplesPtr->volsamples[i] == 16) {
			cout << "volume sample  at " << i<< " is " << ((int)volumeSamplesPtr->volsamples[i])<< endl;
		}
	}*/
	return 0;
	
#else
	throw "loadHighResVolumeSamples : function unavailable because compile does not include zlib";
#endif

failure:		// failure
	return 1;
}
#endif


/******************************************************************************/
/******************************* memory management ****************************/
/******************************************************************************/

/* compartalloc */
compartptr compartalloc(void) {
	compartptr cmpt;

	cmpt=(compartptr) malloc(sizeof(struct compartstruct));
	CHECKMEM(cmpt);
	cmpt->cname=NULL;
	cmpt->selfindex=-1;
	cmpt->nsrf=0;
	cmpt->surflist=NULL;
	cmpt->npts=0;
	cmpt->points=NULL;
	cmpt->ncmptl=0;
	cmpt->cmptl=NULL;
	cmpt->clsym=NULL;
	cmpt->volume=0;
	cmpt->maxbox=0;
	cmpt->nbox=0;
	cmpt->boxlist=NULL;
	cmpt->boxfrac=NULL;
	cmpt->cumboxvol=NULL;
	return cmpt;
 failure:
	simLog(NULL,10,"Failed to allocate memory in compartalloc");
	return NULL; }


/* compartfree */
void compartfree(compartptr cmpt) {
	int k;

	if(!cmpt) return;
	free(cmpt->cumboxvol);
	free(cmpt->boxfrac);
	free(cmpt->boxlist);
	free(cmpt->clsym);
	free(cmpt->cmptl);
	if(cmpt->npts&&cmpt->points)
		for(k=0;k<cmpt->npts;k++) free(cmpt->points[k]);
	free(cmpt->points);
	free(cmpt->surflist);
	free(cmpt);
	return; }


/* compartssalloc */
compartssptr compartssalloc(compartssptr cmptss,int maxcmpt) {
	int c;
	char **newnames;
	compartptr *newcmptlist;

	CHECKBUG(maxcmpt>0,"maxcmpt, in compartssalloc, needs to be >0");

	newnames=NULL;
	newcmptlist=NULL;

	if(!cmptss) {																			// new allocation
		cmptss=(compartssptr) malloc(sizeof(struct compartsuperstruct));
		CHECKMEM(cmptss);
		cmptss->condition=SCinit;
		cmptss->sim=NULL;
		cmptss->maxcmpt=0;
		cmptss->ncmpt=0;
		cmptss->cnames=NULL;
		cmptss->cmptlist=NULL; }
	else {																						// minor check
		if(maxcmpt<cmptss->maxcmpt) return cmptss; }

	if(maxcmpt>cmptss->maxcmpt) {											// allocate new compartment names and compartments
		CHECKMEM(newnames=(char**) calloc(maxcmpt,sizeof(char*)));
		for(c=0;c<maxcmpt;c++) newnames[c]=NULL;
		for(c=0;c<cmptss->maxcmpt;c++) newnames[c]=cmptss->cnames[c];
		for(;c<maxcmpt;c++)
			CHECKMEM(newnames[c]=EmptyString());

		CHECKMEM(newcmptlist=(compartptr*) calloc(maxcmpt,sizeof(compartptr)));	// compartment list
		for(c=0;c<maxcmpt;c++) newcmptlist[c]=NULL;
		for(c=0;c<cmptss->maxcmpt;c++) newcmptlist[c]=cmptss->cmptlist[c];
		for(;c<maxcmpt;c++) {
			CHECKMEM(newcmptlist[c]=compartalloc());
			newcmptlist[c]->cmptss=cmptss;
			newcmptlist[c]->cname=newnames[c];
			newcmptlist[c]->selfindex=c; }}

	cmptss->maxcmpt=maxcmpt;
	free(cmptss->cnames);
	cmptss->cnames=newnames;
	free(cmptss->cmptlist);
	cmptss->cmptlist=newcmptlist;

	return cmptss;

 failure:
 	compartssfree(cmptss);
	simLog(NULL,10,"%s","Failed to allocated memory in compartssalloc");
 	return NULL; }


/* compartssfree */
void compartssfree(compartssptr cmptss) {
	int c;

	if(!cmptss) return;
	if(cmptss->maxcmpt && cmptss->cmptlist)
		for(c=0;c<cmptss->maxcmpt;c++) compartfree(cmptss->cmptlist[c]);
	free(cmptss->cmptlist);
	if(cmptss->maxcmpt && cmptss->cnames)
		for(c=0;c<cmptss->maxcmpt;c++) free(cmptss->cnames[c]);
	free(cmptss->cnames);
	free(cmptss);
	return; }


/******************************************************************************/
/***************************** data structure output **************************/
/******************************************************************************/

/* compartoutput */
void compartoutput(simptr sim) {
	compartssptr cmptss;
	compartptr cmpt;
	int c,dim,s,k,d,cl;
	char string[STRCHAR];

	cmptss=sim->cmptss;
	if(!cmptss) return;
	simLog(sim,2,"COMPARTMENT PARAMETERS\n");
	dim=sim->dim;
	simLog(sim,2," Compartments allocated: %i, compartments defined: %i\n",cmptss->maxcmpt,cmptss->ncmpt);
	for(c=0;c<cmptss->ncmpt;c++) {
		cmpt=cmptss->cmptlist[c];
		simLog(sim,2," Compartment: %s\n",cmptss->cnames[c]);
		simLog(sim,2,"  %i bounding surfaces:\n",cmpt->nsrf);
		for(s=0;s<cmpt->nsrf;s++)
			simLog(sim,2,"   %s\n",cmpt->surflist[s]->sname);
		simLog(sim,2,"  %i interior-defining points:\n",cmpt->npts);
		for(k=0;k<cmpt->npts;k++) {
			simLog(sim,2,"   %i: (",k);
			for(d=0;d<dim-1;d++)
				simLog(sim,2,"%g,",cmpt->points[k][d]);
			simLog(sim,2,"%g)\n",cmpt->points[k][d]); }
		simLog(sim,2,"  %i logically combined compartments\n",cmpt->ncmptl);
		for(cl=0;cl<cmpt->ncmptl;cl++)
			simLog(sim,2,"   %s %s\n",compartcl2string(cmpt->clsym[cl],string),cmpt->cmptl[cl]->cname);
		simLog(sim,2,"  volume: %g\n",cmpt->volume);
		simLog(sim,2,"  %i virtual boxes listed\n",cmpt->nbox); }
	simLog(sim,2,"\n");
	return; }


/* writecomparts */
void writecomparts(simptr sim,FILE *fptr) {
	compartssptr cmptss;
	compartptr cmpt;
	int c,s,k,d,cl;
	char string[STRCHAR];

	cmptss=sim->cmptss;
	if(!cmptss) return;
	fprintf(fptr,"# Compartment parameters\n");
	fprintf(fptr,"max_compartment %i\n",cmptss->maxcmpt);
	for(c=0;c<cmptss->ncmpt;c++) {
		cmpt=cmptss->cmptlist[c];
		fprintf(fptr,"start_compartment %s\n",cmpt->cname);
		for(s=0;s<cmpt->nsrf;s++)
			fprintf(fptr,"surface %s\n",cmpt->surflist[s]->sname);
		for(k=0;k<cmpt->npts;k++) {
			fprintf(fptr,"point");
			for(d=0;d<sim->dim;d++)
				fprintf(fptr," %g",cmpt->points[k][d]);
			fprintf(fptr,"\n"); }
		for(cl=0;cl<cmpt->ncmptl;cl++)
			fprintf(fptr,"compartment %s %s\n",compartcl2string(cmpt->clsym[cl],string),cmpt->cmptl[cl]->cname);
		fprintf(fptr,"end_compartment\n\n"); }
	return; }


/* checkcompartparams */
int checkcompartparams(simptr sim,int *warnptr) {
	int error,warn,c;
	compartssptr cmptss;
	compartptr cmpt;
	char string[STRCHAR];

	error=warn=0;
	cmptss=sim->cmptss;
	if(!cmptss) {
		if(warnptr) *warnptr=warn;
		return 0; }

	if(cmptss->condition!=SCok) {
		warn++;
		simLog(sim,7," WARNING: compartment structure %s\n",simsc2string(cmptss->condition,string)); }

	for(c=0;c<cmptss->ncmpt;c++) {
		cmpt=cmptss->cmptlist[c];
		if(cmpt->volume<=0) {warn++;simLog(sim,5," WARNING: compartment %s has 0 volume\n",cmpt->cname);}
		if(cmpt->nbox==0) {warn++;simLog(sim,5," WARNING: compartment %s overlaps no virtual boxes\n",cmpt->cname);}
		if(cmpt->nbox>0&&cmpt->cumboxvol[cmpt->nbox-1]!=cmpt->volume) {error++;simLog(sim,10," BUG: compartment %s box volumes do not add to compartment volume\n",cmpt->cname);} }
	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/******************************** structure set up ****************************/
/******************************************************************************/


/* compartsetcondition */
void compartsetcondition(compartssptr cmptss,enum StructCond cond,int upgrade) {
	if(!cmptss) return;
	if(upgrade==0 && cmptss->condition>cond) cmptss->condition=cond;
	else if(upgrade==1 && cmptss->condition<cond) cmptss->condition=cond;
	else if(upgrade==2) cmptss->condition=cond;
	if(cmptss->sim && cmptss->condition<cmptss->sim->condition) {
		cond=cmptss->condition;
		simsetcondition(cmptss->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* compartenablecomparts */
int compartenablecomparts(simptr sim,int maxcmpt) {
	compartssptr cmptss;

	if(sim->cmptss)									// check for redundant function call
		if(maxcmpt==-1 || sim->cmptss->maxcmpt==maxcmpt)
			return 0;
	cmptss=compartssalloc(sim->cmptss,maxcmpt<0?5:maxcmpt);
	if(!cmptss) return 1;
	sim->cmptss=cmptss;
	cmptss->sim=sim;
	compartsetcondition(sim->cmptss,SClists,0);
	return 0; }


/* compartaddcompart */
compartptr compartaddcompart(simptr sim,const char *cmptname) {
	int er,c;
	compartssptr cmptss;
	compartptr cmpt;

	if(!sim->cmptss) {
		er=compartenablecomparts(sim,-1);
		if(er) return NULL; }
	cmptss=sim->cmptss;

	c=stringfind(cmptss->cnames,cmptss->ncmpt,cmptname);
	if(c<0) {
		if(cmptss->ncmpt==cmptss->maxcmpt) {
			er=compartenablecomparts(sim,cmptss->ncmpt*2+1);
			if(er) return NULL; }
		c=cmptss->ncmpt++;
		strncpy(cmptss->cnames[c],cmptname,STRCHAR-1);
		cmptss->cnames[c][STRCHAR-1]='\0';
		cmpt=cmptss->cmptlist[c];
		compartsetcondition(cmptss,SClists,0); }
	else
		cmpt=cmptss->cmptlist[c];

	return cmpt; }


/* compartaddsurf */
int compartaddsurf(compartptr cmpt,surfaceptr srf) {
	int s;
	surfaceptr *newsurflist;

	newsurflist=(surfaceptr*) calloc(cmpt->nsrf+1,sizeof(surfaceptr));
	if(!newsurflist) return 1;
	for(s=0;s<cmpt->nsrf;s++) {
		if(cmpt->surflist[s]==srf) {free(newsurflist);return 2;}
		newsurflist[s]=cmpt->surflist[s]; }
	newsurflist[s]=srf;
	cmpt->nsrf++;
	free(cmpt->surflist);
	cmpt->surflist=newsurflist;
	cmpt->nbox=0;
	cmpt->volume=0;
	compartsetcondition(cmpt->cmptss,SCparams,0);
	return 0; }


/* compartaddpoint */
int compartaddpoint(compartptr cmpt,int dim,double *point) {
	int d,k;
	double **newpoints;

	CHECKMEM(newpoints=(double**) calloc(cmpt->npts+1,sizeof(double*)));
	for(k=0;k<cmpt->npts;k++)
		newpoints[k]=cmpt->points[k];
	CHECKMEM(newpoints[k]=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++) newpoints[k][d]=point[d];
	cmpt->npts++;
	free(cmpt->points);
	cmpt->points=newpoints;
	compartsetcondition(cmpt->cmptss,SCparams,0);
	cmpt->nbox=0;
	cmpt->volume=0;
	return 0;

 failure:
	if(newpoints) free(newpoints);
	simLog(NULL,10,"Failed to allocate memory in compartaddpoint");
	return 1; }


/* compartaddcmptl */
int compartaddcmptl(compartptr cmpt,compartptr cmptl,enum CmptLogic sym) {
	int cl;
	compartptr *newcmptl;
	enum CmptLogic *newclsym;

	if(cmpt==cmptl) return 2;
	newcmptl=(compartptr*) calloc(cmpt->ncmptl+1,sizeof(compartptr));
	if(!newcmptl) return 1;
	newclsym=(enum CmptLogic*) calloc(cmpt->ncmptl+1,sizeof(enum CmptLogic));
	if(!newclsym) {free(newcmptl);return 1;}
	for(cl=0;cl<cmpt->ncmptl;cl++) {
		newcmptl[cl]=cmpt->cmptl[cl];
		newclsym[cl]=cmpt->clsym[cl]; }
	newcmptl[cl]=cmptl;
	newclsym[cl]=sym;
	cmpt->ncmptl++;
	free(cmpt->cmptl);
	free(cmpt->clsym);
	cmpt->cmptl=newcmptl;
	cmpt->clsym=newclsym;
	compartsetcondition(cmpt->cmptss,SCparams,0);
	cmpt->nbox=0;
	cmpt->volume=0;
	return 0; }


/* compartupdatebox */
int compartupdatebox(simptr sim,compartptr cmpt,boxptr bptr,double volfrac) {
	int ptsmax=100;	// number of random points for volume determination
	int bc,max,ptsin,i,bc2;
	double pos[DIMMAX],volfrac2,*newboxfrac,*newcumboxvol,boxvol,vol;
	boxptr *newboxlist;

	newboxlist=NULL;
	newboxfrac=NULL;
	newcumboxvol=NULL;

	for(bc=0;bc<cmpt->nbox && cmpt->boxlist[bc]!=bptr;bc++);	// check for box already in cmpt
	if(bc<cmpt->nbox && volfrac==-2) return 0;				// box is listed and volume ok, so return

	if(volfrac<=0) {																// find actual volume fraction
		ptsin=0;
		for(i=0;i<ptsmax;i++) {
			boxrandpos(sim,pos,bptr);
			if(posincompart(sim,pos,cmpt,0)) ptsin++; }
		volfrac2=(double)ptsin/(double)ptsmax; }
	else if(volfrac>1) volfrac2=1;
	else volfrac2=volfrac;

	if(volfrac2==0) {
		if(bc==cmpt->nbox) return 0;									// box not listed and 0 volume, so return
		cmpt->nbox--;																	// box was listed, so it needs to be removed
		if(cmpt->nbox==0) {
			cmpt->volume=0;
			return 2; }																	// last box was removed
		cmpt->boxlist[bc]=cmpt->boxlist[cmpt->nbox];
		cmpt->boxfrac[bc]=cmpt->boxfrac[cmpt->nbox];
		boxvol=sim->boxs->boxvol;
		vol=(bc==0)?0:cmpt->cumboxvol[bc-1];
		for(bc2=bc;bc2<cmpt->nbox;bc2++) {
			vol+=boxvol*cmpt->boxfrac[bc2];
			cmpt->cumboxvol[bc2]=vol; }
		cmpt->volume=vol;
		return 2; }

	if(bc<cmpt->nbox) {															// box was listed, so just update volume
		if(cmpt->boxfrac[bc]==volfrac2) return 0;			// volume was ok, so return
		cmpt->boxfrac[bc]=volfrac2;										// volume not ok, so update it
		boxvol=sim->boxs->boxvol;
		vol=(bc==0)?0:cmpt->cumboxvol[bc-1];
		for(bc2=bc;bc2<cmpt->nbox;bc2++) {
			vol+=boxvol*cmpt->boxfrac[bc2];
			cmpt->cumboxvol[bc2]=vol; }
		cmpt->volume=vol;
		return 3; }

	if(cmpt->nbox==cmpt->maxbox) {									// expand box list
		max=cmpt->maxbox>0?cmpt->maxbox*2:1;
		CHECKMEM(newboxlist=(boxptr*) calloc(max,sizeof(boxptr)));
		CHECKMEM(newboxfrac=(double*) calloc(max,sizeof(double)));
		CHECKMEM(newcumboxvol=(double*) calloc(max,sizeof(double)));
		for(bc2=0;bc2<cmpt->nbox;bc2++) {
			newboxlist[bc2]=cmpt->boxlist[bc2];
			newboxfrac[bc2]=cmpt->boxfrac[bc2];
			newcumboxvol[bc2]=cmpt->cumboxvol[bc2]; }
		for(;bc2<max;bc2++) {
			newboxlist[bc2]=NULL;
			newboxfrac[bc2]=0;
			newcumboxvol[bc2]=0; }
		cmpt->maxbox=max;
		free(cmpt->boxlist);
		free(cmpt->boxfrac);
		free(cmpt->cumboxvol);
		cmpt->boxlist=newboxlist;
		cmpt->boxfrac=newboxfrac;
		cmpt->cumboxvol=newcumboxvol; }

	bc=cmpt->nbox++;								// put box into cmpt
	cmpt->boxlist[bc]=bptr;
	cmpt->boxfrac[bc]=volfrac2;
	cmpt->volume+=sim->boxs->boxvol*cmpt->boxfrac[bc];
	cmpt->cumboxvol[bc]=cmpt->volume;
	return 1;

 failure:
 	free(newboxlist);
 	free(newboxfrac);
 	free(newcumboxvol);
	simLog(sim,10,"%s","Failed to allocate memory in compartupdatebox");
 	return -1; }


/* compartupdatebox, the volume fraction here is the actual volume fraction for the box inside the compartment*/
/* volfrac is actual volume fraction, for logic compartment the volfrac is assigned -2 */
#ifdef OPTION_VCELL
int compartupdatebox_volumeSample(simptr sim,compartptr cmpt,boxptr bptr,double volfrac) {
	/*int ptsmax=100;*/	// number of random points for volume determination
	int bc,max,bc2;
	double volfrac2,*newboxfrac,*newcumboxvol,boxvol,vol;
	boxptr *newboxlist;

	newboxlist=NULL;
	newboxfrac=NULL;
	newcumboxvol=NULL;

	for(bc=0;bc<cmpt->nbox && cmpt->boxlist[bc]!=bptr;bc++);	// check for box already in cmpt
	if(bc<cmpt->nbox && volfrac==-2) return 0;				// box is listed and volume ok, so return

	if(volfrac>1) volfrac2=1; 
	else volfrac2=volfrac; //the actual volume faction

	if(volfrac2==0) {
		if(bc==cmpt->nbox) return 0;									// box not listed and 0 volume, so return
		cmpt->nbox--;																	// box was listed, so it needs to be removed
		if(cmpt->nbox==0) {
			cmpt->volume=0;
			return 2; }																	// last box was removed
		cmpt->boxlist[bc]=cmpt->boxlist[cmpt->nbox];
		cmpt->boxfrac[bc]=cmpt->boxfrac[cmpt->nbox];
		boxvol=sim->boxs->boxvol;
		vol=(bc==0)?0:cmpt->cumboxvol[bc-1];
		for(bc2=bc;bc2<cmpt->nbox;bc2++) {
			vol+=boxvol*cmpt->boxfrac[bc2];
			cmpt->cumboxvol[bc2]=vol; }
		cmpt->volume=vol;
		return 2; }

	if(bc<cmpt->nbox) {													// box was listed, so just update volume
		if(cmpt->boxfrac[bc]==volfrac2) return 0;			// volume was ok, so return
		cmpt->boxfrac[bc]=volfrac2;										// volume not ok, so update it
		boxvol=sim->boxs->boxvol;
		vol=(bc==0)?0:cmpt->cumboxvol[bc-1];
		for(bc2=bc;bc2<cmpt->nbox;bc2++) {
			vol+=boxvol*cmpt->boxfrac[bc2];
			cmpt->cumboxvol[bc2]=vol; }
		cmpt->volume=vol;
		return 3; }

	if(cmpt->nbox==cmpt->maxbox) {									// expand box list
		max=cmpt->maxbox>0?cmpt->maxbox*2:1;
		CHECKMEM(newboxlist=(boxptr*)calloc(max,sizeof(boxptr)));
		CHECKMEM(newboxfrac=(double*)calloc(max,sizeof(double)));
		CHECKMEM(newcumboxvol=(double*)calloc(max,sizeof(double)));
		for(bc2=0;bc2<cmpt->nbox;bc2++) {
			newboxlist[bc2]=cmpt->boxlist[bc2];
			newboxfrac[bc2]=cmpt->boxfrac[bc2];
			newcumboxvol[bc2]=cmpt->cumboxvol[bc2]; }
		for(;bc2<max;bc2++) {
			newboxlist[bc2]=NULL;
			newboxfrac[bc2]=0;
			newcumboxvol[bc2]=0; }
		cmpt->maxbox=max;
		free(cmpt->boxlist);
		free(cmpt->boxfrac);
		free(cmpt->cumboxvol);
		cmpt->boxlist=newboxlist;
		cmpt->boxfrac=newboxfrac;
		cmpt->cumboxvol=newcumboxvol; }

	bc=cmpt->nbox++;								// put box into cmpt
	cmpt->boxlist[bc]=bptr;
	cmpt->boxfrac[bc]=volfrac2;
	cmpt->volume+=sim->boxs->boxvol*cmpt->boxfrac[bc];
	cmpt->cumboxvol[bc]=cmpt->volume;
	return 1;

 failure:
 	free(newboxlist);
 	free(newboxfrac);
 	free(newcumboxvol);
	simLog(sim,10,"%s","Failed to allocate memory in compartupdatebox_volumeSample");
 	return -1; }
#endif


/* compartreadstring */
compartptr compartreadstring(simptr sim,ParseFilePtr pfp,compartptr cmpt,const char *word,char *line2) {
	char nm[STRCHAR],nm1[STRCHAR];
	int s,er,cl,dim,itct;
	double v1[DIMMAX];
	enum CmptLogic sym;
	int nvar;
	char **varnames;
	double *varvalues;

	dim=sim->dim;
	nvar=sim->nvar;
	varnames=sim->varnames;
	varvalues=sim->varvalues;

	if(!strcmp(word,"name")) {								// name, got[0]
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading compartment name");
		cmpt=compartaddcompart(sim,nm);
		CHECKS(cmpt,"failed to add compartment");
		CHECKS(!strnword(line2,2),"unexpected text following name"); }

	else if(!strcmp(word,"surface")) {						// surface
		CHECKS(cmpt,"name has to be entered before surface");
		CHECKS(sim->srfss,"surfaces need to be entered before compartment surfaces");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"error reading surface name");
		s=stringfind(sim->srfss->snames,sim->srfss->nsrf,nm);
		CHECKS(s>=0,"surface name '%s' not recognized",nm);
		er=compartaddsurf(cmpt,sim->srfss->srflist[s]);
		CHECKS(er!=1,"out of memory adding surface to compartment");
		CHECKS(er!=2,"cannot add surface to compartment more than once");
		CHECKS(!strnword(line2,2),"unexpected text following surface"); }

	else if(!strcmp(word,"point")) {							// point
		CHECKS(cmpt,"name has to be entered before point");
		if(dim==1) itct=strmathsscanf(line2,"%mlg",varnames,varvalues,nvar,&v1[0]);
		else if(dim==2) itct=strmathsscanf(line2,"%mlg %mlg",varnames,varvalues,nvar,&v1[0],&v1[1]);
		else itct=strmathsscanf(line2,"%mlg %mlg %mlg",varnames,varvalues,nvar,&v1[0],&v1[1],&v1[2]);
		CHECKS(itct==dim,"unable to read all point values");
		er=compartaddpoint(cmpt,dim,v1);
		CHECKS(!er,"out of memory adding point to compartment");
		CHECKS(!strnword(line2,dim+1),"unexpected text following point"); }

	else if(!strcmp(word,"compartment")) {				// compartment
		CHECKS(cmpt,"name has to be entered before compartment");
		itct=sscanf(line2,"%s %s",nm1,nm);
		CHECKS(itct==2,"compartment format: symbol name");
		sym=compartstring2cl(nm1);
		CHECKS(sym!=CLnone,"unrecognized logic symbol");
		cl=stringfind(sim->cmptss->cnames,sim->cmptss->ncmpt,nm);
		CHECKS(cl>=0,"cmpartment name not recognized");
		er=compartaddcmptl(cmpt,sim->cmptss->cmptlist[cl],sym);
		CHECKS(er!=1,"out of memory adding compartment to compartment");
		CHECKS(er!=2,"cannot a compartment to itself");
		CHECKS(!strnword(line2,3),"unexpected text following compartment"); }

	else {																				// unknown word
		CHECKS(0,"syntax error within compartment block: statement not recognized"); }

	return cmpt;

 failure:
	simParseError(sim,pfp);
	return NULL; }


/* loadcompart */
int loadcompart(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	compartptr cmpt;

	pfp=*pfpptr;
	done=0;
	cmpt=NULL;
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
		else if(!strcmp(word,"end_compartment")) {		// end_compartment
			CHECKS(!line2,"unexpected text following end_compartment");
			return 0; }
		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }
		else {
			cmpt=compartreadstring(sim,pfp,cmpt,word,line2);
			CHECK(cmpt); }}															// failed but error has already been reported

	CHECKS(0,"end of file encountered before end_compartment statement");	// end of file

 failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* compartsupdateparams */
int compartsupdateparams(simptr sim) {
#ifdef OPTION_VCELL
	return compartsupdateparams_volumeSample(sim);
#else
	return compartsupdateparams_original(sim);
#endif
}


/* compartsupdateparams */
int compartsupdateparams_original(simptr sim) {
	boxssptr boxs;
	boxptr bptr;
	compartssptr cmptss;
	compartptr cmpt;
	int b,c,s,p,inbox,er,cl;
	double pos[3];
	surfaceptr srf;
	enum CmptLogic clsym;

	cmptss=sim->cmptss;
	boxs=sim->boxs;
	if(!boxs || !boxs->nbox) return 2;

	for(c=0;c<cmptss->ncmpt;c++) {
		cmpt=cmptss->cmptlist[c];
		cmpt->nbox=0;

		for(b=0;b<boxs->nbox;b++) {											// find boxes that are in the compartment
			bptr=boxs->blist[b];
			inbox=0;
			for(p=0;p<bptr->npanel && !inbox;p++) {
				srf=bptr->panel[p]->srf;
				for(s=0;s<cmpt->nsrf && !inbox;s++)
					if(cmpt->surflist[s]==srf) inbox=1; }			// a compartment surface is in the box
			if(!inbox && cmpt->ncmptl==0) {
				boxrandpos(sim,pos,bptr);
				if(posincompart(sim,pos,cmpt,0)) inbox=2; }		// compartment contains whole box
			if(inbox) {
				er=compartupdatebox(sim,cmpt,bptr,inbox==2?1:-1);
				if(er==-1) return 1; }}

		for(cl=0;cl<cmpt->ncmptl;cl++) {								// still finding boxes that are in compartment
			clsym=cmpt->clsym[cl];
			if(clsym==CLequal || clsym==CLor || clsym==CLxor)
				for(b=0;b<cmpt->cmptl[cl]->nbox;b++) {
					bptr=cmpt->cmptl[cl]->boxlist[b];
					er=compartupdatebox(sim,cmpt,bptr,-2);
					if(er==-1) return 1; }
			else if(clsym==CLequalnot || clsym==CLornot)
				for(b=0;b<boxs->nbox;b++) {
					bptr=boxs->blist[b];
					er=compartupdatebox(sim,cmpt,bptr,-2); }}}

	return 0; }


#ifdef OPTION_VCELL
/* compartsupdateparams_volumeSample */
int compartsupdateparams_volumeSample(simptr sim) {					//VCELL
	//indecies
	int b,c,d,i,j,k,cl;
	//varibles used to check possible boxes and its volFrac in a specific compartment
	double boxLow[3], boxHigh[3], sampleLow[3], sampleHigh[3];

	int dim = sim->dim;
	double* min = sim->boxs->min; // get origin in the space
	double* size = sim->boxs->size; //get box x,y,z size
	VolumeSamples* volumeSample = sim->volumeSamplesPtr;
	compartssptr cmptss = sim->cmptss;
	boxssptr boxs = sim->boxs;
	if(!boxs || !boxs->nbox) return 2;

	double sampleDz = 1;
	if(dim > 2){
		sampleDz = volumeSample->size[2]/volumeSample->num[2];
	}
	double sampleDy = 1;
	if(dim > 1) {
		sampleDy = volumeSample->size[1]/volumeSample->num[1];
	}
	double sampleDx = volumeSample->size[0]/volumeSample->num[0];

	int inbox,p,s;
	
	surfaceptr srf;
	double pos[DIMMAX];
	for(c=0;c<cmptss->ncmpt;c++) {
		compartptr cmpt=cmptss->cmptlist[c];
		cmpt->nbox=0;
		unsigned char cmptID = getCompartmentID(cmpt->cname, sim->volumeSamplesPtr);
		
		for(b=0;b<boxs->nbox;b++) {											// find boxes that are in the compartment
			boxptr bptr=boxs->blist[b];
			
			inbox=0;
			for(p=0;p<bptr->npanel && !inbox;p++) {
				srf=bptr->panel[p]->srf;
				for(s=0;s<cmpt->nsrf && !inbox;s++){
					if(cmpt->surflist[s]==srf){
						inbox=1;
						for(i=0;i<100;i++) {
							boxrandpos(sim,pos,bptr);
						}
					}
				}
			}
			if(!inbox && cmpt->ncmptl==0) {
				boxrandpos(sim,pos,bptr);
				if(posincompart(sim,pos,cmpt,0)) inbox=2; }

			//finding box low point and high point's indexes in volume sample values.
			for(d=0;d<dim;d++){
				boxLow[d] = min[d]+size[d]*bptr->indx[d];
				boxHigh[d] = min[d]+size[d]*(bptr->indx[d]+1);
			}
			
			//initialize varibles used for each box
			double insideCmptVol = 0;

			for(k = 0; k < volumeSample->num[2]; k++)
			{
				sampleLow[2] = volumeSample->origin[2]+ k*sampleDz;
				sampleHigh[2] = volumeSample->origin[2]+ (k+1)*sampleDz;
				double intersectZ = std::min<double>(boxHigh[2],sampleHigh[2]) - std::max<double>(boxLow[2], sampleLow[2]);
				if(intersectZ<=0)
				{
					continue;
				}
				for(j = 0; j < volumeSample->num[1]; j++)
				{
					sampleLow[1] = volumeSample->origin[1]+ j*sampleDy;
					sampleHigh[1] = volumeSample->origin[1]+ (j+1)*sampleDy;
					double intersectY = std::min<double>(boxHigh[1],sampleHigh[1]) - std::max<double>(boxLow[1], sampleLow[1]);
					if(intersectY<=0)
					{
						continue;
					}
					for(i = 0; i < volumeSample->num[0]; i++)
					{
						sampleLow[0] = volumeSample->origin[0]+ i*sampleDx;
						sampleHigh[0] = volumeSample->origin[0]+ (i+1)*sampleDx;
						double intersectX = std::min<double>(boxHigh[0],sampleHigh[0]) - std::max<double>(boxLow[0], sampleLow[0]);
						if(intersectX<=0)
						{
							continue;
						}
						//find intersect volume
						int sampleIdx = i + j*volumeSample->num[0] + k*volumeSample->num[0]*volumeSample->num[1];
						if(volumeSample->volsamples[sampleIdx] == cmptID)
						{
							insideCmptVol += intersectX*intersectY*intersectZ;
						}
					}
				}
			}
			double boxVolfrac = insideCmptVol/((boxs->size[0])*(boxs->size[1])*(boxs->size[2]));
			if(boxVolfrac <= 1e-8) boxVolfrac = 0; // volume fraction is too small, consider it as 0
			if((boxVolfrac + 1e-8) >= 1) boxVolfrac = 1; //if volume fraction is almost 1, consider it as 1 
			if(boxVolfrac > 0)
			{
				int errorCode = compartupdatebox_volumeSample(sim,cmpt,bptr,boxVolfrac);
				if(errorCode==-1) return 1;
			}
		}
		enum CmptLogic clsym;
		for(cl=0;cl<cmpt->ncmptl;cl++) {								// still finding boxes that are in compartment
			clsym=cmpt->clsym[cl];
			if(clsym==CLequal || clsym==CLor || clsym==CLxor)
				for(b=0;b<cmpt->cmptl[cl]->nbox;b++) {
					boxptr bptr=cmpt->cmptl[cl]->boxlist[b];
					int er=compartupdatebox_volumeSample(sim,cmpt,bptr,-2);
					if(er==-1) return 1; }
			else if(clsym==CLequalnot || CLornot)
				for(b=0;b<boxs->nbox;b++) {
					boxptr bptr=boxs->blist[b];
					int er=compartupdatebox_volumeSample(sim,cmpt,bptr,-2);
					if(er==-1) return 1; }
		}
	}
	//for(c=0;c<cmptss->ncmpt;c++) {
	//	compartptr cmpt=cmptss->cmptlist[c];
	//	using namespace std;
	//	//write to a file the box volume for each compartment
	//	stringstream ss;
	//	ss << "d:\\volumeCmpt" << cmpt->cname << ".txt";
	//	ofstream filestr;
	//	filestr.open (ss.str());
	//	boxptr* boxlist = cmpt->boxlist;
	//	double* boxVol = cmpt->boxfrac;
	//	for(int i=0; i<cmpt->nbox; i++)
	//	{
	//		for(b=0;b<sim->boxs->nbox;b++) {
	//			boxptr bptr=boxlist[b];
	//			if (boxlist[i] == sim->boxs->blist[b]) {
	//				filestr << "box index:" << b << "     " << "volFrac:" << boxVol[i] << ",nneigh=" << bptr->nneigh <<", midneigh="<<bptr->midneigh<<", nwall=" <<bptr->nwall <<",maxpanel="<<bptr->maxpanel<<",npanel="<< bptr->npanel << endl;
	//				break;
	//			}
	//		}
	//	}
	//	filestr.close();
	//}

	return 0; 
}
#endif


/* compartsupdatelists */
int compartsupdatelists(simptr sim) {
	return 0; }


/* compartsupdate */
int compartsupdate(simptr sim) {
	int er;
	compartssptr cmptss;

	cmptss=sim->cmptss;
	if(cmptss) {
		if(cmptss->condition<=SClists) {
			er=compartsupdatelists(sim);
			if(er) return er;
			compartsetcondition(cmptss,SCparams,1); }
		if(cmptss->condition==SCparams) {
			er=compartsupdateparams(sim);
			if(er) return er;
			compartsetcondition(cmptss,SCok,1); }}
	return 0; }


/******************************************************************************/
/************************* core simulation functions **************************/
/******************************************************************************/


void comparttranslate(simptr sim,compartptr cmpt,int code,double *translate) {
	int s,dim,ll,m,d,pt,p,lxp;
	surfaceptr srf;
	moleculeptr mptr;
	molssptr mols;
	double newpos[DIMMAX],crsspt[DIMMAX],cross,epsilon;
	enum PanelShape ps;
	panelptr pnl;
	enum PanelFace face1,face2;
	enum MolecState ms;

	dim=sim->dim;
	mols=sim->mols;
	epsilon=sim->srfss->epsilon;

	if(code&1) {	// translate surfaces and interior-defining points
		for(s=0;s<cmpt->nsrf;s++) {
			srf=cmpt->surflist[s];
			surfupdateoldpos(srf,dim);
			surftranslatesurf(srf,dim,translate); }
		for(pt=0;pt<cmpt->npts;pt++)
			for(d=0;d<dim;d++)
				cmpt->points[pt][d]+=translate[d]; }

	if(code&2) {	// translate surface-bound molecules
		for(s=0;s<cmpt->nsrf;s++) {
			srf=cmpt->surflist[s];
			for(ll=0;ll<srf->nmollist;ll++) {
				for(m=0;m<srf->nmol[ll];m++) {
					mptr=srf->mol[ll][m];
					if(mptr->ident) {
						for(d=0;d<dim;d++)
							mptr->pos[d]+=translate[d];
						ms=mptr->mstate;
						if(ms==MSfront) face1=PFfront;
						else if(ms==MSback) face1=PFback;
						else face1=PFnone;
						fixpt2panel(mptr->pos,mptr->pnl,dim,face1,epsilon); }}}}}

	if(code&4 || code&8) {	// translate molecules
		for(ll=0;ll<mols->nlist;ll++)
			for(m=0;m<mols->nl[ll];m++) {
				mptr=mols->live[ll][m];
				if(mptr->ident && mptr->mstate==MSsoln) {
					if(posincompart(sim,mptr->pos,cmpt,1)) {		// translate molecules within compartment
						if(code&4)
							for(d=0;d<dim;d++)
								mptr->pos[d]+=translate[d]; }
					else if(code&8) {														// translate external molecules
						for(d=0;d<dim;d++)
							newpos[d]=mptr->pos[d]-translate[d];		// in frame of reference of moving surface
						for(s=0;s<cmpt->nsrf;s++) {
							srf=cmpt->surflist[s];
							if(srf->action[mptr->ident][MSsoln][PFfront]!=SAtrans || srf->action[mptr->ident][MSsoln][PFback]!=SAtrans) {
								for(ps=(enum PanelShape)0;ps<PSMAX;ps=(enum PanelShape)(ps+1))
									for(p=0;p<srf->npanel[ps];p++) {
										pnl=srf->panels[ps][p];
										lxp=lineXpanel(mptr->pos,newpos,pnl,dim,crsspt,&face1,&face2,&cross,NULL,NULL,1);
										if(lxp && srf->action[mptr->ident][MSsoln][face1]!=SAtrans) {
											for(d=0;d<dim;d++) {
												mptr->via[d]=mptr->pos[d];
												mptr->pos[d]+=translate[d]; }
											checksurfaces1mol(sim,mptr,1.0-cross);
											}}}}}}}}
	sim->mols->touch++;
	return; }


