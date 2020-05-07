//    Description:
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include <iostream>
#include <vector>

using namespace std;

#include "../lib/random2.h"

#include "Smoldyn.h"

// Global variables.
simptr_uptr_type_ pSim_ = simptr_uptr_type_(nullptr, smolFreeSim);

// State monitor.
unique_ptr<StateMonitor> pStateMonitor_(new StateMonitor());

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
    if(pSim_.get())
        pSim_.get()->dim = dim;
}

void setRandomSeed(size_t seed)
{
    if(!pSim_.get())
        return;
    pSim_.get()->randseed = randomize(seed);
}

size_t getRandomSeed(void)
{
    return pSim_.get()->randseed;
}

bool initialize()
{
    if(pSim_.get())
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

    pSim_.reset(smolNewSim(getDim(), &lowbounds_[0], &highbounds_[0]));
    if(debug_)
        smolSetDebugMode(1);
    return pSim_.get() ? true : false;
}

void runUntil(const double breaktime, const double dt, bool display)
{
    // If dt>0, reset dt else use the old one.
    if(dt > 0.0)
        smolSetTimeStep(pSim_.get(), dt);

    if(display and (!initDisplay_)) {
        smolDisplaySim(pSim_.get());
        initDisplay_ = true;
    }
    smolRunSimUntil(pSim_.get(), breaktime);
}

bool run(double stoptime, double dt, bool display)
{
    if(!pSim_.get()) {
        if(!initialize()) {
            cerr << __FUNCTION__ << ": Could not initialize sim." << endl;
            return false;
        }
    }

    smolSetSimTimes(pSim_.get(), curtime_, stoptime, dt);
    smolUpdateSim(pSim_.get());

    if(display and !initDisplay_) {
        smolDisplaySim(pSim_.get());
        initDisplay_ = true;
    }
    auto r   = smolRunSimUntil(pSim_.get(), stoptime);
    curtime_ = stoptime;
    return r == ErrorCode::ECok;
}

// Almost same as scmdaddcommand but with a state monitor rather than stdout
// writer.
int addCommand(cmdssptr cmds, char ch, double tmin, double tmax, double dt,
    double on, double off, double step, double multiplier,
    const char *commandstring, const StateMonitor *pSM)
{
    cmdptr cmd;

    if(!cmds)
        return 2;
    if(!commandstring)
        return 0;
    if(!(cmd = scmdalloc()))
        return 1;
    cmd->cmds = cmds;

    if(ch == 'b' || ch == 'a' || ch == '@' || ch == 'i' || ch == 'x') {
        cmd->dt = dt;
        if(ch == 'b')
            cmd->on = cmd->off = tmin - dt;
        else if(ch == 'a')
            cmd->on = cmd->off = tmax + dt;
        else if(ch == '@') {
            cmd->on  = on;
            cmd->off = cmd->on;
        }
        else if(ch == 'i') {
            cmd->on  = on;
            cmd->off = off;
            cmd->dt  = step;
            if(cmd->on < tmin)
                cmd->on = tmin;
            if(cmd->off > tmax)
                cmd->off = tmax;
            if(cmd->dt <= 0)
                return 5;
        }
        else if(ch == 'x') {
            cmd->on  = on;
            cmd->off = off;
            cmd->dt  = step;
            cmd->xt  = multiplier;
            if(cmd->on < tmin)
                cmd->on = tmin;
            if(cmd->off > tmax)
                cmd->off = tmax;
            if(cmd->dt <= 0)
                return 5;
            if(cmd->xt <= 1)
                return 8;
        }
        if(!cmds->cmd)
            if(scmdqalloc(cmds, 10) == 1) {
                scmdfree(cmd);
                return 7;
            }
        if(q_insert(NULL, 0, cmd->on, 0, (void *)cmd, cmds->cmd) == 1)
            if(q_expand(cmds->cmd, q_length(cmds->cmd))) {
                scmdfree(cmd);
                return 7;
            }
    }

    else if(ch == 'B' || ch == 'A' || ch == '&' || ch == 'j' || ch == 'I' ||
            ch == 'E' || ch == 'N' || ch == 'e' || ch == 'n') {
        cmd->oni = 0;
        if(dt == 0 || tmin >= tmax)
            cmd->offi = Q_LLONG_MAX;
        else
            cmd->offi = (Q_LONGLONG)((tmax - tmin) / dt + 0.5);
        cmd->dti = 1;
        if(ch == 'B')
            cmd->oni = cmd->offi = -1;
        else if(ch == 'A')
            cmd->oni = cmd->offi =
                (cmd->offi == Q_LLONG_MAX ? cmd->offi : cmd->offi + 1);
        else if(ch == '&') {
            cmd->oni  = (Q_LONGLONG)on;
            cmd->offi = cmd->oni;
        }
        else if(ch == 'j' || ch == 'I') {
            cmd->oni  = (Q_LONGLONG)on;
            cmd->offi = (Q_LONGLONG)off;
            cmd->dti  = (Q_LONGLONG)step;
            if(cmd->dti <= 0)
                return 5;
        }
        else if(ch == 'e' || ch == 'E')
            ;
        else if(ch == 'n' || ch == 'N') {
            cmd->dti = (Q_LONGLONG)step;
            if(cmd->dti < 1)
                return 5;
        }
        if(!cmds->cmdi)
            if(scmdqalloci(cmds, 10) == 1) {
                scmdfree(cmd);
                return 7;
            }
        if(q_insert(NULL, 0, 0, cmd->oni, (void *)cmd, cmds->cmdi) == 1)
            if(q_expand(cmds->cmdi, q_length(cmds->cmdi))) {
                scmdfree(cmd);
                return 7;
            }
    }

    else
        return 6;
    strncpy(cmd->str, commandstring, STRCHAR);
    if(cmd->str[strlen(cmd->str) - 1] == '\n')
        cmd->str[strlen(cmd->str) - 1] = '\0';
    return 0;
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
    return smolSetTimeStep(pSim_.get(), dt);
}

double getDt()
{
    return pSim_.get()->dt;
}

