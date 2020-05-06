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

bool run(double stoptime, double dt, bool display)
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

ErrorCode setDt(double dt)
{
    return smolSetTimeStep(pSim_, dt);
}

double getDt()
{
    return pSim_->dt;
}
