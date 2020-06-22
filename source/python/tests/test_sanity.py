"""test_sanity.py: 

"""
    
__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2016, Dilawar Singh"
__credits__          = ["NCBS Bangalore"]
__license__          = "GNU GPL"
__version__          = "1.0.0"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"
__status__           = "Development"

from smoldyn import _smoldyn

def main():
    curSim = _smoldyn.getCurSimStruct()
    print('curSim', curSim)   # not initialized yet.
    s1 = _smoldyn.newSim(3, [0, 0, 0], [10,10,10]) 
    assert s1 != curSim
    s2 = _smoldyn.newSim(2, [0,0], [10,10])  
    assert s2 != curSim != s1
    _smoldyn.setCurSimStruct(s1)
    assert s1 == _smoldyn.getCurSimStruct()
    print('s1', s1)
    print('s2', s2)
    

if __name__ == "__main__":
    main()
