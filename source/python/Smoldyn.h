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
    void setDim(size_t dim);

    void setRandomSeed(size_t seed);
    size_t getRandomSeed();

    bool initialize();

    bool run(double simtime, double starttime, double dt);

    // Bounds.
    void setLowerBounds(const vector<double> bounds);
    vector<double> getLowerBounds() const;

    void setHigherBounds(const vector<double> bounds);
    vector<double> getHigherBounds() const;

    void setBounds(const vector<pair<double, double>>& bounds);
    std::vector<pair<double, double>> getBounds() const;

    void setPartitions(const string& name, double val);

    void addSpecies(const string& name, const string& param);

    void setSpeciesMobility(const string& name, MolecState state, 
        double difc, double* drift, double *difmatrix);

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
