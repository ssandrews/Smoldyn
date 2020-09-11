/* Steven Andrews, 2/17/06
Simple routines for 1D, 2D, and 3D geometry.
See documentation called Geometry_doc.doc and Geometry_doc.pdf.
Copyright 2006-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include "Geometry.h"
#include "math2.h"

#define EPSILON 100*DBL_EPSILON

#ifndef INFINITY			// INFINITY is defined in math.h, so should already be defined
	#include <limits>		// MSVC 2012
	#define INFINITY std::numeric_limits<float>::infinity( );
#endif

/***************** ... Center **************/

void Geo_LineCenter(double **point,double *cent,int dim) {
	int d;

	for(d=0;d<dim;d++)
		cent[d]=0.5*(point[0][d]+point[1][d]);
	return; }


void Geo_RectCenter(double **point,double *cent,int dim) {
	if(dim==1)
		cent[0]=point[0][0];
	else if(dim==2) {
		cent[0]=0.5*(point[0][0]+point[1][0]);
		cent[1]=0.5*(point[0][1]+point[1][1]); }
	else if(dim==3) {
		cent[0]=0.5*(point[0][0]+point[2][0]);
		cent[1]=0.5*(point[0][1]+point[2][1]);
		cent[2]=0.5*(point[0][2]+point[2][2]); }
	return; }


void Geo_TriCenter(double **point,double *cent,int dim) {
	if(dim==1)
		cent[0]=point[0][0];
	else if(dim==2) {
		cent[0]=0.5*(point[0][0]+point[1][0]);
		cent[1]=0.5*(point[0][1]+point[1][1]); }
	else if(dim==3) {
		cent[0]=(1.0/3.0)*(point[0][0]+point[1][0]+point[2][0]);
		cent[1]=(1.0/3.0)*(point[0][1]+point[1][1]+point[2][1]);
		cent[2]=(1.0/3.0)*(point[0][2]+point[1][2]+point[2][2]); }
	return; }


/***************** ... Normal **************/

double Geo_LineNormal(double *pt1,double *pt2,double *ans) {
	double dx,dy,len,leninv;

	dx=pt2[0]-pt1[0];
	dy=pt2[1]-pt1[1];
	len=sqrt(dx*dx+dy*dy);
	if(len>0) {
		leninv=1.0/len;
		ans[0]=leninv*dy;
		ans[1]=-leninv*dx; }
	else {
		ans[0]=1;
		ans[1]=0; }
	return len; }


double Geo_LineNormal2D(double *pt1,double *pt2,double *point,double *ans) {
	double dx,dy,dot,len;

	dx=pt2[0]-pt1[0];
	dy=pt2[1]-pt1[1];
	len=sqrt(dx*dx+dy*dy);
	if(len>EPSILON) {
		ans[0]=dy/len;
		ans[1]=-dx/len;
		dot=(point[0]-pt1[0])*ans[0]+(point[1]-pt1[1])*ans[1];
		if(dot<0) {
			ans[0]*=-1;
			ans[1]*=-1;
			dot*=-1; }
		return sqrt(dot); }

	ans[0]=point[0]-pt1[0];
	ans[1]=point[1]-pt1[1];
	dot=ans[0]*ans[0]+ans[1]*ans[1];
	if(dot<EPSILON) {							// pt1 == pt2 and point == pt1 so return unit x
		ans[0]=1;
		ans[1]=0;
		return 0; }
	dot=sqrt(dot);						// pt1 == pt2 so return normalized point
	ans[0]/=dot;
	ans[1]/=dot;
	return dot; }


double Geo_LineNormal3D(double *pt1,double *pt2,double *point,double *ans) {
	double dot,line[3];

	line[0]=pt2[0]-pt1[0];
	line[1]=pt2[1]-pt1[1];
	line[2]=pt2[2]-pt1[2];
	dot=line[0]*line[0]+line[1]*line[1]+line[2]*line[2];
	if(dot<EPSILON) {								// pt1 == pt2
		ans[0]=point[0]-pt1[0];
		ans[1]=point[1]-pt1[1];
		ans[2]=point[2]-pt1[2];
		dot=ans[0]*ans[0]+ans[1]*ans[1]+ans[2]*ans[2];
		if(dot<EPSILON) {							// pt1 == pt2 and point == pt1 so return unit x
			ans[0]=1;
			ans[1]=0;
			ans[2]=0;
			return 0; }
		dot=sqrt(dot);						// pt1 == pt2 so return normalized point
		ans[0]/=dot;
		ans[1]/=dot;
		ans[2]/=dot;
		return dot; }
	dot=sqrt(dot);
	line[0]/=dot;
	line[1]/=dot;
	line[2]/=dot;

	ans[0]=point[0]-pt1[0];
	ans[1]=point[1]-pt1[1];
	ans[2]=point[2]-pt1[2];
	dot=ans[0]*line[0]+ans[1]*line[1]+ans[2]*line[2];
	ans[0]-=dot*line[0];
	ans[1]-=dot*line[1];
	ans[2]-=dot*line[2];
	dot=ans[0]*line[0]+ans[1]*line[1]+ans[2]*line[2];
	ans[0]-=dot*line[0];
	ans[1]-=dot*line[1];
	ans[2]-=dot*line[2];
	dot=ans[0]*ans[0]+ans[1]*ans[1]+ans[2]*ans[2];
	if(dot<EPSILON) {											// point is on line
		if(line[0]==0 && line[1]==0) {		// line parallel to z so return unit x
			ans[0]=1;
			ans[1]=0;
			ans[2]=0;
			return 0; }
		ans[0]=line[1];									// return right side perpendicular in x,y plane
		ans[1]=-line[0];
		ans[2]=0;
		dot=sqrt(ans[0]*ans[0]+ans[1]*ans[1]+ans[2]*ans[2]);
		ans[0]/=dot;
		ans[1]/=dot;
		ans[2]/=dot;
		return 0; }
	dot=sqrt(dot);
	ans[0]/=dot;
	ans[1]/=dot;
	ans[2]/=dot;
	return dot; }


double Geo_LineNormPos(double *pt1,double *pt2,double *point,int dim,double *distptr) {
	double dot,len2,pointlen2;
	int d;

	len2=dot=pointlen2=0;
	for(d=0;d<dim;d++) {
		dot+=(point[d]-pt1[d])*(pt2[d]-pt1[d]);
		len2+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]);
		pointlen2+=(point[d]-pt1[d])*(point[d]-pt1[d]); }
	if(distptr) *distptr=sqrt(pointlen2-dot*dot/len2);
	return dot/len2; }


double Geo_TriNormal(double *pt1,double *pt2,double *pt3,double *ans) {
	double dx1,dy1,dz1,dx2,dy2,dz2,area;

	dx1=pt2[0]-pt1[0];
	dy1=pt2[1]-pt1[1];
	dz1=pt2[2]-pt1[2];
	dx2=pt3[0]-pt2[0];
	dy2=pt3[1]-pt2[1];
	dz2=pt3[2]-pt2[2];
	ans[0]=dy1*dz2-dz1*dy2;			// v1 cross v2
	ans[1]=-dx1*dz2+dz1*dx2;
	ans[2]=dx1*dy2-dy1*dx2;
	area=sqrt(ans[0]*ans[0]+ans[1]*ans[1]+ans[2]*ans[2]);
	if(area>EPSILON) {
		ans[0]/=area;
		ans[1]/=area;
		ans[2]/=area; }
	else {
		Geo_LineNormal(pt1,pt2,ans);
		ans[2]=0; }
	return area/2; }


double Geo_SphereNormal(double *cent,double *pt,int front,int dim,double *ans) {
	int d;
	double dist;

	dist=0;
	for(d=0;d<dim;d++) {
		ans[d]=front*(pt[d]-cent[d]);
		dist+=ans[d]*ans[d]; }
	if(dist>0) {
		dist=sqrt(dist);
		for(d=0;d<dim;d++) ans[d]/=dist; }
	else {
		ans[0]=1;
		for(d=1;d<dim;d++) ans[d]=0; }
	return dist; }


/******** Unit Vectors *******/


double Geo_UnitCross(double *v1start,double *v1end,double *v2start,double *v2end,double *ans) {
	double ax,ay,az,bx,by,bz,cx,cy,cz,len;

	if(v1start) {
		ax=v1end[0]-v1start[0];
		ay=v1end[1]-v1start[1];
		az=v1end[2]-v1start[2]; }
	else {
		ax=v1end[0];
		ay=v1end[1];
		az=v1end[2]; }
	if(v2start) {
		bx=v2end[0]-v2start[0];
		by=v2end[1]-v2start[1];
		bz=v2end[2]-v2start[2]; }
	else {
		bx=v2end[0];
		by=v2end[1];
		bz=v2end[2]; }
	cx=ay*bz-az*by;
	cy=az*bx-ax*bz;
	cz=ax*by-ay*bx;
	len=sqrt(cx*cx+cy*cy+cz*cz);
	if(len>EPSILON) {
		ans[0]=cx/len;
		ans[1]=cy/len;
		ans[2]=cz/len;
		return len; }
	ans[0]=ans[1]=ans[2]=0;
	return 0; }


double Geo_TriUnitVects(double *pt1,double *pt2,double *pt3,double *unit0,double *unit1,double *unit2) {
	double area,dist;

	area=Geo_TriNormal(pt1,pt2,pt3,unit0);									// unit0 is triangle normal
	dist=sqrt((pt2[0]-pt1[0])*(pt2[0]-pt1[0]) + (pt2[1]-pt1[1])*(pt2[1]-pt1[1]) + (pt2[2]-pt1[2])*(pt2[2]-pt1[2]));	// distance pt1 to pt2
	unit1[0]=(pt2[0]-pt1[0])/dist;													// unit1 is line from pt1 to pt2
	unit1[1]=(pt2[1]-pt1[1])/dist;
	unit1[2]=(pt2[2]-pt1[2])/dist;
	unit2[0]=unit0[1]*unit1[2]-unit0[2]*unit1[1];						// unit2 is cross product unit0 x unit1
	unit2[1]=unit0[2]*unit1[0]-unit0[0]*unit1[2];
	unit2[2]=unit0[0]*unit1[1]-unit0[1]*unit1[0];
	return area; }


double Geo_SphereUnitVects(double *cent,double *top,double *point,int front,double *unit0,double *unit1,double *unit2) {
	double radius;

	radius=Geo_SphereNormal(cent,point,front,3,unit0);			// unit0 is sphere normal
	Geo_LineNormal3D(cent,point,top,unit1);									// unit1 is normal of normal line, pointing to sphere top
	unit2[0]=unit0[1]*unit1[2]-unit0[2]*unit1[1];						// unit2 is cross product unit0 x unit1
	unit2[1]=unit0[2]*unit1[0]-unit0[0]*unit1[2];
	unit2[2]=unit0[0]*unit1[1]-unit0[1]*unit1[0];
	return radius; }


double Geo_CylUnitVects(double *pt1,double *pt2,double *point,int front,double *unit0,double *unit1,double *unit2) {
	double radius,dist;

	radius=Geo_LineNormal3D(pt1,pt2,point,unit0);						// unit0 is cylinder normal
	dist=sqrt((pt2[0]-pt1[0])*(pt2[0]-pt1[0]) + (pt2[1]-pt1[1])*(pt2[1]-pt1[1]) + (pt2[2]-pt1[2])*(pt2[2]-pt1[2]));	// distance pt1 to pt2
	unit1[0]=(pt2[0]-pt1[0])/dist;													// unit1 is line from pt1 to pt2
	unit1[1]=(pt2[1]-pt1[1])/dist;
	unit1[2]=(pt2[2]-pt1[2])/dist;
	unit2[0]=unit0[1]*unit1[2]-unit0[2]*unit1[1];						// unit2 is cross product unit0 x unit1
	unit2[1]=unit0[2]*unit1[0]-unit0[0]*unit1[2];
	unit2[2]=unit0[0]*unit1[1]-unit0[1]*unit1[0];
	return radius; }


