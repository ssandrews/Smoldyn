# Bimolecular reactions between molecules on different surfaces

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn as sm

sm.setBounds(low=[0, 0], high=[100, 100], types="r")
A = sm.Species("A", color={"all": "red"}, difc={"all": 1}, display_size=dict(all=1))
B = sm.Species("B", color={"all": "green"}, difc={"all": 1}, display_size=dict(all=1))
C = sm.Species("C", color={"all": "blue"}, difc={"all": 1}, display_size=dict(all=1))

p1 = sm.Rectangle(corner=(0, 0), dimensions=[100], axis="+x")
p2 = sm.Rectangle(corner=(100, 0), dimensions=[100], axis="-x")
p3 = sm.Rectangle(corner=(0, 0), dimensions=[100], axis="+y")
p4 = sm.Rectangle(corner=(0, 100), dimensions=[100], axis="-y")
walls = sm.Surface("walls", panels=[p1, p2, p3, p4])
walls.both.addAction([A, B, C], "reflect")

r1 = sm.Rectangle(corner=[49, 30], dimensions=[20], axis="+x", name="r1")
t1 = sm.Triangle(vertices=[[49, 50], [29, 70]], name="t1")
left = sm.Surface(name="left", panels=[r1, t1])
r1.neighbor = t1
t1.neighbor = r1
left.both.addAction([A, B, C], "reflect")
left.addMolecules((A, "up"), 20)

r1 = sm.Rectangle(corner=[50, 30], dimensions=[20], axis="+x", name="r1")
t1 = sm.Triangle(vertices=[[50, 30], [70, 10]], name="t1")
right = sm.Surface(name="right", panels=[r1, t1])
r1.neighbor = t1
t1.neighbor = r1
right.both.addAction([A, B, C], "reflect")
right.addMolecules((B, "up"), 20)

rxn1 = sm.Reaction(
    name="rxn1",
    subs=[(A, "up"), (B, "up")],
    prds=[(A, "up"), (C, "bsoln")],
    binding_radius=2,
)
rxn1.setIntersurface([1, 1])

s = sm.Simulation(1000, 0.1)
s.addGraphics("opengl_good")
s.run()
