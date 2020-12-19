/***
 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simulation class.
 */

#ifndef SIMULATION_H
#define SIMULATION_H

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

protected:
private:
    simptr         sim_;
    size_t         dim_;
    vector<string> boundary_type_;
    vector<double> low_;
    vector<double> high_;
};

#endif /* end of include guard:  */
