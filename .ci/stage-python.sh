#!/usr/bin/env bash
# Stage the python package tree for cibuildwheel.
#
# cibuildwheel validates that package-dir contains a pyproject.toml BEFORE
# running CIBW_BEFORE_BUILD, so we configure and run copy_python_tree up
# front. CIBW_BEFORE_BUILD (see .ci/build-wheel.sh) then reconfigures with
# each interpreter and builds _smoldyn.
set -euo pipefail

cd "$(dirname "$0")/.."

cmake -S . -B wheelbuild -DCMAKE_BUILD_TYPE=Release \
  -DOPTION_PYTHON=ON -DOPTION_EXAMPLES=OFF
cmake --build wheelbuild --target copy_python_tree
