"""Simple bouncing molecules in 3-D system. This file is translated from
port.txt file.
"""

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"
__date__ = "2020-06-30"


import smoldyn

sim = smoldyn.Simulation(low=[0, 0, 0], high=[100, 100, 100])

# Set it after Simulation object is created.
sim.seed = 0

spRed = sim.addSpecies("red", color="red", difc=3, display_size=3)
spRed.addToSolution(20, highpos=[10, 50, 50])
# sim.addMolecules(spRed, 20, highpos=[10,50,50])

spGreen = sim.addSpecies("green", color="green", difc=1, display_size=3)
# spGreen.addToSolution(10)
sim.addMolecules(spGreen, 20, highpos=[10,50,50])

# Add Surfaces
r1 = smoldyn.Rectangle(corner=[100, 0, 0], dimensions=[100, 100], axis="-x")
r2 = smoldyn.Rectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+y")
r3 = smoldyn.Rectangle(corner=[0, 100, 0], dimensions=[100, 100], axis="-y")
r4 = smoldyn.Rectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+z")
r5 = smoldyn.Rectangle(corner=[0, 0, 100], dimensions=[100, 100], axis="-z")

s1 = sim.addSurface("walls", panels=[r1, r2, r3, r4, r5])
# s1.both.setStyle(drawmode='edge')
s1.setStyle("both", drawmode="edge")
s1.setAction('both', [spRed, spGreen], "reflect")

# portsurf
rr = smoldyn.Rectangle(corner=[0, 0, 0], dimensions=[100, 100], axis="+x")
portSurf = sim.addSurface("portsurf", panels=[rr])
portSurf.setStyle('front', drawmode="face", color="gray")
portSurf.setStyle('back', drawmode="face", color=[0.2, 0, 0, 1])
portSurf.setAction('front', [spRed, spGreen], "port")
portSurf.setAction('back', [spRed, spGreen], "reflect")

# Ports
testport = sim.addPort(name="testport", surface=portSurf, panel="front")
sim.setGraphics("opengl", 20)
sim.run(dt=0.01, stop=100)
