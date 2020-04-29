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

class Smoldyn {

public:
    Smoldyn();
    Smoldyn(bool debug);
    ~Smoldyn();

    SmoldynDefine& getDefine();

    size_t getDim() const;
    void   setDim(size_t dim);

    void   setRandomSeed(size_t seed);
    size_t getRandomSeed();

    bool initialize();

    // Smoldyn.
    bool run(double simtime, double dt, bool display);
    void runUntil(const double breaktime, const double dt, bool display);

    // Bounds.
    void           setLowerBounds(const vector<double> bounds);
    vector<double> getLowerBounds() const;

    void           setHigherBounds(const vector<double> bounds);
    vector<double> getHigherBounds() const;

    void setBounds(const vector<pair<double, double>>& bounds);
    std::vector<pair<double, double>> getBounds() const;

    void setPartitions(const char* name, double val);

    void addSpecies(const char* name, const char* param);

    void setSpeciesMobility(const char* name, MolecState state, double difc,
        vector<double>& drift, vector<double>& difmatrix);

    void addSurface(const char* name);

    void setSurfaceAction(const char* name, enum PanelFace face,
        const char* species, enum MolecState state, enum SrfAction action);

    void addPanel(const char* surface, enum PanelShape panelShape,
        const char* panel, const char* axisstring, vector<double>& params);

    void addCompartment(const char* compartment);

    void addCompartmentSurface(const char* compt, const char* surface);

    void addCompartmentPoint(const char* compt, vector<double> point);

    void addCompartmentMolecules(
        const char* species, size_t number, const char* compt);

    void addSurfaceMolecules(const char* species, enum MolecState state,
        size_t number, const char* surface, enum PanelShape panelShape,
        const char* panel, vector<double>& position);

    void addReaction(const char* reaction, const char* reactant1,
        enum MolecState rstate1, const char* reactant2, enum MolecState rstate2,
        vector<string> productSpecies, vector<enum MolecState> productStates,
        double rate);

    void setReactionRegion(
        const char* reac, const char* compt, const char* surface);

    ErrorCode setSimTimes(const double start, const double end, const double step);

    int getMoleculeCount(const char* name, enum MolecState state);

    // Inline functions.
    inline simptr simPtr() const
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

    inline ErrorCode setMoleculeStyle(const char* molecule, enum MolecState state,
        double size, vector<double>& color)
    {
        return smolSetMoleculeStyle(pSim_, molecule, state, size, &color[0]);
    }

    inline ErrorCode setGraphicsParams(const char* method, int timestep, int delay)
    {
        return smolSetGraphicsParams(pSim_, method, timestep, delay);
    }

    inline ErrorCode update()
    {
        return smolUpdateSim(pSim_);
    }

    ErrorCode   setDt(double dt);
    double getDt() const;

private:
    simptr pSim_;
    size_t dim_;  // Dimention of the system.

    bool debug_;

    bool initDisplay_{false};

    // As large as dims.
    vector<double> lowbounds_;
    vector<double> highbounds_;

    double curtime_;

    SmoldynDefine define_;
};

#endif /* end of include guard: SIMULTION_H */
