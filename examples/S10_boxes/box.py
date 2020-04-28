__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

import smoldyn as sm

# Every simulation starts with a Simualtion object.
b = sm.Boundaries(low=[0,0,0], high=[100,100,100], types='ppp')
print(b)
S = sm.Simulation(bounds=b)

## Or,
#S = sm.Simulation()
#S.boundaries.low = [0,0,0]
#S.boundaries.high = [100,100,100]
#S.boundaries.types = ['p', 'p', 'p']

a = sm.Species('A', diffc=1, color='red')
b = sm.Species('B')

b.diffc = 1
b.color = 'green'

c = sm.Species('C', diffc=1.0)
c.color = 'blue'

box = sm.Box(size=10)
box.addMolecules(a, 1000, x='u', y='u', z='u')
box.addMolecules(b, 1000, xyz='u')

# revrxn = Reaction(subs=[c], prds=[a, b], k=0.1)
# fwdrxn = Reaction(subs=[a, b], prds=[c], k=100)
r = sm.Reaction(subs=[c], prds=[a, b], kf=0.1, kb=100)

S.run(100, dt=0.01)
