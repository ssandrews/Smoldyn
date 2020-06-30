"""Simple bouncing molecules in 3-D system. This file is translated from 
port.txt file.
"""

__author__ = "Dilawar Singh"
__email__  = "dilawars@ncbs.res.in"
__date__   = "2020-06-30"


import smoldyn as S

b = S.Boundaries(low=[0,0,0], high=[100,100,100])
S.setSeed(0)

spRed = S.Species('red', color='red', difc=3, display_size=3)
spRed.addToSolution(20, highpos=[10,50,50])

spGreen = S.Species('green', color='green', difc=1, display_size=3)
spGreen.addToSolution(10)

# Add Surfaces
r1 = S.Rectangle(corner=[100,0,0], dimensions=[100,100], axis='-x')
r2 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+y')
r3 = S.Rectangle(corner=[0,100,0], dimensions=[100,100], axis='-y')
r4 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+z')
r5 = S.Rectangle(corner=[0,0,100], dimensions=[100,100], axis='-z')
s1 = S.Surface('walls', panels=[r1,r2,r3,r4,r5])
s1.both.setStyle(drawmode='edge')
s1.both.addAction('all', 'reflect')

# portsurf
rr = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+x')
portSurf = S.Surface('portsurf', panels=[rr])
portSurf.front.setStyle(drawmode='face', color='gray')
portSurf.back.setStyle(drawmode='face', color=[0.2,0,0,1])
portSurf.front.addAction('all', 'port')
portSurf.back.addAction('all', 'reflect')

# Ports
testport = S.Port('testport', surface=portSurf, panel='front')
s = S.Simulation(step=0.01, stop=100)
s.setGraphics('opengl', 20)
s.run()
