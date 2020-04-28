//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>
#include <vector>

using namespace std;

#include "../lib/random2.h"
#include "Smoldyn.h"

Smoldyn::Smoldyn()
    : pSim_(nullptr), dim_(0), debug_(false), curtime_(0.0)
{
}

Smoldyn::Smoldyn(bool debug)
    : pSim_(nullptr), dim_(0), debug_(false), curtime_(0.0)
{
}

Smoldyn::~Smoldyn()
{
    smolFreeSim(pSim_);
}

SmoldynDefine& Smoldyn::getDefine()
{
    return define_;
}

size_t Smoldyn::getDim() const
{
    return dim_;
}

void Smoldyn::setDim(size_t dim)
{
    dim_ = dim;
}


void Smoldyn::setRandomSeed(size_t seed)
{
    if(!pSim_)
        return;
    pSim_->randseed = randomize(seed);
}

size_t Smoldyn::getRandomSeed(void)
{
    return pSim_->randseed;
}

bool Smoldyn::initialize()
{
    if(pSim_)
        return true;
    if(dim_ <= 0 || dim_ > 3) {
        cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got " << dim_
             << endl;
        return false;
    }

    if(lowbounds_.size() != dim_) {
        cerr << __FUNCTION__ << ": missing lowbounds" << endl;
        return false;
    }

    if(highbounds_.size() != dim_) {
        cerr << __FUNCTION__ << ": missing highbounds" << endl;
        return false;
    }

    for(size_t d = 0; d < dim_; d++) {
        if(lowbounds_[d] >= highbounds_[d]) {
            cerr << __FUNCTION__ << ": lowbounds must be < highbounds"
                 << " which is not true at index " << d
                 << " where lowbounds is " << lowbounds_[d]
                 << " and highbound is " << highbounds_[d] << endl;
            return false;
        }
    }

    pSim_ = smolNewSim(dim_, &lowbounds_[0], &highbounds_[0]);
    if(debug_)
        smolSetDebugMode(1);
    return pSim_ ? true : false;
}

void Smoldyn::runUntil(const double breaktime, const double dt, bool display)
{
    // If dt>0, reset dt else use the old one.
    if(dt > 0.0)
        smolSetTimeStep(pSim_, dt);

    if(display and (! initDisplay_)) {
        smolDisplaySim(pSim_);
        initDisplay_ = true;
    }
    smolRunSimUntil(pSim_, breaktime);
}

bool Smoldyn::run(double stoptime, double dt, bool display)
{
    if(!pSim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return false;
        }
    }

    smolSetSimTimes(pSim_, curtime_, stoptime, dt);
    smolUpdateSim(pSim_);

    if(display and ! initDisplay_) {
        smolDisplaySim(pSim_);
        initDisplay_ = true;
    }
    auto r   = smolRunSimUntil(pSim_, stoptime);
    curtime_ = stoptime;
    return r == ErrorCode::ECok;
}

void Smoldyn::setLowerBounds(const vector<double> bounds)
{
    dim_       = bounds.size();
    lowbounds_ = bounds;
}

vector<double> Smoldyn::getLowerBounds(void) const
{
    return lowbounds_;
}

void Smoldyn::setHigherBounds(const vector<double> bounds)
{
    dim_        = bounds.size();
    highbounds_ = bounds;
}

vector<double> Smoldyn::getHigherBounds(void) const
{
    return highbounds_;
}

void Smoldyn::setBounds(const vector<pair<double, double>>& bounds)
{
    dim_ = bounds.size();
    lowbounds_.resize(dim_);
    highbounds_.resize(dim_);
    for(size_t i = 0; i < dim_; i++) {
        lowbounds_[i]  = bounds[i].first;
        highbounds_[i] = bounds[i].second;
    }
    initialize();
}

vector<pair<double, double>> Smoldyn::getBounds() const
{
    vector<pair<double, double>> bounds(dim_);
    for(size_t i = 0; i < dim_; i++)
        bounds[i] = {lowbounds_[i], highbounds_[i]};
    return bounds;
}

void Smoldyn::setPartitions(const char* name, double val)
{
    initialize();
    smolSetPartitions(pSim_, name, val);
}

void Smoldyn::addSpecies(const char* name, const char* param)
{
    smolAddSpecies(pSim_, name, param);
}

void Smoldyn::setSpeciesMobility(const char* name, MolecState state,
    double difc, vector<double>& drift, vector<double>& difmatrix)
{
    smolSetSpeciesMobility(pSim_, name, state, difc, &drift[0], &difmatrix[0]);
}

void Smoldyn::addSurface(const char* name)
{
    smolAddSurface(pSim_, name);
}

void Smoldyn::setSurfaceAction(const char* name, enum PanelFace face,
    const char* species, enum MolecState state, enum SrfAction action)
{
    smolSetSurfaceAction(pSim_, name, face, species, state, action);
}

void Smoldyn::addPanel(const char* surface, enum PanelShape panelShape,
    const char* panel, const char* axisstring, vector<double>& params)
{
    smolAddPanel(pSim_, surface, panelShape, panel, axisstring, &params[0]);
}

void Smoldyn::addCompartment(const char* compartment)
{
    smolAddCompartment(pSim_, compartment);
}

void Smoldyn::addCompartmentSurface(const char* compt, const char* surface)
{
    smolAddCompartmentSurface(pSim_, compt, surface);
}

void Smoldyn::addCompartmentPoint(const char* compt, vector<double> point)
{
    smolAddCompartmentPoint(pSim_, compt, &point[0]);
}

void Smoldyn::addCompartmentMolecules(
    const char* species, size_t number, const char* compt)
{
    smolAddCompartmentMolecules(pSim_, species, number, compt);
}

void Smoldyn::addSurfaceMolecules(const char* species, enum MolecState state,
    size_t number, const char* surface, enum PanelShape panelShape,
    const char* panel, vector<double>& position)
{
    smolAddSurfaceMolecules(pSim_, species, state, number, surface, panelShape,
        panel, &position[0]);
}

void Smoldyn::addReaction(const char* reaction,   // Name of the reaction.
    const char*                          reactant1,  // First reactant
    enum MolecState                      rstate1,    // First reactant state
    const char*                          reactant2,  // Second reactant.
    enum MolecState                      rstate2,    // second reactant state.
    vector<string>                       productSpeciesStr,  // product species.
    vector<enum MolecState>              productStates,      // product state.
    double                               rate                // rate
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

    smolAddReaction(pSim_, reaction, reactant1, rstate1, reactant2, rstate2,
        productSpecies.size(), &productSpecies[0], &productStates[0], rate);
}

void Smoldyn::setReactionRegion(
    const char* reac, const char* compt, const char* surface)
{
    smolSetReactionRegion(pSim_, reac, compt, surface);
}

void Smoldyn::setSimTimes(
    const double start, const double stop, const double dt)
{
    if(!pSim_)
        initialize();
    smolSetSimTimes(pSim_, start, stop, dt);
}

int Smoldyn::getMoleculeCount(const char* name, enum MolecState state)
{
    return smolGetMoleculeCount(pSim_, name, state);
}

void Smoldyn::setDt(double dt)
{
    smolSetTimeStep(pSim_, dt);
}

double Smoldyn::getDt() const
{
    return pSim_->dt;
}
