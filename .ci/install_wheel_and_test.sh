#!/usr/bin/env bash

set -e
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

install_and_run()
{
    PIPARG="$1"

    # build and test dependencies.
    python -m pip install numpy    # may be this should be installed by smoldyn.

    # pass extra args PIPARG, it is usually --pre
    python -m pip install smoldyn[dev,biosimulators,biosimulators-tests] $PIPARG

    # Run pytest
    for TEST_SCRIPT in "../tests/"test_*.py; do
        echo "Running test ${TEST_SCRIPT}"
        SMOLDYN_NO_PROMPT=1 python $TEST_SCRIPT
    done;
    echo "Ran all tests"
}

# test normal release
(
    echo "Installing and testing normal release"
    cd $SCRIPT_DIR
    install_and_run || echo "Failed with previous stable release which is OK."
)

# test the --pre release
(
    echo "Installing and testing with --pre (this must not fail)"
    cd $SCRIPT_DIR
    install_and_run --pre
)
