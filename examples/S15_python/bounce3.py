__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn

s = smoldyn.Simulation(low=(0, 0, 0), high=(100, 100, 100), types=["r", "r", "r"])
red = s.addSpecies("red", difc=3, color="red")
s.addMolecules(red, 100)    # or, red.addToSolution(100)
s.addMolecules(red, 30, highpos=(20, 30,20))  # red.addToSolution(30, highpos=[20, 30, 20])

green = s.addSpecies("green", difc=1, color="green")
s.addMolecules(green, 30)
s.setGraphics("opengl", 10)
s = s.run(100, 0.01, quit_at_end=True)
