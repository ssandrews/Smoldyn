/* Steve Andrews, 2/07; rewritten 11/08 */
/* Converter for wrl format triangulated data to Smoldyn format data. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define STRCHAR 256
#define CHECKS(A,B) if(!(A)) {strncpy(erstr,B,STRCHAR);goto failure;}

double **expandpoints(int nnew,int nold,double **points);
void pointsfree(int npts,double **points);
int **expandcoords(int nnew,int nold,int **coords);
void coordsfree(int ncds,int **coords);
int fliptriangles(int **coords,int *aligndone,int ncds,int *nflipptr);



double **expandpoints(int nnew,int nold,double **points) {
	int i,j;
	double **ptsnew;
	char erstr[STRCHAR];

	ptsnew=NULL;
	CHECKS(ptsnew=(double**)calloc(nnew,sizeof(double*)),"Error allocating points");
	for(i=0;i<nnew;i++) ptsnew[i]=NULL;
	for(i=0;i<nnew;i++)
		CHECKS(ptsnew[i]=(double*)calloc(3,sizeof(double)),"Error allocating points");
	for(i=0;i<nnew;i++)
		for(j=0;j<3;j++) ptsnew[i][j]=0;

	if(points) {
		for(i=0;i<nold&&i<nnew;i++)
			for(j=0;j<3;j++) ptsnew[i][j]=points[i][j];
		pointsfree(nold,points); }
	return ptsnew;

 failure:
	fprintf(stderr,"%s\n",erstr);
	pointsfree(nnew,ptsnew);
	pointsfree(nold,points);
	return NULL; }



void pointsfree(int npts,double **points) {
	int i;

	if(npts<=0||!points) return;
	for(i=0;i<npts;i++) free(points[i]);
	free(points);
	return; }



int **expandcoords(int nnew,int nold,int **coords) {
	int i,j;
	int **cdsnew;
	char erstr[STRCHAR];

	cdsnew=NULL;
	CHECKS(cdsnew=(int**)calloc(nnew,sizeof(int*)),"Error allocating coordinates");
	for(i=0;i<nnew;i++) cdsnew[i]=NULL;
	for(i=0;i<nnew;i++)
		CHECKS(cdsnew[i]=(int*)calloc(3,sizeof(int)),"Error allocating coordinates");
	for(i=0;i<nnew;i++)
		for(j=0;j<3;j++) cdsnew[i][j]=0;

	if(coords) {
		for(i=0;i<nold&&i<nnew;i++)
			for(j=0;j<3;j++) cdsnew[i][j]=coords[i][j];
		coordsfree(nold,coords); }
	return cdsnew;

 failure:
	fprintf(stderr,"%s\n",erstr);
	coordsfree(nnew,cdsnew);
	coordsfree(nold,coords);
	return NULL; }



void coordsfree(int ncds,int **coords) {
	int i;

	if(ncds<=0||!coords) return;
	for(i=0;i<ncds;i++) free(coords[i]);
	free(coords);
	return; }



int fliptriangles(int **coords,int *aligndone,int ncds,int *nflipptr) {
	int i,j,match,flip,nflip,done;
	double temp;

	nflip=0;
	done=0;
	for(i=0;i<ncds;i++) {													// i is triangle for alignment
		for(j=0;j<ncds && !aligndone[i];j++) {			// j is already aligned neighbor triangle
			if(aligndone[j] && j!=i) {
				match=0;
				if(coords[j][0]==coords[i][0] || coords[j][1]==coords[i][0] || coords[j][2]==coords[i][0]) match++;
				if(coords[j][0]==coords[i][1] || coords[j][1]==coords[i][1] || coords[j][2]==coords[i][1]) match++;
				if(coords[j][0]==coords[i][2] || coords[j][1]==coords[i][2] || coords[j][2]==coords[i][2]) match++;
				if(match==2) {
					if(coords[j][0]==coords[i][0] && (coords[j][1]==coords[i][1] || coords[j][2]==coords[i][2])) flip=1;
					else if(coords[j][0]==coords[i][1] && (coords[j][1]==coords[i][2] || coords[j][2]==coords[i][0])) flip=1;
					else if(coords[j][0]==coords[i][2] && (coords[j][1]==coords[i][0] || coords[j][2]==coords[i][1])) flip=1;
					else if(coords[j][1]==coords[i][0] && coords[j][2]==coords[i][1]) flip=1;
					else if(coords[j][1]==coords[i][1] && coords[j][2]==coords[i][2]) flip=1;
					else if(coords[j][1]==coords[i][2] && coords[j][2]==coords[i][0]) flip=1;
					else flip=0;
					if(flip) {
						nflip++;
						temp=coords[i][1];
						coords[i][1]=coords[i][2];
						coords[i][2]=temp; }
					aligndone[i]=1;
					done++; }}}}
	if(nflipptr) *nflipptr=nflip;
	return done; }



int main(void) {
	double **points,temp,trans[3],scale[3];
	int **coords;
	int itct,i,j,maxpts,maxcds,npts,ncds,nneigh,ci,more,trinum,neigh,match,neighcode,ipt,icd,d;
	char fnamein[STRCHAR],fnamewrite[STRCHAR],fnameout[STRCHAR],erstr[STRCHAR],line[STRCHAR],word[STRCHAR],triname[STRCHAR];
	FILE *fin,*fout;
	int join,njoinpts,njoincds;
	int align,*aligndone,done,nflip,newdone,flip;

	maxpts=0;
	maxcds=0;
	points=NULL;
	coords=NULL;
	fin=fout=NULL;

// ********* Load wrl files *********

	ipt=0;
	icd=0;
	npts=0;
	ncds=0;
	done=0;
	fnamewrite[0]='\0';
	while(!done) {
		printf("Enter name of input wrl file or 'done' if finished: ");				// get input file name
		itct=scanf("%s",fnamein);
		if(!strcmp(fnamein,"done")) done=1;
		else {
			CHECKS(itct==1 && (fin=fopen(fnamein,"r")),"File not found");
			strncat(fnamewrite," ",STRCHAR-strlen(fnamewrite)-1);
			strncat(fnamewrite,fnamein,STRCHAR-strlen(fnamewrite)-1);

			do {																						// skip useless portion of input file
				CHECKS(fgets(line,STRCHAR,fin)!=NULL,"No 'point' listed in wrl file");
				itct=sscanf(line,"%s",word); }
				while(!(itct==1&&strstr(word,"point")==word));

			more=1;																					// read points from input file
			while(more) {
				if(ipt==maxpts) {
					points=expandpoints(maxpts*2+2,maxpts,points);
					CHECKS(points,"Error allocating points");
					maxpts=maxpts*2+2; }
				CHECKS(fgets(line,STRCHAR,fin)!=NULL,"Unexpected end of file");
				itct=sscanf(line,"%s",word);
				if(itct==1&&word[0]==']') more=0;
				else {
					itct=sscanf(line,"%lf %lf %lf%s",&points[ipt][0],&points[ipt][1],&points[ipt][2],word);
					CHECKS(itct>=3,"Error reading points");
					ipt++;
					if(word[0]==']') more=0; }}
			printf("%i points read.\n",ipt-npts);

			do {																						// skip next useless portion of input file
				CHECKS(fgets(line,STRCHAR,fin)!=NULL,"No 'coordIndex' listed in wrl file");
				itct=sscanf(line,"%s",word); }
				while(!(itct==1&&strstr(word,"coordIndex")==word));

			more=1;																					// read coordinates from input file
			while(more) {
				if(icd==maxcds) {
					coords=expandcoords(maxcds*2+2,maxcds,coords);
					CHECKS(coords,"Error allocating coordinates");
					maxcds=maxcds*2+2; }
				CHECKS(fgets(line,STRCHAR,fin)!=NULL,"Unexpected end of file");
				itct=sscanf(line,"%s",word);
				if(itct==1 && word[0]==']') more=0;
				else {
					itct=sscanf(line,"%i, %i, %i, %i%s",&coords[icd][0],&coords[icd][1],&coords[icd][2],&j,word);
					if(itct==1) itct=sscanf(line,"%i %i %i %i%s",&coords[icd][0],&coords[icd][1],&coords[icd][2],&j,word);
					CHECKS(itct>=3,"Error reading coordinates");
					coords[icd][0]+=npts;
					coords[icd][1]+=npts;
					coords[icd][2]+=npts;
					icd++;
					if(word[0]==']') more=0; }}
			printf("%i coordinates read.\n",icd-ncds);

			npts=ipt;
			ncds=icd; }}

//********** join adjacent regions *************

	printf("Join any adjacent but currently disjoint triangles (y/n): ");
	itct=scanf("%s",word);
	CHECKS(itct==1,"Missing answer");
	if(word[0]=='y' || word[0]=='Y') join=1;
	else if(word[0]=='n' || word[0]=='N') join=0;
	else CHECKS(0,"Invalid answer");

	njoinpts=0;
	njoincds=0;
	if(join) {
		for(i=1;i<npts;i++)					// find any duplicate points, and set any coordinates to former
			for(j=0;j<i;j++)
				if(fabs(points[j][0]-points[i][0])<1e-8 && fabs(points[j][1]-points[i][1])<1e-8 && fabs(points[j][2]-points[i][2])<1e-8) {
					njoinpts++;
					for(icd=0;icd<ncds;icd++) {
						if(coords[icd][0]==j) {coords[icd][0]=i;njoincds++;}
						if(coords[icd][1]==j) {coords[icd][1]=i;njoincds++;}
						if(coords[icd][2]==j) {coords[icd][2]=i;njoincds++;} }}
		printf("%i points matched\n%i triangle coordinates joined\n",njoinpts,njoincds); }

//************ translation *****************

	printf("Enter surface translation vector ('0 0 0' for no translation): ");
	itct=scanf("%lf %lf %lf",&trans[0],&trans[1],&trans[2]);
	CHECKS(itct==3,"Failed to read translation vector");

	for(d=0;d<3;d++)
		if(trans[d]!=0)
			for(i=0;i<npts;i++)
				points[i][d]+=trans[d];


//************ scaling *****************

	printf("Enter surface scaling vector ('1 1 1' for no scaling): ");
	itct=scanf("%lf %lf %lf",&scale[0],&scale[1],&scale[2]);
	CHECKS(itct==3,"Failed to read scaling vector");

	for(d=0;d<3;d++)
		if(scale[d]!=1.0)
			for(i=0;i<npts;i++)
				points[i][d]*=scale[d];


//************ align triangles *****************

	printf("Align orientations of neighboring triangles (y/n): ");
	itct=scanf("%s",word);
	CHECKS(itct==1,"Missing answer");
	if(word[0]=='y' || word[0]=='Y') align=1;
	else if(word[0]=='n' || word[0]=='N') align=0;
	else CHECKS(0,"Invalid answer");

	if(align) {																			// align neighboring triangles if needed
		nflip=0;
		CHECKS(aligndone=(int*) calloc(ncds,sizeof(int)),"Unable to allocated memory");
		for(i=0;i<ncds;i++) aligndone[i]=0;
		done=0;
		while(done<ncds) {
			newdone=fliptriangles(coords,aligndone,ncds,&flip);
			done+=newdone;
			nflip+=flip;
			if(newdone==0) {
				for(i=0;aligndone[i]==1;i++);
				printf("flip triangle %i:",i);
				ci=coords[i][0];
				printf(" (%g %g %g)",points[ci][0],points[ci][1],points[ci][2]);
				ci=coords[i][1];
				printf(" (%g %g %g)",points[ci][0],points[ci][1],points[ci][2]);
				ci=coords[i][2];
				printf(" (%g %g %g)? ",points[ci][0],points[ci][1],points[ci][2]);
				scanf("%s",word);
				if(word[0]=='y' || word[0]=='Y') {
					nflip++;
					temp=coords[i][1];
					coords[i][1]=coords[i][2];
					coords[i][2]=temp; }
				aligndone[i]=1;
				done++; }}
		free(aligndone);
		printf("%i triangles flipped for alignment.\n",nflip); }


//************** output results ********************

	printf("Enter name for Smoldyn output file: ");
	itct=scanf("%s",fnameout);
	CHECKS(itct==1&&(fout=fopen(fnameout,"w")),"Output file cannot be created");

	fprintf(fout,"# Smoldyn surface data file automatically generated by wrl2smol\n");
	fprintf(fout,"# input file:%s\n",fnamewrite);
	fprintf(fout,"# output file: %s\n\n",fnameout);
	fprintf(fout,"max_panels tri %i\n\n",ncds);

	printf("Enter triangle name and starting number (e.g. tri 1): ");
	itct=scanf("%s %i",triname,&trinum);
	CHECKS(itct==2,"Didn't read triangle name or starting number");

	printf("Print (1) no neighbors, (2) edge neighbors, (3) all neighbors: ");
	itct=scanf("%i",&neighcode);
	CHECKS(itct==1&&neighcode>=1&&neighcode<=3,"Invalid neighbor code");

	for(i=0;i<ncds;i++) {														// print out triangle data
		fprintf(fout,"panel tri");
		for(j=0;j<3;j++) {
			ci=coords[i][j];
			fprintf(fout,"  %g %g %g",points[ci][0],points[ci][1],points[ci][2]); }
		fprintf(fout,"  %s%i\n",triname,trinum+i); }
	printf("%i triangles output.\n",ncds);

	nneigh=0;
	if(neighcode>1) {																// print out neighbor data
		for(i=0;i<ncds;i++) {
			neigh=0;
			for(j=0;j<ncds;j++)
				if(j!=i) {
					match=0;
					if(coords[j][0]==coords[i][0]||coords[j][1]==coords[i][0]||coords[j][2]==coords[i][0]) match++;
					if(coords[j][0]==coords[i][1]||coords[j][1]==coords[i][1]||coords[j][2]==coords[i][1]) match++;
					if(coords[j][0]==coords[i][2]||coords[j][1]==coords[i][2]||coords[j][2]==coords[i][2]) match++;
					if((neighcode==2&&match==2)||(neighcode==3&&match>0)) {
						if(!neigh) {
							neigh=1;
							fprintf(fout,"neighbors %s%i",triname,trinum+i); }
						fprintf(fout," %s%i",triname,trinum+j);
						nneigh++; }}
			if(neigh) fprintf(fout,"\n"); }
		printf("%i neighbor data output.\n",nneigh); }

	fprintf(fout,"\nend_file\n");

	return 0;

 failure:
	pointsfree(maxpts,points);
	coordsfree(maxcds,coords);
	if(fin) fclose(fin);
	if(fout) fclose(fout);
	printf("%s\n",erstr);
	return 0; }


