"""
File: test_sanity.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Description: 
    User can create multiple simptr and assign one of them as current simptr.
"""
# todo: Test a full simulation with configuration changes.

import smoldyn


def test_cursim():
    print("test_cursim")
    smoldyn.clearAllSimStructs()
    curSim = smoldyn.getCurSimStruct()
    print("cursim", curSim)
    # assert curSim == None, curSim # Not initialized yet.
    s1 = smoldyn.newSim([0, 0, 0], [10, 10, 10])
    # assert s1 != curSim, (s1, curSim)

    s2 = smoldyn.newSim([0, 0], [10, 10])
    assert s2 != s1, (s1, s2)
    smoldyn.setCurSimStruct(s1)
    assert s1 == smoldyn.getCurSimStruct()


def test_newcursim():
    print("test_newcursim")
    smoldyn.setBoundaries([(0, 1), (0, 1)])
    cs = smoldyn.getCurSimStruct()
    assert cs != None
    print(cs)


def main():
    """TODO: Docstring for main.
    :returns: TODO

    """
    test_newcursim()
    test_cursim()
