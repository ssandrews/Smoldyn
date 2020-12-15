# Bistable reaction system
__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn

s = smoldyn.Simulation(
    low=[0, 0], high=[10, 10], types="p", output_files=["bistableout.txt"]
)

# species X A B A2 B2
X = s.addSpecies("X", difc=0, color="green", display_size=3)
A = s.addSpecies("A", difc=1, color="red", display_size=3)
B = s.addSpecies("B", difc=1, color="blue", display_size=3)
A2 = s.addSpecies("A2", difc=1, color="red", display_size=5)
B2 = s.addSpecies("B2", difc=1, color="blue", display_size=5)

# mol 1 X 5 5
X.addToSolution(1, pos=[5, 5])

express = s.addReaction("express", subs=[X], prds=[X, A, B], rate=1)
Adimer = s.addBidirectionalReaction("Adimer", subs=[A, A], prds=[A2], kf=1, kb=1)
Bdimer = s.addBidirectionalReaction("Bdimer", subs=[B, B], prds=[B2], kf=1, kb=1)
AxB = s.addReaction("AxB", subs=[A2, B], prds=[A2], rate=1)
BxA = s.addReaction("BxA", subs=[B2, A], prds=[B2], rate=1)
Adegrade = s.addReaction("Adegrade", subs=[A], prds=[], rate=0.01)
Bdegrade = s.addReaction("Bdegrade", subs=[B], prds=[], rate=0.01)

s.addCommand("molcountheader bistableout.txt", cmd_type="B")
s.addCommand("molcount bistableout.txt", cmd_type="N", step=10)
s.setGraphics("opengl")
s.run(100, dt=0.01)
