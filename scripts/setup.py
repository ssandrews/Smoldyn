"""
# There are two equivalent ways to build smoldyn: cmake and setup.py
#
# This script can be called directly to build and install the smoldyn
# module. It is neccessary when a pip is used `pip install smoldyn`.
#
# Alternatively, you can use cmake build system which provides finer control
# over the build. 
#
"""

__author__ = "Dilawar Singh"
__copyright__ = "Copyright 2020-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__ = "dilawar.s.rajput@gmail.com"

import os
import sys
import multiprocessing
import subprocess
import datetime
from pathlib import Path

try:
    cmakeVersion = subprocess.call(["cmake", "--version"], stdout=subprocess.PIPE)
except Exception as e:
    print(e)
    print("[ERROR] cmake is not found. Please install cmake.")
    quit(-1)

from setuptools import setup, Extension, Command
from setuptools.command.build_ext import build_ext as _build_ext
import subprocess

# Global variables.
sdir_ = Path(__file__).parent

stamp = datetime.datetime.now().strftime("%Y%m%d%H")
builddir_ = sdir_ / "_temp__build"

if builddir_.exists():
    builddir_.mkdir(exist_ok=True, parents=True)

numCores_ = multiprocessing.cpu_count()

version_ = "2.63.dev%s" % stamp


class TestCommand(Command):
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        print("[INFO ] Running tests... ")
        os.chdir(builddir_)
        self.spawn(["ctest", "--output-on-failure", "-j%d" % numCores_])
        os.chdir(sdir_)


class build_ext(_build_ext):
    user_options = [
        ("debug", None, "Build smoldyn in debugging mode (OFF)"),
        ("with-doc", None, "Build documents"),
        ("no-build", None, "DO NOT BUILD. (for debugging/development)"),
    ] + _build_ext.user_options

    def initialize_options(self):
        # Initialize options.
        self.debug = 0
        self.with_doc = 0
        self.no_build = 0
        self.cmake_options = {}
        super().initialize_options()

    def finalize_options(self):
        # Finalize options.
        super().finalize_options()
        self.cmake_options["Python3_EXECUTABLE"] = os.path.realpath(sys.executable)
        if self.debug:
            self.cmake_options["CMAKE_BUILD_TYPE"] = "Debug"
        else:
            self.cmake_options["CMAKE_BUILD_TYPE"] = "Release"

    def run(self):
        if self.no_build:
            return
        for ext in self.extensions:
            self.build_cmake(ext)
        #  super().run()
        _build_ext.run(self)

    def build_cmake(self, ext):
        global numCores_
        global sdir_
        print("\n==========================================================\n")
        print("[INFO ] Building smoldyn Python extension in %s ..." % builddir_)
        cmake_args = []
        for k, v in self.cmake_options.items():
            cmake_args.append("-D%s=%s" % (k, v))
        os.chdir(str(builddir_))
        self.spawn(["cmake", str(sdir_)] + cmake_args)
        if not self.dry_run:
            self.spawn(["make", "-j%d" % numCores_])
        os.chdir(str(sdir_))


with open(sdir_ / "README.md") as f:
    readme = f.read()

setup(
    name="smoldyn",
    version=version_,
    description="Python module of the Smoldyn simulator (alpha)",
    long_description=readme,
    long_description_content_type="text/markdown",
    author="Dilawar Singh",  # author of this python package.
    author_email="dilawar.s.rajput@gmail.com",
    url="http://www.smoldyn.org/",
    license="Gnu LGPL",
    include_package_data=True,
    packages=["smoldyn"],
    # NOTE: Beware there paths are also used by cmake.
    package_dir={"smoldyn": builddir_ / "py" / "smoldyn"},
    package_data={"smoldyn": ["_smoldyn.so"]},
    install_requires=["numpy", "dataclasses;python_version=='3.6'"],
    python_requires=">=3.6",
    extra_requires={"dev": ["coverage", "pytest", "pytest-cov"]},
    cmdclass={"build_ext": build_ext, "test": TestCommand},
    classifiers=["Programming Language :: Python :: 3",],
)
