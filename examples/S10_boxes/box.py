# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29

# This file is reimplementation of box.txt

import smoldyn as sm
sm.setBounds(low=[0,0,0], high=[100,100,100], types='ppp')
sm.setAccuracy(10)

sm.Box(size=10)   

# declaration of species A, B, and C with attributes. 
a = sm.Species('A', state='soln', difc=1, color='red')
a.mol_list = 'Alist'
b = sm.Species('B', state="soln", color='green', difc=1, mol_list='Blist')
c = sm.Species('C', state='soln', difc=1.0, color='blue', mol_list='Clist')

sm.addSolutionMolecules(a.name, 1000)
sm.addSolutionMolecules(b.name, 1000)
r = sm.Reaction(subs=[c], prds=(a,b), kf=0.1, kb=100)
sm.run(100, dt=0.01)
