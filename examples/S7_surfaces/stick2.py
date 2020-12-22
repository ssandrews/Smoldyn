"""Python equivalent of stick2.txt

For the txt file, green is chaned to black. green and red together is bad idea
for colorblindness (10% of population has it, me included!).
"""

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn

s = smoldyn.Simulation(low=[0, 0], high=[100, 100])
red = s.addSpecies("red", color="red", difc=dict(all=3, front=0), display_size=5)
yellow = s.addSpecies("yellow", color="black", difc=dict(soln=3, back=1), display_size=5)
blue = s.addSpecies("blue", color="blue", difc=3, display_size=5)

red.addToSolution(100)
yellow.addToSolution(50, pos=(50, 50))
blue.addToSolution(50, pos=(20, 20))

# Construct a closed path in 2D.
p = s.addPath2D((0, 0), (100, 0), (100, 100), (0, 100), closed=True)
walls = s.addSurface("walls", panels=p.panels)
walls.setAction('both', [red, yellow, blue], "reflect")
walls.setStyle('both', color="black")

sph = smoldyn.Sphere(center=(50, 50), radius=20, slices=20)
surf = s.addSurface("stick", panels=[sph])
surf.setRate(red, "fsoln", "front", rate=1, revrate=0.1)
surf.setRate(yellow, "bsoln", "back", rate=1, revrate=0.1)
surf.setRate(blue, "fsoln", "bsoln", rate=1, revrate=0.1)
surf.setStyle('front', color=(1, 0.7, 0))
surf.setStyle('back', color=(0.6, 0, 0.6))
surf.addMolecules((red, "front"), 100)

s.setGraphics("opengl")
s.addCommand("killmolinsphere red all", "b")
sim = s.run(1000, dt=0.01)
