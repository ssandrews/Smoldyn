# Python version of cmddata.txt

import smoldyn 

s = smoldyn.Simulation(low=[0, 0, 0], high=[100, 100, 100])

R = s.addSpecies("R", difc=1, color="red")
B = s.addSpecies("B", difc=1, color="blue")

r1 = s.addReaction("r1", subs=[R], prds=[], rate=0.1 )

R.addToSolution(400)
B.addToSolution(1, pos=[50,50,50])

smoldyn.addOutputData('mydata')
s.addCommand(cmd="molcount mydata", cmd_type="E")

# s.setGraphics( "opengl" )

s = s.run(stop=10, dt=0.01)

data = smoldyn.getOutputData('mydata', 0)

print(data)
