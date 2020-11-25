#!/bin/bash
set -e -x
sphinx-build -T -E -d _build/doctrees-readthedocs -D language=en . _build/html
