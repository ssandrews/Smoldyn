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
extern simptr pSim_;
extern vector<double> lowbounds_;
extern vector<double> highbounds_;
extern bool debug_;
extern double curtime_;
extern bool initDisplay_;

size_t getDim();
void   setDim(size_t dim);

void   setRandomSeed(size_t seed);
size_t getRandomSeed();

bool initialize();

// Smoldyn.
bool runSim(double simtime, double dt, bool display);
void runUntil(const double breaktime, const double dt, bool display);

// Bounds.
void           setLowerBounds(const vector<double> bounds);
vector<double> getLowerBounds();

void           setHigherBounds(const vector<double> bounds);
vector<double> getHigherBounds();

void setBounds(const vector<pair<double, double>>& bounds);
std::vector<pair<double, double>> getBounds();

void setPartitions(const char* name, double val);

ErrorCode addSpecies(const char* name, const char* mollist);

ErrorCode setSpeciesMobility(const char* name, MolecState state, double difc,
    vector<double>& drift, vector<double>& difmatrix);

ErrorCode setSpeciesStyle(
    const char* name, const MolecState state, double size, char* color);

ErrorCode addSurface(const char* name);

ErrorCode setSurfaceAction(const char* name, enum PanelFace face,
    const char* species, enum MolecState state, enum SrfAction action);

ErrorCode addPanel(const char* surface, enum PanelShape panelShape,
    const char* panel, const char* axisstring, vector<double>& params);

ErrorCode addCompartment(const char* compartment);

void addCompartmentSurface(const char* compt, const char* surface);

void addCompartmentPoint(const char* compt, vector<double> point);

void addCompartmentMolecules(
    const char* species, size_t number, const char* compt);

void addSurfaceMolecules(const char* species, enum MolecState state,
    size_t number, const char* surface, enum PanelShape panelShape,
    const char* panel, vector<double>& position);

ErrorCode addReaction(const char* reaction, const char* reactant1,
    enum MolecState rstate1, const char* reactant2, enum MolecState rstate2,
    vector<string> productSpecies, vector<enum MolecState> productStates,
    double rate);

void setReactionRegion(
    const char* reac, const char* compt, const char* surface);

ErrorCode setSimTimes(const double start, const double end, const double step);

int getMoleculeCount(const char* name, enum MolecState state);

// Inline functions.
inline simptr simPtr()
{
    return pSim_;
}

inline ErrorCode setBoundaryType(int dim, int highside, char type)
{
    return smolSetBoundaryType(pSim_, dim, highside, type);
}

inline ErrorCode addMolList(const char* mollist)
{
    return smolAddMolList(pSim_, mollist);
}

inline ErrorCode addSolutionMolecules(const char* name, int nums,
    vector<double>& lowposition, vector<double>& highposition)
{
    return smolAddSolutionMolecules(
        pSim_, name, nums, &lowposition[0], &highposition[0]);
}

inline ErrorCode setGraphicsParams(const char* method, int timestep, int delay)
{
    return smolSetGraphicsParams(pSim_, method, timestep, delay);
}

inline ErrorCode updateSim()
{
    return smolUpdateSim(pSim_);
}

ErrorCode setDt(double dt);
double    getDt();


#endif /* end of include guard: SIMULTION_H */
