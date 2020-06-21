//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>
#include <vector>

using namespace std;

#include "../lib/random2.h"

#include "Smoldyn.h"

// Global variables.
// simptr_uptr_type_ pSim_ = simptr_uptr_type_(nullptr, smolFreeSim);

// Keep all simptrs in a vector. A user can use them for different
// configurations.
vector<simptr> simptrs_;
// This is the current simptr in use.
simptr cursim_;

size_t         dim_ = 0;
vector<double> lowbounds_;
vector<double> highbounds_;
bool           debug_       = false;
double         curtime_     = 0.0;
bool           initDisplay_ = false;

size_t getDim()
{
    return dim_;
}

void setDim(size_t dim)
{
    dim_ = dim;
    if(cursim_)
        cursim_->dim = dim;
}

void setRandomSeed(size_t seed)
{
    if(!cursim_)
        return;
    cursim_->randseed = randomize(seed);
}

size_t getRandomSeed(void)
{
    return cursim_->randseed;
}

bool initialize()
{
    if(cursim_)
        return true;
    if(getDim() <= 0 || getDim() > 3) {
        cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got "
             << getDim() << endl;
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

    cursim_ = smolNewSim(getDim(), &lowbounds_[0], &highbounds_[0]);
    if(debug_)
        smolSetDebugMode(1);
    return cursim_ ? true : false;
}

void runUntil(const double breaktime, const double dt, bool display)
{
    // If dt>0, reset dt else use the old one.
    if(dt > 0.0)
        smolSetTimeStep(cursim_, dt);

    if(display and (!initDisplay_)) {
        smolDisplaySim(cursim_);
        initDisplay_ = true;
    }
    smolRunSimUntil(cursim_, breaktime);
}

bool run(double stoptime, double dt, bool display)
{
    if(!cursim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return false;
        }
    }

    smolSetSimTimes(cursim_, curtime_, stoptime, dt);
    smolUpdateSim(cursim_);

    if(display and !initDisplay_) {
        smolDisplaySim(cursim_);
        initDisplay_ = true;
    }
    auto r   = smolRunSimUntil(cursim_, stoptime);
    curtime_ = stoptime;
    return r == ErrorCode::ECok;
}

void setBoundaries(const vector<pair<double, double>> &bounds)
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
    return smolSetTimeStep(cursim_, dt);
}

double getDt()
{
    return cursim_->dt;
}

