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
for it in range(5):
	value += sigma * (numpy.random.randint(2)-0.5)
	S.Simulation.setReactionRate(sim,"rxn1b",value,0)
	S.Simulation.setSimTimes(sim,0,10,0.005)		# Stop time should be 100, but use 10 for testing
	sim.run(stop=100,dt=0.005,start=0)
	data=sim.getOutputData('moments',0)
	dataT = numpy.array(data).T.tolist()
	max = numpy.amax(numpy.absolute(dataT[2]))
	if max > oldmax:
		print("Iteration=",it,", Attempt=",value,". Success! Max=",max)
		oldvalue = value
		oldmax = max
		sigma *= 1.1
	else:
		print("Iteration=",it,", Attempt=",value,". Worse. Max=",max)
		value = oldvalue
		sigma *= 0.9


#sim.run(100,0.005)
#sim.display()

#data=sim.getOutputData('moments',0)
#print(data)

#dataT = numpy.array(data).T.tolist()
#print(dataT)
#plt.plot(dataT[0], dataT[2], "r")
#plt.xlabel("time")
#plt.ylabel("x position")
#plt.show()

#max = numpy.amax(numpy.absolute(dataT[2]))
#print(max)

