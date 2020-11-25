# Periodic boundaries with surfaces
import smoldyn as sm

sm.setBounds(low=[0, 0], high=[100, 100])
A = sm.Species("A", difc=1, color="red")
B = sm.Species("B", difc=1, color="green")
C = sm.Species("C", difc=1, color="blue")

A.addToSolution(200, pos=[10, 50])
B.addToSolution(200, pos=[90, 50])

s1 = sm.Sphere(center=[50, 50], radius=20, slices=50)
ball = sm.Surface("ball", panels=[s1])
ball.both.addAction([A, B, C], "reflect")
ball.both.setStyle(color=[0, 0.5, 0], thickness=1)

r1 = sm.Rectangle([0, 0], dimensions=[100], axis="+0", name="r1")
r2 = sm.Rectangle([100, 0], dimensions=[100], axis="-0", name="r2")
r3 = sm.Rectangle([0, 0], dimensions=[100], axis="+1", name="r3")
r4 = sm.Rectangle([0, 100], dimensions=[100], axis="-1", name="r4")
sides = sm.Surface("sides", panels=[r1, r2, r3, r4])
sides.front.addAction([A, B, C], "jump")
sides.back.addAction([A, B, C], "reflect")
sides.front.setStyle(color=[0.2, 0, 0])
sides.back.setStyle(color=[0, 0, 0.5])
sides.both.setStyle(thickness=1)
r1.front.jumpTo(r2.front, True)
r3.front.jumpTo(r4.front, True)

rxt = sm.Reaction("rxn", subs=[A, B], prds=[C], kf=1, kb=0.2)
rxt.reverse.setProductPlacement("pgemmax", param=0.2)

s = sm.Simulation(2000, 0.01)
s.addGraphics("opengl", iter=10)
s.run()
