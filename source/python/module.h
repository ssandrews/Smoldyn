// Author:  Dilawar Singh  dilawar.s.rajput@gmail.com

#ifndef MODULE_H
#define MODULE_H

#include "pybind11/pybind11.h"

#include "util.h"
namespace py = pybind11;

#include "../Smoldyn/libsmoldyn.h"

/* gl2glutInit */
extern void gl2glutInit(int* argc, char** argv);

// defined in smolgraphics.c file.
extern int graphicsreadcolor(char** stringptr, double* rgba);

ErrorCode setDt(double dt);
double getDt();

#endif /* end of include guard: MODULE_H */
