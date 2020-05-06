// =====================================================================================
//
//       Filename:  Smoldyn.h
//
//    Description: Header file.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore
//
// =====================================================================================

#ifndef SIMULTION_H
#define SIMULTION_H

#include <sstream>
#include <map>
using namespace std;

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "SmoldynDefine.hh"

#include "../Smoldyn/libsmoldyn.h"

// defined in smolgraphics.c file.
extern int graphicsreadcolor(char** stringptr, double* rgba);

// Global variables for module.
extern simptr         pSim_;
extern size_t         dim_;
extern vector<double> lowbounds_;
extern vector<double> highbounds_;
extern bool           debug_;
extern double         curtime_;
extern bool           initDisplay_;

size_t getDim();
void   setDim(size_t dim);

void   setRandomSeed(size_t seed);
size_t getRandomSeed();

bool initialize();

// Smoldyn.
bool run(double simtime, double dt, bool display);
void runUntil(const double breaktime, const double dt, bool display);

ErrorCode setDt(double dt);
double    getDt();

inline void cleanup()
{
    if(pSim_)
        smolFreeSim(pSim_);
}

void setBoundaries(const vector<pair<double, double>>& bounds);
std::vector<pair<double, double>> getBoundaries();

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Split a filepath between dirname/filename. For examples::
 *  /a/b/c.txt -> (/a/b, c.txt)
 *
 * @Param filepath
 *
 * @Returns
 */
/* ----------------------------------------------------------------------------*/
inline pair<string, string> splitPath(const string& p)
{
    string filename, fileroot;
    auto   pos = p.find_last_of('/');
    fileroot   = p.substr(0, pos + 1);
    filename   = p.substr(pos + 1);
    return {fileroot, filename};
}

inline array<double, 4> color2RGBA(char* color)
{
    array<double, 4> rgba = {0, 0, 0, 1.0};
    graphicsreadcolor(&color, &rgba[0]);
    return rgba;
}

#endif /* end of include guard: SIMULTION_H */
