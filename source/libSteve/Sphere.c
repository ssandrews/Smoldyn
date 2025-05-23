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


/* *** Copied over from Rn.c to avoid a dependency *** */

void Sph_crossVVD(const double *a,const double *b,double *c) {
	c[0]=a[1]*b[2]-a[2]*b[1];
	c[1]=a[2]*b[0]-a[0]*b[2];
	c[2]=a[0]*b[1]-a[1]*b[0];
	return; }


/* *** Start of Sphere.c library functions *** */

void Sph_Cart2Sc(const double *Cart,double *Sc) {
	double Work[3];

	Work[0]=sqrt(Cart[0]*Cart[0]+Cart[1]*Cart[1]+Cart[2]*Cart[2]);
	if(Work[0]>0) {
		Work[1]=acos(Cart[2]/Work[0]);
		Work[2]=atan2(Cart[1],Cart[0]); }
	else
		Work[1]=Work[2]=0;
	Sc[0]=Work[0];
	Sc[1]=Work[1];
	Sc[2]=Work[2];
	return; }


void Sph_Sc2Cart(const double *Sc,double *Cart) {
	double Work[3];

	Work[0]=Sc[0]*sin(Sc[1])*cos(Sc[2]);
	Work[1]=Sc[0]*sin(Sc[1])*sin(Sc[2]);
	Work[2]=Sc[0]*cos(Sc[1]);
	Cart[0]=Work[0];
	Cart[1]=Work[1];
	Cart[2]=Work[2];
	return; }


void Sph_Cart2Cart(const double *Cart1,double *Cart2) {
	Cart2[0]=Cart1[0];
	Cart2[1]=Cart1[1];
	Cart2[2]=Cart1[2];
	return; }


void Sph_Ypr2Ypr(const double *Ypr1,double *Ypr2) {
	Ypr2[0]=Ypr1[0];
	Ypr2[1]=Ypr1[1];
	Ypr2[2]=Ypr1[2];
	return; }


void Sph_Eax2Ypr(const double *Eax,double *Ypr) {
	double cf,cq,cy,sf,sq,sy;

	cq=cos(Eax[0]);
	cf=cos(Eax[1]);
	cy=cos(Eax[2]);
	sq=sin(Eax[0]);
	sf=sin(Eax[1]);
	sy=sin(Eax[2]);
	Ypr[0]=atan2(cy*sf+cq*cf*sy,cy*cf-cq*sf*sy);
	Ypr[1]=asin(-sy*sq);
	Ypr[2]=atan2(cy*sq,cq);
	return;	}


