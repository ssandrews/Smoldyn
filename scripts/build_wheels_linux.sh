#!/bin/bash
set -e 

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Place to store wheels.
WHEELHOUSE=${1-$HOME/wheelhouse}
echo "Path to store wheels : $WHEELHOUSE"
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE

SMOLDYN_VERSION=$(bash ${SCRIPT_DIR}/get_version.sh)

PYTHON=/opt/python/cp37-cp37m/bin/python

# dependencies
$PYTHON -m pip install twine auditwheel pytest
mkdir -p _build_wheel_linux
(
    cd _build_wheel_linux
    cmake ../../ \
        -DPYTHON_EXECUTABLE=$PYTHON \
        -DSMOLDYN_VERSION=${SMOLDYN_VERSION}
    make -j`nproc` VERBOSE=1
    make pytest 
    $PYTHON -m smoldyn $SCRIPT_DIR/../examples/S4_molecules/mollist.txt

    # This generates a wheel in wheel/ directory. Repair and put in wheelhouse.
    $PYTHON -m auditwheel repair wheel/*.whl -w $WHEELHOUSE
)

ls -lh $WHEELHOUSE/*.whl
$PYTHON -m auditwheel show $WHEELHOUSE/*.whl

echo "Installing before testing ... "
VENV=/tmp/smoldyn/venv
$PYTHON -m venv $VENV
(
    source $VENV/bin/activate
    python -m pip install $WHEELHOUSE/smoldyn*-py*.whl
    python -c 'import smoldyn; print(smoldyn.__version__ )'
    python -c 'import smoldyn; print(dir(smoldyn))'
    deactivate
)
	
# If successful, upload using twine.
if [ -n "$PYPI_PASSWORD" ]; then
    $PYTHON -m twine upload $WHEELHOUSE/smoldyn*.whl \
        --repository testpypi \
        --user dilawar \
        --password $PYPI_PASSWORD \
        --skip-existing
else
    echo "PYPI password is not set. Not uploading...."
fi
