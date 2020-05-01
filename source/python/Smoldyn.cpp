//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>
#include <vector>

using namespace std;

#include "../lib/random2.h"

#include "Smoldyn.h"

// Global variables.
simptr pSim_;
size_t dim_ = 0;
vector<double> lowbounds_;
vector<double> highbounds_;
bool debug_ = false;
double curtime_ = 0.0;
bool initDisplay_ = false;

size_t getDim()
{
    return dim_;
}

void setDim(size_t dim)
{
    dim_ = dim;
    if(pSim_)
        pSim_->dim = dim;
}

void setRandomSeed(size_t seed)
{
    if(!pSim_)
        return;
    pSim_->randseed = randomize(seed);
}

size_t getRandomSeed(void)
{
    return pSim_->randseed;
}

bool initialize()
{
    if(pSim_)
        return true;
    if(getDim() <= 0 || getDim() > 3) {
        cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got " << getDim()
             << endl;
        return false;
    }

    if(lowbounds_.size() != getDim()) {
        cerr << __FUNCTION__ << ": missing lowbounds" << endl;
        return false;
    }

    if(highbounds_.size() != getDim()) {
        cerr << __FUNCTION__ << ": missing highbounds" << endl;
        return false;
    }

    for(size_t d = 0; d < getDim(); d++) {
        if(lowbounds_[d] >= highbounds_[d]) {
            cerr << __FUNCTION__ << ": lowbounds must be < highbounds"
                 << " which is not true at index " << d
                 << " where lowbounds is " << lowbounds_[d]
                 << " and highbound is " << highbounds_[d] << endl;
            return false;
        }
    }

    pSim_ = smolNewSim(getDim(), &lowbounds_[0], &highbounds_[0]);
    if(debug_)
        smolSetDebugMode(1);
    return pSim_ ? true : false;
}

void runUntil(const double breaktime, const double dt, bool display)
{
    // If dt>0, reset dt else use the old one.
    if(dt > 0.0)
        smolSetTimeStep(pSim_, dt);

    if(display and (!initDisplay_)) {
        smolDisplaySim(pSim_);
        initDisplay_ = true;
    }
    smolRunSimUntil(pSim_, breaktime);
}

bool runSim(double stoptime, double dt, bool display)
{
    if(!pSim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return false;
        }
    }

    smolSetSimTimes(pSim_, curtime_, stoptime, dt);
    smolUpdateSim(pSim_);

    if(display and !initDisplay_) {
        smolDisplaySim(pSim_);
        initDisplay_ = true;
    }
    auto r   = smolRunSimUntil(pSim_, stoptime);
    curtime_ = stoptime;
    return r == ErrorCode::ECok;
}

void setLowerBounds(const vector<double> bounds)
{
    setDim(bounds.size());
    lowbounds_ = bounds;
}

vector<double> getLowerBounds(void)
{
    return lowbounds_;
}

void setHigherBounds(const vector<double> bounds)
{
    setDim(bounds.size());
    highbounds_ = bounds;
}

vector<double> getHigherBounds(void)
{
    return highbounds_;
}

void setBoundaries(const vector<pair<double, double>>& bounds)
{
    setDim(bounds.size());
    lowbounds_.resize(dim_);
    highbounds_.resize(dim_);
    for(size_t i = 0; i < dim_; i++) {
        lowbounds_[i]  = bounds[i].first;
        highbounds_[i] = bounds[i].second;
    }
    initialize();
}

vector<pair<double, double>> getBoundaries()
{
    vector<pair<double, double>> bounds(getDim());
    for(size_t i = 0; i < getDim(); i++)
        bounds[i] = {lowbounds_[i], highbounds_[i]};
    return bounds;
}

void setPartitions(const char* name, double val)
{
    initialize();
    smolSetPartitions(pSim_, name, val);
}

