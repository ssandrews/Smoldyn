"""
File: port.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Date: 2020-06-27
Description: 
    port.txt in python.
"""

import smoldyn as S

# declare boundaries first. 
b = S.Boundaries(low=[0,0,0], high=[100,100,100])

# Now declare a model with this boundary. After this statement the
# configuration will be initialized.
# m = S.Model(bounds=b)

# set the seed.
S.setSeed(0)

# Declare the species.
spRed = S.Species('red', color='red', difc=3)
spRed.addToSolution(20, highpos=[10,50,50])

spGreen = S.Species('green', color='green', difc=1)
spGreen.addToSolution(10)

# Add surfaces

# walls
r1 = S.Rectangle(corner=[100,0,0], dimensions=[100,100], axis='-x')
r2 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+y')
r3 = S.Rectangle(corner=[0,100,0], dimensions=[100,100], axis='-y')
r4 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='+z')
r5 = S.Rectangle(corner=[0,0,100], dimensions=[100,100], axis='-z')
s1 = S.Surface('walls', panels=[r1,r2,r3,r4,r5])
s1.setStyle('both', 'edge')
s1.addAction('all', 'both', 'reflect')

# portsurf
r1 = S.Rectangle(corner=[0,0,0], dimensions=[100,100], axis='-x')
portSurf = S.Surface('portsurf', panels=[r1])
portSurf.setStyle('both', 'face')
portSurf.addAction('all', 'front', 'port')
portSurf.addAction('all', 'back', 'reflect')

# add port
testport = S.Port('testport', surface=portSurf, panel='front')

s = S.Simulation(step=0.01, stop=100)
s.setGraphics('opengl', 10)
s.run()
