/***
 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simulation class.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>
#include <iostream>
using namespace std;

#include "../Smoldyn/libsmoldyn.h"
#include "../libSteve/opengl2.h"

#include "util.h"

#include "pybind11/pybind11.h"

using namespace std;

/*! \class Simulation
 *  \brief Simulation class.
 *
 *  Detailed description
 */

class Simulation {
public:
    Simulation(vector<double>& low, vector<double>& high, vector<string>& boundary_type)
        : low_(low),
          high_(high),
          curtime_(0.0),
          initDisplay_(false),
          debug_(false)
    {
        assert(low.size() == high.size());

        sim_ = smolNewSim(low.size(), &low[0], &high[0]);
        assert(sim_);

        assert(boundary_type.size() == getDim());

        for(size_t d = 0; d < getDim(); d++) {
            string t = boundary_type[d];
            if(t.size() == 1)
                smolSetBoundaryType(sim_, d, -1, t[0]);
            else
                for(size_t ii = 0; ii < 2; ii++)
                    smolSetBoundaryType(sim_, d, ii, t[ii]);
        }
    }

    Simulation(simptr sim)
        : sim_(sim), initDisplay_(false), debug_(false), curtime_(0.0)
    {
        // Just copy the simptr.
        // FIXME: I am forgetting anything here.
    }

    ~Simulation()
    {
        smolFreeSim(sim_);
    }

    size_t getDim() const
    {
        assert(sim_->dim >= 0);
        return (size_t)sim_->dim;
    }

    bool initialize()
    {
        if(sim_)
            return true;

        if(getDim() == 0 || getDim() > 3) {
            cerr << __FUNCTION__ << ": dim must be between 0 and 3. Got " << getDim()
                 << endl;
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
        assert(sim_);

        if(debug_)
            smolSetDebugMode(1);
        return sim_ ? true : false;
    }

    /**
     * @brief Set filepath and filename on sim_->getSimPtr(). When python scripts are
     * used, these variables are not set my smolsimulate and related fucntions.
     *
     * @param modelpath modelpath e.g. when using command `python3 ~/Work/smoldyn.py`
     * in terminal, modelpath is set to `/home/user/Work/Smoldyn.py`.
     *
     * @return true.
     */
    inline bool setModelpath(const string& modelpath)
    {
        assert(sim_);
        auto p = splitPath(modelpath);
        strncpy(sim_->filepath, p.first.c_str(), p.first.size());
        strncpy(sim_->filename, p.second.c_str(), p.second.size());
        return true;
    }

    inline ErrorCode runSim(double stoptime, double dt, bool display, bool overwrite)
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
        if(er != ErrorCode::ECok) {
            cerr << __FUNCTION__ << ": Could not set sim times." << endl;
            return er;
        }

        er = smolUpdateSim(sim_);
        if(er != ErrorCode::ECok) {
            cerr << __FUNCTION__ << ": Could not update simstruct." << endl;
            return er;
        }

        if(display && !initDisplay_) {
            smolDisplaySim(sim_);
            initDisplay_ = true;
        }

        er       = smolRunSim(sim_);
        curtime_ = stoptime;

        return er;
    }

    inline ErrorCode runUntil(
        const double breaktime, const double dt, bool display, bool overwrite)
    {
        assert(sim_);

        if(!sim_) {
            if(!initialize()) {
                cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
                return ECerror;
            }
        }

        auto er = smolOpenOutputFiles(sim_, overwrite);
        if(er != ErrorCode::ECok) {
            cerr << __FUNCTION__ << ": Simulation skipped." << endl;
        }

        // If dt>0, reset dt else use the old one.
        if(dt > 0.0)
            smolSetTimeStep(sim_, dt);
        smolUpdateSim(sim_);

        if(display && (!initDisplay_)) {
            smolDisplaySim(sim_);
            initDisplay_ = true;
        }
        return smolRunSimUntil(sim_, breaktime);
    }

    inline bool connect(const py::function& func, const py::object& target,
        const size_t step, const py::list& args)
    {
        assert(sim_->ncallbacks < MAX_PY_CALLBACK);
        if(sim_->ncallbacks >= MAX_PY_CALLBACK) {
            py::print("Error: Maximum of ", MAX_PY_CALLBACK,
                " callbacks are allowed. Current number of callbacks: ",
                sim_->ncallbacks);
            return false;
        }

        // Note: cleanup is the job of simfree.
        // I thought of using unique_ptr here but then gave up on that idea.
        // simfree is already being used for cleaning up.
        auto f = new CallbackFunc();
        f->setFunc(func);
        f->setStep(step);
        f->setTarget(target);
        f->setArgs(args);
        sim_->callbacks[sim_->ncallbacks] = f;
        sim_->ncallbacks += 1;
        return true;
    }

    // get the pointer
    inline simptr getSimPtr()
    {
        assert(sim_);
        return sim_;
    }

    // set the pointer.
    void setSimPtr(simptr sim)
    {
        sim_ = sim;
    }

    void setCurtime(double curtime)
    {
        curtime_ = curtime;
    }

    double getCurtime() const
    {
        return curtime_;
    }

    bool isValid()
    {
        return sim_ ? true : false;
    }

    pair<vector<double>, vector<double>> getBoundaries(void)
    {
        vector<double> low;
        vector<double> high;
        for(size_t d = 0; d < sim_->dim; d++) {
            low.push_back(sim_->wlist[2 * d]->pos);
            high.push_back(sim_->wlist[2 * d + 1]->pos);
        }
        return make_pair(low, high);
    }

private:
    simptr         sim_;
    vector<double> low_;
    vector<double> high_;
    double         curtime_;
    bool           initDisplay_;
    bool           debug_;
};

#endif /* end of include guard:  */
