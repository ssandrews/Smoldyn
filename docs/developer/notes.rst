================
Developer Notes
================

Build System
############

The build tool is `CMake <https://cmake.org/cmake/help/v3.12/>`
version 3.4 or higher. To install cmake, use your package manager e.g.
`sudo apt install cmake`, `sudo dnf install cmake` etc or the latest version can
also be installed using `pip` e.g., `pip install camke --user`. 

The cmake build tool does two things. first, it compiles the C++/C codebase
and generates Smoldyn libraries and binary. When building Python extention, it
also compiles the extention `_smoldyn.so`. It also takes care of finding
dependencies, setting approprite compile time flags and other housekeeping.

After `_smoldyn.so` is built, it gnerates a `setup.py` file and copies it to
`py` folder. It also moves the pure Python codebase to this directory. At this
point, the `py` folder contains a standalone Python module which can be used by
setting `PYTHONPATH` to this location (e.g., `export PYTHONPATH=/path/to/py`) or
by running `cd /path/to/py && python3 setup.py install --user`.

The content of the `py` folder. 
::

    py/
    ├── setup.py
    └── smoldyn
        ├── config.py
        ├── __init__.py
        ├── __main__.py
        ├── _smoldyn.cpython-38-x86_64-linux-gnu.so
        ├── smoldyn.py
        ├── types.py
        └── utils.py


Alternatively, you can type `make pyinstall` command. cmake will execute
appropriate `setuptools` command to install the Python module to your system. You
can read about `setuptools` at https://setuptools.readthedocs.io/en/latest/.

Notes
######

`make test` or `ctest` collects tests and run them with a timeout of 10 seconds.
Also, it sets `SMOLDYN_NO_PROMPT` to disable user interaction (effectively
setting `quit_at_end` option).
