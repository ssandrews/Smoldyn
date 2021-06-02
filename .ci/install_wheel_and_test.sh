#!/usr/bin/env bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

install_and_run()
(
    PIPARG="$1"

    cd $SCRIPT_DIR

    VENV_DIR=__VENV_$PIPARG

    # Create a virtualenvironment
    python -m venv $VENV_DIR
    source $VENV_DIR/bin/activate

    # build and test dependencies.
    python -m pip install numpy    # may be this should be installed by smoldyn.

    # pass extra args PIPARG, it is usually --pre
    python -m pip install smoldyn[dev,biosimulators,biosimulators-tests] $PIPARG

    # Run pytest
    for TEST_SCRIPT in "../tests/"test_*.py; do
        echo "Running test ${TEST_SCRIPT}"
        SMOLDYN_NO_PROMPT=1 python $TEST_SCRIPT \
            > ${TEST_SCRIPT}_.stdout \
            2> ${TEST_SCRIPT}_.stderr
    done;

    deactivate
    rm -rf $VENV_DIR
)

# test normal release
echo "Installing and testing normal release"
install_and_run

# test the --pre release
echo "Installing and testing with --pre"
install_and_run --pre
