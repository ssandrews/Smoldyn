"""Molecules that form a cluster. Similar to cluster3D.txt.
"""

__author__ = "Steve Andrews"
__date__   = "2020-11-23"

import smoldyn

s = smoldyn.Simulation(low=[-10,-10,-10], high=[10,10,10])
blue = s.addSpecies('blue',color='blue',difc=1,display_size=0.3)
red = s.addSpecies('red',color='red',difc=0,display_size=0.3)
blue.addToSolution(200)
red.addToSolution(1,pos=[0,0,0])

rxn = s.addReaction(name='stick',subs=[blue,red],prds=[red,red], rate=20)
rxn.productPlacement(method='bounce', param=0.6)

s.addOutputData('counts')
s.addCommand(cmd="molcount counts", cmd_type="E")

s.setGraphics('opengl_good',1)

s.run(200, dt=0.1)

import matplotlib.pyplot as plt
import numpy as np

data = s.getOutputData('counts', 0)

dataT = np.array(data).T.tolist()
plt.plot(dataT[0],dataT[2],"r")
plt.xlabel("time")
plt.ylabel("red molecules")
plt.show()

