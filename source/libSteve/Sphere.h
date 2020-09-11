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
Ep = Euler parameters (e0, e1, e2, e3)
Xyz = xyz- or ypr- or Tait-Bryan convention (yaw, pitch, roll): rotate on z, then y, then x
*/

void Sph_Cart2Sc(double *Cart,double *Sc);
void Sph_Sc2Cart(double *Sc,double *Cart);
void Sph_Eay2Ep(double *Eay,double *Ep);
void Sph_Xyz2Xyz(double *Xyz1,double *Xyz2);

void Sph_Eax2Dcm(double *Eax,double *Dcm);
void Sph_Eay2Dcm(double *Eay,double *Dcm);
void Sph_Xyz2Dcm(double *Xyz,double *Dcm);
void Sph_Xyz2Dcmt(double *Xyz,double *Dcmt);
void Sph_Dcm2Xyz(double *Dcm,double *Xyz);
void Sph_Dcm2Dcm(double *Dcm1,double *Dcm2);
void Sph_Dcm2Dcmt(double *Dcm1,double *Dcm2);

void Sph_DcmxDcm(double *Dcm1,double *Dcm2,double *Dcm3);
void Sph_DcmxDcmt(double *Dcm1,double *Dcmt,double *Dcm3);
void Sph_DcmtxDcm(double *Dcmt,double *Dcm2,double *Dcm3);

void Sph_One2Dcm(double *Dcm);
void Sph_Xyz2Xyzr(double *Xyz,double *Xyzr);
void Sph_Dcm2Dcmr(double *Dcm,double *Dcmr);
void Sph_Rot2Dcm(char axis,double angle,double *Dcm);
void Sph_Newz2Dcm(double *Newz,double psi,double *Dcm);

void Sph_DcmtxUnit(double *Dcmt,char unit,double *vect,double *add,double mult);

double Sph_RotateVectWithNormals3D(double *pt1,double *pt2,double *newpt2,double *oldnorm,double *newnorm,int sign);

#endif
