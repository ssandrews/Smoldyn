#!/bin/bash -
#===============================================================================
#
#          FILE: test_and_upload.sh
#
#         USAGE: ./test_and_upload.sh
#
#   DESCRIPTION: Test each wheels and upload.
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Dilawar Singh (), dilawars@ncbs.res.in
#  ORGANIZATION: NCBS Bangalore
#       CREATED: Saturday 03 March 2018 09:56:20  IST
#      REVISION:  ---
#===============================================================================

set -e 
set -x
set -o nounset                                  # Treat unset variables as an error
GLOBAL_PIP=/opt/python/cp37-cp37m/bin/pip
GLOBAL_PY=/opt/python/cp37-cp37m/bin/python
$GLOBAL_PIP install twine 
TWINE=/opt/python/cp37-cp37m/bin/twine

cat <<EOF >/tmp/test.py
import smoldyn
import smoldyn
print( smoldy.__version__ )

def test_library():
    """We test the C API here
    """
    s = smoldyn.Smoldyn()
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

    MS = smoldyn.MS     # enum MolcState
    SA = smoldyn.SA     # enum SrfAction
    PF = smoldyn.PF     # enum PanelFace
    PS = smoldyn.PS     # enum PanelShape

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
    s.run(200, dt=0.01, display=False)

def main():
    test_library()

if __name__ == '__main__':
    main()
EOF

for whl in `find $HOME/wheelhouse -name "*.whl"`; do
    echo "Wheel $whl"
    if [[ $whl = *"-py2-"* ]]; then 
       echo "no py2"
    else
       echo "++ Python3 wheel $whl";
       PYTHON=/opt/python/cp36-cp36m/bin/python
       PIP=/opt/python/cp36-cp36m/bin/pip
       set -e
       $PIP install $whl
       $PYTHON /tmp/test.py
    fi
done

# upload to PYPI.
for whl in $WHEELS; do
    mkdir -p /tmp/wheelhouse
    # If successful, upload using twine.
    if [ -n "$PYPI_PASSWORD" ]; then
        $TWINE upload $whl --user dilawar --password $PYPI_PASSWORD --skip-existing
    else
        echo "PYPI password is not set"
        echo "Copying to /tmp/wheelhouse"
        cp $whl /tmp/wheelhouse/
    fi
done
