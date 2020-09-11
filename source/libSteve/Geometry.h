/* Steven Andrews 2/17/06
See documentation called Geometry_doc.doc.
Copyright 2006-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */


#ifndef __Geometry_h
#define __Geometry_h

// Center
void Geo_LineCenter(double **point,double *cent,int dim);
void Geo_RectCenter(double **point,double *cent,int dim);
void Geo_TriCenter(double **point,double *cent,int dim);

// Normal
double Geo_LineNormal(double *pt1,double *pt2,double *ans);
double Geo_LineNormal2D(double *pt1,double *pt2,double *point,double *ans);
double Geo_LineNormal3D(double *pt1,double *pt2,double *point,double *ans);
double Geo_LineNormPos(double *pt1,double *pt2,double *point,int dim,double *distptr);
double Geo_TriNormal(double *pt1,double *pt2,double *pt3,double *ans);
double Geo_SphereNormal(double *cent,double *pt,int front,int dim,double *ans);

// Unit Vectors
double Geo_UnitCross(double *v1start,double *v1end,double *v2start,double *v2end,double *ans);
double Geo_TriUnitVects(double *pt1,double *pt2,double *pt3,double *unit0,double *unit1,double *unit2);
double Geo_SphereUnitVects(double *cent,double *top,double *point,int front,double *unit0,double *unit1,double *unit2);
double Geo_CylUnitVects(double *pt1,double *pt2,double *point,int front,double *unit0,double *unit1,double *unit2);
double Geo_DiskUnitVects(double *cent,double *front,double *point,double *unit0,double *unit1,double *unit2);

// Area
double Geo_LineLength(double *pt1,double *pt2,int dim);
double Geo_TriArea2(double *pt1,double *pt2,double *pt3);
double Geo_TriArea3D(double *pt1,double *pt2,double *pt3);
double Geo_TriArea3(double *pt1,double *pt2,double *pt3,double *norm);
double Geo_QuadArea(double *pt1,double *pt2,double *pt3,double *pt4,int dim);

// Inside point
double Geo_InsidePoints2(double *pt1,double *pt2,double margin,double *ans1,double *ans2,int dim);
void Geo_InsidePoints3(double *pt1,double *pt2,double *pt3,double margin,double *ans1,double *ans2,double *ans3);
void Geo_InsidePoints32(double **point,double margin,double **ans);

// Point in
int Geo_PtInTriangle(double *pt1,double *pt2,double *pt3,double *norm,double *test);
int Geo_PtInTriangle2(double **point,double *test);
int Geo_PtInSlab(double *pt1,double *pt2,double *test,int dim);
int Geo_PtInSphere(double *test,double *cent,double rad,int dim);

// Nearest
void Geo_NearestSlabPt(double *pt1,double *pt2,double *point,double *ans,int dim);
int Geo_NearestLineSegPt(double *pt1,double *pt2,double *point,double *ans,int dim,double margin);
void Geo_NearestTriPt(double *pt1,double *pt2,double *pt3,double *norm,double *point,double *ans);
void Geo_NearestTriPt2(double **point,double **edgenorm,double *norm,double *testpt,double *ans);
int Geo_NearestTrianglePt(double *pt1,double *pt2,double *pt3,double *norm,double *point,double *ans);
int Geo_NearestTrianglePt2(double **point,double *norm,double *testpt,double *ans,double margin);
double Geo_NearestSpherePt(double *cent,double rad,int front,int dim,double *point,double *ans);
void Geo_NearestRingPt(double *cent,double *axis,double rad,int dim,double *point,double *ans);
void Geo_NearestCylPt(double *pt1,double *axis,double rad,int dim,double *point,double *ans);
int Geo_NearestCylinderPt(double *pt1,double *pt2,double rad,int dim,double *point,double *ans,double margin);
int Geo_NearestDiskPt(double *cent,double *axis,double rad,int dim,double *point,double *ans,double margin);
double Geo_NearestLine2LineDist(double *ptA1,double *ptA2,double *ptB1,double *ptB2);
double Geo_NearestSeg2SegDist(double *ptA1,double *ptA2,double *ptB1,double *ptB2);
double Geo_NearestAabbPt(const double *bpt1,const double *bpt2,int dim,const double *point,double *ans);

