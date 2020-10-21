#!/bin/bash
set -e 
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Place to store wheels.
WHEELHOUSE=${1-$HOME/wheelhouse}
echo "Path to store wheels : $WHEELHOUSE"
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE

SMOLDYN_VERSION=$(bash ${SCRIPT_DIR}/get_version.sh)

PYDIR37=/opt/python/cp36-cp36m/
PYDIR37=/opt/python/cp37-cp37m/
PYDIR38=/opt/python/cp38-cp38/
PYDIR39=/opt/python/cp39-cp39/

for PYDIR in $PYDIR36 $PYDIR37 $PYDIR38; do
    PYTHON=$PYDIR/bin/python

    # dependencies
    $PYTHON -m pip install twine auditwheel pytest
    mkdir -p _build_wheel_linux
    (
        cd _build_wheel_linux
	# cmake version must be higher than 3.12
	PYLIB=$(ls -d $PYDIR/lib/python3.*)
        cmake ../../ \
            -DCMAKE_INSTALL_PREFIX=/usr \
            -DPython3_EXECUTABLE=$PYTHON \
            -DPython3_LIBRARY=$PYLIB \
            -DSMOLDYN_VERSION=${SMOLDYN_VERSION}
        make -j`nproc` VERBOSE=1
        make wheel 
        export PYTHONPATH=$(pwd)/py
        $PYTHON -m smoldyn $SCRIPT_DIR/../examples/S4_molecules/mollist.txt
        $PYTHON -m auditwheel repair *.whl -w $WHEELHOUSE

	# install and test it
        $PYTHON -m pip install *.whl
        $PYTHON -c 'import smoldyn; print(dir(smoldyn))'
        $PYTHON -c 'import smoldyn; print(smoldyn.__version__ )'

	# remove it
        rm -rf *.whl
    )
done

ls -lh $WHEELHOUSE/*.whl
$PYTHON -m auditwheel show $WHEELHOUSE/*.whl

# If successful, upload using twine.
if [ -n "$PYPI_PASSWORD" ]; then
    $PYTHON -m twine upload $WHEELHOUSE/smoldyn*.whl \
        --user __token__ \
        --password $PYPI_PASSWORD \
        --skip-existing
else
    echo "PYPI password is not set. Not uploading...."
fi
