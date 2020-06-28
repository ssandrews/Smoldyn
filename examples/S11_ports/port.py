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

red = S.Species('red', color='red')

