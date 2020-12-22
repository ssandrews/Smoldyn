# Periodic boundaries with surfaces

import smoldyn

s = smoldyn.Simulation(low=[0, 0], high=[100, 100])

A = s.addSpecies("A", difc=1, color="red")
B = s.addSpecies("B", difc=1, color="green")
C = s.addSpecies("C", difc=1, color="blue")

A.addToSolution(200, pos=[10, 50])
B.addToSolution(200, pos=[90, 50])

s1 = s.addSphere(center=[50, 50], radius=20, slices=50)
ball = s.addSurface("ball", panels=[s1])

ball.setAction('both', [A, B, C], "reflect")
ball.setStyle('both', color=[0, 0.5, 0], thickness=1)

r1 = smoldyn.Rectangle(corner=[0, 0], dimensions=[100], axis="+0", name="r1")
r2 = smoldyn.Rectangle(corner=[100, 0], dimensions=[100], axis="-0", name="r2")
r3 = smoldyn.Rectangle(corner=[0, 0], dimensions=[100], axis="+1", name="r3")
r4 = smoldyn.Rectangle(corner=[0, 100], dimensions=[100], axis="-1", name="r4")
sides = s.addSurface("sides", panels=[r1, r2, r3, r4])

# sides.front.setAction([A, B, C], "jump")
# sides.back.setAction([A, B, C], "reflect")
sides.setAction('front', [A, B, C], 'jump')
sides.setAction('back', [A, B, C], 'reflect')

sides.setStyle('front', color=[0.2, 0, 0])
sides.setStyle('back', color=[0, 0, 0.5])
sides.setStyle('both', thickness=1)

## Both styles are equivalent.
# r1.front.jumpTo(r2.front, True)
r1.jumpTo('front', r2, 'front', True)
r3.jumpTo('front', r4, 'front', True)

# One can also use s.addBidirectionalReaction and refer to `forward` and
# `reverse` fields.
r1 = s.addReaction("rfwd", subs=[A, B], prds=[C], rate=1)
r2 = s.addReaction("rback", subs=[C], prds=[A, B], rate=0.2)
r2.productPlacement("pgemmax", param=0.2)

s.addGraphics("opengl", iter=10)
s.run(100, dt=0.01)