ErrorCode addSpecies(const char* name, const char* mollist)
{
    return smolAddSpecies(pSim_, name, mollist);
}

ErrorCode setSpeciesMobility(const char* name, MolecState state,
    double difc, vector<double>& drift, vector<double>& difmatrix)
{
    return smolSetSpeciesMobility(
        pSim_, name, state, difc, &drift[0], &difmatrix[0]);
}

ErrorCode setSpeciesStyle(const char* name, const MolecState state,
    double size, char* color)
{
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetMoleculeStyle(pSim_, name, state, size, &rgba[0]);
}

ErrorCode addSurface(const char* name)
{
    return smolAddSurface(pSim_, name);
}

ErrorCode setSurfaceAction(const char* name, enum PanelFace face,
    const char* species, enum MolecState state, enum SrfAction action)
{
    return smolSetSurfaceAction(pSim_, name, face, species, state, action);
}

ErrorCode addPanel(const char* surface, enum PanelShape panelShape,
    const char* panel, const char* axisstring, vector<double>& params)
{
    return smolAddPanel(
        pSim_, surface, panelShape, panel, axisstring, &params[0]);
}

ErrorCode addCompartment(const char* compartment)
{
    return smolAddCompartment(pSim_, compartment);
}

void addCompartmentSurface(const char* compt, const char* surface)
{
    smolAddCompartmentSurface(pSim_, compt, surface);
}

void addCompartmentPoint(const char* compt, vector<double> point)
{
    smolAddCompartmentPoint(pSim_, compt, &point[0]);
}

void addCompartmentMolecules(
    const char* species, size_t number, const char* compt)
{
    smolAddCompartmentMolecules(pSim_, species, number, compt);
}

void addSurfaceMolecules(const char* species, enum MolecState state,
    size_t number, const char* surface, enum PanelShape panelShape,
    const char* panel, vector<double>& position)
{
    smolAddSurfaceMolecules(pSim_, species, state, number, surface, panelShape,
        panel, &position[0]);
}

ErrorCode addReaction(const char* reaction,   // Name of the reaction.
    const char*                       reactant1,  // First reactant
    enum MolecState                   rstate1,    // First reactant state
    const char*                       reactant2,  // Second reactant.
    enum MolecState                   rstate2,    // second reactant state.
    vector<string>                    productSpeciesStr,  // product species.
    vector<enum MolecState>           productStates,      // product state.
    double                            rate                // rate
)
{
    // NOTE: Can't use vector<const char*> in the function argument. We'll
    // runinto wchar_t vs char* issue from python2/python3 str/unicode fiasco.
    // Be safe, use string and cast to const char* by ourselves.
    vector<const char*> productSpecies;
    for(auto s : productSpeciesStr)
        productSpecies.push_back(s.c_str());

    assert(productSpecies.size() == productSpecies.size());

#if 0
    cout << __FUNCTION__ << "Adding reaction: " << reaction << " reac1=" 
        << reactant1 << " reac2=" << reactant2 
        << " prd size=" << productSpecies.size() 
        << " prd[0]=" << productSpecies[0] << " rate: " << rate << endl;
#endif

    return smolAddReaction(pSim_, reaction, reactant1, rstate1, reactant2, rstate2,
        productSpecies.size(), &productSpecies[0], &productStates[0], rate);
}

void setReactionRegion(
    const char* reac, const char* compt, const char* surface)
{
    smolSetReactionRegion(pSim_, reac, compt, surface);
}

ErrorCode setSimTimes(
    const double start, const double stop, const double dt)
{
    if(!pSim_)
        initialize();
    return smolSetSimTimes(pSim_, start, stop, dt);
}

int getMoleculeCount(const char* name, enum MolecState state)
{
    return smolGetMoleculeCount(pSim_, name, state);
}

ErrorCode setDt(double dt)
{
    return smolSetTimeStep(pSim_, dt);
}

double getDt()
{
    return pSim_->dt;
}
