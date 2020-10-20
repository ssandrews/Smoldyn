#!/bin/bash

# For homebrew.

set -e 

brew install libtiff || echo "Failed to install libtiff"
brew install cmake || echo "Failed to install cmake"
brew install freeglut || echo "Failed to install freeglut"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

SMOLDYN_VERSION=$(bash $SCRIPT_DIR/get_version.sh)

WHEELHOUSE=$HOME/wheelhouse
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE


# Always prefer brew version.
PYTHON=/usr/local/bin/python3

if [ ! -f $PYTHON ]; then
    echo "Not found $PYTHON"
    continue
fi

$PYTHON -m pip install setuptools --upgrade 
$PYTHON -m pip install wheel --upgrade
$PYTHON -m pip install numpy --upgrade
$PYTHON -m pip install delocate --upgrade
$PYTHON -m pip install twine  --upgrade
$PYTHON -m pip install pytest  --upgrade

PLATFORM=$($PYTHON -c "import distutils.util; print(distutils.util.get_platform())")

( 
    mkdir -p _build_wheel && cd _build_wheel
    echo "Building wheel for $PLATFORM"
    ls -ltr
    cmake ../.. \
        -DSMOLDYN_VERSION:STRING=${SMOLDYN_VERSION} \
        -DPYTHON_EXECUTABLE=$PYTHON

    make -j4 
    # cmake generates whl file in the wheel folder.
    /usr/local/bin/delocate-wheel -w $WHEELHOUSE -v wheel/*.whl

    ls $WHEELHOUSE/smoldyn*-py*.whl

    # create a virtualenv and test this.
    VENV=/tmp/venv
    rm -rf $VENV
    (
        $PYTHON -m venv $VENV
        source $VENV/bin/activate
        python --version
        python -m pip install $WHEELHOUSE/smoldyn*-py*.whl
        python -c 'import smoldyn; print(smoldyn.__version__ )'
        python -m smoldyn $SCRIPT_DIR/../examples/S4_molecules/mollist.txt
        deactivate
    )
)

if [ -n "$PYPI_PASSWORD" ]; then
    echo "Did you test the wheels?"
    $PYTHON -m twine upload \
        -u __token__ -p $PYPI_PASSWORD \
        --skip-existing \
        $WHEELHOUSE/smoldyn*.whl
fi
