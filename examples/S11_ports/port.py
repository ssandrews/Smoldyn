"""
File: port.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Date: 2020-06-27
Description: 
    port.txt in python.
"""

import smoldyn as S
print(f"[INFO] Using smoldyn from {S.__file__}")

b = S.Boundaries(low=[0,0,0], high=[100,100,100])
print(b)
m = S.Model(bounds=b)
S.setSeed(0)

assert S.getSeed() == 10
assert S.getDim() == 3

