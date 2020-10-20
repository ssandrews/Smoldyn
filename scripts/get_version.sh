#!/bin/bash

# +(git hash) is not supported by PyPi.
# echo "3.0.dev$(date +%Y%m%d)+$(git rev-parse --short HEAD)"
echo "2.63.dev$(date +%Y%m%d%H)"
