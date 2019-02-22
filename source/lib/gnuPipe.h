/* Steven Andrews, started 10/22/01
See documentation called gnuPipe_doc.doc
Copyright 2003-2017 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#ifndef __gnuPipe_h__
#define __gnuPipe_h__

// File functions
FILE *gnuWindow();
void gnuClose(FILE *gnu);

// Direct gnuplot access
void gnuCommand(FILE *gnu,char *s);
void gnuPrompt(FILE *gnu);

// Setup functions
void gnuSetScales(FILE *gnu,double xa,double xb,double ya,double yb);

// Data plotting
void gnuData(FILE *gnu,double *x,double *y,int m,int col);
void gnuData2(FILE *gnu,double *x,double *y,int *ct,int col);

#endif
