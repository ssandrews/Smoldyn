# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29
# Python implementation of box.txt

import smoldyn as sm
b = sm.setBounds(low=[0,0,0], high=[100,100,100], types='ppp')

sm.Box(size=10)   

# declaration of species A, B, and C with attributes. 
a = sm.Species('A', state='soln', difc=1, color='red', mol_list='Alist')
b = sm.Species('B', state="soln", color='green', difc=1, mol_list='Blist')
c = sm.Species('C', state='soln', difc=1.0, color='blue', mol_list='Clist')

a.addToSolution(1000)
b.addToSolution(1000)

r = sm.Reaction(subs=[c], prds=(a,b), kf=0.1, kb=100)
M = sm.StateMonitor([a, b, c], 'molcount')
s = sm.Simulation(100, step=0.01, accuracy=10)
s.run()

# Now getting data and printing line.
for row in M.data():
    print(row)
