/***
 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simulation class.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

#include <vector>

using namespace std;

/*! \class Simulation
 *  \brief Simulation class.
 *
 *  Detailed description
 */

class Simulation {
public:
    Simulation(vector<double>& low, vector<double>& high, vector<string>& boundary_type);
    ~Simulation();

    simptr getSimptr() const;

    size_t getDim() const;

    bool initialize();
    bool run(double stop, double dt, bool display, bool overwrite);

    bool connect(const py::function& func, const py::object& target, const size_t step,
        const py::list& args);

protected:
private:
    simptr         sim_;
    size_t         dim_;
    vector<string> boundary_type_;
    vector<double> low_;
    vector<double> high_;
    double         curtime_;
    bool           initDisplay_;
    bool           debug_;
};

#endif /* end of include guard:  */
