__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbS.reS.in"

# Test only extension module.
import smoldyn as S

def test_library():
    """We test the C API here
    """
    S.setBoundaries(([-50, 50], [-50, 50]))
    assert S.getDim() == 2
    S.setRandomSeed(1)
    print('Bounds', S.getBoundaries())

    S.setPartitions("molperbox", 4);
    S.setPartitions("boxsize", 12.5);

    S.addSpecies("ACA")
    S.addSpecies("ATP")
    S.addSpecies("cAMP")
    S.addSpecies("cAR1")

    MS = S.MolecState     # enum MolcState
    SA = S.SrfAction      # enum SrfAction
    PF = S.PanelFace      # enum PanelFace
    PS = S.PanelShape     # enum PanelShape

    S.setSpeciesMobility("ACA", MS.all, 1.0)
    S.setSpeciesMobility("ATP", MS.all, 1.0)
    S.setSpeciesMobility("cAMP", MS.all, 1.0)
    S.setSpeciesMobility("cAR1", MS.all, 1.0)

    # Adding surface.
    S.addSurface("Membrane00")
    S.setSurfaceAction("Membrane00", PF.both, "ATP", MS.soln, SA.reflect,"")

    params = [-20.0, 20.0, 10.0, 20.0, 20.0]
    S.addPanel("Membrane00", PS.sph, "", "", params)

    S.addCompartment("Cell00")
    S.addCompartmentSurface("Cell00", "Membrane00")
    S.addCompartmentPoint("Cell00", params)
    S.addSurfaceMolecules("ACA", MS.down, 30, "Membrane00", PS.all, "all", [])
    S.addSurfaceMolecules("cAR1", MS.up, 30, "Membrane00", PS.all, "all", [])
    S.addReaction("r100", "", MS.all, "", MS.all, ["ATP"], [MS.soln], 0.02)
    S.setReactionRegion("r100", "Cell00", "");

    S.run(200, dt=0.01, display=True)


def main():
    test_library()

if __name__ == '__main__':
    main()

