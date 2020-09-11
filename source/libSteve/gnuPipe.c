/* Steven Andrews, started 10/22/01
See documentation called gnuPipe_doc.doc
Copyright 2003-2017 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <stdio.h>
#include <string.h>


/************ File functions ****************/

/* gnuWindow */
FILE *gnuWindow() {
  FILE *gnu;

  gnu=popen("gnuplot","w");
  if(gnu) fprintf(gnu,"set key off\n");
  return gnu; }


/* gnuClose */
void gnuClose(FILE *gnu) {
  fclose(gnu);
  return; }


/********* Direct gnuplot access *************/


/* gnuCommand */
void gnuCommand(FILE *gnu,char *s) {
  fprintf(gnu,"%s\n",s);
  fflush(gnu);
  return; }


/* gnuPrompt */
void gnuPrompt(FILE *gnu) {
  char s[256];

  fprintf(stderr,"Press enter to continue program.\n");
  fprintf(stderr,"gnuPlot> ");
  fgets(s,256,stdin);
  while(strcmp(s,"\n")!=0) {
    fprintf(gnu,"%s",s);
    fflush(gnu);
    fprintf(stderr,"gnuPlot> ");
    fgets(s,256,stdin); }
  return; }


/*************** Setup functions *******************/


/* gnuSetScales */
void gnuSetScales(FILE *gnu,double xa,double xb,double ya,double yb) {
  if(xa==xb) fprintf(gnu,"set autoscale x\n");
  else fprintf(gnu,"set xrange[%g:%g]\n",xa,xb);
  if(ya==yb) fprintf(gnu,"set autoscale y\n");
  else fprintf(gnu,"set yrange[%g:%g]\n",ya,yb);
  fflush(gnu);
  return; }


/***************** Data plotting **************/


/* gnuData */
void gnuData(FILE *gnu,double *x,double *y,int rows,int cols) {
  int i,j;

  fprintf(gnu,"plot");
  for(j=0;j<cols;j++)
    fprintf(gnu," '-'%c",j==cols-1?'\n':',');

  for(j=0;j<cols;j++) {
    for(i=0;i<rows;i++)
      fprintf(gnu,"%g %g\n",x[i],y[cols*i+j]);
    fprintf(gnu,"e\n"); }
  fflush(gnu);
  return; }


/* gnuData2 */
void gnuData2(FILE *gnu,double *x,double *y,int *ct,int col) {
  int i,j;

  fprintf(gnu,"plot ");
  for(j=0;j<col-1;j++)
    fprintf(gnu,"'-' with points %i, ",j+1);
  fprintf(gnu,"'-' with points %i\n",j+1);
  for(j=0;j<col;j++) {
    for(i=0;i<ct[j];i++)
      fprintf(gnu,"%f %f\n",x[col*i+j],y[col*i+j]);
    fprintf(gnu,"e\n"); }
  fflush(gnu);
  return; }


