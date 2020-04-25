//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>

#include "../lib/random2.h"

#include "Smoldyn.h"

Smoldyn::Smoldyn() : pSim_(nullptr), dim_(0)
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
    cout << "Initializing sim struct " << endl;
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
    return pSim_ ? true : false;
}

bool Smoldyn::run(double stoptime, double starttime, double dt,
                       bool display = true)
{
    if(!pSim_)
        if(!initialize())
            return false;

    smolSetSimTimes(pSim_, starttime, stoptime, dt);
    if(display)
        smolDisplaySim(pSim_);
    auto r = smolRunSim(pSim_);
    return r == ErrorCode::ECok;
}

void Smoldyn::setLowerBounds(const vector<double> bounds)
{
    dim_ = bounds.size();
    lowbounds_ = bounds;
}

vector<double> Smoldyn::getLowerBounds(void) const
{
    return lowbounds_;
}

void Smoldyn::setHigherBounds(const vector<double> bounds)
{
    dim_ = bounds.size();
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
        lowbounds_[i] = bounds[i].first;
        highbounds_[i] = bounds[i].second;
    }
}

vector<pair<double, double>> Smoldyn::getBounds() const
{
    vector<pair<double, double>> bounds(dim_);
    for(size_t i = 0; i < dim_; i++)
        bounds[i] = {lowbounds_[i], highbounds_[i]};
    return bounds;
}

void Smoldyn::setPartitions(const string& name, double val)
{
    initialize();
    smolSetPartitions(pSim_, name.c_str(), val);
}

void Smoldyn::addSpecies(const string& name, const string& param)
{
    smolAddSpecies(pSim_, name.c_str(), param.c_str());
}

void Smoldyn::setSpeciesMobility(const string& name, MolecState state,
                                 double difc, double* drift = nullptr,
                                 double* difmatrix = nullptr)
{
    smolSetSpeciesMobility(pSim_, name.c_str(), state, difc, drift, difmatrix);
}

void Smoldyn::addSurface(const string& name)
{
    smolAddSurface(pSim_, name.c_str());
}

void Smoldyn::setSurfaceAction(const string& name, enum PanelFace face,
                               const string& species, enum MolecState state,
                               enum SrfAction action)
{
    smolSetSurfaceAction(pSim_, name.c_str(), face, species.c_str(), state,
                         action);
}

void Smoldyn::addPanel(const string& surface, enum PanelShape panelShape,
                       const string& panel, const string& axisstring,
                       vector<double>& params)
{
    smolAddPanel(pSim_, surface.c_str(), panelShape, panel.c_str(),
                 axisstring.c_str(), &params[0]);
}

void Smoldyn::addCompartment(const string& compartment)
{
    smolAddCompartment(pSim_, compartment.c_str());
}

void Smoldyn::addCompartmentSurface(const string& compt, const string& surface)
{
    smolAddCompartmentSurface(pSim_, compt.c_str(), surface.c_str());
}

void Smoldyn::addCompartmentPoint(const string& compt, vector<double> point)
{
    smolAddCompartmentPoint(pSim_, compt.c_str(), &point[0]);
}

void Smoldyn::addCompartmentMolecules(const string& species, size_t number,
                                      const string& compt)
{
    smolAddCompartmentMolecules(pSim_, species.c_str(), number, compt.c_str());
}

void Smoldyn::addSurfaceMolecules(const string& species, enum MolecState state,
                                  size_t number, const string& surface,
                                  enum PanelShape panelShape,
                                  const string& panel, vector<double>& position)
{
    smolAddSurfaceMolecules(pSim_, species.c_str(), state, number,
                            surface.c_str(), panelShape, panel.c_str(),
                            &position[0]);
}

void Smoldyn::addReaction(const string& reaction, const string& reactant1,
                          enum MolecState rstate1, const string& reactant2,
                          enum MolecState rstate2, size_t nproduct,
                          vector<const char*>& productSpecies,
                          vector<enum MolecState> productStates, double rate)
{
    smolAddReaction(pSim_, reaction.c_str(), reactant1.c_str(), rstate1,
                    reactant2.c_str(), rstate2, nproduct, &productSpecies[0],
                    &productStates[0], rate);
}

void Smoldyn::setReactionRegion(const string& reac, const string& compt,
                                const string& surface)
{
    smolSetReactionRegion(pSim_, reac.c_str(), compt.c_str(), surface.c_str());
}
