# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29

import smoldyn as sm

# Every simulation starts with a Simualtion object, and every Simulation has
# boundaries
b = sm.Boundaries(low=[0,0,0], high=[100,100,100], types='ppp')
S = sm.Simulation(bounds=b)

a = sm.Species('A', state='all', difc=1, color='red')
b = sm.Species('B')
b.state = 'all'
b.difc = 1
b.color = 'green'
c = sm.Species('C', state='all', difc=1.0)
c.color = 'blue'

box = sm.Box(size=10)
box.addMolecules(a, 1000, x='u', y='u', z='u')
box.addMolecules(b, 1000, xyz='u')

# revrxn = Reaction(subs=[c], prds=[a, b], k=0.1)
# fwdrxn = Reaction(subs=[a, b], prds=[c], k=100)
r = sm.Reaction(subs=[c], prds=[a, b], kf=0.1, kb=100)

S.run(100, dt=0.01)
