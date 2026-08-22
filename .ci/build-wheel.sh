#!/usr/bin/env bash
# Build the _smoldyn module for the interpreter cibuildwheel is currently
# targeting ($PYTHON) and stage the python package tree into wheelbuild/py.
# Used as CIBW_BEFORE_BUILD.
set -euo pipefail

cd "$(dirname "$0")/.."

# cibuildwheel puts the target interpreter's venv on PATH (there is no
# $PYTHON env var), so use `python` from PATH.
PY=$(command -v python)

EXTRA_ARGS=()
if [[ "$(uname -s)" == MINGW* || "$(uname -s)" == MSYS* || "$(uname -s)" == CYGWIN* ]]; then
  # Mirror the windows.yml release workflow.
  EXTRA_ARGS+=(-DCMAKE_SYSTEM_VERSION=10.0.19041.0)
fi

cmake -S . -B wheelbuild -DCMAKE_BUILD_TYPE=Release \
  -DOPTION_PYTHON=ON -DOPTION_EXAMPLES=OFF \
  -DPython3_EXECUTABLE="$PY" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
cmake --build wheelbuild --config Release \
  --target _smoldyn copy_python_tree --parallel
