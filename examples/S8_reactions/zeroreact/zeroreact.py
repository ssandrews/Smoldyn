# Zeroth order reactions
import smoldyn

s = smoldyn.Simulation(low=[0, 0, 0], high=(10, 10, 10), boundary_type="r")
red = s.addSpecies("red", difc=1, color=[1, 0, 0])
green = s.addSpecies("green", difc=1, color=[0, 1, 0])
blue = s.addSpecies("blue", difc=1, color=[0, 0, 1])
slow = s.addReaction("slow", [], [red], rate=0.001)
med = s.addReaction("med", [], [green], rate=0.01)
fast = s.addReaction("fast", [], [blue], rate=0.1)

s.setGraphics("opengl")
s.setOutputFile("zeroreactout.txt")
s.addCommand("molcount zeroreactout.txt", "e")
s = s.run(stop=10, dt=0.01, overwrite=True)
