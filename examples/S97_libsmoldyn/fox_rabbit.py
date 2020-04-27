# this mimics testcase.c

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"

import smoldyn
import numpy as np
import matplotlib.pyplot as plt

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

# We don't need numproducts anymore
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
    print(f"t={t:.1f}, rabbit={ctr}, fox={ctf}")

plt.plot(T, F, label='Fox')
plt.plot(T, R, label='Rabbit')
plt.show()
