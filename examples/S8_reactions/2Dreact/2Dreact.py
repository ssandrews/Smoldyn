"""
2Dreact.txt in Python
"""
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn as sm

sm.setBounds(low=[-5, -5, -5], high=[5, 5, 5])
red = sm.Species("red", difc=dict(all=0.1), color=dict(all="red"))
green = sm.Species("green", difc=dict(all=0.1), color=dict(all="green"))
blue = sm.Species("blue", difc=dict(all=0.1), color=dict(all="blue"))

h1 = sm.Hemisphere(
    name="h1", center=[-3, 0, 0], radius=2, vector=[1, 0, 0], slices=10, stacks=5
)
h2 = sm.Hemisphere(
    center=[3, 0, 0], radius=2, vector=[-1, 0, 0], slices=10, stacks=5, name="h2"
)
c1 = sm.Cylinder(
    start=[-3, 0, 0], end=[3, 0, 0], radius=2, slices=10, stacks=5, name="c1"
)
ecoli = sm.Surface("ecoli", panels=[h1, h2, c1])
ecoli.both.setStyle(drawmode="edge", color="black")

# h1.neighbor = c1
# h2.neighbor = c1
# c1.neighbors = [h1, h2]
c1.setNeighbors([h1, h2], reciprocal=True)

rxn = sm.Reaction("rxn", subs=[(red, "up"), (green, "up")], prds=[(blue, "up")], kf=0.1)

ecoli.addMolecules((red, "up"), 1000, panels=ecoli.panels)
ecoli.addMolecules((green, "up"), 1000, panels=ecoli.panels)

s = sm.Simulation(100, 0.01, output_files=["2Dreactout.txt"])
s.setGraphics("opengl")
s.addCommand("molcount 2Dreactout.txt", type="i", on=0, off=100, step=0.1)
s.run()
