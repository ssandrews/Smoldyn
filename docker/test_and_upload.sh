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

SCRIPT_DIR="$(cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)"
WHEELHOUSE=/tmp/wheelhouse
mkdir -p $WHEELHOUSE

GLOBAL_PIP=/opt/python/cp37-cp37m/bin/pip
GLOBAL_PY=/opt/python/cp37-cp37m/bin/python
$GLOBAL_PIP install twine 
TWINE=/opt/python/cp37-cp37m/bin/twine

for whl in `find $HOME/wheelhouse -name "*.whl"`; do
    echo "Wheel $whl"
    if [[ $whl = *"-py2-"* ]]; then 
       echo "no py2"
    else
       echo "++ Python3 wheel $whl";
       PYTHON=/opt/python/cp37-cp37m/bin/python
       PIP=/opt/python/cp37-cp37m/bin/pip
       set -e
       $PIP install $whl

       # Run these tests. These don't use GUI.
       $PYTHON $SCRIPT_DIR/../source/python/tests/test_libsmoldyn.py
       $PYTHON $SCRIPT_DIR/../examples/S4_molecules/mollist.txt
    fi
done

# upload to PYPI.
for whl in $WHEELS; do
    # If successful, upload using twine.
    if [ -n "$PYPI_PASSWORD" ]; then
        # $TWINE upload $whl --user dilawar --password $PYPI_PASSWORD --skip-existing
        $TWINE upload $whl --repository testpypi \
            --user dilawar --password $PYPI_PASSWORD \
            --skip-existing
    else
        echo "PYPI password is not set"
        echo "Copying to /tmp/wheelhouse"
        cp $whl /tmp/wheelhouse/
    fi
done
