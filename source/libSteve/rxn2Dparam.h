/* File rxn2Dparam.h, written by Steven Andrews, 2017.
This code is in the public domain.  It is not copyrighted and may not be
copyrighted.

This is a header file for rxn2Dparam.c.  See rxnp2Dparam_doc.doc or rxn2Dparam_doc.pdf
for documentation. */

#ifndef __rxn2Dparam_h
#define __rxn2Dparam_h


double rxn2Dactrate(double step,double sigmab);

/************    FUNCTIONS FOR INVESTIGATING AN ABSORBING DISK    ***********/

double rdf2Dabsorbprob(double *r,double *rdf,int n,double prob);
void rdf2Ddiffuse(double *r,double *rdfa,double *rdfd,int n,double step);
void rdf2Dreverserxn(double *r,double *rdf,int n,double step,double b,double flux);
double rdf2Dsteadystate(double *r,double *rdfa,double *rdfd,int n,double step,double b,double eps);

#endif
