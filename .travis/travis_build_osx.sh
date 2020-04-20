#!/bin/bash -
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

set -o nounset                              # Treat unset variables as an error
set -e

# NOTE: On travis, don't enable -j`nproc` option. It may not compile properly.

(
    # Make sure not to pick up python from /opt.
    PATH=/usr/local/bin:/usr/bin:$PATH

    mkdir -p _BUILD && cd _BUILD \
        && cmake ..
    make && make pyinstall && ctest --output-on-failure
)
set +e

