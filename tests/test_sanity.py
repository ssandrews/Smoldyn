"""
File: test_sanity.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Description:
    User can create multiple simptr and assign one of them as current simptr.
"""
# todo: Test a full simulation with configuration changes.

import smoldyn

print(f'Using smoldyn from {smoldyn.__file__}')

def test_newcursim():
    print("test_newcursim")
    s = smoldyn.Simulation(low=(0,0), high=(1,1))
    cs = s.simptr
    assert cs != None
    print(cs)

def test_get_err():
    print('Testing test_get_err')
    s = smoldyn.Simulation(low=(0,0), high=(1,1))
    a = smoldyn.getError()
    print(a)

def main():
    test_newcursim()
    test_get_err()

if __name__ == "__main__":
    main()
