#!/usr/bin/env bash
# Headless smoke test run inside each wheel's venv by cibuildwheel
# (CIBW_TEST_COMMAND). cwd is the package dir, hence ../.. reaches the
# repo root.
set -euo pipefail

python -c "import smoldyn; print(smoldyn.__version__)"
python -m smoldyn ../../examples/S4_molecules/mollist.txt
