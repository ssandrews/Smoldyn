#!/bin/bash
set -e 

BRANCH=$(cat ./BRANCH)
VERSION=3.2.0.dev$(date +%Y%m%d)

echo "[INFO] Create virtualenv by yourself"

brew install gsl 

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

MOOSE_SOURCE_DIR=`pwd`/moose-core

if [ ! -d $MOOSE_SOURCE_DIR ]; then
    git clone https://github.com/BhallaLab/moose-core -b $BRANCH --depth 10
fi
cd moose-core && git pull
WHEELHOUSE=$HOME/wheelhouse
mkdir -p $WHEELHOUSE

# Always prefer brew version.
for _py in 3 2; do
    PYTHON=/usr/local/bin/python$_py

    if [ ! -f $PYTHON ]; then
        echo "Not found $PYTHON"
        continue
    fi

    $PYTHON -m pip install setuptools --upgrade --user
    $PYTHON -m pip install wheel --upgrade --user
    $PYTHON -m pip install numpy --upgrade --user
    $PYTHON -m pip install delocate --upgrade  --user
    $PYTHON -m pip install twine  --upgrade  --user

    PLATFORM=$($PYTHON -c "import distutils.util; print(distutils.util.get_platform())")

    ( 
        cd $MOOSE_SOURCE_DIR
        mkdir -p _build && cd _build
        echo "Building wheel for $PLATFORM"
        cmake -DVERSION_MOOSE=$VERSION \
            -DCMAKE_RELEASE_TYPE=Release \
            -DWITH_HDF=OFF \
            -DPYTHON_EXECUTABLE=$PYTHON \
            ..

        make -j4
        ( 
            cd python 
            ls *.py
            sed "s/from distutils.*setup/from setuptools import setup/g" setup.cmake.py > setup.wheel.py
            $PYTHON setup.wheel.py bdist_wheel -p $PLATFORM 
            # Now fix the wheel using delocate.
            delocate-wheel -w $WHEELHOUSE -v dist/*.whl
        )

        ls $WHEELHOUSE/pymoose*-py${_py}-*.whl

        # create a virtualenv and test this.
        rm -rf $HOME/Py${_py}
        (
            virtualenv -p $PYTHON $HOME/Py${_py}
            source $HOME/Py${_py}/bin/activate
            set +x 
            python -m pip install $WHEELHOUSE/pymoose*-py${_py}-*.whl
            set -x
            which python
            python --version
            python -c 'import moose; print( moose.__version__ )'
            deactivate
        )
    )

    if [ -n "$PYPI_PASSWORD" ]; then
        echo "Did you test the wheels?"
        $PYTHON -m twine upload -u dilawar -p $PYPI_PASSWORD $HOME/wheelhouse/pymoose*.whl
    fi
done
