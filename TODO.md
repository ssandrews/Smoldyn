## To do list for Python API

- [x] Creating new simulation (e.g. s=Simulation([0,0],[10,10]))

The quit_at_end and log_level inputs don't appear to work. Fixed by 3fa3da017 .

- [ ] UpdateSim

This is a C/C++ API function but not in Python. It would be very useful to
expose this to Python, too.

- [ ] runTimeStep

This doesn't appear to work (see examples/S15_Python/bounce2.py). It does run,
but it isn't showing graphics or outputting any results, so I'm not sure what's
wrong. I don't think that I've checked the C/C++ library for this function, so
the problem might be in there.

- [ ] run

The log_level input doesn't appear to work.

- [ ] fromSimptr

At present, it's possible to load a model using prepareSimFromFile, but doing
so returns a simstruct object. There needs to be a way to convert this to a
Simulation object.


- [ ]  loadSimFromFile

The version of this used as smoldyn.Simulation.fromFile() works well. However,
there's also the low-level API function loadSimFromFile, which I can't seem to
get to work. As part of this, I'm not sure what variable the simulation is
supposed to go into because the function returns an error code rather than a
simstruct object.


- [ ] getSpeciesName

This function requires a string as its last parameter, but shouldn't because
it's not used.


- [ ] addBox, Box, moleculePerBox

I'd like to remove and/or rename these functions, changing to just
setPartitions. The reason is that boxes aren't physical objects but are really
just virtual partitions within the simulation volume that are only there to
accelerate the code. Thus, this is a simulation setting, not something that's
added to the model.
