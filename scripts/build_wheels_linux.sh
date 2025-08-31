#!/bin/bash
set -e 
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Place to store wheels.
WHEELHOUSE=${1-$HOME/wheelhouse}
echo "Path to store wheels : $WHEELHOUSE"
rm -rf $WHEELHOUSE && mkdir -p $WHEELHOUSE

PYDIR39=/opt/python/cp39-cp39/
PYDIR310=/opt/python/cp310-cp310/
PYDIR311=/opt/python/cp311-cp311/
PYDIR312=/opt/python/cp312-cp312/
PYDIR313=/opt/python/cp313-cp313/

for PYDIR in $PYDIR313 $PYDIR312 $PYDIR311 $PYDIR310 $PYDIR39; do
    PYTHON=$PYDIR/bin/python

    # dependencies
    $PYTHON -m pip install auditwheel pytest setuptools
    rm -rf _build_wheel_linux
    mkdir _build_wheel_linux
    (
        cd _build_wheel_linux
	    # cmake version must be higher than 3.12
	    PYLIB=$(ls -d $PYDIR/lib/python3.*)
	    PYINDIR=$(ls -d $PYDIR/include/python3.*)
        cmake ../../ \
            -DOPTION_PYTHON=ON \
            -DOPTION_EXAMPLES=ON \
            -DCMAKE_INSTALL_PREFIX=/usr \
	        -DPython3_EXECUTABLE=$PYTHON \
	        -DPython3_INCLUDE_DIR=$PYINDIR \
	        -DPython3_LIBRARY=$PYLIB 
        make -j`nproc` 
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

PYTHON=$PYDIR39/bin/python
$PYTHON -m pip install twine

ls -lh $WHEELHOUSE/*.whl

# If successful, upload using twine.
if [ -n "$PYPI_PASSWORD" ]; then
    $PYTHON -m twine upload $WHEELHOUSE/smoldyn*.whl \
        --user __token__ \
        --password $PYPI_PASSWORD \
        --skip-existing
else
    echo "PYPI password is not set. Not uploading...."
fi
