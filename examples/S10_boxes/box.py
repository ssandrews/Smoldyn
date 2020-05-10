# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29

# This file is reimplementation of box.txt

import smoldyn as sm
b = sm.Boundaries(low=[0,0,0], high=[100,100,100], types='ppp')
model = sm.Model(bounds=b, accuracy=10)

sm.Box(size=10)   

# declaration of species A, B, and C with attributes. 
a = sm.Species('A', state='soln', difc=1, color='red')
a.mol_list = 'Alist'
b = sm.Species('B', state="soln", color='green', difc=1, mol_list='Blist')
c = sm.Species('C', state='soln', difc=1.0, color='blue', mol_list='Clist')

model.addMolecules(a, 1000, pos='uuu')
model.addMolecules(b, 1000, pos='u')

r = sm.Reaction(subs=[c], prds=(a,b), kf=0.1, kb=100)

M = sm.StateMonitor([a, b, c], 'molcount')

## time_start 0
## time_stop 100
## time_step 0.01
# TODO
model.run(100, dt=0.01)
for l in model.data():
    print(l)

