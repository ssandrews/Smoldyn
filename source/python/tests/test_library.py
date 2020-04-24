__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

import sys
import os
import smoldyn

def test_library():
    s = smoldyn.Model()
    s.bounds = ([-50, 50], [-50, 50])
    assert s.dim == 2
    s.seed = 1
    print('Bounds', s.bounds)

    s.setPartitions("molperbox", 4);
    s.setPartitions("boxsize", 12.5);

    s.addSpecies("ACA")
    s.addSpecies("ATP")
    s.addSpecies("cAMP")
    s.addSpecies("cAR1")


    mState = smoldyn.mState     #  enum  MolcState
    print(type(mState.MSall))

    s.setSpeciesMobility("ACA", mState.MSall, 1.0)
    s.setSpeciesMobility("ATP", mState.MSall, 1.0)
    s.setSpeciesMobility("cAMP", mState.MSall, 1.0)
    s.setSpeciesMobility("cAR1", mState.MSall, 1.0)

    s.run(10)


def main():
    test_library()

if __name__ == '__main__':
    main()

