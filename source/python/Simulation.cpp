/***
 *        Created:  2020-12-19

 *         Author:  Dilawar Singh <dilawar@subcom.tech>
 *    Description:  Simualtion class.
 */

#include <vector>
using namespace std;

#include "../source/Smoldyn/smoldyn.h"
#include "../source/Smoldyn/libsmoldyn.h"
#include "Simulation.h"

Simulation::Simulation(
    vector<double>& low, vector<double>& high, vector<string>& boundary_type)
    : low_(low), high_(high), dim_(low.size()), boundary_type_(boundary_type)
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
}


simptr Simulation::getSimptr() const
{
    return sim_;
}
