#!/usr/bin/env bash
git subtree pull --prefix source/pybind11 \
    https://github.com/pybind/pybind11 master --squash