void Sph_Eax2Dcm(const double *Eax,double *Dcm) {
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


void Sph_Eay2Dcm(const double *Eay,double *Dcm) {
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


void Sph_Ypr2Dcm(const double *Ypr,double *Dcm) {
	double cf,cq,cy,sf,sq,sy;
	
	cf=cos(Ypr[0]);
	cq=cos(Ypr[1]);
	cy=cos(Ypr[2]);
	sf=sin(Ypr[0]);
	sq=sin(Ypr[1]);
	sy=sin(Ypr[2]);
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


void Sph_Ypr2Dcmt(const double *Ypr,double *Dcmt) {
	double cf,cq,cy,sf,sq,sy;
	
	cf=cos(Ypr[0]);
	cq=cos(Ypr[1]);
	cy=cos(Ypr[2]);
	sf=sin(Ypr[0]);
	sq=sin(Ypr[1]);
	sy=sin(Ypr[2]);
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


void Sph_Dcm2Ypr(const double *Dcm,double *Ypr) {
	double Work[3];

	Work[0]=atan2(Dcm[1],Dcm[0]);
	Work[1]=asin(-Dcm[2]);
	Work[2]=atan2(Dcm[5],Dcm[8]);
	Ypr[0]=Work[0];
	Ypr[1]=Work[1];
	Ypr[2]=Work[2];
	return; }


void Sph_Dcm2Dcm(const double *Dcm1,double *Dcm2) {
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


void Sph_Dcm2Dcmt(const double *Dcm1,double *Dcm2) {
	double Work[9];

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


void Sph_DcmxDcm(const double *Dcm1,const double *Dcm2,double *Dcm3) {
	double Work[9];

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


void Sph_DcmxDcmt(const double *Dcm1,const double *Dcmt,double *Dcm3) {
	double Work[9];

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


void Sph_DcmtxDcm(const double *Dcmt,const double *Dcm2,double *Dcm3) {
	double Work[9];

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


void Sph_DcmxCart(const double *Dcm,const double *Cart,double *Cart2) {
	double Work[3];

	Work[0]=Dcm[0]*Cart[0]+Dcm[1]*Cart[1]+Dcm[2]*Cart[2];
	Work[1]=Dcm[3]*Cart[0]+Dcm[4]*Cart[1]+Dcm[5]*Cart[2];
	Work[2]=Dcm[6]*Cart[0]+Dcm[7]*Cart[1]+Dcm[8]*Cart[2];
	Cart2[0]=Work[0];
	Cart2[1]=Work[1];
	Cart2[2]=Work[2];
	return; }


void Sph_DcmtxCart(const double *Dcm,const double *Cart,double *Cart2) {
	double Work[3];

	Work[0]=Dcm[0]*Cart[0]+Dcm[3]*Cart[1]+Dcm[6]*Cart[2];
	Work[1]=Dcm[1]*Cart[0]+Dcm[4]*Cart[1]+Dcm[7]*Cart[2];
	Work[2]=Dcm[2]*Cart[0]+Dcm[5]*Cart[1]+Dcm[8]*Cart[2];
	Cart2[0]=Work[0];
	Cart2[1]=Work[1];
	Cart2[2]=Work[2];
	return; }


void Sph_One2Dcm(double *Dcm) {
	Dcm[0]=Dcm[4]=Dcm[8]=1;
	Dcm[1]=Dcm[2]=Dcm[3]=Dcm[5]=Dcm[6]=Dcm[7]=0;
	return; }


void Sph_Ypr2Yprr(const double *Ypr,double *Yprr) {
	double cf,cq,cy,sf,sq,sy;

	cf=cos(Ypr[0]);
	cq=cos(Ypr[1]);
	cy=cos(Ypr[2]);
	sf=sin(Ypr[0]);
	sq=sin(Ypr[1]);
	sy=sin(Ypr[2]);
	Yprr[0]=atan2(sy*sq*cf-cy*sf,cq*cf);
	Yprr[1]=-asin(-cy*sq*cf-sy*sf);
	Yprr[2]=atan2(-cy*sq*sf+sy*cf,cq*cy);
	return; }


void Sph_Dcm2Dcmr(const double *Dcm,double *Dcmr) {
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


void Sph_Newz2Dcm(const double *Newz,double psi,double *Dcm) {
	double Work[4];

	Sph_Cart2Sc(Newz,Work);
	Work[2]+=PI/2.0;
	Work[3]=psi-Work[2];
	Sph_Eax2Dcm(Work+1,Dcm);
	Sph_Dcm2Dcmt(Dcm,Dcm);
	return; }


void Sph_DcmtxUnit(const double *Dcmt,char axis,double *vect,const double *add,double mult) {
	double Work[3];

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


/* *** Quaternions *** */

void Sph_One2Qtn(double *Qtn) {
	Qtn[0]=1;
	Qtn[1]=Qtn[2]=Qtn[3]=0;
	return; }


void Sph_Qtn2Qtn(const double *Qtn1,double *Qtn2) {
	Qtn2[0]=Qtn1[0];
	Qtn2[1]=Qtn1[1];
	Qtn2[2]=Qtn1[2];
	Qtn2[3]=Qtn1[3];
	return; }


void Sph_Ypr2Qtn(const double *Ypr,double *Qtn) {
	double cf2,cq2,cy2,sf2,sq2,sy2;

	cf2=cos(Ypr[0]/2);
	sf2=sin(Ypr[0]/2);
	cq2=cos(Ypr[1]/2);
	sq2=sin(Ypr[1]/2);
	cy2=cos(Ypr[2]/2);
	sy2=sin(Ypr[2]/2);
	Qtn[0]=cy2*cq2*cf2+sy2*sq2*sf2;
	Qtn[1]=-sy2*cq2*cf2+cy2*sq2*sf2;
	Qtn[2]=-cy2*sq2*cf2-sy2*cq2*sf2;
	Qtn[3]=-cy2*cq2*sf2+sy2*sq2*cf2;
	return; }


void Sph_Yaw2Qtn(double Yaw,double *Qtn) {
	Qtn[0]=cos(Yaw/2);
	Qtn[1]=0;
	Qtn[2]=0;
	Qtn[3]=-sin(Yaw/2);
	return; }


void Sph_Ypr2Qtni(const double *Ypr,double *Qtni) {
	double cf2,cq2,cy2,sf2,sq2,sy2;

	cf2=cos(Ypr[0]/2);
	sf2=sin(Ypr[0]/2);
	cq2=cos(Ypr[1]/2);
	sq2=sin(Ypr[1]/2);
	cy2=cos(Ypr[2]/2);
	sy2=sin(Ypr[2]/2);
	Qtni[0]=cy2*cq2*cf2+sy2*sq2*sf2;
	Qtni[1]=sy2*cq2*cf2-cy2*sq2*sf2;
	Qtni[2]=cy2*sq2*cf2+sy2*cq2*sf2;
	Qtni[3]=cy2*cq2*sf2-sy2*sq2*cf2;
	return; }


void Sph_Qtn2Ypr(const double *Qtn,double *Ypr) {
	Ypr[0]=atan2(-2*Qtn[0]*Qtn[3]+2*Qtn[1]*Qtn[2],Qtn[0]*Qtn[0]+Qtn[1]*Qtn[1]-Qtn[2]*Qtn[2]-Qtn[3]*Qtn[3]);
	Ypr[1]=asin(-2*Qtn[0]*Qtn[2]-2*Qtn[1]*Qtn[3]);
	Ypr[2]=atan2(-2*Qtn[0]*Qtn[1]+2*Qtn[2]*Qtn[3],Qtn[0]*Qtn[0]-Qtn[1]*Qtn[1]-Qtn[2]*Qtn[2]+Qtn[3]*Qtn[3]);
	return; }


double Sph_Qtn2Yaw(const double *Qtn) {
	return atan2(-2*Qtn[0]*Qtn[3],Qtn[0]*Qtn[0]-Qtn[3]*Qtn[3]); }


void Sph_Dcm2Qtn(const double *Dcm,double *Qtn) {
	double factor;

	Qtn[0]=Dcm[0]+Dcm[4]+Dcm[8];
	Qtn[1]=Dcm[0]-Dcm[4]-Dcm[8];
	Qtn[2]=-Dcm[0]+Dcm[4]-Dcm[8];
	Qtn[3]=-Dcm[0]-Dcm[4]+Dcm[8];
	if(Qtn[0]>=Qtn[1] && Qtn[0]>=Qtn[2] && Qtn[0]>=Qtn[3]) {
		Qtn[0]=0.5*sqrt(1+Qtn[0]);
		factor=0.25/Qtn[0];
		Qtn[1]=factor*(Dcm[7]-Dcm[5]);
		Qtn[2]=factor*(Dcm[2]-Dcm[6]);
		Qtn[3]=factor*(Dcm[3]-Dcm[1]); }
	else if(Qtn[1]>=Qtn[2] && Qtn[1]>=Qtn[3]) {
		Qtn[1]=0.5*sqrt(1+Qtn[1]);
		factor=0.25/Qtn[1];
		Qtn[0]=factor*(Dcm[7]-Dcm[5]);
		Qtn[2]=factor*(Dcm[1]+Dcm[3]);
		Qtn[3]=factor*(Dcm[2]+Dcm[6]); }
	else if(Qtn[2]>=Qtn[3]) {
		Qtn[2]=0.5*sqrt(1+Qtn[2]);
		factor=0.25/Qtn[2];
		Qtn[0]=factor*(Dcm[2]-Dcm[6]);
		Qtn[1]=factor*(Dcm[1]+Dcm[3]);
		Qtn[3]=factor*(Dcm[5]+Dcm[7]); }
	else {
		Qtn[3]=0.5*sqrt(1+Qtn[3]);
		factor=0.25/Qtn[3];
		Qtn[0]=factor*(Dcm[3]-Dcm[1]);
		Qtn[1]=factor*(Dcm[2]+Dcm[6]);
		Qtn[2]=factor*(Dcm[5]+Dcm[7]); }
	return; }


void Sph_Qtn2Dcm(const double *Qtn,double *Dcm) {
	Dcm[0]=1-2*Qtn[2]*Qtn[2]-2*Qtn[3]*Qtn[3];
	Dcm[1]=2*Qtn[1]*Qtn[2]-2*Qtn[0]*Qtn[3];
	Dcm[2]=2*Qtn[1]*Qtn[3]+2*Qtn[0]*Qtn[2];
	Dcm[3]=2*Qtn[1]*Qtn[2]+2*Qtn[0]*Qtn[3];
	Dcm[4]=1-2*Qtn[1]*Qtn[1]-2*Qtn[3]*Qtn[3];
	Dcm[5]=2*Qtn[2]*Qtn[3]-2*Qtn[0]*Qtn[1];
	Dcm[6]=2*Qtn[1]*Qtn[3]-2*Qtn[0]*Qtn[2];
	Dcm[7]=2*Qtn[2]*Qtn[3]+2*Qtn[0]*Qtn[1];
	Dcm[8]=1-2*Qtn[1]*Qtn[1]-2*Qtn[2]*Qtn[2];
	return; }


void Sph_XZ2Qtni(const double *x,const double *z,double *Qtn) {
	double *x2,*y2,*z2,dcm[9],len;

	x2=dcm;
	y2=dcm+3;
	z2=dcm+6;
	len=sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]);
	x2[0]=x[0]/len;																		// normalize x and store in first row
	x2[1]=x[1]/len;
	x2[2]=x[2]/len;
	Sph_crossVVD(z,x2,y2);														// y2 = z cross x2
	len=sqrt(y2[0]*y2[0]+y2[1]*y2[1]+y2[2]*y2[2]);
	y2[0]/=len;																				// normalize y2
	y2[1]/=len;
	y2[2]/=len;
	Sph_crossVVD(x2,y2,z2);														// z2 = x2 cross y2
	Sph_Dcm2Qtn(dcm,Qtn);															// convert matrix to qtn
	return; }


void Sph_QtnxQtn(const double *Qtn1,const double *Qtn2,double *Qtn3) {
	double Work[4];

	Work[0]=Qtn1[0]*Qtn2[0]-Qtn1[1]*Qtn2[1]-Qtn1[2]*Qtn2[2]-Qtn1[3]*Qtn2[3];
	Work[1]=Qtn1[0]*Qtn2[1]+Qtn1[1]*Qtn2[0]-Qtn1[2]*Qtn2[3]+Qtn1[3]*Qtn2[2];
	Work[2]=Qtn1[0]*Qtn2[2]+Qtn1[1]*Qtn2[3]+Qtn1[2]*Qtn2[0]-Qtn1[3]*Qtn2[1];
	Work[3]=Qtn1[0]*Qtn2[3]-Qtn1[1]*Qtn2[2]+Qtn1[2]*Qtn2[1]+Qtn1[3]*Qtn2[0];
	Qtn3[0]=Work[0];
	Qtn3[1]=Work[1];
	Qtn3[2]=Work[2];
	Qtn3[3]=Work[3];
	return; }


void Sph_QtnixQtn(const double *Qtn1,const double *Qtn2,double *Qtn3) {
	double Work[4];

	Work[0]=Qtn1[0]*Qtn2[0]+Qtn1[1]*Qtn2[1]+Qtn1[2]*Qtn2[2]+Qtn1[3]*Qtn2[3];
	Work[1]=Qtn1[0]*Qtn2[1]-Qtn1[1]*Qtn2[0]+Qtn1[2]*Qtn2[3]-Qtn1[3]*Qtn2[2];
	Work[2]=Qtn1[0]*Qtn2[2]-Qtn1[1]*Qtn2[3]-Qtn1[2]*Qtn2[0]+Qtn1[3]*Qtn2[1];
	Work[3]=Qtn1[0]*Qtn2[3]+Qtn1[1]*Qtn2[2]-Qtn1[2]*Qtn2[1]-Qtn1[3]*Qtn2[0];
	Qtn3[0]=Work[0];
	Qtn3[1]=Work[1];
	Qtn3[2]=Work[2];
	Qtn3[3]=Work[3];
	return; }


void Sph_QtnxQtni(const double *Qtn1,const double *Qtn2,double *Qtn3) {
	double Work[4];

	Work[0]=Qtn1[0]*Qtn2[0]+Qtn1[1]*Qtn2[1]+Qtn1[2]*Qtn2[2]+Qtn1[3]*Qtn2[3];
	Work[1]=-Qtn1[0]*Qtn2[1]+Qtn1[1]*Qtn2[0]+Qtn1[2]*Qtn2[3]-Qtn1[3]*Qtn2[2];
	Work[2]=-Qtn1[0]*Qtn2[2]-Qtn1[1]*Qtn2[3]+Qtn1[2]*Qtn2[0]+Qtn1[3]*Qtn2[1];
	Work[3]=-Qtn1[0]*Qtn2[3]+Qtn1[1]*Qtn2[2]-Qtn1[2]*Qtn2[1]+Qtn1[3]*Qtn2[0];
	Qtn3[0]=Work[0];
	Qtn3[1]=Work[1];
	Qtn3[2]=Work[2];
	Qtn3[3]=Work[3];
	return; }


void Sph_QtnRotate(const double *Qtn,const double *Cart,double *Cart2) {
	double qxc[4];

	qxc[0]=Cart[0]*Qtn[1]+Cart[1]*Qtn[2]+Cart[2]*Qtn[3];
	qxc[1]=Cart[0]*Qtn[0]+Cart[2]*Qtn[2]-Cart[1]*Qtn[3];
	qxc[2]=Cart[1]*Qtn[0]-Cart[2]*Qtn[1]+Cart[0]*Qtn[3];
	qxc[3]=Cart[2]*Qtn[0]+Cart[1]*Qtn[1]-Cart[0]*Qtn[2];
	Cart2[0]=Qtn[1]*qxc[0]+Qtn[0]*qxc[1]-Qtn[3]*qxc[2]+Qtn[2]*qxc[3];
	Cart2[1]=Qtn[2]*qxc[0]+Qtn[3]*qxc[1]+Qtn[0]*qxc[2]-Qtn[1]*qxc[3];
	Cart2[2]=Qtn[3]*qxc[0]-Qtn[2]*qxc[1]+Qtn[1]*qxc[2]+Qtn[0]*qxc[3];
	return; }


void Sph_QtniRotate(const double *Qtn,const double *Cart,double *Cart2) {
	double qxc[4];

	qxc[0]=-Cart[0]*Qtn[1]-Cart[1]*Qtn[2]-Cart[2]*Qtn[3];
	qxc[1]= Cart[0]*Qtn[0]-Cart[2]*Qtn[2]+Cart[1]*Qtn[3];
	qxc[2]= Cart[1]*Qtn[0]+Cart[2]*Qtn[1]-Cart[0]*Qtn[3];
	qxc[3]= Cart[2]*Qtn[0]-Cart[1]*Qtn[1]+Cart[0]*Qtn[2];
	Cart2[0]=-Qtn[1]*qxc[0]+Qtn[0]*qxc[1]+Qtn[3]*qxc[2]-Qtn[2]*qxc[3];
	Cart2[1]=-Qtn[2]*qxc[0]-Qtn[3]*qxc[1]+Qtn[0]*qxc[2]+Qtn[1]*qxc[3];
	Cart2[2]=-Qtn[3]*qxc[0]+Qtn[2]*qxc[1]-Qtn[1]*qxc[2]+Qtn[0]*qxc[3];
	return; }


void Sph_QtniRotateUnitx(const double *Qtni,double *vect,const double *add,double mult) {
	vect[0]=add[0]+mult*(Qtni[0]*Qtni[0]+Qtni[1]*Qtni[1]-Qtni[2]*Qtni[2]-Qtni[3]*Qtni[3]);
	vect[1]=add[1]+mult*(2*Qtni[1]*Qtni[2]-2*Qtni[0]*Qtni[3]);
	vect[2]=add[2]+mult*(2*Qtni[0]*Qtni[2]+2*Qtni[1]*Qtni[3]);
	return; }


void Sph_QtniRotateUnitz(const double *Qtni,double *vect,const double *add,double mult) {
	vect[0]=add[0]+mult*(-2*Qtni[0]*Qtni[2]+2*Qtni[1]*Qtni[3]);
	vect[1]=add[1]+mult*(2*Qtni[0]*Qtni[1]+2*Qtni[2]*Qtni[3]);
	vect[2]=add[2]+mult*(Qtni[0]*Qtni[0]-Qtni[1]*Qtni[1]-Qtni[2]*Qtni[2]+Qtni[3]*Qtni[3]);
	return; }


/* *** Unit normals *** */

double Sph_RotateVectWithNormals3D(const double *pt1,const double *pt2,double *newpt2,double *oldnorm,double *newnorm,int sign) {
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


void Sph_RotateVectorAxisAngle(const double *vect,const double *axis,double angle,double *rotated) {
	double ax[3],len,ca,sa,ans[3];

	len=sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
	ax[0]=axis[0]/len;
	ax[1]=axis[1]/len;
	ax[2]=axis[2]/len;

	ca=cos(angle);
	sa=sin(angle);

	ans[0]=vect[0]*(ca+ax[0]*ax[0]*(1-ca)) + vect[1]*(ax[0]*ax[1]*(1-ca)-ax[2]*sa) + vect[2]*(ax[0]*ax[2]*(1-ca)+ax[1]*sa);
	ans[1]=vect[0]*(ax[1]*ax[0]*(1-ca)+ax[2]*sa) + vect[1]*(ca+ax[1]*ax[1]*(1-ca)) + vect[2]*(ax[1]*ax[2]*(1-ca)-ax[0]*sa);
	ans[2]=vect[0]*(ax[2]*ax[0]*(1-ca)-ax[1]*sa) + vect[1]*(ax[2]*ax[1]*(1-ca)+ax[0]*sa) + vect[2]*(ca+ax[2]*ax[2]*(1-ca));
	rotated[0]=ans[0];
	rotated[1]=ans[1];
	rotated[2]=ans[2];
	return; }



