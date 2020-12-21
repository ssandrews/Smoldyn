//         Author:  Dilawar Singh  dilawar.s.rajput@gmail.com
//   Organization:  Subconscious Compute Pvt. Ltd., Bangalore

#ifndef MODULE_H
#define MODULE_H

#include <map>
using namespace std;

#include "util.h"

#include "pybind11/pybind11.h"

namespace py = pybind11;

#include "../Smoldyn/libsmoldyn.h"

/* gl2glutInit */
extern void gl2glutInit(int* argc, char** argv);

// defined in smolgraphics.c file.
extern int graphicsreadcolor(char** stringptr, double* rgba);

/*************************************
 *  Global variables for the module  *
 *************************************/

// Unique Ptr to hold simptr with custom deleter.
// using simptr_uptr_type_ = unique_ptr<simstruct, decltype(&smolFreeSim)>;

Simulation* cursim_;

size_t getRandomSeed();

// Smoldyn.
ErrorCode runSimulation(double simtime, double dt, bool display, bool overwrite);
ErrorCode runUntil(const double breaktime, const double dt, bool display, bool overwrite);

ErrorCode setDt(double dt);
double    getDt();

simptr setBoundaries(vector<double>& lows, vector<double>& highs, vector<string>&);

#endif /* end of include guard: MODULE_H */
