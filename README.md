[![Test status](https://github.com/ssandrews/Smoldyn/actions/workflows/linux.yml/badge.svg)](https://github.com/ssandrews/Smoldyn/actions/workflows/linux.yml)
[![Linux build status](https://github.com/ssandrews/Smoldyn/actions/workflows/wheels_linux.yml/badge.svg)](https://github.com/ssandrews/Smoldyn/actions/workflows/wheels_linux.yml)
[![OS X build status](https://github.com/ssandrews/Smoldyn/actions/workflows/osx.yml/badge.svg)](https://github.com/ssandrews/Smoldyn/actions/workflows/osx.yml)
[![Windows build status](https://github.com/ssandrews/Smoldyn/actions/workflows/windows.yml/badge.svg)](https://github.com/ssandrews/Smoldyn/actions/workflows/windows.yml)
[![PyPI version](https://badge.fury.io/py/smoldyn.svg)](https://badge.fury.io/py/smoldyn)
[![BioSimulators](https://img.shields.io/badge/BioSimulators-registered-brightgreen)](https://biosimulators.org/simulators/smoldyn)
[![runBioSimulations](https://img.shields.io/badge/runBioSimulations-simulate-brightgreen)](https://run.biosimulations.org/run)
[![Documentation status](https://readthedocs.org/projects/smoldyn/badge/?version=latest)](https://smoldyn.readthedocs.io/en/latest/?badge=latest)

# Smoldyn

Smoldyn is a particle-based biochemical simulator for modeling molecular
diffusion, surface interactions, and chemical reactions.

More information about Smoldyn is available at http://www.smoldyn.org. 
Please visit this website to learn about Smoldyn, download the latest
release, etc.

## Installing Smoldyn
The Smoldyn command-line program and Python package can be installed by
executing the following command.
```sh
pip install smoldyn
```

Compiled binaries for Mac OS and Windows are also available at
http://www.smoldyn.org/download.html.

## Executing Smoldyn simulations online
Smoldyn simulations can be executed online through
[runBioSimulations](https://run.biosimulations.org/run). This requires
Smoldyn simulations to be driven by a [SED-ML file](https://sed-ml.org) 
and packaged into a [COMBINE archive](http://combinearchive.org/).
runBioSimulations provides tools for created SED-ML files and COMBINE
archives.

## Using Smoldyn with SED-ML and COMBINE archives
Smoldyn provides a BioSimulators-compliant interface for executing Smoldyn
simulations through SED-ML and COMBINE archives. More information about
using Smoldyn with SED-ML and COMBINE archives is available
[here](Using-Smoldyn-with-SED-ML-COMBINE-BioSimulators.md).

## Documentation about simulation algorithms
Information about the simulation algorithms employed by Smoldyn
is available at [BioSimulators](https://biosimulators.org/simulators/smoldyn).

## Smoldyn source code
This is the official source code repository for Smoldyn. (Formerly this
repository was called `Smoldyn-official`.) This repository is run by 
Smoldyn's main author, Steve Andrews. The purpose of this repository is
enable developers to collaborate on the code for Smoldyn.

## Contributing to Smoldyn
New contributions to the code are welcome. To maximize the impact of
contributions to Smoldyn, please contact the main author to discuss
potential additions. Steve can be reached at steven.s.andrews@gmail.com.
