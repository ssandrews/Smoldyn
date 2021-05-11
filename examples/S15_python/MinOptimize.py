"""
This file is supposed to optimize the E. coli Min system to get the largest possible oscillations. This
file uses a greedy random walk, which is simple, but isn't ideal here due to the large effects of
stochasticity.
"""

import smoldyn
import smoldyn._smoldyn as S
import numpy
import matplotlib.pyplot as plt
from pathlib import Path

sim=smoldyn.Simulation.fromFile(Path(__file__).parent / "Min1.txt","tq")

sim.addOutputData('moments')
sim.addCommand(cmd="molmoments MinD_ATP(front) moments", cmd_type="N", step=10)

value = 0.0015
sigma = value/5
oldmax = 0
oldvalue = value
for it in range(10):
	value += sigma * (numpy.random.randint(2)-0.5)
	S.Simulation.setReactionRate(sim,"rxn1b",value,0)
	sim.run(stop=100,dt=0.005,start=0)
	data=sim.getOutputData('moments',1)
	dataT = numpy.array(data).T.tolist()
	max = numpy.amax(numpy.absolute(dataT[2]))
	if max > oldmax:
		print("Iteration=",it,", Attempt=",value,"Current value=",max,". Success! Best=",max)
		oldvalue = value
		oldmax = max
		sigma *= 1.1
	else:
		print("Iteration=",it,", Attempt=",value,"Current value=",max,". Worse. Best=",oldmax)
		value = oldvalue
		sigma *= 0.9
