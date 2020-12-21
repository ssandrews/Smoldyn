/***
 *        Created:  2020-12-19

 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simualtion class.
 */

#include <vector>
#include <iostream>
using namespace std;

#include "../source/Smoldyn/smoldyn.h"
#include "../source/Smoldyn/libsmoldyn.h"
#include "../source/libSteve/opengl2.h"

#include "pybind11/pybind11.h"

#include "Simulation.h"

Simulation::Simulation(
    vector<double>& low, vector<double>& high, vector<string>& boundary_type)
    : low_(low),
      high_(high),
      dim_(low.size()),
      boundary_type_(boundary_type),
      curtime_(0.0),
      initDisplay_(false),
      debug_(false)
{
    assert(low.size() == high.size());
    dim_ = low.size();
    sim_ = smolNewSim(low.size(), &low[0], &high[0]);

    // for _d, _t in zip(range(self.dim), self.types):
    for(size_t d = 0; d < dim_; d++) {
        string t = boundary_type[d];
        if(t.size() == 1)
            smolSetBoundaryType(sim_, d, -1, t[0]);
        else
            for(size_t ii = 0; ii < 2; ii++)
                smolSetBoundaryType(sim_, d, ii, t[ii]);
    }
}

Simulation::~Simulation()
{
    if(sim_)
        delete sim_;
}

simptr Simulation::getSimptr() const
{
    return sim_;
}

size_t Simulation::getDim() const
{
    return dim_;
}

bool Simulation::initialize()
{
    if(sim_)
        return true;
    if(getDim() <= 0 || getDim() > 3) {
        cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got " << getDim() << endl;
        return false;
    }

    if(low_.size() != getDim()) {
        cerr << __FUNCTION__ << ": missing lowbounds" << endl;
        return false;
    }

    if(high_.size() != getDim()) {
        cerr << __FUNCTION__ << ": missing highbounds" << endl;
        return false;
    }

    for(size_t d = 0; d < getDim(); d++) {
        if(low_[d] >= high_[d]) {
            cerr << __FUNCTION__ << ": lowbounds must be < highbounds"
                 << " which is not true at index " << d << " where lowbounds is "
                 << low_[d] << " and highbound is " << high_[d] << endl;
            return false;
        }
    }

    sim_ = smolNewSim(getDim(), &low_[0], &high_[0]);

    if(debug_)
        smolSetDebugMode(1);
    return sim_ ? true : false;
}

bool Simulation::run(
    double stoptime, double dt, bool display = true, bool overwrite = false)
{
    ErrorCode er;

    if(!sim_) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return ECerror;
        }
    }

    gl2glutInit(0, NULL);

    er = smolOpenOutputFiles(sim_, overwrite);
    if(er != ErrorCode::ECok) {
        cerr << __FUNCTION__ << ": Could not open output files." << endl;
        return er;
    }

    er = smolSetSimTimes(sim_, curtime_, stoptime, dt);
    er = smolUpdateSim(sim_);

    if(display && !initDisplay_) {
        smolDisplaySim(sim_);
        initDisplay_ = true;
    }

    er       = smolRunSim(sim_);
    curtime_ = stoptime;

    return er;
}

bool Simulation::connect(const py::function& func, const py::object& target,
    const size_t step, const py::list& args)
{
    assert(sim_->ncallbacks < MAX_PY_CALLBACK);
    if(sim_->ncallbacks >= MAX_PY_CALLBACK) {
        py::print("Error: Maximum of ", MAX_PY_CALLBACK,
            " callbacks are allowed. Current number of callbacks: ", sim_->ncallbacks);
        return false;
    }

    // cleanup is the job of simfree
    auto f = new CallbackFunc();
    f->setFunc(func);
    f->setStep(step);
    f->setTarget(target);
    f->setArgs(args);
    sim_->callbacks[sim_->ncallbacks] = f;
    sim_->ncallbacks += 1;
    return true;
}

