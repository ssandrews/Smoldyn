================
Developer Notes
================

Build System
############

The build system is based on `CMake <https://cmake.org/cmake/help/v3.12/>`
version 3.12 or higher. You can install cmake using your package manager e.g.
`sudo apt install cmake`, `sudo dnf install cmake` etc. The latest version can
also be installed using `pip` e.g., `pip install camke --user`. 

The cmake build system does two things. First it compiles the C++/C codebase
and generates Smoldyn libraries and binary. When building Python extention, it
also compiles the extention `_smoldyn.so`.

After `_smoldyn.so` is built, it creates the `setup.py` file to `py` folder and
also moves the pure Python codebase to this directory. At this point, the `py`
folder contains a standalone Smoldyn module which can be used as a standard
Python module either by setting `PYTHONPATH` to this location (e.g., `export
PYTHONPATH=/path/to/py`) or by running `python3 setup.py install` command
(after moving to this directory e.g., `cd py`).

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
        ├── stypes.py
        └── utils.py


Or alternatively, you can type `make pyinstall` command. cmake will execute
appropriate `setuptools` command to install the Python module to your system. You
can read about `setuptools` at https://setuptools.readthedocs.io/en/latest/.
