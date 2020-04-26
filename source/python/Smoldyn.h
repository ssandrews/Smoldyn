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

#ifndef SMOLDYN_H
#define SMOLDYN_H

#include <sstream>
#include <map>
using namespace std;

#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "SmoldynDefine.hh"
#include "smoldyn.h"

#include "../Smoldyn/libsmoldyn.h"

class Smoldyn {

public:
    Smoldyn();
    ~Smoldyn();

    SmoldynDefine& getDefine();

    size_t getDim() const;
    void   setDim(size_t dim);

    void   setRandomSeed(size_t seed);
    size_t getRandomSeed();

    bool initialize();

    bool run(double simtime, double starttime, double dt, bool display);

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

    void setSimTimes(const double start, const double end, const double step);
    inline simptr simPtr() const
    {
        return pSim_;
    }

private:
    /* data */
    SmoldynDefine define_;

    // simstruct sim_;
    simptr pSim_;

    size_t dim_;

    // As large as dims.
    vector<double> lowbounds_;
    vector<double> highbounds_;

    double curtime_;
};

#endif /* end of include guard: SMOLDYN_H */