double Geo_DiskUnitVects(double *cent,double *front,double *point,double *unit0,double *unit1,double *unit2) {
	double dist;

	unit0[0]=front[0];																			// unit0 is simply front vector
	unit0[1]=front[1];
	unit0[2]=front[2];
	dist=sqrt((point[0]-cent[0])*(point[0]-cent[0]) + (point[1]-cent[1])*(point[1]-cent[1]) + (point[2]-cent[2])*(point[2]-cent[2]));	// distance cent to point
	unit1[0]=(point[0]-cent[0])/dist;												// unit1 is radial line from cent to point
	unit1[1]=(point[1]-cent[1])/dist;
	unit1[2]=(point[2]-cent[2])/dist;
	unit2[0]=unit0[1]*unit1[2]-unit0[2]*unit1[1];						// unit2 is cross product unit0 x unit1
	unit2[1]=unit0[2]*unit1[0]-unit0[0]*unit1[2];
	unit2[2]=unit0[0]*unit1[1]-unit0[1]*unit1[0];
	return dist; }


/********* Area ***********/

double Geo_LineLength(double *pt1,double *pt2,int dim) {
	double len;
	int d;

	len=0;
	for(d=0;d<dim;d++) len+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]);
	return sqrt(len); }


double Geo_TriArea2(double *pt1,double *pt2,double *pt3) {
	return -0.5*((pt2[1]-pt1[1])*(pt3[0]-pt1[0])-(pt2[0]-pt1[0])*(pt3[1]-pt1[1])); }


double Geo_TriArea3D(double *pt1,double *pt2,double *pt3) {
	double a,b,c;

	a = sqrt((pt1[0]-pt2[0])*(pt1[0]-pt2[0]) + (pt1[1]-pt2[1])*(pt1[1]-pt2[1]) + (pt1[2]-pt2[2])*(pt1[2]-pt2[2])); //distance p1 to p2
	b = sqrt((pt1[0]-pt3[0])*(pt1[0]-pt3[0]) + (pt1[1]-pt3[1])*(pt1[1]-pt3[1]) + (pt1[2]-pt3[2])*(pt1[2]-pt3[2])); //distance p1 to p3
	c = sqrt((pt2[0]-pt3[0])*(pt2[0]-pt3[0]) + (pt2[1]-pt3[1])*(pt2[1]-pt3[1]) + (pt2[2]-pt3[2])*(pt2[2]-pt3[2])); //distance p2 to p3
	return sqrt((a+(b+c))*(c-(a-b))*(c+(a-b))*(a+(b-c)))/4.0; }


double Geo_TriArea3(double *pt1,double *pt2,double *pt3,double *norm) {
	double base[3],cross[3];

	base[0]=pt2[0]-pt1[0];
	base[1]=pt2[1]-pt1[1];
	base[2]=pt2[2]-pt1[2];
	cross[0]=base[1]*norm[2]-base[2]*norm[1];
	cross[1]=base[2]*norm[0]-base[0]*norm[2];
	cross[2]=base[0]*norm[1]-base[1]*norm[0];
	return -0.5*(cross[0]*(pt3[0]-pt1[0])+cross[1]*(pt3[1]-pt1[1])+cross[2]*(pt3[2]-pt1[2])); }


double Geo_QuadArea(double *pt1,double *pt2,double *pt3,double *pt4,int dim) {
	double area,norm[3];

	if(dim==2) {
		area=Geo_TriArea2(pt1,pt2,pt3);
		area+=Geo_TriArea2(pt1,pt3,pt4); }
	else if(dim==3) {
		area=Geo_TriNormal(pt1,pt2,pt3,norm);
		if(area<EPSILON) area=Geo_TriNormal(pt1,pt3,pt3,norm);
		else area+=Geo_TriArea3(pt1,pt3,pt4,norm); }
	else area=0;
	return area; }


/********* Inside point ... ***********/

double Geo_InsidePoints2(double *pt1,double *pt2,double margin,double *ans1,double *ans2,int dim) {
	int d;
	double len,delta[3];

	len=0;
	for(d=0;d<dim;d++) {
		delta[d]=pt2[d]-pt1[d];
		len+=delta[d]*delta[d]; }
	len=sqrt(len);
	for(d=0;d<dim;d++) {
		delta[d]=delta[d]/len*margin;
		ans1[d]=pt1[d]+delta[d];
		ans2[d]=pt2[d]-delta[d]; }
	return len; }


void Geo_InsidePoints3(double *pt1,double *pt2,double *pt3,double margin,double *ans1,double *ans2,double *ans3) {
	double l122,l232,l312,l12,l23,l31,s,factor;

	l122=(pt2[0]-pt1[0])*(pt2[0]-pt1[0])+(pt2[1]-pt1[1])*(pt2[1]-pt1[1])+(pt2[2]-pt1[2])*(pt2[2]-pt1[2]);
	l232=(pt3[0]-pt2[0])*(pt3[0]-pt2[0])+(pt3[1]-pt2[1])*(pt3[1]-pt2[1])+(pt3[2]-pt2[2])*(pt3[2]-pt2[2]);
	l312=(pt1[0]-pt3[0])*(pt1[0]-pt3[0])+(pt1[1]-pt3[1])*(pt1[1]-pt3[1])+(pt1[2]-pt3[2])*(pt1[2]-pt3[2]);
	l12=sqrt(l122);
	l23=sqrt(l232);
	l31=sqrt(l312);
	s=0.5*(l12+l23+l31);

	factor=margin*sqrt(l12*l31/(s*(s-l23)*(2*l122+2*l312-l232)));
	ans1[0]=pt1[0]+factor*((pt2[0]-pt1[0])/l12-(pt1[0]-pt3[0])/l31);
	ans1[1]=pt1[1]+factor*((pt2[1]-pt1[1])/l12-(pt1[1]-pt3[1])/l31);
	ans1[2]=pt1[2]+factor*((pt2[2]-pt1[2])/l12-(pt1[2]-pt3[2])/l31);

	factor=margin*sqrt(l23*l12/(s*(s-l31)*(2*l232+2*l122-l312)));
	ans2[0]=pt2[0]+factor*((pt3[0]-pt2[0])/l23-(pt2[0]-pt1[0])/l12);
	ans2[1]=pt2[1]+factor*((pt3[1]-pt2[1])/l23-(pt2[1]-pt1[1])/l12);
	ans2[2]=pt2[2]+factor*((pt3[2]-pt2[2])/l23-(pt2[2]-pt1[2])/l12);

	factor=margin*sqrt(l31*l23/(s*(s-l12)*(2*l312+2*l232-l122)));
	ans3[0]=pt3[0]+factor*((pt1[0]-pt3[0])/l31-(pt3[0]-pt2[0])/l23);
	ans3[1]=pt3[1]+factor*((pt1[1]-pt3[1])/l31-(pt3[1]-pt2[1])/l23);
	ans3[2]=pt3[2]+factor*((pt1[2]-pt3[2])/l31-(pt3[2]-pt2[2])/l23);

	return; }


void Geo_InsidePoints32(double **point,double margin,double **ans) {
	double *pt1,*pt2,*pt3,*en12,*en23,*en31,len1,len2,len3;

	pt1=point[0];
	pt2=point[1];
	pt3=point[2];
	en12=point[3];
	en23=point[4];
	en31=point[5];

	len1=margin/(1.0+en31[0]*en12[0]+en31[1]*en12[1]+en31[2]+en12[2]);
	len2=margin/(1.0+en12[0]*en23[0]+en12[1]*en23[1]+en12[2]+en23[2]);
	len3=margin/(1.0+en23[0]*en31[0]+en23[1]*en31[1]+en23[2]+en31[2]);

	ans[0][0]=pt1[0]-len1*en31[0]-len1*en12[0];
	ans[0][1]=pt1[1]-len1*en31[1]-len1*en12[1];
	ans[0][2]=pt1[2]-len1*en31[2]-len1*en12[2];
	ans[1][0]=pt2[0]-len2*en12[0]-len2*en23[0];
	ans[1][1]=pt2[1]-len2*en12[1]-len2*en23[1];
	ans[1][2]=pt2[2]-len2*en12[2]-len2*en23[2];
	ans[2][0]=pt3[0]-len3*en23[0]-len3*en31[0];
	ans[2][1]=pt3[1]-len3*en23[1]-len3*en31[1];
	ans[2][2]=pt3[2]-len3*en23[2]-len3*en31[2];

	return; }



/********* Point In ... ***********/

int Geo_PtInTriangle(double *pt1,double *pt2,double *pt3,double *norm,double *test) {
	double dx1,dy1,dz1,dx2,dy2,dz2,crss[3],dot;

	dx1=pt2[0]-pt1[0];
	dy1=pt2[1]-pt1[1];
	dz1=pt2[2]-pt1[2];
	dx2=test[0]-pt2[0];
	dy2=test[1]-pt2[1];
	dz2=test[2]-pt2[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	dot=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];
	if(dot<0) return 0;

	dx1=pt3[0]-pt2[0];
	dy1=pt3[1]-pt2[1];
	dz1=pt3[2]-pt2[2];
	dx2=test[0]-pt3[0];
	dy2=test[1]-pt3[1];
	dz2=test[2]-pt3[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	dot=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];
	if(dot<0) return 0;

	dx1=pt1[0]-pt3[0];
	dy1=pt1[1]-pt3[1];
	dz1=pt1[2]-pt3[2];
	dx2=test[0]-pt1[0];
	dy2=test[1]-pt1[1];
	dz2=test[2]-pt1[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	dot=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];
	if(dot<0) return 0;
	return 1; }


int Geo_PtInTriangle2(double **point,double *test) {
	double *pt,*en,dot;

	pt=point[0];
	en=point[3];
	dot=(test[0]-pt[0])*en[0]+(test[1]-pt[1])*en[1]+(test[2]-pt[2])*en[2];
	if(dot>0) return 0;

	pt=point[1];
	en=point[4];
	dot=(test[0]-pt[0])*en[0]+(test[1]-pt[1])*en[1]+(test[2]-pt[2])*en[2];
	if(dot>0) return 0;

	pt=point[2];
	en=point[5];
	dot=(test[0]-pt[0])*en[0]+(test[1]-pt[1])*en[1]+(test[2]-pt[2])*en[2];
	if(dot>0) return 0;

	return 1; }


int Geo_PtInSlab(double *pt1,double *pt2,double *test,int dim) {
	double dot;
	int d;

	dot=0;
	for(d=0;d<dim;d++) dot+=(test[d]-pt1[d])*(pt2[d]-pt1[d]);
	if(dot<0) return 0;
	dot=0;
	for(d=0;d<dim;d++) dot+=(test[d]-pt2[d])*(pt1[d]-pt2[d]);
	if(dot<0) return 0;
	return 1; }


int Geo_PtInSphere(double *test,double *cent,double rad,int dim) {
	int d;
	double r2;

	r2=0;
	for(d=0;d<dim;d++) r2+=(test[d]-cent[d])*(test[d]-cent[d]);
	return r2>rad*rad?0:1; }


/*************** Nearest ****************/

void Geo_NearestSlabPt(double *pt1,double *pt2,double *point,double *ans,int dim) {
	double x,thick;
	int d;
	
	x=thick=0;
	for(d=0;d<dim;d++) {
		x+=(point[d]-pt1[d])*(pt2[d]-pt1[d]);
		thick+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]); }
	x/=thick;
	if(x<0)
		for(d=0;d<dim;d++) ans[d]=point[d]-x*(pt2[d]-pt1[d]);
	else if(x>1)
		for(d=0;d<dim;d++) ans[d]=point[d]+(1.0-x)*(pt2[d]-pt1[d]);
	else
		for(d=0;d<dim;d++) ans[d]=point[d];
	return; }


int Geo_NearestLineSegPt(double *pt1,double *pt2,double *point,double *ans,int dim,double margin) {
	double x,thick;
	int d,edgenum;

	x=thick=0;
	for(d=0;d<dim;d++) {
		x+=(point[d]-pt1[d])*(pt2[d]-pt1[d]);
		thick+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]); }
	x/=thick;
	margin/=sqrt(thick);
	if(x<=margin) {
		for(d=0;d<dim;d++) ans[d]=pt1[d];
		edgenum=1; }
	else if(x>=1-margin) {
		for(d=0;d<dim;d++) ans[d]=pt2[d];
		edgenum=2; }
	else {
		for(d=0;d<dim;d++) ans[d]=pt1[d]+x*(pt2[d]-pt1[d]);
		edgenum=0; }
	return edgenum; }


