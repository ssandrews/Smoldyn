/* Steven Andrews 2/05.
See documentation called Sphere_doc.doc.
Copyright 2005-2007 by Steven Andrews.  This work is distributed under the terms
of the Gnu Lesser General Public License (LGPL). */


#ifndef __Sphere_h
#define __Sphere_h

/*
Cart = Cartesian coordinates (x, y, z)
Sc = Spherical coordinates (r, theta, phi)
Dcm = Direction cosine matrix (A00, A01, A02, A10, A11, A12, A20, A21, A22)
Eax = Euler angles x-convention (theta, phi, psi)
Eay = Euler angles y-convention (theta, phi, chi)
Qtn = Quaternions (q0, q1, q2, q3)
Ypr = xyz- or ypr- or Tait-Bryan convention (yaw, pitch, roll): rotate on z, then y, then x
*/

void Sph_Cart2Sc(const double *Cart,double *Sc);
void Sph_Sc2Cart(const double *Sc,double *Cart);
void Sph_Cart2Cart(const double *Cart1,double *Cart2);
void Sph_Ypr2Ypr(const double *Ypr1,double *Ypr2);
void Sph_Eax2Ypr(const double *Eax,double *Ypr);

void Sph_Eax2Dcm(const double *Eax,double *Dcm);
void Sph_Eay2Dcm(const double *Eay,double *Dcm);
void Sph_Ypr2Dcm(const double *Ypr,double *Dcm);
void Sph_Ypr2Dcmt(const double *Ypr,double *Dcmt);
void Sph_Dcm2Ypr(const double *Dcm,double *Ypr);
void Sph_Dcm2Dcm(const double *Dcm1,double *Dcm2);
void Sph_Dcm2Dcmt(const double *Dcm1,double *Dcm2);

void Sph_DcmxDcm(const double *Dcm1,const double *Dcm2,double *Dcm3);
void Sph_DcmxDcmt(const double *Dcm1,const double *Dcmt,double *Dcm3);
void Sph_DcmtxDcm(const double *Dcmt,const double *Dcm2,double *Dcm3);
void Sph_DcmxCart(const double *Dcm,const double *Cart,double *Cart2);
void Sph_DcmtxCart(const double *Dcm,const double *Cart,double *Cart2);

void Sph_One2Dcm(double *Dcm);
void Sph_Ypr2Yprr(const double *Ypr,double *Yprr);
void Sph_Dcm2Dcmr(const double *Dcm,double *Dcmr);
void Sph_Rot2Dcm(char axis,double angle,double *Dcm);
void Sph_Newz2Dcm(const double *Newz,double psi,double *Dcm);

void Sph_DcmtxUnit(const double *Dcmt,char unit,double *vect,const double *add,double mult);

void Sph_One2Qtn(double *Qtn);
void Sph_Qtn2Qtn(const double *Qtn1,double *Qtn2);
void Sph_Ypr2Qtn(const double *Ypr,double *Qtn);
void Sph_Yaw2Qtn(double Yaw,double *Qtn);
void Sph_Ypr2Qtni(const double *Ypr,double *Qtni);
void Sph_Qtn2Ypr(const double *Qtn,double *Ypr);
double Sph_Qtn2Yaw(const double *Qtn);
void Sph_Dcm2Qtn(const double *Dcm,double *Qtn);
void Sph_Qtn2Dcm(const double *Qtn,double *Dcm);
void Sph_XZ2Qtni(const double *x,const double *z,double *Qtn);
void Sph_QtnxQtn(const double *Qtn1,const double *Qtn2,double *Qtn3);
void Sph_QtnixQtn(const double *Qtn1,const double *Qtn2,double *Qtn3);
void Sph_QtnxQtni(const double *Qtn1,const double *Qtn2,double *Qtn3);
void Sph_QtnRotate(const double *Qtn,const double *Cart,double *Cart2);
void Sph_QtniRotate(const double *Qtn,const double *Cart,double *Cart2);
void Sph_QtniRotateUnitx(const double *Qtni,double *vect,const double *add,double mult);
void Sph_QtniRotateUnitz(const double *Qtni,double *vect,const double *add,double mult);

double Sph_RotateVectWithNormals3D(const double *pt1,const double *pt2,double *newpt2,double *oldnorm,double *newnorm,int sign);
void Sph_RotateVectorAxisAngle(const double *vect,const double *axis,double angle,double *rotated);

#endif
