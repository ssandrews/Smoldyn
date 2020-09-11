/* Steven Andrews, 2/05.
See documentation called Sphere_doc.doc.
Functions for coordinate rotation and manipulation.
Copyright 2005-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include "random2.h"
#include "Sphere.h"

#define PI 3.14159265358979323846
#define EPSILON 100*DBL_EPSILON

double Work[9],Work2[9];


void Sph_Cart2Sc(double *Cart,double *Sc) {
	Work[0]=sqrt(Cart[0]*Cart[0]+Cart[1]*Cart[1]+Cart[2]*Cart[2]);
	Work[1]=Work[0]>0?acos(Cart[2]/Work[0]):0;
	Work[2]=atan2(Cart[1],Cart[0]);
	if(Work[2]<0) Work[2]+=2.0*PI;
	Sc[0]=Work[0];
	Sc[1]=Work[1];
	Sc[2]=Work[2];
	return; }


void Sph_Sc2Cart(double *Sc,double *Cart) {
	Work[0]=Sc[0]*sin(Sc[1])*cos(Sc[2]);
	Work[1]=Sc[0]*sin(Sc[1])*sin(Sc[2]);
	Work[2]=Sc[0]*cos(Sc[1]);
	Cart[0]=Work[0];
	Cart[1]=Work[1];
	Cart[2]=Work[2];
	return; }


void Sph_Eay2Ep(double *Eay,double *Ep) {
	Work[0]=cos(0.5*(Eay[3]+Eay[2]))*cos(0.5*Eay[1]);
	Work[1]=sin(0.5*(Eay[3]-Eay[2]))*sin(0.5*Eay[1]);
	Work[2]=cos(0.5*(Eay[3]-Eay[2]))*sin(0.5*Eay[1]);
	Work[3]=sin(0.5*(Eay[3]+Eay[2]))*cos(0.5*Eay[1]);
	Ep[0]=Work[0];
	Ep[1]=Work[1];
	Ep[2]=Work[2];
	Ep[3]=Work[3];
	return; }


void Sph_Xyz2Xyz(double *Xyz1,double *Xyz2) {
	Xyz2[0]=Xyz1[0];
	Xyz2[1]=Xyz1[1];
	Xyz2[2]=Xyz1[2];
	while(Xyz2[0]>PI) Xyz2[0]-=2.0*PI;
	while(Xyz2[0]<-PI) Xyz2[0]+=2.0*PI;
	while(Xyz2[1]>PI) Xyz2[1]-=2.0*PI;
	while(Xyz2[1]<-PI) Xyz2[1]+=2.0*PI;
	while(Xyz2[2]>PI) Xyz2[2]-=2.0*PI;
	while(Xyz2[2]<-PI) Xyz2[2]+=2.0*PI;
	return; }


void Sph_Eax2Dcm(double *Eax,double *Dcm) {
	double cf,cq,cy,sf,sq,sy;

	cq=cos(Eax[0]);
	cf=cos(Eax[1]);
	cy=cos(Eax[2]);
	sq=sin(Eax[0]);
	sf=sin(Eax[1]);
	sy=sin(Eax[2]);
	Dcm[0]=cy*cf-cq*sf*sy;
	Dcm[1]=cy*sf+cq*cf*sy;
	Dcm[2]=sy*sq;
	Dcm[3]=-sy*cf-cq*sf*cy;
	Dcm[4]=-sy*sf+cq*cf*cy;
	Dcm[5]=cy*sq;
	Dcm[6]=sq*sf;
	Dcm[7]=-sq*cf;
	Dcm[8]=cq;
	return; }


void Sph_Eay2Dcm(double *Eay,double *Dcm) {
	double cf,cq,cy,sf,sq,sy;

	cq=cos(Eay[0]);
	cf=cos(Eay[1]);
	cy=cos(Eay[2]);
	sq=sin(Eay[0]);
	sf=sin(Eay[1]);
	sy=sin(Eay[2]);
	Dcm[0]=-sy*sf+cq*cf*cy;
	Dcm[1]=sy*cf+cq*sf*cy;
	Dcm[2]=-cy*sq;
	Dcm[3]=-cy*sf-cq*cf*sy;
	Dcm[4]=cy*cf-cq*sf*sy;
	Dcm[5]=sy*sq;
	Dcm[6]=sq*cf;
	Dcm[7]=sq*sf;
	Dcm[8]=cq;
	return; }


void Sph_Xyz2Dcm(double *Xyz,double *Dcm) {
	double cf,cq,cy,sf,sq,sy;
	
	cf=cos(Xyz[0]);
	cq=cos(Xyz[1]);
	cy=cos(Xyz[2]);
	sf=sin(Xyz[0]);
	sq=sin(Xyz[1]);
	sy=sin(Xyz[2]);
	Dcm[0]=cq*cf;
	Dcm[1]=cq*sf;
	Dcm[2]=-sq;
	Dcm[3]=sy*sq*cf-cy*sf;
	Dcm[4]=sy*sq*sf+cy*cf;
	Dcm[5]=cq*sy;
	Dcm[6]=cy*sq*cf+sy*sf;
	Dcm[7]=cy*sq*sf-sy*cf;
	Dcm[8]=cq*cy;
	return; }


void Sph_Xyz2Dcmt(double *Xyz,double *Dcmt) {
	double cf,cq,cy,sf,sq,sy;
	
	cf=cos(Xyz[0]);
	cq=cos(Xyz[1]);
	cy=cos(Xyz[2]);
	sf=sin(Xyz[0]);
	sq=sin(Xyz[1]);
	sy=sin(Xyz[2]);
	Dcmt[0]=cq*cf;
	Dcmt[3]=cq*sf;
	Dcmt[6]=-sq;
	Dcmt[1]=sy*sq*cf-cy*sf;
	Dcmt[4]=sy*sq*sf+cy*cf;
	Dcmt[7]=cq*sy;
	Dcmt[2]=cy*sq*cf+sy*sf;
	Dcmt[5]=cy*sq*sf-sy*cf;
	Dcmt[8]=cq*cy;
	return; }


void Sph_Dcm2Xyz(double *Dcm,double *Xyz) {
	Work[0]=atan2(Dcm[1],Dcm[0]);
	Work[1]=asin(-Dcm[2]);
	Work[2]=atan2(Dcm[5],Dcm[8]);
	Xyz[0]=Work[0];
	Xyz[1]=Work[1];
	Xyz[2]=Work[2];
	return; }


void Sph_Dcm2Dcm(double *Dcm1,double *Dcm2) {
	Dcm2[0]=Dcm1[0];
	Dcm2[1]=Dcm1[1];
	Dcm2[2]=Dcm1[2];
	Dcm2[3]=Dcm1[3];
	Dcm2[4]=Dcm1[4];
	Dcm2[5]=Dcm1[5];
	Dcm2[6]=Dcm1[6];
	Dcm2[7]=Dcm1[7];
	Dcm2[8]=Dcm1[8];
	return; }


void Sph_Dcm2Dcmt(double *Dcm1,double *Dcm2) {
	Dcm2[0]=Dcm1[0];
	Dcm2[4]=Dcm1[4];
	Dcm2[8]=Dcm1[8];
	Work[1]=Dcm1[1];
	Work[2]=Dcm1[2];
	Work[3]=Dcm1[3];
	Work[5]=Dcm1[5];
	Work[6]=Dcm1[6];
	Work[7]=Dcm1[7];
	Dcm2[1]=Work[3];
	Dcm2[2]=Work[6];
	Dcm2[3]=Work[1];
	Dcm2[5]=Work[7];
	Dcm2[6]=Work[2];
	Dcm2[7]=Work[5];
	return; }


void Sph_DcmxDcm(double *Dcm1,double *Dcm2,double *Dcm3) {
	Work[0]=Dcm1[0]*Dcm2[0]+Dcm1[1]*Dcm2[3]+Dcm1[2]*Dcm2[6];
	Work[1]=Dcm1[0]*Dcm2[1]+Dcm1[1]*Dcm2[4]+Dcm1[2]*Dcm2[7];
	Work[2]=Dcm1[0]*Dcm2[2]+Dcm1[1]*Dcm2[5]+Dcm1[2]*Dcm2[8];
	Work[3]=Dcm1[3]*Dcm2[0]+Dcm1[4]*Dcm2[3]+Dcm1[5]*Dcm2[6];
	Work[4]=Dcm1[3]*Dcm2[1]+Dcm1[4]*Dcm2[4]+Dcm1[5]*Dcm2[7];
	Work[5]=Dcm1[3]*Dcm2[2]+Dcm1[4]*Dcm2[5]+Dcm1[5]*Dcm2[8];
	Work[6]=Dcm1[6]*Dcm2[0]+Dcm1[7]*Dcm2[3]+Dcm1[8]*Dcm2[6];
	Work[7]=Dcm1[6]*Dcm2[1]+Dcm1[7]*Dcm2[4]+Dcm1[8]*Dcm2[7];
	Work[8]=Dcm1[6]*Dcm2[2]+Dcm1[7]*Dcm2[5]+Dcm1[8]*Dcm2[8];
	Dcm3[0]=Work[0];
	Dcm3[1]=Work[1];
	Dcm3[2]=Work[2];
	Dcm3[3]=Work[3];
	Dcm3[4]=Work[4];
	Dcm3[5]=Work[5];
	Dcm3[6]=Work[6];
	Dcm3[7]=Work[7];
	Dcm3[8]=Work[8];
	return; }


void Sph_DcmxDcmt(double *Dcm1,double *Dcmt,double *Dcm3) {
	Work[0]=Dcm1[0]*Dcmt[0]+Dcm1[1]*Dcmt[1]+Dcm1[2]*Dcmt[2];
	Work[1]=Dcm1[0]*Dcmt[3]+Dcm1[1]*Dcmt[4]+Dcm1[2]*Dcmt[5];
	Work[2]=Dcm1[0]*Dcmt[6]+Dcm1[1]*Dcmt[7]+Dcm1[2]*Dcmt[8];
	Work[3]=Dcm1[3]*Dcmt[0]+Dcm1[4]*Dcmt[1]+Dcm1[5]*Dcmt[2];
	Work[4]=Dcm1[3]*Dcmt[3]+Dcm1[4]*Dcmt[4]+Dcm1[5]*Dcmt[5];
	Work[5]=Dcm1[3]*Dcmt[6]+Dcm1[4]*Dcmt[7]+Dcm1[5]*Dcmt[8];
	Work[6]=Dcm1[6]*Dcmt[0]+Dcm1[7]*Dcmt[1]+Dcm1[8]*Dcmt[2];
	Work[7]=Dcm1[6]*Dcmt[3]+Dcm1[7]*Dcmt[4]+Dcm1[8]*Dcmt[5];
	Work[8]=Dcm1[6]*Dcmt[6]+Dcm1[7]*Dcmt[7]+Dcm1[8]*Dcmt[8];
	Dcm3[0]=Work[0];
	Dcm3[1]=Work[1];
	Dcm3[2]=Work[2];
	Dcm3[3]=Work[3];
	Dcm3[4]=Work[4];
	Dcm3[5]=Work[5];
	Dcm3[6]=Work[6];
	Dcm3[7]=Work[7];
	Dcm3[8]=Work[8];
	return; }


void Sph_DcmtxDcm(double *Dcmt,double *Dcm2,double *Dcm3) {
	Work[0]=Dcmt[0]*Dcm2[0]+Dcmt[3]*Dcm2[3]+Dcmt[6]*Dcm2[6];
	Work[1]=Dcmt[0]*Dcm2[1]+Dcmt[3]*Dcm2[4]+Dcmt[6]*Dcm2[7];
	Work[2]=Dcmt[0]*Dcm2[2]+Dcmt[3]*Dcm2[5]+Dcmt[6]*Dcm2[8];
	Work[3]=Dcmt[1]*Dcm2[0]+Dcmt[4]*Dcm2[3]+Dcmt[7]*Dcm2[6];
	Work[4]=Dcmt[1]*Dcm2[1]+Dcmt[4]*Dcm2[4]+Dcmt[7]*Dcm2[7];
	Work[5]=Dcmt[1]*Dcm2[2]+Dcmt[4]*Dcm2[5]+Dcmt[7]*Dcm2[8];
	Work[6]=Dcmt[2]*Dcm2[0]+Dcmt[5]*Dcm2[3]+Dcmt[8]*Dcm2[6];
	Work[7]=Dcmt[2]*Dcm2[1]+Dcmt[5]*Dcm2[4]+Dcmt[8]*Dcm2[7];
	Work[8]=Dcmt[2]*Dcm2[2]+Dcmt[5]*Dcm2[5]+Dcmt[8]*Dcm2[8];
	Dcm3[0]=Work[0];
	Dcm3[1]=Work[1];
	Dcm3[2]=Work[2];
	Dcm3[3]=Work[3];
	Dcm3[4]=Work[4];
	Dcm3[5]=Work[5];
	Dcm3[6]=Work[6];
	Dcm3[7]=Work[7];
	Dcm3[8]=Work[8];
	return; }


void Sph_One2Dcm(double *Dcm) {
	Dcm[0]=Dcm[4]=Dcm[8]=1;
	Dcm[1]=Dcm[2]=Dcm[3]=Dcm[5]=Dcm[6]=Dcm[7]=0;
	return; }


void Sph_Xyz2Xyzr(double *Xyz,double *Xyzr) {
	double cf,cq,cy,sf,sq,sy;

	cf=cos(Xyz[0]);
	cq=cos(Xyz[1]);
	cy=cos(Xyz[2]);
	sf=sin(Xyz[0]);
	sq=sin(Xyz[1]);
	sy=sin(Xyz[2]);
	Xyzr[0]=atan2(sy*sq*cf-cy*sf,cq*cf);
	Xyzr[1]=-asin(-cy*sq*cf-sy*sf);
	Xyzr[2]=atan2(-cy*sq*sf+sy*cf,cq*cy);
	return; }


void Sph_Dcm2Dcmr(double *Dcm,double *Dcmr) {
	Dcmr[0]=-Dcm[0];
	Dcmr[1]=-Dcm[1];
	Dcmr[2]=-Dcm[2];
	Dcmr[3]=-Dcm[3];
	Dcmr[4]=-Dcm[4];
	Dcmr[5]=-Dcm[5];
	Dcmr[6]=Dcm[6];
	Dcmr[7]=Dcm[7];
	Dcmr[8]=Dcm[8];
	return; }


void Sph_Rot2Dcm(char axis,double angle,double *Dcm) {
	if(axis=='x'||axis=='X') {
		Dcm[0]=1;
		Dcm[1]=Dcm[2]=Dcm[3]=Dcm[6]=0;
		Dcm[4]=Dcm[8]=cos(angle);
		Dcm[7]=-(Dcm[5]=sin(angle)); }
	else if(axis=='y'||axis=='Y') {
		Dcm[4]=1;
		Dcm[1]=Dcm[3]=Dcm[5]=Dcm[7]=0;
		Dcm[0]=Dcm[8]=cos(angle);
		Dcm[2]=-(Dcm[6]=sin(angle)); }
	else if(axis=='z'||axis=='Z') {
		Dcm[8]=1;
		Dcm[2]=Dcm[5]=Dcm[6]=Dcm[7]=0;
		Dcm[0]=Dcm[4]=cos(angle);
		Dcm[3]=-(Dcm[1]=sin(angle)); }
	return; }


void Sph_Newz2Dcm(double *Newz,double psi,double *Dcm) {
	Sph_Cart2Sc(Newz,Work2);
	Work2[2]+=PI/2.0;
	Work2[3]=psi-Work2[2];
	Sph_Eax2Dcm(Work2+1,Dcm);
	Sph_Dcm2Dcmt(Dcm,Dcm);
	return; }


void Sph_DcmtxUnit(double *Dcmt,char axis,double *vect,double *add,double mult) {
	if(add) {
		Work[0]=add[0];
		Work[1]=add[1];
		Work[2]=add[2]; }
	if(axis=='x' || axis=='X') {
		vect[0]=mult*Dcmt[0];
		vect[1]=mult*Dcmt[1];
		vect[2]=mult*Dcmt[2]; }
	else if(axis=='y' || axis=='Y') {
		vect[0]=mult*Dcmt[3];
		vect[1]=mult*Dcmt[4];
		vect[2]=mult*Dcmt[5]; }
	else if(axis=='z' || axis=='Z') {
		vect[0]=mult*Dcmt[6];
		vect[1]=mult*Dcmt[7];
		vect[2]=mult*Dcmt[8]; }
	if(add) {
		vect[0]+=Work[0];
		vect[1]+=Work[1];
		vect[2]+=Work[2]; }
	return; }


double Sph_RotateVectWithNormals3D(double *pt1,double *pt2,double *newpt2,double *oldnorm,double *newnorm,int sign) {
	double costheta,ax,ay,az,sintheta,oldnormint[3],*oldnormptr;
	double costheta1,dcmxx,dcmxy,dcmxz,dcmyx,dcmyy,dcmyz,dcmzx,dcmzy,dcmzz,x,y,z;
	int sinsign;

	if(oldnorm) oldnormptr=oldnorm;
	else {
		oldnormint[0]=unirandCCD(-1,1);
		oldnormint[1]=unirandCCD(-1,1);
		oldnormint[2]=unirandCCD(-1,1);
		oldnormptr=oldnormint; }

	costheta=oldnormptr[0]*newnorm[0]+oldnormptr[1]*newnorm[1]+oldnormptr[2]*newnorm[2];
	if((sign==1 && costheta<0) || (sign==-1 && costheta>=0)) {
		costheta*=-1;
		sinsign=-1; }
	else
		sinsign=1;

	if(costheta>-1+EPSILON && costheta<1-EPSILON) {
		ax=oldnormptr[1]*newnorm[2]-oldnormptr[2]*newnorm[1];			// compute the rotation axis
		ay=oldnormptr[2]*newnorm[0]-oldnormptr[0]*newnorm[2];
		az=oldnormptr[0]*newnorm[1]-oldnormptr[1]*newnorm[0];
		sintheta=sqrt(ax*ax+ay*ay+az*az);
		if(sintheta<EPSILON) {
			if(costheta>0) goto parallel;					// oops, got wrong condition
			else goto antiparallel; }
		ax/=sintheta;
		ay/=sintheta;
		az/=sintheta;
		sintheta*=sinsign;

		costheta1=1-costheta;
		dcmxx=costheta+ax*ax*costheta1;
		dcmxy=ax*ay*costheta1-az*sintheta;
		dcmxz=ax*az*costheta1+ay*sintheta;
		dcmyx=ay*ax*costheta1+az*sintheta;
		dcmyy=costheta+ay*ay*costheta1;
		dcmyz=ay*az*costheta1-ax*sintheta;
		dcmzx=az*ax*costheta1-ay*sintheta;
		dcmzy=az*ay*costheta1+ax*sintheta;
		dcmzz=costheta+az*az*costheta1;
		x=pt2[0]-pt1[0];
		y=pt2[1]-pt1[1];
		z=pt2[2]-pt1[2];

		newpt2[0]=pt1[0]+dcmxx*x+dcmxy*y+dcmxz*z;
		newpt2[1]=pt1[1]+dcmyx*x+dcmyy*y+dcmyz*z;
		newpt2[2]=pt1[2]+dcmzx*x+dcmzy*y+dcmzz*z; }

	else if(costheta>0) {
		parallel:
		newpt2[0]=pt2[0];
		newpt2[1]=pt2[1];
		newpt2[2]=pt2[2]; }

	else {
		antiparallel:
		newpt2[0]=pt1[0]-(pt2[0]-pt1[0]);
		newpt2[1]=pt1[1]-(pt2[1]-pt1[1]);
		newpt2[2]=pt1[2]-(pt2[2]-pt1[2]); }

	return costheta; }


