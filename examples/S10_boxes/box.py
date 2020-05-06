# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29

import smoldyn as sm

## Original box file is below.
## # Simple A+B <-> C equilibrium
## # Box sizes were varied from 1 to 10 to see how it affected runtime.


## graphics none
## graphic_iter 10


## dim 3
## boundaries x 0 100 p
## boundaries y 0 100 p
## boundaries z 0 100 p

b = sm.Boundaries(low=[0,0,0], high=[100,100,100], types='ppp')
model = sm.Model(bounds=b)
 
## species A B C
## boxsize 10
sm.Box(size=10)

## accuracy 10
## TODO:

## difc A 1
## difc B 1
## difc C 1
## 
## color A red
## color B green
## color C blue

# declaration of species A, B, and C with attributes. 
a = sm.Species('A', state='all', difc=1, color='red')
b = sm.Species('B', state="all", color='green', difc=1)
c = sm.Species('C', state='all', difc=1.0, color='blue')

## time_start 0
## time_stop 100
## time_step 0.01
# TODO
 
## molecule_lists Alist Blist Clist
## mol_list A Alist
## mol_list B Blist
## mol_list C Clist
# TODO

## cmd i 0 100 10 molcount
## 
## mol 1000 A u u u
## mol 1000 B u u u
model.addMolecules(a, 1000, pos='uuu')
model.addMolecules(b, 1000, pos='u')


## reaction revrxn C -> A + B 0.1
## reaction fwdrxn A + B -> C 100
## 
## end_file




# revrxn = Reaction(subs=[c], prds=[a, b], k=0.1)
# fwdrxn = Reaction(subs=[a, b], prds=[c], k=100)
r = sm.Reaction(subs=[c], prds=[a, b], kf=0.1, kb=100)

model.run(100, dt=0.01)
