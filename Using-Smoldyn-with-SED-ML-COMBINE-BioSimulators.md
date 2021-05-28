# Using Smoldyn with SED-ML and COMBINE archives
Smoldyn provides a BioSimulators-compliant interface for executing Smoldyn
simulations through SED-ML and COMBINE archives.

## COMBINE archive format for Smoldyn configuration files

OMEX manifests of COMBINE archives should use the format `http://purl.org/NET/mediatypes/text/smoldyn+plain` 
for Smoldyn configuration files as illustrated below.

```xml
<content location="model.txt" format="http://purl.org/NET/mediatypes/text/smoldyn+plain" master="false"/>
```

## SED-ML model language for Smoldyn configuration files

SED-ML files for Smoldyn simulations should use the language `urn:sedml:language:smoldyn` 
for Smoldyn configuration files.

```xml
<model id="model" source="model.txt" language="urn:sedml:language:smoldyn"/>
```

## SED-ML model changes
Changes to configurations for Smoldyn simulations can be encoded into SED-ML `changeAttribute` model changes
as illustrated below.

| Smoldyn syntax                                                               | `target` of SED-ML change                                           | Format of `newValue` of SED-ML change |
| ---------------------------------------------------------------------------- | ------------------------------------------------------------------- | ------------------------------------- |
| `dim {integer}`                                                              | `dim`                                                               | `{integer}`                           |
| `low_wall {dim} {float} {string}`                                            | `low_wall {dim}`                                                    | `{float} {string}`                    |
| `high_wall {dim} {float} {string}`                                           | `high_wall {dim}`                                                   | `{float} {string}`                    |
| `boundaries {dim} {float} {float} {string}?`                                 | `boundaries {dim}`                                                  | `{float} {float} {string}?`           |
| `define {name} {float}`                                                      | `define {name}`                                                     | `{float}`                             |
| `difc {species} {float}`                                                     | `difc {species}`                                                    | `{float}`                             |
| `difc {species}({state}) {float}`                                            | `difc {species}({state})`                                           | `{float}`                             |
| `difc_rule {species}({state}) {float}`                                       | `difc_rule {species}({state})`                                      | `{float}`                             |
| `difm {species} {float}+`                                                    | `difm {species}`                                                    | `{float}+`                            |
| `difm {species}({state}) {float}+`                                           | `difm {species}({state})`                                           | `{float}+`                            |
| `difm_rule {species}({state}) {float}+`                                      | `difm_rule {species}({state})`                                      | `{float}+`                            |
| `drift {species} {float}+ {float}+`                                          | `drift {species}`                                                   | `{float}+`                            |
| `drift {species}({state}) {float}+`                                          | `drift {species}({state})`                                          | `{float}+`                            |
| `drift_rule {species}({state})`                                              | `drift_rule {species}({state})`                                     | `{float}+`                            |
| `surface_drift {species}({state}) {surface} {panel-shape} {float}+`          | `surface_drift {species}({state}) {surface} {panel-shape}`          | `{float}+`                            |
| `surface_drift_rule {species}({state}) {surface} {panel-shape} {float}+`     | `surface_drift_rule {species}({state}) {surface} {panel-shape}`     | `{float}+`                            |
| `mol {integer} {species} [{pos} ...]`                                        | `mol {species} [{pos} ...]`                                         | `{integer}`                           |
| `compartment_mol {integer} {species} {compartment-name}`                     | `compartment_mol {species} {compartment-name}`                      | `{integer}`                           |
| `surface_mol {integer} {species}({state}) {panel-shape} {panel} [{pos} ...]` | `surface_mol {species}({state}) {panel-shape} {panel} [{pos} ...]`  | `{integer}`                           |

```xml
<changeAttribute target="dim" newValue="2"/>
<changeAttribute target="define k_cat" newValue="1.2"/>
<changeAttribute target="difc red" newValue="4"/>
<changeAttribute target="mol red" newValue="10"/>
<changeAttribute target="compartment_mol red cytosol" newValue="10"/>
```

## SED-ML simulation algorithms
SED-ML files for Smoldyn support the following algorithms and algorithm parameters:

