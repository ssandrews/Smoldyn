"""Molecules that form a cluster. Similar to cluster3D.txt.
"""

__author__ = "Steve Andrews"
__date__   = "2020-11-23"


import smoldyn as S

S.Boundaries(low=[-10,-10,-10], high=[10,10,10])
blue = S.Species('blue',color='blue',difc=1,display_size=0.3)
red = S.Species('red',color='red',difc=0,display_size=0.3)
blue.addToSolution(100,lowpos=[-5,-5,-5],highpos=[5,5,5])
red.addToSolution(1,lowpos=[0,0,0],highpos=[0,0,0])

s1 = S.Surface('membrane',panels=[S.Sphere(center=[0,0,0],radius=10,slices=10,stacks=10)])
s1.setStyle('both',drawmode='edge',color='green')
s1.both.addAction(species=[blue, red], action='reflect')

rxn = S.Reaction(name='stick',subs=[blue,red],prds=[red,red],kf=20)
rxn.forward.setProductPlacement(type='bounce', parameter=0.6)

sim = S.Simulation(step=0.1,stop=100)
sim.setGraphics('opengl_good',1)
sim.run()

