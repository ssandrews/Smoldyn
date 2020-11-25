# Bistable reaction system
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn as sm

sm.setBounds(low=[0, 0], high=[10, 10], types="p")

# species X A B A2 B2
X = sm.Species("X", difc=0, color="green", display_size=3)
A = sm.Species("A", difc=1, color="red", display_size=3)
B = sm.Species("B", difc=1, color="blue", display_size=3)
A2 = sm.Species("A2", difc=1, color="red", display_size=5)
B2 = sm.Species("B2", difc=1, color="blue", display_size=5)

# mol 1 X 5 5
X.addToSolution(1, highpos=[5, 5])

express = sm.Reaction("express", subs=[X], prds=[X, A, B], rate=1)
Adimer = sm.Reaction("Adimer", subs=[A, A], prds=[A2], rate=1)
Bdimer = sm.Reaction("Bdimer", subs=[B, B], prds=[B2], rate=1)
BdimerBack = sm.Reaction("Bdimer_back", subs=[B2], prds=[B, B], rate=1)

AxB = sm.Reaction("AxB", subs=[A2, B], prds=[A2], rate=1)
BxA = sm.Reaction("BxA", subs=[B2, A], prds=[B2], rate=1)
Adegrade = sm.Reaction("Adegrade", subs=[A], prds=[], rate=0.01)
Bdegrade = sm.Reaction("Bdegrade", subs=[B], prds=[], rate=0.01)

s = sm.Simulation(stop=100, step=0.01, output_files=["bistableout.txt"])
sm.addCommand("molcountheader bistableout.txt", type="B")
sm.addCommand("molcount bistableout.txt", type="N", step=10)
sm.setGraphics("opengl")
s.run()
