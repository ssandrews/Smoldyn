__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

# Test only extension module.
from smoldyn import _smoldyn

def test_library():
    """We test the C API here
    """
    s = _smoldyn.Smoldyn()
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

    MS = _smoldyn.MS     # enum MolcState
    SA = _smoldyn.SA     # enum SrfAction
    PF = _smoldyn.PF     # enum PanelFace
    PS = _smoldyn.PS     # enum PanelShape

    s.setSpeciesMobility("ACA", MS.all, 1.0)
    s.setSpeciesMobility("ATP", MS.all, 1.0)
    s.setSpeciesMobility("cAMP", MS.all, 1.0)
    s.setSpeciesMobility("cAR1", MS.all, 1.0)

    # Adding surface.
    s.addSurface("Membrane00")
    s.setSurfaceAction("Membrane00", PF.both, "ATP", MS.soln, SA.reflect)

    params = [-20.0, 20.0, 10.0, 20.0, 20.0]
    s.addPanel("Membrane00", PS.sph, "", "", params)

    s.addCompartment("Cell00")
    s.addCompartmentSurface("Cell00", "Membrane00")
    s.addCompartmentPoint("Cell00", params)
    s.addSurfaceMolecules("ACA", MS.down, 30, "Membrane00", PS.all, "all", [])
    s.addSurfaceMolecules("cAR1", MS.up, 30, "Membrane00", PS.all, "all", [])
    s.addReaction("r100", "", MS.all, "", MS.all, ["ATP"], [MS.soln], 0.02)
    s.setReactionRegion("r100", "Cell00", "");

    s.run(200, dt=0.01, display=True)


def main():
    test_library()

if __name__ == '__main__':
    main()

