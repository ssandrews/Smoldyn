__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import smoldyn as S
import random

S.setBounds(low=(0, 0), high=(10, 10))
a = S.Species("a", color="red", difc=1)
print(a.difc)

# First argument is always t.
def new_dif(t, args):
    x, y = args
    print(a.difc, x, y)
    return t + random.random()


S.connect(new_dif, "a.difc", step=10, args=[0, 1])
s = S.Simulation(100, 1)
s.run()
print("All done")
