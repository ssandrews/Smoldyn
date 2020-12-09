#!/bin/bash

set -e 
set -x

brew install libtiff || echo "Failed to install libtiff"
brew install cmake || echo "Failed to install cmake"
brew install freeglut || echo "Failed to install freeglut"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export PATH=/usr/local/bin:$PATH

WHEELHOUSE=$HOME/wheelhouse
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE


# Always prefer brew version.
PYTHON=$(which python3)

if [ ! -f $PYTHON ]; then
    echo "Not found $PYTHON"
    exit -1
fi

$PYTHON -m pip install setuptools --upgrade 
$PYTHON -m pip install wheel --upgrade
$PYTHON -m pip install numpy --upgrade
$PYTHON -m pip install delocate --upgrade
$PYTHON -m pip install twine  --upgrade
$PYTHON -m pip install pytest  --upgrade
$PYTHON -m pip install matplotlib  --upgrade

PLATFORM=$($PYTHON -c "import distutils.util; print(distutils.util.get_platform())")

( 
    mkdir -p _build_wheel && cd _build_wheel
    echo "Building wheel for $PLATFORM"
    ls -ltr
    cmake ../.. \
        -DOPTION_PYTHON=ON \
        -DOPTION_EXAMPLES=ON \
        -DPython3_EXECUTABLE=$PYTHON
    make -j4 
    make wheel
    ctest --output-on-failure
    /usr/local/bin/delocate-wheel -w $WHEELHOUSE -v *.whl
    ls $WHEELHOUSE/smoldyn*.whl

    ## NOTE: I am contantly getting  the following error in venv.
    ## $ python -c 'import smoldyn; print(smoldyn.__version__ )'
    ## Fatal Python error: PyMUTEX_LOCK(gil->mutex) failed

    ## create a virtualenv and test this.
    ##VENV=$(pwd)/venv
    ##rm -rf $VENV
    (
        # $PYTHON -m venv $VENV
        # source $VENV/bin/activate
        # which python
        # now use venv pyhton.
        $PYTHON --version
        $PYTHON -m pip install $WHEELHOUSE/smoldyn*.whl
        $PYTHON -c 'import smoldyn; print(smoldyn.__version__ )'
        $PYTHON -m pip uninstall -y smoldyn
    )
)

if [ -n "$PYPI_PASSWORD" ]; then
    echo "Did you test the wheels?"
    $PYTHON -m twine upload \
        -u __token__ -p $PYPI_PASSWORD \
        --skip-existing \
        $WHEELHOUSE/smoldyn*.whl
fi
