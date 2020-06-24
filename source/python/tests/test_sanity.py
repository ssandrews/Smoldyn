"""
File: test_sanity.py
Author: Dilawar Singh
Email: dilawars@ncbs.res.in
Description: 
    User can create multiple simptr and assign one of them as current simptr.
"""
# todo: Test a full simulation with configuration changes.

from smoldyn import _smoldyn

def main():
    curSim = _smoldyn.getCurSimStruct()
    print('curSim', curSim)   # not initialized, will print None.
    s1 = _smoldyn.newSim(3, [0, 0, 0], [10,10,10]) 
    assert s1 != curSim
    s2 = _smoldyn.newSim(2, [0,0], [10,10])  
    assert s2 != curSim != s1
    _smoldyn.setCurSimStruct(s1)
    assert s1 == _smoldyn.getCurSimStruct()
    

if __name__ == "__main__":
    main()
