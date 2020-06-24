# this mimics testcase.c

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"

import smoldyn as S
import numpy as np
import matplotlib.pyplot as plt

MS = S.MolecState

v1 = [-100.0, -100.0, -10.0]
v2 = [100.0, 100, 10.0]
S.setBoundaries(list(zip(v1, v2)))
S.setBoundaryType(-1, -1, 'p')
S.addMolList("rlist")
S.addMolList("flist")
S.addSpecies("rabbit", "rlist")
S.addSpecies("fox", "flist")
S.setSpeciesMobility("all", MS.all, 100)

# We don't need numproducts anymore
S.addReaction("r1", "rabbit", MS.soln, "", MS.none, ["rabbit"]*2, [MS.soln]*2, 10)
S.addReaction("r2", "rabbit", MS.soln, "fox", MS.soln, ["fox"]*2, [MS.soln]*2, 8000)
S.addReaction("r3", "fox", MS.soln, "", MS.none, [], [], 10)
S.addSolutionMolecules("rabbit", 1000, v1, v2)
S.addSolutionMolecules("fox", 1000, v1, v2)

S.setGraphicsParams("opengl", 5, 0)
S.setMoleculeStyle("rabbit", MS.all, 2, 'red')
S.setMoleculeStyle("fox", MS.all, 3, 'green')

T, F, R = [], [], []
for t in np.arange(0.1, 2.0, 0.1):
    T.append(t)
    S.runUntil(t, dt=0.001, display=False)
    ctr = S.getMoleculeCount("rabbit", MS.all)
    ctf = S.getMoleculeCount("fox", MS.all)
    F.append(ctf)
    R.append(ctr)
    print(f"t={t:.1f}, rabbit={ctr}, fox={ctf}")

plt.plot(T, F, label='Fox')
plt.plot(T, R, label='Rabbit')
plt.xlabel("Time (au)")
plt.legend()
plt.show()
