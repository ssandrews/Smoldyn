# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Smoldyn is a particle-based biochemical simulator (molecular diffusion, surface interactions, chemical reactions). The repo produces three artifacts from a shared C/C++ core:

1. `smoldyn` — stand-alone CLI executable (reads `.txt` model files).
2. `libsmoldyn` — C/C++ library (`libsmoldyn.so` / `.a`) exposed via `source/Smoldyn/libsmoldyn.h`.
3. `smoldyn` Python package — `pybind11` bindings shipped as a wheel.

The same C/C++ sources are compiled **twice**: once for the executable and shared library, and again for the Python module with extra defines (`-DENABLE_PYTHON_CALLBACK -DCOMPILE_AS_PY_MODULE`). When changing core sources, be aware both builds must succeed.

## Build

CMake out-of-source build (the `build/` dir at repo root is the conventional location, and is already gitignored):

```sh
mkdir -p build && cd build
cmake -DOPTION_PYTHON=ON -DOPTION_EXAMPLES=ON ..
make -j$(nproc)
```

Selected CMake options (see `CMakeLists.txt:42-70` for the full list):
- `OPTION_TARGET_SMOLDYN` / `OPTION_TARGET_LIBSMOLDYN` — which artifacts to build.
- `OPTION_PYTHON` (ON) — build the `_smoldyn` Python module; implicitly forces `OPTION_TARGET_LIBSMOLDYN=ON`.
- `OPTION_USE_OPENGL` (ON), `OPTION_USE_LIBTIFF` (ON), `OPTION_NSV` (ON, NextSubVolume hybrid sims), `OPTION_VTK` (OFF).
- `OPTION_ASAN` — AddressSanitizer (Debug builds).
- `OPTION_VCELL`, `OPTION_MINGW`, `OPTION_STATIC` — alternate build flavors.

OpenGL (GL+GLU+GLUT) and libtiff are required by default. On Debian/Ubuntu: `cmake libjpeg-dev libtiff-dev freeglut3-dev libxi-dev libxmu-dev`. Headless test runs need `xvfb-run`.

Common make targets defined in addition to the CMake defaults:
- `make wheel` — build a Python wheel into `build/`.
- `make pyinstall` / `make pyinstall_venv` / `make pydevel` — install the wheel (system, venv, or editable).
- `make lint` — runs `uvx flake8` (on `biosimulators/`) and `uvx mypy --strict` on the Python sources. Requires `uv` on PATH.
- `make fmt` — `uvx ruff format` over `source/python`.
- `make install_deps` — pip-installs build-time Python deps.

## Test

Tests are registered with CTest (both C++ examples and Python tests under `tests/`). The Python tests are **invoked directly with `python` rather than `pytest`** because they rely on global state — see the comment at `source/python/CMakeLists.txt:141`. Each test file is a runnable script.

```sh
# From build/
xvfb-run --auto-servernum ctest --output-on-failure -j2     # all tests (GUI headless)
ctest -R test_sanity --output-on-failure                     # single test by name
ctest -N                                                     # list registered tests
```

To run a Python test file directly (matching what CTest does), set the path so the freshly built module is found:

```sh
PYTHONPATH=build/py SMOLDYN_NO_PROMPT=1 python tests/test_sanity.py
```

C++ libsmoldyn example tests live in `examples/` and are only built when `-DOPTION_EXAMPLES=ON`.

## Code layout

- `source/Smoldyn/` — core simulator. C files (`smolboxes.c`, `smolmolec.c`, `smolreact.c`, `smolsurface.c`, `smolwall.c`, …) implement subsystems; `smolsim.cpp` is the main simulation loop; `libsmoldyn.cpp` is the library entry layer; `smoldyn.cpp` is the CLI `main()`. Public header for the library: `libsmoldyn.h`; full data structures in `smoldyn.h`.
- `source/libSteve/` — Steve Andrews' general-purpose C utility library (parsers, lists, math helpers). Built as `Steve` object library and linked into everything.
- `source/NextSubVolume/` — NSV stochastic spatial solver, enabled by `OPTION_NSV`.
- `source/python/` — pybind11 bindings (`module.cpp`, `Simulation.cpp`, `Command.cpp`, `CallbackFunc.cpp`) plus the user-facing pure-Python package under `source/python/smoldyn/` (`smoldyn.py` is the public API, `_smoldyn` is the compiled extension, `biosimulators/` is the SED-ML/COMBINE adapter).
- `source/pybind11/` — vendored pybind11 (do not edit; `scripts/update_pybind11.sh` updates it).
- `source/vcell/`, `source/SmolCrowd/`, `source/wrl2smol/`, `source/SmolEmulate/`, `source/BioNetGen/` — optional integrations / utility programs.
- `tests/` — Python tests (`test_*.py`), plus `fixtures/` and `issues/` containing regression `.txt` models.
- `examples/` — Smoldyn `.txt` model files organized by feature (`S4_molecules`, `S7_surfaces`, `S97_libsmoldyn`, …); double as integration tests.
- `cmake/` — custom Find modules (e.g. VTK).
- `scripts/` — wheel-builder scripts, Dockerfiles, regression runner (`regression.py`), perf script.

## Conventions worth knowing

- The Python module's source-of-truth lives in `source/python/smoldyn/`. CMake **copies** it to `build/py/smoldyn/` and builds the wheel from there — editing files under `build/py/` will be overwritten.
- The compiled `_smoldyn.*.so` is also placed back into the source tree at `source/python/smoldyn/_smoldyn.*.so` (see `LIBRARY_OUTPUT_DIRECTORY` in `source/python/CMakeLists.txt`). This is intentional so `import smoldyn` from `source/python/` works without installing the wheel.
- `source/smoldynconfigure.h.in` is configured into `build/smoldynconfigure.h` and carries build-time options (HAVE_OPENGL, HAVE_LIBTIFF, VERSION, etc.). When adding new options, wire them through here.
- Version: if a git tag points at HEAD (e.g. `v2.76`), that becomes `SMOLDYN_VERSION`; otherwise `2.76.devYYYYMMDDHHMM`. CI can override via `-DSMOLDYN_VERSION=...`.
- Windows MSVC builds reduce warnings to `/W1` deliberately; Linux/macOS use `-Wall -Wextra`. `OPTION_WARNING_AS_ERROR` upgrades to `-Werror`.
