#!/bin/bash
set -e
Xvfb $DISPLAY -ac -screen 0 "$XVFB_RES" -nolisten tcp $XVFB_ARGS &
XVFB_PROC=$!
sleep 1
"$@"
result=$?
kill $XVFB_PROC || true
exit $result
