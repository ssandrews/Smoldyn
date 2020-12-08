//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

#include "../libSteve/random2.h"

#include "Smoldyn.h"
#include "CallbackFunc.h"

// Global variables.
// Keep all simptrs in a vector. A user can use them for different
// configurations.
// vector<simptr_uptr_type_> simptrs_;
vector<simptr> simptrs_;

// This is the current simptr in use.
simptr cursim_;

size_t dim_ = 0;
vector<double> lowbounds_;
vector<double> highbounds_;
bool debug_       = false;
double curtime_   = 0.0;
bool initDisplay_ = false;

bool connect(const py::function& func, const py::object& target, const size_t step,
    const py::list& args)
{
    assert(cursim_->ncallbacks < MAX_PY_CALLBACK);
    if(cursim_->ncallbacks >= MAX_PY_CALLBACK) {
        py::print("Error: Maximum of ", MAX_PY_CALLBACK,
            " callbacks are allowed. Current number of callbacks: ", cursim_->ncallbacks);
        return false;
    }

    // cleanup is the job of simfree
    auto f = new CallbackFunc();
    f->setFunc(func);
    f->setStep(step);
    f->setTarget(target);
    f->setArgs(args);
    cursim_->callbacks[cursim_->ncallbacks] = f;
    cursim_->ncallbacks += 1;
    return true;
}

bool addToSimptrVec(simptr ptr)
{
    auto p = std::find(simptrs_.begin(), simptrs_.end(), ptr);
    if(p == simptrs_.end()) {
        simptrs_.push_back(ptr);
        return true;
    }
    return false;
}

/**
 * @brief Delete a given simptr (use it with care).
 *
 * @param ptr simptr aka simstruct*
 *
 * @return 
 */
bool deleteSimptr(simptr ptr)
{
    auto p = std::find(simptrs_.begin(), simptrs_.end(), ptr);
    if(p != simptrs_.end()) {
        simptrs_.erase(p);
        simfree(ptr);
        return true;
    }
    return false;
}

/**
 * @brief Get dimension of bouds.
 *
 * @return 
 */
size_t getDim()
{
    assert(lowbounds_.size() == (size_t)cursim_->dim);
    return cursim_->dim;
}

void setDim(size_t dim)
{
    assert(cursim_);
    cursim_->dim = dim;
}

void printSimptrNotInitWarning(const char* funcname)
{
    py::print("Warn:", funcname, "simptr is not initialized. set boundaries/dim first.");
}

/**
 * @brief Get the random seed.
 *
 * @return 
 */
size_t getRandomSeed(void)
{
    if(!cursim_) {
        printSimptrNotInitWarning(__FUNCTION__);
        return -1;
    }
    return cursim_->randseed;
}

/**
 * @brief Initialize current simptr structure.
 *
 * @return 
 */
bool initialize()
{
    if(cursim_)
        return true;
    if(getDim() <= 0 || getDim() > 3) {
        cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got " << getDim() << endl;
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
                 << " which is not true at index " << d << " where lowbounds is "
                 << lowbounds_[d] << " and highbound is " << highbounds_[d] << endl;
            return false;
        }
    }

    cursim_ = smolNewSim(getDim(), &lowbounds_[0], &highbounds_[0]);

    if(debug_)
        smolSetDebugMode(1);
    return cursim_ ? true : false;
}

ErrorCode runUntil(
    const double breaktime, const double dt, bool display, bool overwrite = false)
{
    if(!cursim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return ECerror;
        }
    }

    auto er = smolOpenOutputFiles(cursim_, overwrite);
    if(er != ErrorCode::ECok) {
        cerr << __FUNCTION__ << ": Simulation skipped." << endl;
    }

    // If dt>0, reset dt else use the old one.
    if(dt > 0.0)
        smolSetTimeStep(cursim_, dt);
    smolUpdateSim(cursim_);

    if(display && (!initDisplay_)) {
        smolDisplaySim(cursim_);
        initDisplay_ = true;
    }
    return smolRunSimUntil(cursim_, breaktime);
}

/**
 * @brief Run the simulation for given time.
 *
 * @param stoptime, time to run (second).
 * @param dt, step size (second)
 * @param display, if `true`, display graphics.
 * @param overwrite, if `true`, overwrite existing data files.
 *
 * @return 
 */
ErrorCode runSimulation(double stoptime, double dt, bool display, bool overwrite = false)
{
    ErrorCode er;

    if(!cursim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return ECerror;
        }
    }

    er = smolOpenOutputFiles(cursim_, overwrite);
    if(er != ErrorCode::ECok) {
        cerr << __FUNCTION__ << ": Could not open output files." << endl;
        return er;
    }

    er = smolSetSimTimes(cursim_, curtime_, stoptime, dt);
    er = smolUpdateSim(cursim_);

    if(display && !initDisplay_) {
        smolDisplaySim(cursim_);
        initDisplay_ = true;
    }

    er   = smolRunSim(cursim_);
    curtime_ = stoptime;

    return er;
}

/**
 * @brief Set boundary. Also see the overloaded function.
 *
 * @param lowbounds, lower values e.g. { xlow, ylow, zlow }.
 * @param highbounds, High values e.g, { xhigh, yhigh, zhigh }
 */
void setBoundaries(vector<double>& lowbounds, vector<double>& highbounds)
{
    lowbounds_  = lowbounds;
    highbounds_ = highbounds;
    assert(lowbounds.size() == highbounds.size());
    if(cursim_)
        simfree(cursim_);
    cursim_ = smolNewSim(lowbounds.size(), &lowbounds[0], &highbounds[0]);
    return;
}

/**
 * @brief Set bounds on current simulation.
 *
 * @param bounds, a vector of (low, high) values. For examples [(xlow, xhigh),
 * (ylow, yhigh), (zlow, zhigh)].
 */
void setBoundaries(const vector<pair<double, double>>& bounds)
{
    vector<double> lowbounds, highbounds;
    for(const auto v : bounds) {
        lowbounds.push_back(v.first);
        highbounds.push_back(v.second);
    }
    setBoundaries(lowbounds, highbounds);
    return;
}

/**
 * @brief Get current simulation bounds.
 *
 * @return a 2-tuple of list: (low, high).
 */
pair<vector<double>, vector<double>> getBoundaries()
{
    // vector<pair<double, double>> bounds(getDim());
    pair<vector<double>, vector<double>> bounds;
    bounds.first.resize(getDim());
    bounds.second.resize(getDim());

    for(size_t i = 0; i < getDim(); i++) {
        bounds.first[i]  = lowbounds_[i];
        bounds.second[i] = highbounds_[i];
    }
    return bounds;
}

/**
 * @brief Set dt for current simulation.
 *
 * @param dt (float, second).
 *
 * @return ECok on success.
 */
ErrorCode setDt(double dt)
{
    return smolSetTimeStep(cursim_, dt);
}

/**
 * @brief Get dt of current simulation.
 *
 * @return dt (float)
 */
double getDt()
{
    return cursim_->dt;
}

/**
 * @brief Set filepath and filename on cursim_. When python scripts are used,
 * these variables are not set my smolsimulate and related fucntions. 
 *
 * @param modelpath modelpath e.g. when using command `python3 ~/Work/smoldyn.py`
 * in terminal, modelpath is set to `/home/user/Work/Smoldyn.py`.
 *
 * @return true.
 */
bool setModelpath(const string& modelpath)
{
    auto p = splitPath(modelpath);
    strncpy(cursim_->filepath, p.first.c_str(), p.first.size());
    strncpy(cursim_->filename, p.second.c_str(), p.second.size());
    return true;
}
