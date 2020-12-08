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

#include "../Smoldyn/libsmoldyn.h"

// defined in smolgraphics.c file.
extern int graphicsreadcolor(char** stringptr, double* rgba);

/*************************************
 *  Global variables for the module  *
 *************************************/

// Unique Ptr to hold simptr with custom deleter.
// using simptr_uptr_type_ = unique_ptr<simstruct, decltype(&smolFreeSim)>;

// Collect all simptr here.
// extern vector<simptr_uptr_type_> simptrs_;
extern vector<simptr> simptrs_;
extern simptr cursim_;

extern size_t dim_;
extern vector<double> lowbounds_;
extern vector<double> highbounds_;
extern bool debug_;
extern double curtime_;
extern bool initDisplay_;

extern void simfree(simptr ptr);

bool addToSimptrVec(simptr ptr);
bool deleteSimptr(simptr ptr);

size_t getDim();
void setDim(size_t dim);

size_t getRandomSeed();

bool initialize();


// Smoldyn.
ErrorCode runSimulation(double simtime, double dt, bool display, bool overwrite);
ErrorCode runUntil(const double breaktime, const double dt, bool display, bool overwrite);

bool connect(const py::function& func, const py::object& target, const size_t step,
    const py::list& args);

ErrorCode setDt(double dt);
double getDt();

inline void cleanup()
{
    // smart pointer does the cleanup.
    simptrs_.clear();
}

void setBoundaries(const vector<pair<double, double>>& bounds);
void setBoundaries(vector<double>& lows, vector<double>& highs);

// std::vector<pair<double, double>> getBoundaries();
std::pair<vector<double>, vector<double>> getBoundaries();

bool setModelpath(const string& modelpath);

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
    auto pos = p.find_last_of('/');
    fileroot = p.substr(0, pos + 1);
    filename = p.substr(pos + 1);
    return {fileroot, filename};
}

inline array<double, 4> color2RGBA(char* color)
{
    array<double, 4> rgba = {0, 0, 0, 1.0};
    graphicsreadcolor(&color, &rgba[0]);
    return rgba;
}


#endif /* end of include guard: SIMULTION_H */
