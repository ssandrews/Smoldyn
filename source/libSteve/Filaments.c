/* Steven Andrews, 11/4/26
Assorted functions for filaments
See documentation called Filaments.tex.
Copyright 2026 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <math.h>
#include <string.h>
#include "Filaments.h"


/***************** Statistics **************/

double Fil_Statistics(const char *model,const char *eqn,double value1,double value2,double value3,double value4) {
	double kbt,kphi,ktheta,kpsi,len,persist,contour,ans;

	ans=0;
	if(!strcmp(eqn,"Persist")) {								// persistence length
		kbt=value1;
		kphi=value2;
		ktheta=value3;
		len=value4;
		if(!strcmp(model,"ABC")) ans=2*len*ktheta*kphi/(kbt*(ktheta+kphi));
		else if(!strcmp(model,"WLC")) ans=2*ktheta*kphi/(kbt*(ktheta+kphi));
		else ans=NAN; }

	else if(!strcmp(eqn,"TorsionPersist")) {		// torsional persistence length
		kbt=value1;
		kpsi=value2;
		len=value3;
		if(!strcmp(model,"ABC")) ans=2*len*kpsi/kbt;
		else if(!strcmp(model,"WLC")) ans=2*kpsi/kbt;
		else ans=NAN; }

	else if(!strcmp(eqn,"kBend_Persist")) {					// bending force constant from persistence length
		kbt=value1;
		persist=value2;
		len=value3;
		if(!strcmp(model,"ABC")) ans=kbt*persist/len;
		else if(!strcmp(model,"WLC")) ans=kbt*persist;
		else ans=NAN; }

	else if(!strcmp(eqn,"kTwist_Persist")) {				// twisting force constant
		kbt=value1;
		persist=value2;
		len=value3;
		if(!strcmp(model,"ABC")) ans=kbt*persist/(2*len);
		else if(!strcmp(model,"WLC")) ans=kbt*persist/2;
		else ans=NAN; }

	else if(!strcmp(eqn,"Kuhn")) {								// Kuhn length (effective segment length for FJC model)
		len=value1;
		persist=value2;
		if(!strcmp(model,"FJC")) ans=len;
		else if(!strcmp(model,"GC")) ans=len;
		else if(!strcmp(model,"ABC")) ans=len/tanh(len/(2*persist));
		else if(!strcmp(model,"WLC")) ans=2*persist;
		else ans=NAN; }

	else if(!strcmp(eqn,"RmsEnd2End")) {						// Rms end-to-end length
		len=value1;
		contour=value2;
		persist=value3;
		if(!strcmp(model,"FJC")) ans=sqrt(len*contour);
		else if(!strcmp(model,"GC")) ans=sqrt(len*contour);
		else if(!strcmp(model,"ABC")) ans=sqrt((len*contour*sinh(len/persist)-len*len*(1-exp(-contour/persist)))/(cosh(len/persist)-1));
		else if(!strcmp(model,"WLC")) ans=sqrt(2*contour*persist-2*persist*persist*(1-exp(-contour/persist)));
		else ans=NAN; }

	return ans;	}



