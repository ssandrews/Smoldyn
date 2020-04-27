#!/bin/bash -
#         USAGE: ./travis_prepare_linux.sh
#   DESCRIPTION:  Prepare linux build environment on travis.
#        AUTHOR: Dilawar Singh (), dilawars@ncbs.res.in
#  ORGANIZATION: NCBS Bangalore

set -o nounset                              # Treat unset variables as an error
set +e  # Let installation fail in some command

apt-get install -qq make cmake
apt-get install -qq libtiff-dev libzip-dev freeglut3-dev python3-dev 
apt-get install -qq python3-numpy
apt-get install -qq python3-setuptools python3-pip python3-wheel
apt-get install -qq python3-pytest
