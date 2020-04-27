#!/bin/bash -
#   DESCRIPTION: Script to prepare OSX build on Travis CI.
#
#        AUTHOR: Dilawar Singh (), dilawars@ncbs.res.in
#  ORGANIZATION: NCBS Bangalore

set -o nounset                              # Treat unset variables as an error
set +e
#rvm get head
brew update || echo "failed to update"
brew outdated cmake || brew install cmake
brew install boost
brew install python@3 || echo "Allready installed"


# To make sure that we do not pick python from /opt etc.
PATH=/usr/local/bin:/usr/bin:$PATH

python3 -m pip install setuptools wheel --user
python3 -m pip install pytest --user
