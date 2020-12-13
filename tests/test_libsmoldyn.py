"""Some tests for CPP API.

"""

__author__           = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

# Test only the extension module. It has to be imported by the user.
import smoldyn._smoldyn as CppApi

def test_library():
    """We test the C API here
    """
    CppApi.setBoundaries(([-50, 50], [-50, 50]))
    assert CppApi.getDim() == 2
    CppApi.setRandomSeed(1)
    print('Bounds', CppApi.getBoundaries())

    CppApi.setPartitions("molperbox", 4);
    CppApi.setPartitions("boxsize", 12.5);

    CppApi.addSpecies("ACA")
    CppApi.addSpecies("ATP")
    CppApi.addSpecies("cAMP")
    CppApi.addSpecies("cAR1")

    MS = CppApi.MolecState     # enum MolcState
    SA = CppApi.SrfAction      # enum SrfAction
    PF = CppApi.PanelFace      # enum PanelFace
    PS = CppApi.PanelShape     # enum PanelShape

    CppApi.setSpeciesMobility("ACA", MS.all, 1.0)
    CppApi.setSpeciesMobility("ATP", MS.all, 1.0)
    CppApi.setSpeciesMobility("cAMP", MS.all, 1.0)
    CppApi.setSpeciesMobility("cAR1", MS.all, 1.0)

    # Adding surface.
    CppApi.addSurface("Membrane00")
    CppApi.setSurfaceAction("Membrane00", PF.both, "ATP", MS.soln, SA.reflect,"")

    params = [-20.0, 20.0, 10.0, 20.0, 20.0]
    CppApi.addPanel("Membrane00", PS.sph, "", "", params)

    CppApi.addCompartment("Cell00")
    CppApi.addCompartmentSurface("Cell00", "Membrane00")
    CppApi.addCompartmentPoint("Cell00", params)
    CppApi.addSurfaceMolecules("ACA", MS.down, 30, "Membrane00", PS.all, "all", [])
    CppApi.addSurfaceMolecules("cAR1", MS.up, 30, "Membrane00", PS.all, "all", [])
    CppApi.addReaction("r100", "", MS.all, "", MS.all, ["ATP"], [MS.soln], 0.02)
    CppApi.setReactionRegion("r100", "Cell00", "");

    CppApi.run(200, dt=0.01, display=True)


def main():
    test_library()

if __name__ == '__main__':
    main()

