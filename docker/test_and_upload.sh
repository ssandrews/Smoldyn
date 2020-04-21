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
GLOBAL_PIP=/opt/python/cp27-cp27m/bin/pip
GLOBAL_PY=/opt/python/cp27-cp27m/bin/python
$GLOBAL_PIP install twine 
TWINE=/opt/python/cp27-cp27m/bin/twine

cat <<EOF >/tmp/test.py
import moose
import moose.utils as mu
print( moose.__version__ )
import rdesigneur as rd
rdes = rd.rdesigneur()
rdes.buildModel()
moose.showfields( rdes.soma )
moose.reinit( )
moose.start( 1 )
EOF

for whl in `find $HOME/wheelhouse -name "*.whl"`; do
    echo "Wheel $whl"
    if [[ $whl = *"-py2-"* ]]; then 
       echo "++ Python2 wheel $whl";
       PYTHON=/opt/python/cp27-cp27m/bin/python
       PIP=/opt/python/cp27-cp27m/bin/pip
       set -e
       $PIP install $whl
       $PYTHON /tmp/test.py
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
        $TWINE upload $whl --user bhallalab --password $PYPI_PASSWORD --skip-existing
    else
        echo "PYPI password is not set"
        echo "Copying to /tmp/wheelhouse"
        cp $whl /tmp/wheelhouse/
    fi
done
