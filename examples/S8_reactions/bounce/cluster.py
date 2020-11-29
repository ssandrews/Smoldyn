"""Molecules that form a cluster. Similar to cluster3D.txt.
"""

__author__ = "Steve Andrews"
__date__   = "2020-11-23"

import smoldyn

s = smoldyn.Simulation(low=[-10,-10,-10], high=[10,10,10])
blue = s.addSpecies('blue',color='blue',difc=1,display_size=0.3)
red = s.addSpecies('red',color='red',difc=0,display_size=0.3)
blue.addToSolution(100,lowpos=[-5,-5,-5],highpos=[5,5,5])
red.addToSolution(1,lowpos=[0,0,0],highpos=[0,0,0])

s1 = s.addSurface('membrane', panels=[smoldyn.Sphere(center=[0,0,0],radius=10,slices=10,stacks=10)])
s1.setStyle('both', drawmode='edge',color='green')
s1.setAction('both', species=[blue, red], action='reflect')

rxn = s.addReaction(name='stick',subs=[blue,red],prds=[red,red], rate=20)
rxn.productPlacement(method='bounce', param=0.6)

s.setGraphics('opengl_good',1)
s.run(100, dt=0.1)
