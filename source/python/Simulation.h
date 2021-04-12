/***
 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simulation class.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <iostream>
#include <memory>
#include <vector>

using namespace std;

#include "../Smoldyn/libsmoldyn.h"
#include "../libSteve/opengl2.h"

#include "Command.h"
#include "util.h"

#include "pybind11/pybind11.h"
namespace py = pybind11;

using namespace std;

/*! \class Simulation
 *  \brief Simulation class.
 *
 * Simulation is the top-most object.
 */

class Simulation
{

  public:
    Simulation(vector<double> &low, vector<double> &high,
               vector<string> &boundary_type);

    // Factory function.
    Simulation(const char *filepath, const char *flags);

    ~Simulation();

    size_t getDim() const;

    bool initialize();

    /**
     * @brief Set filepath and filename on sim_->getSimPtr(). When python
     * scripts are used, these variables are not set my smolsimulate and related
     * fucntions.
     *
     * @param modelpath modelpath e.g. when using command `python3
     * ~/Work/smoldyn.py` in terminal, modelpath is set to
     * `/home/user/Work/Smoldyn.py`.
     *
     * @return true.
     */
    bool setModelpath(const string &modelpath);

    ErrorCode runSim(double stoptime, double dt, bool display, bool overwrite);

    ErrorCode runUntil(const double breaktime, const double dt, bool display,
                       bool overwrite);

    bool connect(const py::function &func, const py::object &target,
                 const size_t step, const py::list &args);

    // get the pointer
    simptr getSimPtr() const;

    // set the pointer.
    void setSimPtr(simptr sim);

    void setCurtime(double curtime);

    double getCurtime() const;

    bool isValid();

    pair<vector<double>, vector<double>> getBoundaries(void);

    // Commands
    void addCommand(const string &cmd, char cmd_type,
                    const map<string, double> &kwargs);
    void addCommandStr(char *cmd);

  private:
    simptr sim_;
    vector<double> low_;
    vector<double> high_;
    double curtime_;
    bool initDisplay_;
    bool debug_;

    vector<std::unique_ptr<Command>> commands_;
};

#endif /* end of include guard:  */
