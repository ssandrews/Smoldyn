# -*- coding: utf-8 -*-
# Dilawar Singh <dilawar.s.rajput@gmail.com>, 2020-04-29
# Python implementation of box.txt

import smoldyn as sm

s = sm.Simulation(low=[0, 0, 0], high=[100, 100, 100], boundary_type="ppp")
s.addBox(size=10)

# declaration of species A, B, and C with attributes.
a = s.addSpecies("A", state="soln", difc=1, color="red", mol_list="Alist")
b = s.addSpecies("B", state="soln", color="green", difc=1, mol_list="Blist")
c = s.addSpecies("C", state="soln", difc=1.0, color="blue", mol_list="Clist")
s.addMolecules(a, 1000)
s.addMolecules(b, 1000)

s.addBidirectionalReaction("r1", subs=[c], prds=(a, b), kf=0.1, kb=100)

# FIXME: Prints only upto 10 (2 iterations rather than 100)
s.setOutputFile("box.dat", True)
c = s.addCommand("molcount box.dat", cmd_type="i", on=0, off=100, step=10)
s.run(100, dt=1, log_level=1)
