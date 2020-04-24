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

bool Smoldyn::run(double stoptime, double starttime, double dt)
{
    if(!pSim_)
        if(!initialize())
            return false;
    cout << "Running sim for " << starttime << "--> " << stoptime << " s "
         << " dt=" << dt << "." << endl;
    smolSetSimTimes(pSim_, starttime, stoptime, dt);

    return true;
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
        double difc, double* drift=nullptr, double *difmatrix=nullptr)
{
    smolSetSpeciesMobility(pSim_, name.c_str(), state, difc, drift, difmatrix);
}