* Brownian diffusion Smoluchowski method ([KISAO_0000057](https://www.ebi.ac.uk/ols/ontologies/kisao/terms?iri=http%3A%2F%2Fwww.biomodels.net%2Fkisao%2FKISAO%23KISAO_0000057))
  * Accuracy ([KISAO_0000254](https://www.ebi.ac.uk/ols/ontologies/kisao/terms?iri=http%3A%2F%2Fwww.biomodels.net%2Fkisao%2FKISAO%23KISAO_0000254), float, default: 10.0)
  * Random seed ([KISAO_0000488](https://www.ebi.ac.uk/ols/ontologies/kisao/terms?iri=http%3A%2F%2Fwww.biomodels.net%2Fkisao%2FKISAO%23KISAO_0000488), integer, default: null)

```xml
<algorithm kisaoID="KISAO:0000057">
    <listOfAlgorithmParameters>
        <algorithmParameter kisaoID="KISAO_0000254" value="5.0"/>
    </listOfAlgorithmParameters>
</algorithm>
```

## Variables of SED-ML data generators

Variables for SED-ML data generators should use symbols and targets as 
illustrated below.

* Time - symbol: `urn:sedml:symbol:time`
* Species counts and distributions - target

| Smoldyn output file                                                                                                                           | SED variable target                                                                                                                         | Shapes                              |
| --------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------- |
| ``molcount``                                                                                                                                  | ``molcount {species}``                                                                                                                      | (numberOfSteps + 1,)                |
| ``molcountspecies {species}({state})``                                                                                                        | ``molcountspecies {species}({state})``                                                                                                      | (numberOfSteps + 1,)                |
| ``molcountspecieslist {species}({state})+``                                                                                                   | ``molcountspecies {species}({state})``                                                                                                      | (numberOfSteps + 1,)                |
| ``molcountinbox {low-x} {hi-x}``                                                                                                              | ``molcountinbox {species} {low-x} {hi-x}``                                                                                                  | (numberOfSteps + 1,)                |
| ``molcountinbox {low-x} {hi-x} {low-y} {hi-y}``                                                                                               | ``molcountinbox {species} {low-x} {hi-x} {low-y} {hi-y}``                                                                                   | (numberOfSteps + 1,)                |
| ``molcountinbox {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z}``                                                                                | ``molcountinbox {species} {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z}``                                                                    | (numberOfSteps + 1,)                |
| ``molcountincmpt {compartment}``                                                                                                              | ``molcountincmpt {species} {compartment}``                                                                                                  | (numberOfSteps + 1,)                |
| ``molcountincmpts {compartment}+``                                                                                                            | ``molcountincmpt {species} {compartment}``                                                                                                  | (numberOfSteps + 1,)                |
| ``molcountincmpt2 {compartment} {state}``                                                                                                     | ``molcountincmpt2 {species} {compartment} {state}``                                                                                         | (numberOfSteps + 1,)                |
| ``molcountonsurf {surface}``                                                                                                                  | ``molcountonsurf {species} {surface}``                                                                                                      | (numberOfSteps + 1,)                |
| ``molcountspace {species}({state}) {axis} {low} {hi} {bins} 0``                                                                               | ``molcountspace {species}({state}) {axis} {low} {hi} {bins}``                                                                               | (numberOfSteps + 1, bins)           |
| ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} 0``                                                                    | ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi}``                                                                    | (numberOfSteps + 1, bins)           |
| ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} {low} {hi} 0``                                                         | ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} {low} {hi}``                                                         | (numberOfSteps + 1, bins)           |
| ``molcountspace2d {species}({state}) z {low-x} {hi-x} {bins-x} {low-y} {hi-y} {bins-y} 0``                                                    | ``molcountspace2d {species}({state}) z {low-x} {hi-x} {bins-x} {low-y} {hi-y} {bins-y}``                                                    | (numberOfSteps + 1, bins-x, bins-y) |
| ``molcountspace2d {species}({state}) {axis} {low-1} {hi-1} {bins-1} {low-2} {hi-2} {bins-2} {low-3} {hi-3} 0``                                | ``molcountspace2d {species}({state}) {axis} {low-1} {hi-1} {bins-1} {low-2} {hi-2} {bins-2} {low-3} {hi-3}``                                | (numberOfSteps + 1, bins-1, bins-2) |
| ``molcountspaceradial {species}({state}) {center-x} {radius} {bins} 0``                                                                       | ``molcountspaceradial {species}({state}) {center-x} {radius} {bins}``                                                                       | (numberOfSteps + 1, bins)           |
| ``molcountspaceradial {species}({state}) {center-x} {center-y} {radius} {bins} 0``                                                            | ``molcountspaceradial {species}({state}) {center-x} {center-y} {radius} {bins}``                                                            | (numberOfSteps + 1, bins)           |
| ``molcountspaceradial {species}({state}) {center-x} {center-y} {center-z} {radius} {bins} 0``                                                 | ``molcountspaceradial {species}({state}) {center-x} {center-y} {center-z} {radius} {bins}``                                                 | (numberOfSteps + 1, bins)           |
| ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {pole-x} {pole-y} {radius-min} {radius-max} {bins} 0``                     | ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {pole-x} {pole-y} {radius-min} {radius-max} {bins}``                     | (numberOfSteps + 1, bins)           |
| ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {center-z} {pole-x} {pole-y} {pole-z} {radius-min} {radius-max} {bins} 0`` | ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {center-z} {pole-x} {pole-y} {pole-z} {radius-min} {radius-max} {bins}`` | (numberOfSteps + 1, bins)           |
| ``radialdistribution {species-1}({state-1}) {species-2}({state-2}) {radius} {bins} 0``                                                        | ``radialdistribution {species-1}({state-1}) {species-2}({state-2}) {radius} {bins}``                                                        | (numberOfSteps + 1, bins)           |
| ``radialdistribution2 {species-1}({state-1}) {species-2}({state-2}) {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z} {radius} {bins} 0``          | ``radialdistribution2 {species-1}({state-1}) {species-2}({state-2}) {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z} {radius} {bins}``          | (numberOfSteps + 1, bins)           |


```xml
<variable id="time" symbol="urn:sedml:symbol:time">
<variable id="red" target="molcount red">
```

## Simulation results

The results of Smoldyn simulations executed through SED-ML and COMBINE archives
are saved in HDF5 format as described in the 
[BioSimulators documentation](https://biosimulators.org/conventions/simulation-reports).
These files can be read with a variety of tools such as 
[HDFView](https://www.hdfgroup.org/downloads/hdfview/),
[h5py](https://www.h5py.org/) for Python, and 
[h5read](https://www.mathworks.com/help/matlab/ref/h5read.html) for MATLAB.

## Executing Smoldyn through SED-ML and COMBINE archives

COMBINE archives and SED-ML files for Smoldyn simulations can be executed
in multiple ways.

### Web application and REST API

[runBioSimulations](https://run.biosimulations.org/run) is a web application
that can execute COMBINE archives and SED-ML files for Smoldyn simulations.
runBioSimulations also provides a REST API for executing simulations.

### Command-line program

Simulations can be run with the `biosimulations-smoldyn` command-line progam 
provided with the Smoldyn Python package as illustrated below. Note, this
requires Smoldyn to be installed with the `biosimulators` option.
```sh
pip install smoldyn[biosimulators]
python -m biosimulators.biosimulators -i /path/to/simulation.omex -o /path/to/save/outputs
```

### Python API

Simulations can be run with the `exec_sedml_docs_in_combine_archive` method 
provided with the Smoldyn Python package as illustrated below. Note, this
requires Smoldyn to be installed with the `biosimulators` option.

```sh
pip install smoldyn[biosimulators]
```

```python
from smoldyn.biosimulators.combine import exec_sedml_docs_in_combine_archive
archive_filename = '/path/to/simulation.omex'
out_dir = '/path/to/save/outputs'
exec_sedml_docs_in_combine_archive(archive_filename, out_dir)
```

### Docker image

Simulations can be run with the Biosimulators-Smoldyn Docker image as illustrated
below:
```sh
docker pull ghcr.io/biosimulators/smoldyn
docker run \
  --tty \
  --rm \
  --mount type=bind,source=/path/to/simulation.omex,target=/tmp/project,readonly \
  --mount type=bind,source=/path/to/save/outputs,target=/tmp/results \
  ghcr.io/biosimulators/smoldyn \
    --archive /tmp/project/simulation.omex \
    --out-dir /tmp/results
```