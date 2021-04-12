import smoldyn
import smoldyn._smoldyn as S
import numpy
import matplotlib.pyplot as plt
from pathlib import Path

sim=smoldyn.Simulation.fromFile(Path(__file__).parent / "Min1.txt","")

#sim=smoldyn.Simulation(low=[-2,-0.5,-0.5],high=[2,0.5,0.5])

#MinD_ATP = sim.addSpecies('MinD_ATP',state='all',color='darkblue',difc=2.5,display_size=2)
#MinD_ADP = sim.addSpecies('MinD_ADP',state='all',color='lightblue',difc=0.01,display_size=2)
#MinE = sim.addSpecies('MinE',state='all',color='red',difc=2.5,display_size=2)
#MinDMinE = sim.addSpecies('MinDMinE',state='all',color='lightgreen',difc=0.01,display_size=2)

#hemi0 = smoldyn.Hemisphere(center=[-1.5,0,0],radius=0.5,vector=[1,0,0],slices=20,stacks=5)
#hemi1 = smoldyn.Hemisphere(center=[1.5,0,0],radius=0.5,vector=[-1,0,0],slices=20,stacks=5)
#cyl0 = smoldyn.Cylinder(start=[-1.5,0,0],end=[1.5,0,0],radius=0.5,slices=20,stacks=5)
#membrane = sim.addSurface('membrane',panels=[hemi0,hemi1,cyl0])
#membrane.setAction('both',[MinD_ATP,MinD_ADP,MinE,MinDMinE],'reflect')
#membrane.setStyle('both',drawmode='edge',color='blue')

#cell = sim.addCompartment('cell',surface='membrane',point=[0,0,0])

#rxn1b = sim.addReaction('rxn1b',subs=[(MinD_ATP,'fsoln'),(MinD_ATP,'up')],prds=[(MinD_ATP,'up'),(MinD_ATP,'up')],rate=0.0015)

#sim.addGraphics("opengl", iter=10)

#cell.addMolecules(MinE, 1400)
#membrane.addMolecules((MinD_ATP,'up'),4000)

sim.addOutputData('moments')
sim.addCommand(cmd="molmoments MinD_ATP(front) moments", cmd_type="N", step=10)

value = 0.0015
sigma = value/20
oldmax = 0
oldvalue = value
for it in range(5):
	print("*** MinOptimize.py it=",it)
	value += sigma * (numpy.random.randint(2)-0.5)
	S.Simulation.setReactionRate(sim,"rxn1b",value,0)
	S.Simulation.setSimTimes(sim,0,10,0.005)		# Stop time should be 100, but use 10 for testing
	print("*** MinOptimize.py about to add command")
	#sim.addCommand(cmd="molmoments MinD_ATP(front) moments", cmd_type="N", step=10)
	print("*** MinOptimize.py done with add command")
	print("=====================================================================")
	sim.display()
	print("*** MinOptimize.py about to run")
	sim.run(stop=10,dt=0.005,start=0)
	print("*** MinOptimize.py done with run")
	data=sim.getOutputData('moments',0)
	dataT = numpy.array(data).T.tolist()
	max = numpy.amax(numpy.absolute(dataT[2]))
	print(it,max,value)
	if max > oldmax:
		print("success! max=",max)
		oldvalue = value
		oldmax = max
		sigma *= 1.1
	else:
		print("worse. max=",max)
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