// To Rect
void Geo_Semic2Rect(double *cent,double rad,double *outvect,double *r1,double *r2,double *r3);
void Geo_Hemis2Rect(double *cent,double rad,double *outvect,double *r1,double *r2,double *r3,double *r4);
void Geo_Cyl2Rect(double *pt1,double *pt2,double rad,double *r1,double *r2,double *r3,double *r4);

// Cross
double Geo_LineXLine(double *l1p1,double *l1p2,double *l2p1,double *l2p2,double *crss2ptr);
double Geo_LineXPlane(double *pt1,double *pt2,double *v,double *norm,double *crsspt);
double Geo_LineXSphs(double *pt1,double *pt2,double *cent,double rad,int dim,double *crss2ptr,double *nrdistptr,double *nrposptr);
double Geo_LineXCyl2s(double *pt1,double *pt2,double *cp1,double *cp2,double *norm,double rad,double *crss2ptr,double *nrdistptr,double *nrposptr);
double Geo_LineXCyls(double *pt1,double *pt2,double *cp1,double *cp2,double rad,double *crss2ptr,double *nrdistptr,double *nrposptr);

// Reflection
double Geo_SphereReflectSphere(const double *a0,const double *a1,const double *b0,const double *b1,int dim,double radius2,double *a1p,double *b1p);

// Exit
double Geo_LineExitRect(double *pt1,double *pt2,double *front,double *corner1,double *corner3,double *exitpt,int *exitside);
double Geo_LineExitLine2(double *pt1,double *pt2,double *end1,double *end2,double *exitpt,int *exitend);
void Geo_LineExitArc2(double *pt1,double *pt2,double *cent,double radius,double *norm,double *exitpt,int *exitend);
double Geo_LineExitTriangle(double *pt1,double *pt2,double *normal,double *v1,double *v2,double *v3,double *exitpt,int *exitside);
double Geo_LineExitTriangle2(double *pt1,double *pt2,double **point,double *exitpt,int *exitside);
double Geo_LineExitSphere(double *pt1,double *pt2,double *cent,double rad,double *exitpt);
void Geo_LineExitHemisphere(double *pt1,double *pt2,double *cent,double rad,double *normal,double *exitpt);
double Geo_LineExitCylinder(double *pt1,double *pt2,double *end1,double *end2,double rad,double *exitpt,int *exitend);

// Cross aabbb
int Geo_LineXaabb2(double *pt1,double *pt2,double *norm,double *bpt1,double *bpt2);
int Geo_LineXaabb(double *pt1,double *pt2,double *bpt1,double *bpt2,int dim,int infline);
int Geo_TriXaabb3(double *pt1,double *pt2,double *pt3,double *norm,double *bpt1,double *bpt2);
int Geo_RectXaabb2(double *r1,double *r2,double *r3,double *bpt1,double *bpt2);
int Geo_RectXaabb3(double *r1,double *r2,double *r3,double *r4,double *bpt1,double *bpt2);
int Geo_CircleXaabb2(double *cent,double rad,double *bpt1,double *bpt2);
int Geo_SphsXaabb3(double *cent,double rad,double *bpt1,double *bpt2);
int Geo_CylisXaabb3(double *pt1,double *pt2,double rad,double *bpt1,double *bpt2);
int Geo_DiskXaabb3(double *cent,double rad,double *norm,double *bpt1,double *bpt2);

// Approx. cross aabb
int Geo_SemicXaabb2(double *cent,double rad,double *outvect,double *bpt1,double *bpt2);
int Geo_HemisXaabb3(double *cent,double rad,double *outvect,double *bpt1,double *bpt2);
int Geo_CylsXaabb3(double *pt1,double *pt2,double rad,double *bpt1,double *bpt2);

// Volumes
double Geo_SphVolume(double rad,int dim);
double Geo_SphOLSph(double *cent1,double *cent2,double r1,double r2,int dim);

#endif
