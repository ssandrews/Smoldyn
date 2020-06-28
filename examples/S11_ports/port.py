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
m = S.Model(bounds=b)

# set the seed.
S.setSeed(0)

# Declare the species.
spRed = S.Species('red', color='red', difc=3)
spRed.addToSolution(20, highpos=[10,50,50])

spGreen = S.Species('green', color='green', difc=1)
spGreen.addToSolution(10)
