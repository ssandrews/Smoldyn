#!/bin/bash
set -e 
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Place to store wheels.
WHEELHOUSE=$HOME/wheelhouse
echo "Path to store wheels : $WHEELHOUSE"
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE

SMOLDYN_VERSION=$(bash ${SCRIPT_DIR}/get_version.sh)

# install latest cmake using pip. centos-6 cmake is way too old.
PYDIR=/opt/python/cp37-cp37m/
PYTHON=$PYDIR/bin/python
# install cmake using pip.
$PYTHON -m pip install cmake 
CMAKE=$PYDIR/bin/cmake

BUILDDIR=$SCRIPT_DIR/_build_wheel
mkdir -p $BUILDDIR
for _py in 37 38 39; do
    PYTHON=$(ls /opt/python/cp${_py}-cp${_py}*/bin/python)
    PYDIR=$(dirname $PYTHON)
    echo "Python $PYTHON"
    # dependencies
    $PYTHON -m pip install twine auditwheel pytest
    PYLIB=$(ls -d $PYDIR/../lib/python?.?)
    echo "Buiding using $PYTHON and lib dir ${PYLIB}"
    cd $BUILDDIR
    $CMAKE ../../ \
        -DPython3_EXECUTABLE=$PYTHON \
        -DPython3_LIBRARY=$PYLIB \
        -DSMOLDYN_VERSION=${SMOLDYN_VERSION}
    make -j`nproc` VERBOSE=1
    ctest --output-on-failure
    make wheel
    $PYTHON -m auditwheel repair *.whl -w $WHEELHOUSE
    rm -rf *.whl

    echo "Installing before testing ... "
    VENV=/tmp/smoldyn/venv
    rm -rf $VENV
    $PYTHON -m venv $VENV
    (
        source $VENV/bin/activate
        python -m pip install $WHEELHOUSE/smoldyn*-cp${_py}*.whl
        python -c 'import smoldyn; print(smoldyn.__version__ )'
        python -c 'import smoldyn; print(dir(smoldyn))'
        deactivate
    )

done
	
# If successful, upload using twine.
if [ -n "$PYPI_PASSWORD" ]; then
    $PYTHON -m twine upload $WHEELHOUSE/smoldyn*.whl \
        --user __token__ \
        --password $PYPI_PASSWORD \
        --skip-existing
else
    echo "PYPI password is not set. Not uploading...."
fi
