#!/bin/bash

set -x

# when set, smoldyn won't ask for user to press shift+Q at the end of
# simulation.
export SMOLDYN_NO_PROMPT=1

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SMOLDYN_BIN=$(pwd)/smoldyn
ALL_DONE=0

function quit_ctrlc()
{
    echo "CTLR+C is pressed?"
    ALL_DONE=1
}

trap quit_ctrlc SIGINT

EXAMPLES=$(find "*.txt" "$SCRIPT_DIR/../examples" -type f -name "*.txt")
for _f in $EXAMPLES; do
    fname=$(basename "$_f")
    timeout 30 perf record -o "$fname".perf "$SMOLDYN_BIN" "$_f"
    if [ "$ALL_DONE" -ne 0 ]; then
        break;
    fi
done
