#!/bin/bash -
#===============================================================================
#
#          FILE: travis_prepare_osx.sh
#
#         USAGE: ./travis_prepare_osx.sh
#
#   DESCRIPTION: Script to prepare OSX build on Travis CI.
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Dilawar Singh (), dilawars@ncbs.res.in
#  ORGANIZATION: NCBS Bangalore
#       CREATED: 01/02/2017 10:09:00 AM
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error
set +e
#rvm get head
brew update || echo "failed to update"
brew outdated cmake || brew install cmake
brew install boost
brew install python@3 || echo "Allready installed"


# To make sure that we do not pick python from /opt etc.
PATH=/usr/local/bin:/usr/bin:$PATH

python3 -m pip install setuptools --user
