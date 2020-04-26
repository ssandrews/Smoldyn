---
title: Python bindings
author: Dilawar Singh
---

 __This is a work in progress.__

To build Smoldyn's python module `_smoldyn.so`, we use excellent
[pybind11](https://pybind11.readthedocs.io/en/stable/) library. It is included
in the source tree. The compilation `_smoldyn.so` requires a `c++11` compliant
compiler. Most compilers these days support `-std=c++11` flags.

To begin with, we expose [Smoldyn C library](LibsmoldynManual.md) to Python
using `pybind11`. An example is shown below.

<table>
<tr><th>C library</th><th>Python Version</th></tr>
<tr><td>
```c
#include "/usr/local/include/libsmoldyn.h"
int main(int argc, char* argv[])
{
    simptr sim;
    double lbounds[2];
    double hbounds[2];
    double params[10];
    const char* species[1] = {"ATP"};
    enum MolecState states[1] = {MSsoln};

    smolSetDebugMode(1);

    lbounds[0] = -50.0;
    lbounds[1] = -50.0;
    hbounds[0] = 50.0;
    hbounds[1] = 50.0;

    sim = smolNewSim(2, lbounds, hbounds);

    smolSetRandomSeed(sim, 1);
    smolSetSimTimes(sim, 0.0, 250.0, 0.01);

    smolSetPartitions(sim, "molperbox", 4);
    smolSetPartitions(sim, "boxsize", 12.5);

    smolAddSpecies(sim, "ACA", "");
    smolAddSpecies(sim, "ATP", "");
    smolAddSpecies(sim, "cAMP", "");
    smolAddSpecies(sim, "cAR1", "");

    smolSetSpeciesMobility(sim, "ACA", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "ATP", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "cAMP", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "cAR1", MSall, 1, 0, 0);

    smolAddSurface(sim, "Membrane00");
    smolSetSurfaceAction(sim, "Membrane00", PFboth,
        "ATP", MSsoln, SAreflect);

    params[0] = -20.0;
    params[1] = 20.0;
    params[2] = 10.0;
    params[3] = 20.0;
    params[4] = 20.0;
    smolAddPanel(sim, "Membrane00", PSsph, "", "", params);

    smolAddCompartment(sim, "Cell00");
    smolAddCompartmentSurface(sim, "Cell00", "Membrane00");
    smolAddCompartmentPoint(sim, "Cell00", params);

    smolAddSurfaceMolecules(sim, "ACA", MSdown, 30
        , "Membrane00", PSall, "all", 0);
    smolAddSurfaceMolecules(sim, "cAR1", MSup, 30
        , "Membrane00", PSall, "all", 0);

    smolAddReaction(sim, "r100", "", MSall, "", MSall
        , 1, species, states, 0.02);
    smolSetReactionRegion(sim, "r100", "Cell00", "");

    smolUpdateSim(sim);
    smolDisplaySim(sim);
    smolRunSim(sim);
    smolFreeSim(sim);
    return 0;
}
```
</td><td>
```python
import smoldyn
s = smoldyn.Simulation()
s.bounds = ([-50, 50], [-50, 50])
assert s.dim == 2
s.seed = 1
print('Bounds', s.bounds)

# TODO:
# s = smoldyn.Simulation(dim=2, bounds=([-50,50], [-50,50]))

# This is handled by run( ) now.
#  s.setSimTimes(0, 250.0, 0.01)

s.setPartitions("molperbox", 4);
s.setPartitions("boxsize", 12.5);

s.addSpecies("ACA")
s.addSpecies("ATP")
s.addSpecies("cAMP")
s.addSpecies("cAR1")

MS = smoldyn.MS     # enum MolcState
SA = smoldyn.SA     # enum SrfAction
PF = smoldyn.PF     # enum PanelFace
PS = smoldyn.PS     # enum PanelShape

s.setSpeciesMobility("ACA", MS.all, 1.0)
s.setSpeciesMobility("ATP", MS.all, 1.0)
s.setSpeciesMobility("cAMP", MS.all, 1.0)
s.setSpeciesMobility("cAR1", MS.all, 1.0)

# Adding surface.
s.addSurface("Membrane00")
s.setSurfaceAction("Membrane00", PF.both, "ATP", MS.soln, SA.reflect)

params = [-20.0, 20.0, 10.0, 20.0, 20.0]
s.addPanel("Membrane00", PS.sph, "", "", params)

s.addCompartment("Cell00")
s.addCompartmentSurface("Cell00", "Membrane00")
s.addCompartmentPoint("Cell00", params)
s.addSurfaceMolecules("ACA", MS.down, 30, "Membrane00", PS.all, "all", [])
s.addSurfaceMolecules("cAR1", MS.up, 30, "Membrane00", PS.all, "all", [])
s.addReaction("r100", "", MS.all, "", MS.all, ["ATP"], [MS.soln], 0.02)
s.setReactionRegion("r100", "Cell00", "");

s.run(200, dt=0.01, display=True)
```

</td></tr>
</table>

The Python code can be further reduced.
