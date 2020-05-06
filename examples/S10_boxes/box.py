# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29

# This file is reimplementation of box.txt

import smoldyn as sm
b = sm.Boundaries(low=[0,0,0], high=[100,100,100], types='ppp')
model = sm.Model(bounds=b)
sm.Box(size=10)

## accuracy 10
## TODO:

# declaration of species A, B, and C with attributes. 
a = sm.Species('A', state='soln', difc=1, color='red')
b = sm.Species('B', state="soln", color='green', difc=1)
c = sm.Species('C', state='soln', difc=1.0, color='blue')

## time_start 0
## time_stop 100
## time_step 0.01
# TODO
 
## molecule_lists Alist Blist Clist
## mol_list A Alist
## mol_list B Blist
## mol_list C Clist
# TODO

model.addMolecules(a, 1000, pos='uuu')
model.addMolecules(b, 1000, pos='u')

# revrxn = Reaction(subs=[c], prds=[a, b], k=0.1)
# fwdrxn = Reaction(subs=[a, b], prds=[c], k=100)
r = sm.Reaction(subs=[c], prds=[a, b], kf=0.1, kb=100)

model.run(100, dt=0.01)
