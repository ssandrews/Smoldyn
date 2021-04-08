"""
2Dreact.txt in Python
"""
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn

s = smoldyn.Simulation(
    low=[-5, -5, -5], high=[5, 5, 5], output_files=["2Dreactout.txt"]
)

red = s.addSpecies("red", difc=dict(all=0.1), color=dict(all="red"))
green = s.addSpecies("green", difc=dict(all=0.1), color=dict(all="green"))
blue = s.addSpecies("blue", difc=dict(all=0.1), color=dict(all="blue"))

h1 = s.addHemisphere(
    name="h1", center=[-3, 0, 0], radius=2, vector=[1, 0, 0], slices=10, stacks=5
)
h2 = s.addHemisphere(
    center=[3, 0, 0], radius=2, vector=[-1, 0, 0], slices=10, stacks=5, name="h2"
)
c1 = s.addCylinder(
    start=[-3, 0, 0], end=[3, 0, 0], radius=2, slices=10, stacks=5, name="c1"
)
ecoli = s.addSurface("ecoli", panels=[h1, h2, c1])
ecoli.setStyle("both", drawmode="edge", color="black")

# h1.neighbor = c1
# h2.neighbor = c1
# c1.neighbors = [h1, h2]
c1.setNeighbors([h1, h2], reciprocal=True)

rxn = s.addReaction(
    "rxn", subs=[(red, "up"), (green, "up")], prds=[(blue, "up")], rate=0.1
)
ecoli.addMolecules((red, "up"), 1000, panels=ecoli.panels)
ecoli.addMolecules((green, "up"), 1000, panels=ecoli.panels)
s.setGraphics("opengl")
s.addCommand("molcount 2Dreactout.txt", "i", on=0, off=100, step=0.1)
s.run(100, dt=0.01)
