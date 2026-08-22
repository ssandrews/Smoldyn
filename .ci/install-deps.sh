#!/usr/bin/env bash
# Install system dependencies needed to build smoldyn with OpenGL/libtiff.
# Used by the Wheels workflow (cibuildwheel) on GitHub Actions.
set -euo pipefail

case "$(uname -s)" in
  Linux*)
    sudo apt-get update
    sudo apt-get install -y cmake libjpeg-dev libtiff-dev freeglut3-dev \
      libxi-dev libxmu-dev
    ;;
  Darwin*)
    brew install cmake libtiff
    ;;
  *)
    echo "No extra dependencies needed on $(uname -s) (Windows runners ship the GL SDK)."
    ;;
esac
