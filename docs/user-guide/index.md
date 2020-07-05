
!!! warn

    __This html is generated from
    `documentation/Smoldyn/SmoldynUsersManual.docx` using pandoc. The
    conversion may not be correct. Please refer to official documentation at
    <https://smoldyn.org>.__

Smoldyn is a computer program for simulating chemical processes on a
microscopic size scale. The size scale is sufficiently detailed that all
molecules of interest are simulated individually, while solvent
molecules and any molecules that are not of immediate interest are only
treated implicitly. In the simulation, molecules diffuse, react, are
confined by surfaces, and bind to membranes, much as they would in a
real chemical system.

In Smoldyn, each molecule is represented by a point in 1-, 2-, or
3-dimensional continuous space. Simulated molecules do not have spatial
orientations or momenta. They can have volumes if desired, but do not
need to. Because of these approximations, simulations are typically
accurate on spatial scales down to about a nanometer and timescales down
to about a microsecond. This accuracy comes at the cost of high
computational intensity. For systems that are larger than tens of
microns, or dynamics that unfold over tens of minutes, simulation
methods that are more computationally efficient but less accurate are
likely to be preferable.

The input to Smoldyn is a plain text configuration file. This file
specifies all of the details of the system, such as the shapes and
positions of membranes, the initial types and locations of molecules,
diffusion coefficients, instructions for the graphical output, and so
on. Smoldyn reads this file, prints out some information about the
system so the user can verify that the file was interpreted correctly,
and then runs the simulation. As the simulation runs, the state of the
system can be displayed to a graphics window to allow the user to watch
what is happening, or to capture the simulation result in a movie. Also,
it is possible to enter commands in the configuration file that are
executed at runtime, and which output quantitative results from the
simulation to text files. Smoldyn quits when the simulation is complete.

## About this User’s Manual

Do not read the manual from end to end. New users should read the
[Installation](#installing-smoldyn) chapter as needed and the [Getting
Started](#getting-started) chapter. The last half of the manual is a reference
section which lists all statements and commands. The first portions of the
other chapters provide helpful introductions on additional topics. Later
portions of those chapters present advanced material that you may want to learn
if you continue with Smoldyn.

