#!/usr/bin/env bash

#===============================================================================
#
#          FILE: travis_build_linux.sh
#
#         USAGE: ./travis_build_linux.sh
#
#   DESCRIPTION:  Build  on linux environment.
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Dilawar Singh (), dilawars@ncbs.res.in
#  ORGANIZATION: NCBS Bangalore
#       CREATED: 01/02/2017 10:11:46 AM
#      REVISION:  ---
#===============================================================================

set -e
set -x

# Bug: `which python` returns /opt/bin/python* etc on travis. For which numpy
# many not be available. Therefore, it is neccessary to use fixed path for
# python executable.

echo "Currently in `pwd`"
(
    mkdir -p _BUILD && cd _BUILD
    cmake -DPYTHON_EXECUTABLE=/usr/bin/python3 ..
    $MAKE && ctest --output-on-failure
    sudo make install && cd  /tmp
)
