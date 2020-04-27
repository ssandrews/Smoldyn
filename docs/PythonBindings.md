---
title: Python bindings
author: Dilawar Singh
---

 __This is a work in progress.__

To build Smoldyn's python module `_smoldyn.so`, we use excellent
[pybind11](https://pybind11.readthedocs.io/en/stable/) library. It is included
in the source tree. The compilation `_smoldyn.so` requires a `c++11` compliant
compiler. Most compilers these days support `-std=c++11` flags.

To begin with, we expose [Smoldyn C API](LibsmoldynManual.md) to Python
using `pybind11`. An example is shown below. The user is not recommended to use this
low-level API directly. See [Python Manual] for details.

<table>
<tr><th>C Version</th><th>Python Version</th></tr>
<tr><td>
```c
#include "libsmoldyn.h"
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
# (c) Dilawar Singh, 2020/04/20
import smoldyn
s = smoldyn.Simulation()
s.bounds = ([-50, 50], [-50, 50])
assert s.dim == 2
s.seed = 1

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

Here is another examples.

<table>
<tr><th>C version</th><th>Python Version</th></tr>
<tr>
<td>
```c
/* Steve Andrews, 3/16/2011 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libsmoldyn.h" 

int main()
{
    simptr          sim;
    double          v1[3], v2[3], color[4], t;
    char **         products;
    enum MolecState prodstates[2];
    int             ctr, ctf;

    sim = NULL;
    printf("Test program.\n");

    products    = (char **)calloc(2, sizeof(char *));
    products[0] = (char *)calloc(256, sizeof(char));
    products[1] = (char *)calloc(256, sizeof(char));

    smolSetDebugMode(1);
    v1[0] = -100;
    v1[1] = -100;
    v1[2] = -10;
    v2[0] = 100;
    v2[1] = 100;
    v2[2] = 10;
    sim   = smolNewSim(3, v1, v2);
    smolSetSimTimes(sim, 0, 2, 0.001);
    smolSetBoundaryType(sim, -1, -1, 'p');
    smolAddMolList(sim, "rlist");
    smolAddMolList(sim, "flist");
    smolAddSpecies(sim, "rabbit", "rlist");
    smolAddSpecies(sim, "fox", "flist");
    smolSetSpeciesMobility(sim, "all", MSall, 100, NULL, NULL);
    strcpy(products[0], "rabbit");
    strcpy(products[1], "rabbit");
    prodstates[0] = MSsoln;
    prodstates[1] = MSsoln;
    smolAddReaction(sim, "r1", "rabbit", MSsoln, NULL, MSnone, 2,
        (const char **)products, prodstates, 10);
    strcpy(products[0], "fox");
    strcpy(products[1], "fox");
    smolAddReaction(sim, "r2", "rabbit", MSsoln, "fox", MSsoln, 2,
        (const char **)products, prodstates, 8000);
    smolAddReaction(sim, "r3", "fox", MSsoln, NULL, MSnone, 0, NULL, NULL, 10);
    smolAddSolutionMolecules(sim, "rabbit", 1000, v1, v2);
    smolAddSolutionMolecules(sim, "fox", 1000, v1, v2);

    smolSetGraphicsParams(sim, "opengl", 5, 0);
    color[0] = 1;
    color[1] = 0;
    color[2] = 0;
    color[3] = 1;
    smolSetMoleculeStyle(sim, "rabbit", MSall, 2, color);
    color[0] = 0;
    color[1] = 1;
    color[2] = 0;
    smolSetMoleculeStyle(sim, "fox", MSall, 3, color);
    smolUpdateSim(sim);
    smolDisplaySim(sim);
    //	smolRunSim(sim);

    for(t = 0.1; t < 2; t += 0.1) {
        smolRunSimUntil(sim, t);
        ctr = smolGetMoleculeCount(sim, "rabbit", MSall);
        ctf = smolGetMoleculeCount(sim, "fox", MSall);
        printf("t= %g, rabbit = %i, fox=%i\n", t, ctr, ctf);
    }

    return 0;
}
```
</td>
<td>
```python 
# (c) Dilawar Singh, 2020/April/27
import smoldyn
import numpy as np
import matplotlib.pyplot as plt

# Getting enum MolecState
MS = smoldyn.MS

s = smoldyn.Simulation(debug=True)
v1 = [-100.0, -100.0, -10.0]
v2 = [100.0, 100, 10.0]
s.bounds = list(zip(v1, v2))
s.setBoundaryType(-1, -1, 'p')
s.addMolList("rlist")
s.addMolList("flist")
s.addSpecies("rabbit", "rlist")
s.addSpecies("fox", "flist")
s.setSpeciesMobility("all", MS.all, 100)

s.addReaction("r1", "rabbit", MS.soln, "", MS.none, ["rabbit"]*2, [MS.soln]*2, 10)
s.addReaction("r2", "rabbit", MS.soln, "fox", MS.soln, ["fox"]*2, [MS.soln]*2, 8000)
s.addReaction("r3", "fox", MS.soln, "", MS.none, [], [], 10)
s.addSolutionMolecules("rabbit", 1000, v1, v2)
s.addSolutionMolecules("fox", 1000, v1, v2)

s.setGraphicsParams("opengl", 5, 0)
s.setMoleculeStyle("rabbit", MS.all, 2, [1,0,0,1])
s.setMoleculeStyle("fox", MS.all, 3, [0, 1, 0])

T, F, R = [], [], []
for t in np.arange(0.1, 2.0, 0.1):
    T.append(t)
    s.runUntil(t, dt=0.002)
    ctr = s.getMoleculeCount("rabbit", MS.all)
    ctf = s.getMoleculeCount("fox", MS.all)
    F.append(ctf)
    R.append(ctr)

plt.plot(T, F, label='Fox')
plt.plot(T, R, label='Rabbit')
plt.legend()
```
</td>
</tr>
</table>