void Geo_NearestTriPt(double *pt1,double *pt2,double *pt3,double *norm,double *point,double *ans) {
	double dx1,dy1,dz1,dx2,dy2,dz2,crss[3],dot,cross12,cross23,cross31,len2;
	int d,corner;

	dx1=pt2[0]-pt1[0];
	dy1=pt2[1]-pt1[1];
	dz1=pt2[2]-pt1[2];
	dx2=point[0]-pt2[0];
	dy2=point[1]-pt2[1];
	dz2=point[2]-pt2[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross12=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	dx1=pt3[0]-pt2[0];
	dy1=pt3[1]-pt2[1];
	dz1=pt3[2]-pt2[2];
	dx2=point[0]-pt3[0];
	dy2=point[1]-pt3[1];
	dz2=point[2]-pt3[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross23=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	dx1=pt1[0]-pt3[0];
	dy1=pt1[1]-pt3[1];
	dz1=pt1[2]-pt3[2];
	dx2=point[0]-pt1[0];
	dy2=point[1]-pt1[1];
	dz2=point[2]-pt1[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross31=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	corner=0;
	if(cross12>=0 && cross23>=0 && cross31>=0)
		for(d=0;d<3;d++) ans[d]=point[d];
	else if(cross12<0) {
		dx2=pt2[0]-pt1[0];
		dy2=pt2[1]-pt1[1];
		dz2=pt2[2]-pt1[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(point[0]-pt1[0])+dy2*(point[1]-pt1[1])+dz2*(point[2]-pt1[2]);
		dot/=len2;
		if(dot<=0) corner=1;
		else if(dot>=1) corner=2;
		else {
			dx1=norm[0];
			dy1=norm[1];
			dz1=norm[2];
			crss[0]=dy1*dz2-dz1*dy2;
			crss[1]=-dx1*dz2+dz1*dx2;
			crss[2]=dx1*dy2-dy1*dx2;
			cross12/=len2;
			for(d=0;d<3;d++) ans[d]=point[d]-cross12*crss[d]; }}
	else if(cross23<0) {
		dx2=pt3[0]-pt2[0];
		dy2=pt3[1]-pt2[1];
		dz2=pt3[2]-pt2[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(point[0]-pt2[0])+dy2*(point[1]-pt2[1])+dz2*(point[2]-pt2[2]);
		dot/=len2;
		if(dot<=0) corner=2;
		else if(dot>=1) corner=3;
		else {
			dx1=norm[0];
			dy1=norm[1];
			dz1=norm[2];
			crss[0]=dy1*dz2-dz1*dy2;
			crss[1]=-dx1*dz2+dz1*dx2;
			crss[2]=dx1*dy2-dy1*dx2;
			cross23/=dx2*dx2+dy2*dy2+dz2*dz2;
			for(d=0;d<3;d++) ans[d]=point[d]-cross23*crss[d]; }}
	else if(cross31<0) {
		dx2=pt1[0]-pt3[0];
		dy2=pt1[1]-pt3[1];
		dz2=pt1[2]-pt3[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(point[0]-pt3[0])+dy2*(point[1]-pt3[1])+dz2*(point[2]-pt3[2]);
		dot/=len2;
		if(dot<=0) corner=3;
		else if(dot>=1) corner=1;
		else {
			dx1=norm[0];
			dy1=norm[1];
			dz1=norm[2];
			crss[0]=dy1*dz2-dz1*dy2;
			crss[1]=-dx1*dz2+dz1*dx2;
			crss[2]=dx1*dy2-dy1*dx2;
			cross31/=dx2*dx2+dy2*dy2+dz2*dz2;
			for(d=0;d<3;d++) ans[d]=point[d]-cross31*crss[d]; }}

	if(!corner);
	else if(corner==1) {
		dot=(point[0]-pt1[0])*norm[0]+(point[1]-pt1[1])*norm[1]+(point[2]-pt1[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt1[d]+dot*norm[d]; }
	else if(corner==2) {
		dot=(point[0]-pt2[0])*norm[0]+(point[1]-pt2[1])*norm[1]+(point[2]-pt2[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt2[d]+dot*norm[d]; }
	else if(corner==3) {
		dot=(point[0]-pt3[0])*norm[0]+(point[1]-pt3[1])*norm[1]+(point[2]-pt3[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt3[d]+dot*norm[d]; }

	return; }


void Geo_NearestTriPt2(double **point,double **edgenorm,double *norm,double *testpt,double *ans) {
	double *pt1,*pt2,*pt3,*en12,*en23,*en31,dot12,dot23,dot31,dx2,dy2,dz2,len2,dot;
	int d,corner;

	pt1=point[0];
	pt2=point[1];
	pt3=point[2];
	en12=edgenorm[0];
	en23=edgenorm[1];
	en31=edgenorm[2];

	dot12=(testpt[0]-pt1[0])*en12[0]+(testpt[1]-pt1[1])*en12[1]+(testpt[2]-pt1[2])*en12[2];
	dot23=(testpt[0]-pt2[0])*en23[0]+(testpt[1]-pt2[1])*en23[1]+(testpt[2]-pt2[2])*en23[2];
	dot31=(testpt[0]-pt3[0])*en31[0]+(testpt[1]-pt3[1])*en31[1]+(testpt[2]-pt3[2])*en31[2];

	corner=0;
	if(dot12<=0 && dot23<=0 && dot31<=0)
		for(d=0;d<3;d++) ans[d]=testpt[d];

	else if(dot12>0) {
		dx2=pt2[0]-pt1[0];
		dy2=pt2[1]-pt1[1];
		dz2=pt2[2]-pt1[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt1[0])+dy2*(testpt[1]-pt1[1])+dz2*(testpt[2]-pt1[2]);
		if(dot<=0) corner=1;
		else if(dot>=len2) corner=2;
		else
			for(d=0;d<3;d++) ans[d]=testpt[d]-dot12*en12[d]; }

	else if(dot23>0) {
		dx2=pt3[0]-pt2[0];
		dy2=pt3[1]-pt2[1];
		dz2=pt3[2]-pt2[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt2[0])+dy2*(testpt[1]-pt2[1])+dz2*(testpt[2]-pt2[2]);
		if(dot<=0) corner=2;
		else if(dot>=len2) corner=3;
		else
			for(d=0;d<3;d++) ans[d]=testpt[d]-dot23*en23[d]; }

	else if(dot31>0) {
		dx2=pt1[0]-pt3[0];
		dy2=pt1[1]-pt3[1];
		dz2=pt1[2]-pt3[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt3[0])+dy2*(testpt[1]-pt3[1])+dz2*(testpt[2]-pt3[2]);
		if(dot<=0) corner=3;
		else if(dot>=len2) corner=1;
		else
			for(d=0;d<3;d++) ans[d]=testpt[d]-dot31*en31[d]; }

	if(!corner);
	else if(corner==1) {
		dot=(testpt[0]-pt1[0])*norm[0]+(testpt[1]-pt1[1])*norm[1]+(testpt[2]-pt1[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt1[d]+dot*norm[d]; }
	else if(corner==2) {
		dot=(testpt[0]-pt2[0])*norm[0]+(testpt[1]-pt2[1])*norm[1]+(testpt[2]-pt2[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt2[d]+dot*norm[d]; }
	else if(corner==3) {
		dot=(testpt[0]-pt3[0])*norm[0]+(testpt[1]-pt3[1])*norm[1]+(testpt[2]-pt3[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=pt3[d]+dot*norm[d]; }

	return; }


int Geo_NearestTrianglePt(double *pt1,double *pt2,double *pt3,double *norm,double *point,double *ans) {
	double dx1,dy1,dz1,dx2,dy2,dz2,crss[3],dot,cross12,cross23,cross31,len2;
	int d,corner,edgenum;

	edgenum=0;

	dx1=pt2[0]-pt1[0];
	dy1=pt2[1]-pt1[1];
	dz1=pt2[2]-pt1[2];
	dx2=point[0]-pt2[0];
	dy2=point[1]-pt2[1];
	dz2=point[2]-pt2[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross12=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	dx1=pt3[0]-pt2[0];
	dy1=pt3[1]-pt2[1];
	dz1=pt3[2]-pt2[2];
	dx2=point[0]-pt3[0];
	dy2=point[1]-pt3[1];
	dz2=point[2]-pt3[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross23=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	dx1=pt1[0]-pt3[0];
	dy1=pt1[1]-pt3[1];
	dz1=pt1[2]-pt3[2];
	dx2=point[0]-pt1[0];
	dy2=point[1]-pt1[1];
	dz2=point[2]-pt1[2];
	crss[0]=dy1*dz2-dz1*dy2;
	crss[1]=-dx1*dz2+dz1*dx2;
	crss[2]=dx1*dy2-dy1*dx2;
	cross31=crss[0]*norm[0]+crss[1]*norm[1]+crss[2]*norm[2];

	corner=0;
	if(cross12>=0 && cross23>=0 && cross31>=0) {
		dot=(point[0]-pt1[0])*norm[0]+(point[1]-pt1[1])*norm[1]+(point[2]-pt1[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=point[d]-dot*norm[d];
		if(cross12==0) edgenum=1;
		else if(cross23==0) edgenum=2;
		else if(cross31==0) edgenum=3;
		else edgenum=0; }
	else if(cross12<0) {
		dx2=pt2[0]-pt1[0];
		dy2=pt2[1]-pt1[1];
		dz2=pt2[2]-pt1[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=(point[0]-pt1[0])*dx2+(point[1]-pt1[1])*dy2+(point[2]-pt1[2])*dz2;
		dot/=len2;
		if(dot<=0) corner=1;
		else if(dot>=1) corner=2;
		else {
			ans[0]=pt1[0]+dot*dx2;
			ans[1]=pt1[1]+dot*dy2;
			ans[2]=pt1[2]+dot*dz2;
			edgenum=1; }}
	else if(cross23<0) {
		dx2=pt3[0]-pt2[0];
		dy2=pt3[1]-pt2[1];
		dz2=pt3[2]-pt2[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=(point[0]-pt2[0])*dx2+(point[1]-pt2[1])*dy2+(point[2]-pt2[2])*dz2;
		dot/=len2;
		if(dot<=0) corner=2;
		else if(dot>=1) corner=3;
		else {
			ans[0]=pt2[0]+dot*dx2;
			ans[1]=pt2[1]+dot*dy2;
			ans[2]=pt2[2]+dot*dz2;
			edgenum=2; }}
	else if(cross31<0) {
		dx2=pt1[0]-pt3[0];
		dy2=pt1[1]-pt3[1];
		dz2=pt1[2]-pt3[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=(point[0]-pt3[0])*dx2+(point[1]-pt3[1])*dy2+(point[2]-pt3[2])*dz2;
		dot/=len2;
		if(dot<=0) corner=3;
		else if(dot>=1) corner=1;
		else {
			ans[0]=pt3[0]+dot*dx2;
			ans[1]=pt3[1]+dot*dy2;
			ans[2]=pt3[2]+dot*dz2;
			edgenum=3; }}

	if(!corner);
	else if(corner==1) {
		for(d=0;d<3;d++) ans[d]=pt1[d];
		edgenum=1; }
	else if(corner==2) {
		for(d=0;d<3;d++) ans[d]=pt2[d];
		edgenum=2; }
	else if(corner==3) {
		for(d=0;d<3;d++) ans[d]=pt3[d];
		edgenum=3; }

	return edgenum; }


int Geo_NearestTrianglePt2(double **point,double *norm,double *testpt,double *ans,double margin) {
	double *pt1,*pt2,*pt3,*en12,*en23,*en31,dot12,dot23,dot31,dx2,dy2,dz2,len2,dot;
	int d,corner,edgenum;

	edgenum=0;

	pt1=point[0];
	pt2=point[1];
	pt3=point[2];
	en12=point[3];
	en23=point[4];
	en31=point[5];

	dot12=(testpt[0]-pt1[0])*en12[0]+(testpt[1]-pt1[1])*en12[1]+(testpt[2]-pt1[2])*en12[2];
	dot23=(testpt[0]-pt2[0])*en23[0]+(testpt[1]-pt2[1])*en23[1]+(testpt[2]-pt2[2])*en23[2];
	dot31=(testpt[0]-pt3[0])*en31[0]+(testpt[1]-pt3[1])*en31[1]+(testpt[2]-pt3[2])*en31[2];

	corner=0;
	if(dot12<=0 && dot23<=0 && dot31<=0) {
		dot=(testpt[0]-pt1[0])*norm[0]+(testpt[1]-pt1[1])*norm[1]+(testpt[2]-pt1[2])*norm[2];
		for(d=0;d<3;d++) ans[d]=testpt[d]-dot*norm[d];
		if(dot12>-margin) edgenum=1;
		else if(dot23>-margin) edgenum=2;
		else if(dot31>-margin) edgenum=3;
		else edgenum=0; }

	else if(dot12>0) {
		dx2=pt2[0]-pt1[0];
		dy2=pt2[1]-pt1[1];
		dz2=pt2[2]-pt1[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt1[0])+dy2*(testpt[1]-pt1[1])+dz2*(testpt[2]-pt1[2]);
		if(dot<=0) corner=1;
		else if(dot>=len2) corner=2;
		else {
			dot/=len2;
			ans[0]=pt1[0]+dot*dx2;
			ans[1]=pt1[1]+dot*dy2;
			ans[2]=pt1[2]+dot*dz2;
			edgenum=1; }}

	else if(dot23>0) {
		dx2=pt3[0]-pt2[0];
		dy2=pt3[1]-pt2[1];
		dz2=pt3[2]-pt2[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt2[0])+dy2*(testpt[1]-pt2[1])+dz2*(testpt[2]-pt2[2]);
		if(dot<=0) corner=2;
		else if(dot>=len2) corner=3;
		else {
			dot/=len2;
			ans[0]=pt2[0]+dot*dx2;
			ans[1]=pt2[1]+dot*dy2;
			ans[2]=pt2[2]+dot*dz2;
			edgenum=2; }}

	else if(dot31>0) {
		dx2=pt1[0]-pt3[0];
		dy2=pt1[1]-pt3[1];
		dz2=pt1[2]-pt3[2];
		len2=dx2*dx2+dy2*dy2+dz2*dz2;
		dot=dx2*(testpt[0]-pt3[0])+dy2*(testpt[1]-pt3[1])+dz2*(testpt[2]-pt3[2]);
		if(dot<=0) corner=3;
		else if(dot>=len2) corner=1;
		else {
			dot/=len2;
			ans[0]=pt3[0]+dot*dx2;
			ans[1]=pt3[1]+dot*dy2;
			ans[2]=pt3[2]+dot*dz2;
			edgenum=3; }}

	if(!corner);
	else if(corner==1) {
		for(d=0;d<3;d++) ans[d]=pt1[d];
		edgenum=1; }
	else if(corner==2) {
		for(d=0;d<3;d++) ans[d]=pt2[d];
		edgenum=2; }
	else if(corner==3) {
		for(d=0;d<3;d++) ans[d]=pt3[d];
		edgenum=3; }

	return edgenum; }


double Geo_NearestSpherePt(double *cent,double rad,int front,int dim,double *point,double *ans) {
	double dist,vect[3];
	int d;

	dist=Geo_SphereNormal(cent,point,front,dim,vect);
	for(d=0;d<dim;d++) ans[d]=cent[d]+rad*vect[d];
	return front*(dist-rad); }


void Geo_NearestRingPt(double *cent,double *axis,double rad,int dim,double *point,double *ans) {
	int d;
	double dot,vect[3];

	dot=0;
	for(d=0;d<dim;d++) {
		vect[d]=point[d]-cent[d];
		dot+=vect[d]*axis[d]; }
	for(d=0;d<dim;d++) vect[d]-=dot*axis[d];
	dot=0;
	for(d=0;d<dim;d++) dot+=vect[d]*vect[d];
	dot=rad/sqrt(dot);
	for(d=0;d<dim;d++) ans[d]=cent[d]+dot*vect[d];
	return; }


void Geo_NearestCylPt(double *pt1,double *axis,double rad,int dim,double *point,double *ans) {
	int d;
	double dot,vect[3];

	dot=0;
	for(d=0;d<dim;d++) {
		vect[d]=point[d]-pt1[d];									// vect points from pt1 to point
		dot+=vect[d]*axis[d]; }										// dot is dot product of vect with axis
	for(d=0;d<dim;d++) vect[d]-=dot*axis[d];		// vect is now from cylinder axis to point
	dot=0;
	for(d=0;d<dim;d++) dot+=vect[d]*vect[d];		// dot is now squared length of new vect
	if(dot<=rad*rad)
		for(d=0;d<dim;d++) ans[d]=point[d];				// if point is inside cylinder, then answer is that point
	else {
		dot=sqrt(dot);														// dot is now length of new vect
		dot=1.0-rad/dot;													// dot is now subtraction factor
		for(d=0;d<dim;d++) ans[d]=point[d]-dot*vect[d]; }		// find point on cylinder surface closest to point
	return; }


int Geo_NearestCylinderPt(double *pt1,double *pt2,double rad,int dim,double *point,double *ans,double margin) {
	int d,edgenum;
	double dot,len,len2,vect[3];

	dot=len=0;
	for(d=0;d<dim;d++) {
		vect[d]=point[d]-pt1[d];									// vect points from pt1 to point
		dot+=vect[d]*(pt2[d]-pt1[d]);							// dot is dot product of vect with cylinder axis
		len+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]); }		// len is squared cylinder length
	dot/=len;																		// dot is now length of vect along cyl. axis, divided by the axis length
	margin/=sqrt(len);
	for(d=0;d<dim;d++) vect[d]-=dot*(pt2[d]-pt1[d]);		// vect now points from cyl. axis to point
	if(dot<=margin) {dot=0;edgenum=1;}
	else if(dot>=1-margin) {dot=1;edgenum=2;}
	else edgenum=0;
	len2=0;
	for(d=0;d<dim;d++) len2+=vect[d]*vect[d];			// len2 is squared length of distance from cyl. axis to point
	len2=rad/sqrt(len2);													// len2 is ratio of radius to length from cyl. axis to point
	for(d=0;d<dim;d++) ans[d]=pt1[d]+dot*(pt2[d]-pt1[d])+len2*vect[d];
	return edgenum; }


int Geo_NearestDiskPt(double *cent,double *axis,double rad,int dim,double *point,double *ans,double margin) {
	int d,edgenum;
	double dot,vect[3];

	dot=0;
	for(d=0;d<dim;d++) {
		vect[d]=point[d]-cent[d];										// vect points from center to point
		dot+=vect[d]*axis[d]; }											// dot is dot product of vect with axis
	for(d=0;d<dim;d++) vect[d]-=dot*axis[d];			// vect now points from center to point projected to infinite disk
	dot=0;
	for(d=0;d<dim;d++) dot+=vect[d]*vect[d];			// dot is now squared length of vect
	dot=sqrt(dot);																// dot is now length of vect
	if(dot>=rad-margin) {
		dot=rad/dot;
		edgenum=1; }
	else {
		dot=1;
		edgenum=0; }
	for(d=0;d<dim;d++) ans[d]=cent[d]+dot*vect[d];
	return edgenum; }


double Geo_NearestLine2LineDist(double *ptA1,double *ptA2,double *ptB1,double *ptB2) {
	double a,b,c,d,e,denom,sc,tc,dist,vect[3];

	a=(ptA2[0]-ptA1[0])*(ptA2[0]-ptA1[0])+(ptA2[1]-ptA1[1])*(ptA2[1]-ptA1[1])+(ptA2[2]-ptA1[2])*(ptA2[2]-ptA1[2]);
	b=(ptA2[0]-ptA1[0])*(ptB2[0]-ptB1[0])+(ptA2[1]-ptA1[1])*(ptB2[1]-ptB1[1])+(ptA2[2]-ptA1[2])*(ptB2[2]-ptB1[2]);
	c=(ptB2[0]-ptB1[0])*(ptB2[0]-ptB1[0])+(ptB2[1]-ptB1[1])*(ptB2[1]-ptB1[1])+(ptB2[2]-ptB1[2])*(ptB2[2]-ptB1[2]);
	d=(ptA2[0]-ptA1[0])*(ptA1[0]-ptB1[0])+(ptA2[1]-ptA1[1])*(ptA1[1]-ptB1[1])+(ptA2[2]-ptA1[2])*(ptA1[2]-ptB1[2]);
	e=(ptB2[0]-ptB1[0])*(ptA1[0]-ptB1[0])+(ptB2[1]-ptB1[1])*(ptA1[1]-ptB1[1])+(ptB2[2]-ptB1[2])*(ptA1[2]-ptB1[2]);
	denom=a*c-b*b;
	if(denom<EPSILON) {
		sc=0.0;
		tc=b>c?d/b:e/c; }
	else {
		sc=(b*e-c*d)/denom;
		tc=(a*e-b*d)/denom; }
	vect[0]=ptA1[0]+sc*(ptA2[0]-ptA1[0])-ptB1[0]-tc*(ptB2[0]-ptB1[0]);
	vect[1]=ptA1[1]+sc*(ptA2[1]-ptA1[1])-ptB1[1]-tc*(ptB2[1]-ptB1[1]);
	vect[2]=ptA1[2]+sc*(ptA2[2]-ptA1[2])-ptB1[2]-tc*(ptB2[2]-ptB1[2]);
	dist=sqrt(vect[0]*vect[0]+vect[1]*vect[1]+vect[2]*vect[2]);
	return dist; }


double Geo_NearestSeg2SegDist(double *ptA1,double *ptA2,double *ptB1,double *ptB2) {
	double a,b,c,d,e,denom,sc,tc,dist,vect[3];
	double sn,tn,sd,td;

	a=(ptA2[0]-ptA1[0])*(ptA2[0]-ptA1[0])+(ptA2[1]-ptA1[1])*(ptA2[1]-ptA1[1])+(ptA2[2]-ptA1[2])*(ptA2[2]-ptA1[2]);
	b=(ptA2[0]-ptA1[0])*(ptB2[0]-ptB1[0])+(ptA2[1]-ptA1[1])*(ptB2[1]-ptB1[1])+(ptA2[2]-ptA1[2])*(ptB2[2]-ptB1[2]);
	c=(ptB2[0]-ptB1[0])*(ptB2[0]-ptB1[0])+(ptB2[1]-ptB1[1])*(ptB2[1]-ptB1[1])+(ptB2[2]-ptB1[2])*(ptB2[2]-ptB1[2]);
	d=(ptA2[0]-ptA1[0])*(ptA1[0]-ptB1[0])+(ptA2[1]-ptA1[1])*(ptA1[1]-ptB1[1])+(ptA2[2]-ptA1[2])*(ptA1[2]-ptB1[2]);
	e=(ptB2[0]-ptB1[0])*(ptA1[0]-ptB1[0])+(ptB2[1]-ptB1[1])*(ptA1[1]-ptB1[1])+(ptB2[2]-ptB1[2])*(ptA1[2]-ptB1[2]);
	denom=a*c-b*b;
	sd=denom;
	td=denom;
	if(denom<EPSILON) {
		sn=0.0;
		sd=1.0;
		tn=e;
		td=c; }
	else {
		sn=b*e-c*d;
		tn=a*e-b*d;
		if(sn<0.0) {
			sn=0.0;
			tn=e;
			td=c; }
		else if(sn>sd) {
			sn=sd;
			tn=e+b;
			td=c; }}

	if(tn<0.0) {
		tn=0.0;
		if(-d<0.0) sn=0.0;
		else if(-d>a) sn=sd;
		else {
			sn=-d;
			sd=a; }}
	else if(tn>td) {
		tn=td;
		if((-d+b)<0.0) sn=0;
		else if((-d+b)>a) sn=sd;
		else {
			sn=(-d+b);
			sd=a; }}
	
	sc=(fabs(sn)<EPSILON?0.0:sn/sd);
	tc=(fabs(tn)<EPSILON?0.0:tn/td);
	vect[0]=ptA1[0]+sc*(ptA2[0]-ptA1[0])-ptB1[0]-tc*(ptB2[0]-ptB1[0]);
	vect[1]=ptA1[1]+sc*(ptA2[1]-ptA1[1])-ptB1[1]-tc*(ptB2[1]-ptB1[1]);
	vect[2]=ptA1[2]+sc*(ptA2[2]-ptA1[2])-ptB1[2]-tc*(ptB2[2]-ptB1[2]);
	dist=sqrt(vect[0]*vect[0]+vect[1]*vect[1]+vect[2]*vect[2]);

	return dist; }


double Geo_NearestAabbPt(const double *bpt1,const double *bpt2,int dim,const double *point,double *ans) {
	int d;
	double dist;

	dist=0;
	for(d=0;d<dim;d++) {
		if(point[d]<=bpt1[d]) ans[d]=bpt1[d];
		else if(point[d]>=bpt2[d]) ans[d]=bpt2[d];
		else ans[d]=point[d];
		dist+=(ans[d]-point[d])*(ans[d]-point[d]); }
	return sqrt(dist); }


/*************** To Rect ****************/

void Geo_Semic2Rect(double *cent,double rad,double *outvect,double *r1,double *r2,double *r3) {
	r1[0]=cent[0]+rad*outvect[1];
	r1[1]=cent[1]-rad*outvect[0];
	r2[0]=cent[0]-rad*outvect[1];
	r2[1]=cent[1]+rad*outvect[0];
	r3[0]=cent[0]+rad*outvect[1]-rad*outvect[0];
	r3[1]=cent[1]-rad*outvect[0]-rad*outvect[1];
	return; }


void Geo_Hemis2Rect(double *cent,double rad,double *outvect,double *r1,double *r2,double *r3,double *r4) {
	double v1[3],v2[3];

	v2[0]=v2[1]=v2[2]=0;
	Geo_LineNormal3D(v2,outvect,v2,v1);
	v2[0]=v1[1]*outvect[2]-v1[2]*outvect[1];		// v2 = v1 x outvect
	v2[1]=v1[2]*outvect[0]-v1[0]*outvect[2];
	v2[2]=v1[0]*outvect[1]-v1[1]*outvect[0];

	r1[0]=cent[0]+rad*(-v1[0]-v2[0]);
	r1[1]=cent[1]+rad*(-v1[1]-v2[1]);
	r1[2]=cent[2]+rad*(-v1[2]-v2[2]);
	r2[0]=cent[0]+rad*(+v1[0]-v2[0]);
	r2[1]=cent[1]+rad*(+v1[1]-v2[1]);
	r2[2]=cent[2]+rad*(+v1[2]-v2[2]);
	r3[0]=cent[0]+rad*(-v1[0]+v2[0]);
	r3[1]=cent[1]+rad*(-v1[1]+v2[1]);
	r3[2]=cent[2]+rad*(-v1[2]+v2[2]);
	r4[0]=cent[0]+rad*(-v1[0]-v2[0]-outvect[0]);
	r4[1]=cent[1]+rad*(-v1[1]-v2[1]-outvect[1]);
	r4[2]=cent[2]+rad*(-v1[2]-v2[2]-outvect[2]);
	return; }


void Geo_Cyl2Rect(double *pt1,double *pt2,double rad,double *r1,double *r2,double *r3,double *r4) {
	double v1[3],v2[3],nrm;

	Geo_LineNormal3D(pt1,pt2,pt1,v1);
	v2[0]=v1[1]*(pt2[2]-pt1[2])-v1[2]*(pt2[1]-pt1[1]);		// v2 = v1 x (pt2 - pt1)
	v2[1]=v1[2]*(pt2[0]-pt1[0])-v1[0]*(pt2[2]-pt1[2]);
	v2[2]=v1[0]*(pt2[1]-pt1[1])-v1[1]*(pt2[0]-pt1[0]);
	nrm=sqrt(v2[0]*v2[0]+v2[1]*v2[1]+v2[2]*v2[2]);        // normalize v2
	v2[0]/=nrm;
	v2[1]/=nrm;
	v2[2]/=nrm;

	r1[0]=pt1[0]+rad*(-v1[0]-v2[0]);
	r1[1]=pt1[1]+rad*(-v1[1]-v2[1]);
	r1[2]=pt1[2]+rad*(-v1[2]-v2[2]);
	r2[0]=pt1[0]+rad*(+v1[0]-v2[0]);
	r2[1]=pt1[1]+rad*(+v1[1]-v2[1]);
	r2[2]=pt1[2]+rad*(+v1[2]-v2[2]);
	r3[0]=pt1[0]+rad*(-v1[0]+v2[0]);
	r3[1]=pt1[1]+rad*(-v1[1]+v2[1]);
	r3[2]=pt1[2]+rad*(-v1[2]+v2[2]);
	r4[0]=pt2[0]+rad*(-v1[0]-v2[0]);
	r4[1]=pt2[1]+rad*(-v1[1]-v2[1]);
	r4[2]=pt2[2]+rad*(-v1[2]-v2[2]);
	return; }


/**************** ...X...  ***************/

double Geo_LineXLine(double *l1p1,double *l1p2,double *l2p1,double *l2p2,double *crss2ptr) {
	double rxcx,rycy,sxrx,syry,dxcx,dycy,cross;

	rxcx=l2p1[0]-l1p1[0];		// rx-cx
	rycy=l2p1[1]-l1p1[1];		// ry-cy
	sxrx=l2p2[0]-l2p1[0];		// sx-rx
	syry=l2p2[1]-l2p1[1];		// sy-ry
	dxcx=l1p2[0]-l1p1[0];		// dx-cx
	dycy=l1p2[1]-l1p1[1];		// dy-cy
	cross=(rxcx*syry-rycy*sxrx)/(dxcx*syry-dycy*sxrx);
	if(crss2ptr) *crss2ptr=(rxcx*dycy-rycy*dxcx)/(dxcx*syry-dycy*sxrx);
	return cross; }


double Geo_LineXPlane(double *pt1,double *pt2,double *v,double *norm,double *crsspt) {
	double lx,ly,lz,sx,sy,sz,a;

	lx=pt2[0]-pt1[0];
	ly=pt2[1]-pt1[1];
	lz=pt2[2]-pt1[2];

	sx=v[0]-pt1[0];
	sy=v[1]-pt1[1];
	sz=v[2]-pt1[2];

	a=(sx*norm[0]+sy*norm[1]+sz*norm[2])/(lx*norm[0]+ly*norm[1]+lz*norm[2]);

	crsspt[0]=pt1[0]+a*lx;
	crsspt[1]=pt1[1]+a*ly;
	crsspt[2]=pt1[2]+a*lz;

	return a; }


double Geo_LineXSphs(double *pt1,double *pt2,double *cent,double rad,int dim,double *crss2ptr,double *nrdistptr,double *nrposptr) {
	double a,b,c,cross,cross2,radical,nrdist;
	int d;

	a=b=c=0;
	for(d=0;d<dim;d++) {
		a+=(pt2[d]-pt1[d])*(pt2[d]-pt1[d]);
		b+=(cent[d]-pt1[d])*(pt2[d]-pt1[d]);
		c+=(pt1[d]-cent[d])*(pt1[d]-cent[d]); }
	nrdist=sqrt(c-b*b/a);
	if(nrdistptr) *nrdistptr=nrdist;
	if(nrposptr) *nrposptr=b/a;
	b*=-2;
	c-=rad*rad;
	radical=b*b-4*a*c;
	if(nrdist<=rad && radical<0) radical=0;
	radical=sqrt(radical);
	cross=(-b-radical)/(2*a);
	cross2=(-b+radical)/(2*a);
	if(crss2ptr) *crss2ptr=cross2;
	return cross; }


double Geo_LineXCyl2s(double *pt1,double *pt2,double *cp1,double *cp2,double *norm,double rad,double *crss2ptr,double *nrdistptr,double *nrposptr) {
	double edge0[2],edge1[2],crossl,crossr,nrpos,nrdist;

	edge0[0]=cp1[0]+norm[0]*rad;
	edge0[1]=cp1[1]+norm[1]*rad;
	edge1[0]=cp2[0]+norm[0]*rad;
	edge1[1]=cp2[1]+norm[1]*rad;
	crossr=Geo_LineXLine(pt1,pt2,edge0,edge1,NULL);
	edge0[0]=cp1[0]-norm[0]*rad;
	edge0[1]=cp1[1]-norm[1]*rad;
	edge1[0]=cp2[0]-norm[0]*rad;
	edge1[1]=cp2[1]-norm[1]*rad;
	crossl=Geo_LineXLine(pt1,pt2,edge0,edge1,NULL);
	nrpos=Geo_LineXLine(pt1,pt2,cp1,cp2,NULL);
	if(!(nrpos>=0 || nrpos<0)) Geo_LineNormPos(cp1,cp2,pt1,2,&nrdist);
	else nrdist=0;
	if(nrdistptr) *nrdistptr=nrdist;
	if(nrposptr) *nrposptr=nrpos;
	if(crossr<crossl) {
		if(crss2ptr) *crss2ptr=crossl;
		return crossr; }
	if(crss2ptr) *crss2ptr=crossr;
	return crossl; }


double Geo_LineXCyls(double *pt1,double *pt2,double *cp1,double *cp2,double rad,double *crss2ptr,double *nrdistptr,double *nrposptr) {
	double r1,r2,norm1[3],norm2[3],zero[3];
	int d;

	r1=Geo_LineNormal3D(cp1,cp2,pt1,norm1);
	r2=Geo_LineNormal3D(cp1,cp2,pt2,norm2);
	for(d=0;d<3;d++) {
		zero[d]=0;
		norm1[d]*=r1;
		norm2[d]*=r2; }
	return Geo_LineXSphs(norm1,norm2,zero,rad,3,crss2ptr,nrdistptr,nrposptr); }


/**************** Reflection functions *************/


double Geo_SphereReflectSphere(const double *a0,const double *a1,const double *b0,const double *b1,int dim,double radius2,double *a1p,double *b1p) {
	double a,b,c,p,f1,f2,deltap[3];
	int d;

	a=b=c=0;
	for(d=0;d<dim;d++) {
		f1=b1[d]-a1[d]-b0[d]+a0[d];
		f2=b0[d]-a0[d];
		a+=f1*f1;
		b+=2*f2*f1;
		c+=f2*f2; }
	c-=radius2;
	p=(-b-sqrt(b*b-4*a*c))/(2*a);

	f1=f2=0;
	for(d=0;d<dim;d++) {
		deltap[d]=(1-p)*(b0[d]-a0[d])+p*(b1[d]-a1[d]);
		f1+=(a1[d]-a0[d])*deltap[d];
		f2+=(b1[d]-b0[d])*deltap[d]; }
	f1*=2*(1-p)/radius2;
	f2*=2*(1-p)/radius2;
	for(d=0;d<dim;d++) {
		a1p[d]=a1[d]-f1*deltap[d];
		b1p[d]=b1[d]-f2*deltap[d]; }

	return p; }


/******************* Exit functions ****************/


double Geo_LineExitRect(double *pt1,double *pt2,double *front,double *corner1,double *corner3,double *exitpt,int *exitside) {
	int indx1,indx2,xside,yside,side;
	double x1,y1,x2,y2,xscale,yscale,a1,a2,a3,a4,ax,ay,a;

	indx1=(int)front[2];								// set indx1 to axis parallel to edge from pt 0 to pt 1
	indx2=(indx1+1)%3;						// set indx2 to axis parallel to edge from pt 1 to pt 2
	if(indx2==(int)front[1]) indx2=(indx2+1)%3;

	x1=pt1[indx1];									// coordinates of the line ends
	y1=pt1[indx2];									// x and y actually are x and y axes if axis is 2 and different otherwise
	x2=pt2[indx1];
	y2=pt2[indx2];

	xscale=(x2!=x1)?1.0/(x2-x1):INFINITY;
	yscale=(y2!=y1)?1.0/(y2-y1):INFINITY;

	a1=(corner1[indx2]-y1)*yscale;
	a2=(corner3[indx1]-x1)*xscale;
	a3=(corner3[indx2]-y1)*yscale;
	a4=(corner1[indx1]-x1)*xscale;

	if(a1>a3) {ay=a1;yside=1;}
	else {ay=a3;yside=3;}

	if(a2>a4) {ax=a2;xside=2;}
	else {ax=a4;xside=4;}

	if((ay<ax && y1!=y2) || x1==x2) {a=ay;side=yside;}
	else {a=ax;side=xside;}

	exitpt[0]=pt1[0]+a*(pt2[0]-pt1[0]);
	exitpt[1]=pt1[1]+a*(pt2[1]-pt1[1]);
	exitpt[2]=pt1[2]+a*(pt2[2]-pt1[2]);

	*exitside=side;

	return a; }


double Geo_LineExitLine2(double *pt1,double *pt2,double *end1,double *end2,double *exitpt,int *exitend) {
	int i1;
	double a,a1,a2;

	i1=(fabs(pt2[1]-pt1[1])>fabs(pt2[0]-pt1[0]))?1:0;			// which axis to work with
	a1=(end1[i1]-pt1[i1])/(pt2[i1]-pt1[i1]);
	a2=(end2[i1]-pt1[i1])/(pt2[i1]-pt1[i1]);

	if(a2>a1) {													// exit through pt2 end
		exitpt[0]=end2[0];
		exitpt[1]=end2[1];
		a=a2;
		*exitend=2; }
	else {															// exit through pt1 end
		exitpt[0]=end1[0];
		exitpt[1]=end1[1];
		a=a1;
		*exitend=1; }

	return a; }


void Geo_LineExitArc2(double *pt1,double *pt2,double *cent,double radius,double *norm,double *exitpt,int *exitend) {
	double zcross;

	zcross=(pt1[0]-cent[0])*(pt2[1]-pt1[1])-(pt1[1]-cent[1])*(pt2[0]-pt1[0]);

	if(zcross<0) {
		exitpt[0]=cent[0]-radius*norm[1];
		exitpt[1]=cent[1]+radius*norm[0];
		*exitend=1; }
	else {
		exitpt[0]=cent[0]+radius*norm[1];
		exitpt[1]=cent[1]-radius*norm[0];
		*exitend=2; }

	return; }


double Geo_LineExitTriangle(double *pt1,double *pt2,double *normal,double *v1,double *v2,double *v3,double *exitpt,int *exitside) {
	double edge12[3],edge23[3],edge31[3],line[3],outnorm12[3],outnorm23[3],outnorm31[3],dot12,dot23,dot31;
	double a,a12,a23,a31,exitpt12[3],exitpt23[3],exitpt31[3],*exitptr;

	edge12[0]=v2[0]-v1[0];
	edge12[1]=v2[1]-v1[1];
	edge12[2]=v2[2]-v1[2];
	edge23[0]=v3[0]-v2[0];
	edge23[1]=v3[1]-v2[1];
	edge23[2]=v3[2]-v2[2];
	edge31[0]=v1[0]-v3[0];
	edge31[1]=v1[1]-v3[1];
	edge31[2]=v1[2]-v3[2];

	line[0]=pt2[0]-pt1[0];
	line[1]=pt2[1]-pt1[1];
	line[2]=pt2[2]-pt1[2];

	outnorm12[0]=edge12[1]*normal[2]-edge12[2]*normal[1];
	outnorm12[1]=edge12[2]*normal[0]-edge12[0]*normal[2];
	outnorm12[2]=edge12[0]*normal[1]-edge12[1]*normal[0];
	dot12=outnorm12[0]*line[0]+outnorm12[1]*line[1]+outnorm12[2]*line[2];

	outnorm23[0]=edge23[1]*normal[2]-edge23[2]*normal[1];
	outnorm23[1]=edge23[2]*normal[0]-edge23[0]*normal[2];
	outnorm23[2]=edge23[0]*normal[1]-edge23[1]*normal[0];
	dot23=outnorm23[0]*line[0]+outnorm23[1]*line[1]+outnorm23[2]*line[2];

	outnorm31[0]=edge31[1]*normal[2]-edge31[2]*normal[1];
	outnorm31[1]=edge31[2]*normal[0]-edge31[0]*normal[2];
	outnorm31[2]=edge31[0]*normal[1]-edge31[1]*normal[0];
	dot31=outnorm31[0]*line[0]+outnorm31[1]*line[1]+outnorm31[2]*line[2];

	a12=(dot12>0)?Geo_LineXPlane(pt1,pt2,v1,outnorm12,exitpt12):INFINITY;
	a23=(dot23>0)?Geo_LineXPlane(pt1,pt2,v2,outnorm23,exitpt23):INFINITY;
	a31=(dot31>0)?Geo_LineXPlane(pt1,pt2,v3,outnorm31,exitpt31):INFINITY;

	if(a12<a23) {
		a=a12;
		exitptr=exitpt12;
		*exitside=1; }
	else {
		a=a23;
		exitptr=exitpt23;
		*exitside=2; }
	if(a31<a) {
		a=a31;
		exitptr=exitpt31;
		*exitside=3; }

	exitpt[0]=exitptr[0];
	exitpt[1]=exitptr[1];
	exitpt[2]=exitptr[2];

	return a; }


double Geo_LineExitTriangle2(double *pt1,double *pt2,double **point,double *exitpt,int *exitside) {
	double *v1,*v2,*v3,*outnorm12,*outnorm23,*outnorm31,line[3];
	double dot12,dot23,dot31,a12,a23,a31,a;

	v1=point[0];							// v1 to v3 are triangle vertices
	v2=point[1];
	v3=point[2];
	outnorm12=point[3];				// outnorms are the outward pointing normals of the edges
	outnorm23=point[4];
	outnorm31=point[5];

	line[0]=pt2[0]-pt1[0];		// line is the vector from pt1 to pt2
	line[1]=pt2[1]-pt1[1];
	line[2]=pt2[2]-pt1[2];

	dot12=outnorm12[0]*line[0]+outnorm12[1]*line[1]+outnorm12[2]*line[2];		// >0 if line leaves through this edge
	dot23=outnorm23[0]*line[0]+outnorm23[1]*line[1]+outnorm23[2]*line[2];
	dot31=outnorm31[0]*line[0]+outnorm31[1]*line[1]+outnorm31[2]*line[2];

	a12=(dot12>0)?((v1[0]-pt1[0])*outnorm12[0]+(v1[1]-pt1[1])*outnorm12[1]+(v1[2]-pt1[2])*outnorm12[2])/dot12:INFINITY;
	a23=(dot23>0)?((v2[0]-pt1[0])*outnorm23[0]+(v2[1]-pt1[1])*outnorm23[1]+(v2[2]-pt1[2])*outnorm23[2])/dot23:INFINITY;
	a31=(dot31>0)?((v3[0]-pt1[0])*outnorm31[0]+(v3[1]-pt1[1])*outnorm31[1]+(v3[2]-pt1[2])*outnorm31[2])/dot31:INFINITY;

	if(a12<a23) {
		a=a12;
		*exitside=1; }
	else {
		a=a23;
		*exitside=2; }
	if(a31<a) {
		a=a31;
		*exitside=3; }

	exitpt[0]=pt1[0]+a*line[0];
	exitpt[1]=pt1[1]+a*line[1];
	exitpt[2]=pt1[2]+a*line[2];

	return a; }


double Geo_LineExitSphere(double *pt1,double *pt2,double *cent,double rad,double *exitpt) {
	double a,b,c,pt12,ptc1,cross,radical;

	a=b=c=0;
	pt12=pt2[0]-pt1[0];
	ptc1=cent[0]-pt1[0];
	a+=pt12*pt12;
	b+=ptc1*pt12;
	c+=ptc1*ptc1;

	pt12=pt2[1]-pt1[1];
	ptc1=cent[1]-pt1[1];
	a+=pt12*pt12;
	b+=ptc1*pt12;
	c+=ptc1*ptc1;

	pt12=pt2[2]-pt1[2];
	ptc1=cent[2]-pt1[2];
	a+=pt12*pt12;
	b+=ptc1*pt12;
	c+=ptc1*ptc1;

	b*=-2;
	c-=rad*rad;
	radical=b*b-4*a*c;
	radical=(radical<=0)?0:sqrt(radical);
	cross=(-b+radical)/(2*a);

	exitpt[0]=pt1[0]+cross*(pt2[0]-pt1[0]);
	exitpt[1]=pt1[1]+cross*(pt2[1]-pt1[1]);
	exitpt[2]=pt1[2]+cross*(pt2[2]-pt1[2]);

	return cross; }


void Geo_LineExitHemisphere(double *pt1,double *pt2,double *cent,double rad,double *normal,double *exitpt) {
	double crsspt[3],vect[3];

	Geo_LineXPlane(pt1,pt2,cent,normal,crsspt);				// find point where line crosses hemisphere plane
	Geo_SphereNormal(cent,crsspt,1,3,vect);						// find sphere normal that goes through crsspt
	exitpt[0]=cent[0]+rad*vect[0];
	exitpt[1]=cent[1]+rad*vect[1];
	exitpt[2]=cent[2]+rad*vect[2];

	return; }


double Geo_LineExitCylinder(double *pt1,double *pt2,double *end1,double *end2,double rad,double *exitpt,int *exitend) {
	double axis[3],dot,*exitcenter,a,crsspt[3];

	axis[0]=end2[0]-end1[0];							// vector for the cylinder axis
	axis[1]=end2[1]-end1[1];
	axis[2]=end2[2]-end1[2];

	dot=(pt2[0]-pt1[0])*axis[0]+(pt2[1]-pt1[1])*axis[1]+(pt2[2]-pt1[2])*axis[2];			// dot product of the line with the cylinder axis
	if(dot>0) {
		exitcenter=end2;
		*exitend=2; }
	else {
		exitcenter=end1;
		*exitend=1; }

	a=Geo_LineXPlane(pt1,pt2,exitcenter,axis,crsspt);
	Geo_NearestCylinderPt(end1,end2,rad,3,crsspt,exitpt,0);

	return a; }


/******************* ...Xaabb ****************/

int Geo_LineXaabb2(double *pt1,double *pt2,double *norm,double *bpt1,double *bpt2) {
	double cmp,b1,b2,b3,b4;

	if(pt1[0]<bpt1[0] && pt2[0]<bpt1[0]) return 0;
	if(pt1[0]>bpt2[0] && pt2[0]>bpt2[0]) return 0;
	if(pt1[1]<bpt1[1] && pt2[1]<bpt1[1]) return 0;
	if(pt1[1]>bpt2[1] && pt2[1]>bpt2[1]) return 0;
	cmp=norm[0]*pt1[0]+norm[1]*pt1[1];
	b1=norm[0]*bpt1[0]+norm[1]*bpt1[1];
	b2=norm[0]*bpt1[0]+norm[1]*bpt2[1];
	b3=norm[0]*bpt2[0]+norm[1]*bpt1[1];
	b4=norm[0]*bpt2[0]+norm[1]*bpt2[1];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp) return 0;
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp) return 0;
	return 1; }


int Geo_LineXaabb(double *pt1,double *pt2,double *bpt1,double *bpt2,int dim,int infline) {
	int d;
	double nearhi,farlo,denom,a1,a2;

	nearhi=-1.0;
	farlo=2.0;
	for(d=0;d<dim;d++) {
		denom=pt2[d]-pt1[d];
		if(denom==0) {
			if(pt1[d]<bpt1[d] || pt1[d]>bpt2[d]) return 0; }
		else {
			a1=(bpt1[d]-pt1[d])/denom;
			a2=(bpt2[d]-pt1[d])/denom;
			if(a1<a2) {
				if(a1>nearhi) nearhi=a1;
				if(a2<farlo) farlo=a2; }
			else {
				if(a2>nearhi) nearhi=a2;
				if(a1<farlo) farlo=a1; }}}

	if(infline) return(nearhi<=farlo);
	else return(nearhi<=farlo && nearhi<=1 && farlo>=0); }


int Geo_TriXaabb3(double *pt1,double *pt2,double *pt3,double *norm,double *bpt1,double *bpt2) {
	double cmp,b1,b2,b3,b4,b5,b6,b7,b8;

	if(pt1[0]<bpt1[0] && pt2[0]<bpt1[0] && pt3[0]<bpt1[0]) return 0;
	if(pt1[0]>bpt2[0] && pt2[0]>bpt2[0] && pt3[0]>bpt2[0]) return 0;
	if(pt1[1]<bpt1[1] && pt2[1]<bpt1[1] && pt3[1]<bpt1[1]) return 0;
	if(pt1[1]>bpt2[1] && pt2[1]>bpt2[1] && pt3[1]>bpt2[1]) return 0;
	if(pt1[2]<bpt1[2] && pt2[2]<bpt1[2] && pt3[2]<bpt1[2]) return 0;
	if(pt1[2]>bpt2[2] && pt2[2]>bpt2[2] && pt3[2]>bpt2[2]) return 0;
	cmp=norm[0]*pt1[0]+norm[1]*pt1[1]+norm[2]*pt1[2];
	b1=norm[0]*bpt1[0]+norm[1]*bpt1[1]+norm[2]*bpt1[2];
	b2=norm[0]*bpt1[0]+norm[1]*bpt1[1]+norm[2]*bpt2[2];
	b3=norm[0]*bpt1[0]+norm[1]*bpt2[1]+norm[2]*bpt1[2];
	b4=norm[0]*bpt1[0]+norm[1]*bpt2[1]+norm[2]*bpt2[2];
	b5=norm[0]*bpt2[0]+norm[1]*bpt1[1]+norm[2]*bpt1[2];
	b6=norm[0]*bpt2[0]+norm[1]*bpt1[1]+norm[2]*bpt2[2];
	b7=norm[0]*bpt2[0]+norm[1]*bpt2[1]+norm[2]*bpt1[2];
	b8=norm[0]*bpt2[0]+norm[1]*bpt2[1]+norm[2]*bpt2[2];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0;
	return 1; }


int Geo_RectXaabb2(double *r1,double *r2,double *r3,double *bpt1,double *bpt2) {
	double v1[2],cmp,b1,b2,b3,b4;

	v1[0]=r2[0]+r3[0]-r1[0];		// point 4
	v1[1]=r2[1]+r3[1]-r1[1];
	if(r1[0]<bpt1[0] && r2[0]<bpt1[0] && r3[0]<bpt1[0] && v1[0]<bpt1[0]) return 0;
	if(r1[0]>bpt2[0] && r2[0]>bpt2[0] && r3[0]>bpt2[0] && v1[0]>bpt2[0]) return 0;
	if(r1[1]<bpt1[1] && r2[1]<bpt1[1] && r3[1]<bpt1[1] && v1[1]<bpt1[1]) return 0;
	if(r1[1]>bpt2[1] && r2[1]>bpt2[1] && r3[1]>bpt2[1] && v1[1]>bpt2[1]) return 0;
	v1[0]=r2[0]-r1[0];					// side 1,2
	v1[1]=r2[1]-r1[1];
	cmp=v1[0]*r1[0]+v1[1]*r1[1];
	b1=v1[0]*bpt1[0]+v1[1]*bpt1[1];
	b2=v1[0]*bpt1[0]+v1[1]*bpt2[1];
	b3=v1[0]*bpt2[0]+v1[1]*bpt1[1];
	b4=v1[0]*bpt2[0]+v1[1]*bpt2[1];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp) return 0;
	cmp=v1[0]*r2[0]+v1[1]*r2[1];
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp) return 0;
	v1[0]=r3[0]-r1[0];					// side 2,3
	v1[1]=r3[1]-r1[1];
	cmp=v1[0]*r1[0]+v1[1]*r1[1];
	b1=v1[0]*bpt1[0]+v1[1]*bpt1[1];
	b2=v1[0]*bpt1[0]+v1[1]*bpt2[1];
	b3=v1[0]*bpt2[0]+v1[1]*bpt1[1];
	b4=v1[0]*bpt2[0]+v1[1]*bpt2[1];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp) return 0;
	cmp=v1[0]*r3[0]+v1[1]*r3[1];
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp) return 0;
	return 1; }


int Geo_RectXaabb3(double *r1,double *r2,double *r3,double *r4,double *bpt1,double *bpt2) {
	double r5[3],r6[3],r7[3],v1[3],b1,b2,b3,b4,b5,b6,b7,b8,cmp;

	r5[0]=r2[0]+r4[0]-r1[0];
	r5[1]=r2[1]+r4[1]-r1[1];
	r5[2]=r2[2]+r4[2]-r1[2];
	r6[0]=r3[0]+r4[0]-r1[0];
	r6[1]=r3[1]+r4[1]-r1[1];
	r6[2]=r3[2]+r4[2]-r1[2];
	r7[0]=r2[0]+r3[0]-r1[0];
	r7[1]=r2[1]+r3[1]-r1[1];
	r7[2]=r2[2]+r3[2]-r1[2];
    //r8 not used
	//r8[0]=r7[0]+r4[0]-r1[0];
	//r8[1]=r7[1]+r4[1]-r1[1];
	//r8[2]=r7[2]+r4[2]-r1[2];
	if(r1[0]<bpt1[0] && r2[0]<bpt1[0] && r3[0]<bpt1[0] && r4[0]<bpt1[0] && r5[0]<bpt1[0] && r6[0]<bpt1[0] && r7[0]<bpt1[0]) return 0;
	if(r1[0]>bpt2[0] && r2[0]>bpt2[0] && r3[0]>bpt2[0] && r4[0]>bpt2[0] && r5[0]>bpt2[0] && r6[0]>bpt2[0] && r7[0]>bpt2[0]) return 0;
	if(r1[1]<bpt1[1] && r2[1]<bpt1[1] && r3[1]<bpt1[1] && r4[1]<bpt1[1] && r5[1]<bpt1[1] && r6[1]<bpt1[1] && r7[1]<bpt1[1]) return 0;
	if(r1[1]>bpt2[1] && r2[1]>bpt2[1] && r3[1]>bpt2[1] && r4[1]>bpt2[1] && r5[1]>bpt2[1] && r6[1]>bpt2[1] && r7[1]>bpt2[1]) return 0;
	if(r1[2]<bpt1[2] && r2[2]<bpt1[2] && r3[2]<bpt1[2] && r4[2]<bpt1[2] && r5[2]<bpt1[2] && r6[2]<bpt1[2] && r7[2]<bpt1[2]) return 0;
	if(r1[2]>bpt2[2] && r2[2]>bpt2[2] && r3[2]>bpt2[2] && r4[2]>bpt2[2] && r5[2]>bpt2[2] && r6[2]>bpt2[2] && r7[2]>bpt2[2]) return 0;
	v1[0]=r2[0]-r1[0];					// side 1,2
	v1[1]=r2[1]-r1[1];
	v1[2]=r2[2]-r1[2];
	cmp=v1[0]*r1[0]+v1[1]*r1[1]+v1[2]*r1[2];
	b1=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b2=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b3=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b4=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	b5=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b6=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b7=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b8=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
	cmp=v1[0]*r2[0]+v1[1]*r2[1]+v1[2]*r2[2];
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0;
	v1[0]=r3[0]-r1[0];					// side 1,3
	v1[1]=r3[1]-r1[1];
	v1[2]=r3[2]-r1[2];
	cmp=v1[0]*r1[0]+v1[1]*r1[1]+v1[2]*r1[2];
	b1=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b2=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b3=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b4=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	b5=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b6=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b7=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b8=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
	cmp=v1[0]*r3[0]+v1[1]*r3[1]+v1[2]*r3[2];
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0;
	v1[0]=r4[0]-r1[0];					// side 1,4
	v1[1]=r4[1]-r1[1];
	v1[2]=r4[2]-r1[2];
	cmp=v1[0]*r1[0]+v1[1]*r1[1]+v1[2]*r1[2];
	b1=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b2=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b3=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b4=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	b5=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	b6=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	b7=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	b8=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
	cmp=v1[0]*r4[0]+v1[1]*r4[1]+v1[2]*r4[2];
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0;
	return 1; }


int Geo_CircleXaabb2(double *cent,double rad,double *bpt1,double *bpt2) {
	double min,max,d2,r2;

	if(cent[0]+rad<bpt1[0]) return 0;					// test projections on x,y
	if(cent[0]-rad>bpt2[0]) return 0;
	if(cent[1]+rad<bpt1[1]) return 0;
	if(cent[1]-rad>bpt2[1]) return 0;
	r2=rad*rad;																// find closest, farthest corners
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1]);
	min=max=d2;
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	if(r2>max) return 0;												// box totally inside circle
	if(r2>=min) return 1;												// at least 1 corner inside, but not all
	if(cent[0]>=bpt1[0] && cent[0]<=bpt2[0]) return 1;	// circle crosses box side
	if(cent[1]>=bpt1[1] && cent[1]<=bpt2[1]) return 1;
	return 0; }																	// circle near box corner, not crossing


int Geo_SphsXaabb3(double *cent,double rad,double *bpt1,double *bpt2) {
	double min,max,d2,r2;

	if(cent[0]+rad<bpt1[0]) return 0;
	if(cent[0]-rad>bpt2[0]) return 0;
	if(cent[1]+rad<bpt1[1]) return 0;
	if(cent[1]-rad>bpt2[1]) return 0;
	if(cent[2]+rad<bpt1[2]) return 0;
	if(cent[2]-rad>bpt2[2]) return 0;
	r2=rad*rad;
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1])+(bpt1[2]-cent[2])*(bpt1[2]-cent[2]);
	min=max=d2;
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1])+(bpt2[2]-cent[2])*(bpt2[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1])+(bpt1[2]-cent[2])*(bpt1[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt1[0]-cent[0])*(bpt1[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1])+(bpt2[2]-cent[2])*(bpt2[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1])+(bpt1[2]-cent[2])*(bpt1[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt1[1]-cent[1])*(bpt1[1]-cent[1])+(bpt2[2]-cent[2])*(bpt2[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1])+(bpt1[2]-cent[2])*(bpt1[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(bpt2[0]-cent[0])*(bpt2[0]-cent[0])+(bpt2[1]-cent[1])*(bpt2[1]-cent[1])+(bpt2[2]-cent[2])*(bpt2[2]-cent[2]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	if(r2>max) return 0;
	if(r2>=min) return 1;
	if(cent[0]>=bpt1[0] && cent[0]<=bpt2[0]) return 1;
	if(cent[1]>=bpt1[1] && cent[1]<=bpt2[1]) return 1;
	if(cent[2]>=bpt1[2] && cent[2]<=bpt2[2]) return 1;
	return 1; }


int Geo_CylisXaabb3(double *pt1,double *pt2,double rad,double *bpt1,double *bpt2) {
	double v1[3],v2[3],v3[3];
	double cent[2],ab1[2],ab2[2],ab3[2],ab4[2],ab5[2],ab6[2],ab7[2],ab8[2];
	double nrm,b1,b2,b3,b4,b5,b6,b7,b8,cmp,r2,d2,min,max,crss1,crss2;

	Geo_LineNormal3D(pt1,pt2,pt1,v1);		// v1 and v2 are normalized and perp. to cyl. axis
	v3[0]=pt2[0]-pt1[0];
	v3[1]=pt2[1]-pt1[1];
	v3[2]=pt2[2]-pt1[2];
	nrm=sqrt(v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2]);
	if(!nrm) return 0;
	v3[0]/=nrm;		// v3 is unit vector along cyl. axis, only used here
	v3[1]/=nrm;
	v3[2]/=nrm;
	v2[0]=v3[1]*v1[2]-v3[2]*v1[1];	// v2 = v3 x v1
	v2[1]=v3[2]*v1[0]-v3[0]*v1[2];
	v2[2]=v3[0]*v1[1]-v3[1]*v1[0];

	cent[0]=v1[0]*pt1[0]+v1[1]*pt1[1]+v1[2]*pt1[2];			// cent is the center of the circle on v1,v2 axes
	cent[1]=v2[0]*pt1[0]+v2[1]*pt1[1]+v2[2]*pt1[2];

	ab1[0]=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];		// ab are box pts in 2-D
	ab1[1]=v2[0]*bpt1[0]+v2[1]*bpt1[1]+v2[2]*bpt1[2];
	ab2[0]=v1[0]*bpt1[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	ab2[1]=v2[0]*bpt1[0]+v2[1]*bpt1[1]+v2[2]*bpt2[2];
	ab3[0]=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	ab3[1]=v2[0]*bpt1[0]+v2[1]*bpt2[1]+v2[2]*bpt1[2];
	ab4[0]=v1[0]*bpt1[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	ab4[1]=v2[0]*bpt1[0]+v2[1]*bpt2[1]+v2[2]*bpt2[2];
	ab5[0]=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt1[2];
	ab5[1]=v2[0]*bpt2[0]+v2[1]*bpt1[1]+v2[2]*bpt1[2];
	ab6[0]=v1[0]*bpt2[0]+v1[1]*bpt1[1]+v1[2]*bpt2[2];
	ab6[1]=v2[0]*bpt2[0]+v2[1]*bpt1[1]+v2[2]*bpt2[2];
	ab7[0]=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt1[2];
	ab7[1]=v2[0]*bpt2[0]+v2[1]*bpt2[1]+v2[2]*bpt1[2];
	ab8[0]=v1[0]*bpt2[0]+v1[1]*bpt2[1]+v1[2]*bpt2[2];
	ab8[1]=v2[0]*bpt2[0]+v2[1]*bpt2[1]+v2[2]*bpt2[2];

	v1[0]=ab2[0]-ab1[0];									// Test1: do SAT on the three box axes
	v1[1]=ab2[1]-ab1[1];
	nrm=sqrt(v1[0]*v1[0]+v1[1]*v1[1]);
	if(nrm>0) {
		v1[0]/=nrm;
		v1[1]/=nrm;
		b1=v1[0]*ab1[0]+v1[1]*ab1[1];
		b2=v1[0]*ab2[0]+v1[1]*ab2[1];
		b3=v1[0]*ab3[0]+v1[1]*ab3[1];
		b4=v1[0]*ab4[0]+v1[1]*ab4[1];
		b5=v1[0]*ab5[0]+v1[1]*ab5[1];
		b6=v1[0]*ab6[0]+v1[1]*ab6[1];
		b7=v1[0]*ab7[0]+v1[1]*ab7[1];
		b8=v1[0]*ab8[0]+v1[1]*ab8[1];
		cmp=v1[0]*cent[0]+v1[1]*cent[1]-rad;
		if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
		cmp=v1[0]*cent[0]+v1[1]*cent[1]+rad;
		if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0; }

	v1[0]=ab3[0]-ab1[0];
	v1[1]=ab3[1]-ab1[1];
	nrm=sqrt(v1[0]*v1[0]+v1[1]*v1[1]);
	if(nrm>0) {
		v1[0]/=nrm;
		v1[1]/=nrm;
		b1=v1[0]*ab1[0]+v1[1]*ab1[1];
		b2=v1[0]*ab2[0]+v1[1]*ab2[1];
		b3=v1[0]*ab3[0]+v1[1]*ab3[1];
		b4=v1[0]*ab4[0]+v1[1]*ab4[1];
		b5=v1[0]*ab5[0]+v1[1]*ab5[1];
		b6=v1[0]*ab6[0]+v1[1]*ab6[1];
		b7=v1[0]*ab7[0]+v1[1]*ab7[1];
		b8=v1[0]*ab8[0]+v1[1]*ab8[1];
		cmp=v1[0]*cent[0]+v1[1]*cent[1]-rad;
		if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
		cmp=v1[0]*cent[0]+v1[1]*cent[1]+rad;
		if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0; }

	v1[0]=ab5[0]-ab1[0];
	v1[1]=ab5[1]-ab1[1];
	nrm=sqrt(v1[0]*v1[0]+v1[1]*v1[1]);
	if(nrm>0) {
		v1[0]/=nrm;
		v1[1]/=nrm;
		b1=v1[0]*ab1[0]+v1[1]*ab1[1];
		b2=v1[0]*ab2[0]+v1[1]*ab2[1];
		b3=v1[0]*ab3[0]+v1[1]*ab3[1];
		b4=v1[0]*ab4[0]+v1[1]*ab4[1];
		b5=v1[0]*ab5[0]+v1[1]*ab5[1];
		b6=v1[0]*ab6[0]+v1[1]*ab6[1];
		b7=v1[0]*ab7[0]+v1[1]*ab7[1];
		b8=v1[0]*ab8[0]+v1[1]*ab8[1];
		cmp=v1[0]*cent[0]+v1[1]*cent[1]-rad;
		if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
		cmp=v1[0]*cent[0]+v1[1]*cent[1]+rad;
		if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0; }

	r2=rad*rad;														// Tests 2,3: check corners inside circle
	d2=(ab1[0]-cent[0])*(ab1[0]-cent[0])+(ab1[1]-cent[1])*(ab1[1]-cent[1]);
	min=max=d2;
	d2=(ab2[0]-cent[0])*(ab2[0]-cent[0])+(ab2[1]-cent[1])*(ab2[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab3[0]-cent[0])*(ab3[0]-cent[0])+(ab3[1]-cent[1])*(ab3[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab4[0]-cent[0])*(ab4[0]-cent[0])+(ab4[1]-cent[1])*(ab4[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab5[0]-cent[0])*(ab5[0]-cent[0])+(ab5[1]-cent[1])*(ab5[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab6[0]-cent[0])*(ab6[0]-cent[0])+(ab6[1]-cent[1])*(ab6[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab7[0]-cent[0])*(ab7[0]-cent[0])+(ab7[1]-cent[1])*(ab7[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	d2=(ab8[0]-cent[0])*(ab8[0]-cent[0])+(ab8[1]-cent[1])*(ab8[1]-cent[1]);
	if(d2<min) min=d2;
	else if(d2>max) max=d2;
	if(r2>max) return 0;
	if(r2>=min) return 1;

	if(Geo_LineXaabb(pt1,pt2,bpt1,bpt2,3,1)) return 1;	// Test 4: check cylinder center crosses aabb

	// Test 5: check edge crossing
	if(((crss1=Geo_LineXSphs(ab1,ab2,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab2,ab4,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab4,ab3,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab3,ab1,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab1,ab5,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab2,ab6,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab3,ab7,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab4,ab8,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab5,ab6,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab6,ab8,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab8,ab7,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	if(((crss1=Geo_LineXSphs(ab7,ab5,cent,rad,2,&crss2,NULL,NULL))>=0 && crss1<=1) || (crss2>=0 && crss2<=1)) return 1;
	
	return 0; }


int Geo_DiskXaabb3(double *cent,double rad,double *norm,double *bpt1,double *bpt2) {
	double dot,cmp,b1,b2,b3,b4,b5,b6,b7,b8;

	dot=sqrt(norm[1]*norm[1]+norm[2]*norm[2]);	// SAT with aabb faces
	if(cent[0]-rad*dot>bpt2[0]) return 0;
	if(cent[0]+rad*dot<bpt1[0]) return 0;
	dot=sqrt(norm[0]*norm[0]+norm[2]*norm[2]);
	if(cent[1]-rad*dot>bpt2[1]) return 0;
	if(cent[1]+rad*dot<bpt1[1]) return 0;
	dot=sqrt(norm[0]*norm[0]+norm[1]*norm[1]);
	if(cent[2]-rad*dot>bpt2[2]) return 0;
	if(cent[2]+rad*dot<bpt1[2]) return 0;

	cmp=norm[0]*cent[0]+norm[1]*cent[1]+norm[2]*cent[2];		// SAT with disk face
	b1=norm[0]*bpt1[0]+norm[1]*bpt1[1]+norm[2]*bpt1[2];
	b2=norm[0]*bpt1[0]+norm[1]*bpt1[1]+norm[2]*bpt2[2];
	b3=norm[0]*bpt1[0]+norm[1]*bpt2[1]+norm[2]*bpt1[2];
	b4=norm[0]*bpt1[0]+norm[1]*bpt2[1]+norm[2]*bpt2[2];
	b5=norm[0]*bpt2[0]+norm[1]*bpt1[1]+norm[2]*bpt1[2];
	b6=norm[0]*bpt2[0]+norm[1]*bpt1[1]+norm[2]*bpt2[2];
	b7=norm[0]*bpt2[0]+norm[1]*bpt2[1]+norm[2]*bpt1[2];
	b8=norm[0]*bpt2[0]+norm[1]*bpt2[1]+norm[2]*bpt2[2];
	if(b1<cmp && b2<cmp && b3<cmp && b4<cmp && b5<cmp && b6<cmp && b7<cmp && b8<cmp) return 0;
	if(b1>cmp && b2>cmp && b3>cmp && b4>cmp && b5>cmp && b6>cmp && b7>cmp && b8>cmp) return 0;

	return 1; }


/******************** approx. Xaabb ************/

int Geo_SemicXaabb2(double *cent,double rad,double *outvect,double *bpt1,double *bpt2) {
	double r1[2],r2[2],r3[2];

	if(!Geo_CircleXaabb2(cent,rad,bpt1,bpt2)) return 0;
	Geo_Semic2Rect(cent,rad,outvect,r1,r2,r3);
	return Geo_RectXaabb2(r1,r2,r3,bpt1,bpt2); }


int Geo_HemisXaabb3(double *cent,double rad,double *outvect,double *bpt1,double *bpt2) {
	double r1[3],r2[3],r3[3],r4[3];

	if(!Geo_SphsXaabb3(cent,rad,bpt1,bpt2)) return 0;
	Geo_Hemis2Rect(cent,rad,outvect,r1,r2,r3,r4);
	return Geo_RectXaabb3(r1,r2,r3,r4,bpt1,bpt2); }


int Geo_CylsXaabb3(double *pt1,double *pt2,double rad,double *bpt1,double *bpt2) {
	double r1[3],r2[3],r3[3],r4[3];

	if(!Geo_CylisXaabb3(pt1,pt2,rad,bpt1,bpt2)) return 0;
	Geo_Cyl2Rect(pt1,pt2,rad,r1,r2,r3,r4);
	return Geo_RectXaabb3(r1,r2,r3,r4,bpt1,bpt2); }


/******************** volumes ****************/

double Geo_SphVolume(double rad,int dim) {
	double vol;

	if(dim==0) vol=1;
	else if(dim==1) vol=2*rad;
	else if(dim==2) vol=PI*rad*rad;
	else if(dim==3) vol=4.0/3.0*PI*rad*rad*rad;
	else vol=2.0/(dim*exp(gammaln(dim/2.0)))*pow(SQRTPI*rad,dim);
	return vol; }


double Geo_SphOLSph(double *cent1,double *cent2,double r1,double r2,int dim) {
	double ol,dist;
	int d;

	dist=0;
	for(d=0;d<dim;d++) dist+=(cent2[d]-cent1[d])*(cent2[d]-cent1[d]);
	dist=sqrt(dist);
	if(dist>=r1+r2) ol=0;
	else if(dist+r2<=r1) ol=Geo_SphVolume(r2,dim);
	else if(dist+r1<=r2) ol=Geo_SphVolume(r1,dim);
	else {
		if(dim==1)
			ol=r1+r2-dist;
		else if(dim==2)
			ol=r1*r1*acos((dist*dist+r1*r1-r2*r2)/(2*dist*r1))+r2*r2*acos((dist*dist+r2*r2-r1*r1)/(2*dist*r2))-0.5*sqrt((-dist+r1+r2)*(dist+r1-r2)*(dist-r1+r2)*(dist+r1+r2));
		else if(dim==3)
			ol=PI*(r2+r1-dist)*(r2+r1-dist)*(dist*dist+2*dist*r1-3*r1*r1+2*dist*r2+6*r1*r2-3*r2*r2)/(12*dist);
		else ol=-1; }
	return ol; }


/******************** testing *******************/

// To run test, rename below function to main and compile and link with:
// gcc -Wall -O0 -g Geometry.c math2.c -o testcode


int main_Geometry(void) {
	return 0; }



