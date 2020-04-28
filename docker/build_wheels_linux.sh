#!/bin/sh

set -e 

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Place to store wheels.
WHEELHOUSE=${1-$HOME/wheelhouse}
echo "Path to store wheels : $WHEELHOUSE"
mkdir -p $WHEELHOUSE

# tag on github and revision number. Make sure that they are there.
BRANCH=$(cat ./BRANCH)
VERSION=$(cat ./VERSION)

SOURCE_DIR=$SCRIPT_DIR/..

for PYV in 37; do
    PYDIR=/opt/python/cp${PYV}-cp${PYV}m
    PYVER=$(basename $PYDIR)
    mkdir -p $PYVER
    (
        cd $PYVER
        echo "Building using $PYDIR in $PYVER"
        PYTHON=$(ls $PYDIR/bin/python?.?)
        $PYTHON -m pip install numpy matplotlib
        git pull || echo "Failed to pull $BRANCH"
        cmake -DPYTHON_EXECUTABLE=$PYTHON  ${SOURCE_DIR}
        make -j`nproc`
        # Now build bdist_wheel
        cd source/python 
        $PYDIR/bin/pip wheel . -w $WHEELHOUSE 
    )
done

# now check the wheels.
for whl in $WHEELHOUSE/*.whl; do
    auditwheel show "$whl"
done

echo "Installing before testing ... "
/opt/python/cp36-cp36m/bin/pip install $WHEELHOUSE/*-py3*.whl
for PYV in 36; do
    PYDIR=/opt/python/cp${PYV}-cp${PYV}m
    PYTHON=$(ls $PYDIR/bin/python?.?)
    echo " -- Installing for $PYTHON ... "
    $PYTHON -c 'import smoldyn; print(smoldyn.__version__ )'
done
	
ls -lh $WHEELHOUSE/*.whl
