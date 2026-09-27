// Read-only accessors for `simstruct` fields that libsmoldyn.h has no getter for.
//
// Rust sees `simstruct` as an opaque type, so reading fields goes through these
// inline functions; that keeps the struct layout in C++ where it is defined.
#pragma once

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "Smoldyn/smoldyn.h"

inline int smolrs_dim(const simstruct &sim) { return sim.dim; }
inline double smolrs_time(const simstruct &sim) { return sim.time; }
inline double smolrs_time_start(const simstruct &sim) { return sim.tmin; }
inline double smolrs_time_stop(const simstruct &sim) { return sim.tmax; }
inline double smolrs_time_step(const simstruct &sim) { return sim.dt; }

// Position of the low (highside=0) or high (highside=1) wall along dimension d.
// Returns NaN if the walls are not allocated or d/highside are out of range.
inline double smolrs_wall_pos(const simstruct &sim, int d, int highside) {
    if (!sim.wlist || d < 0 || d >= sim.dim || highside < 0 || highside > 1)
        return NAN;
    return sim.wlist[2 * d + highside]->pos;
}

// Length of the configuration file path ("" for simulations not loaded from a file).
inline size_t smolrs_filepath_len(const simstruct &sim) {
    return sim.filepath ? strlen(sim.filepath) : 0;
}

// Number of species slots, including the "empty" species at index 0.
inline int smolrs_nspecies(const simstruct &sim) {
    return sim.mols ? sim.mols->nspecies : 0;
}

// Name of species i, or nullptr if out of range. Owned by the simulation.
inline const char *smolrs_species_name(const simstruct &sim, int i) {
    if (!sim.mols || i < 0 || i >= sim.mols->nspecies)
        return nullptr;
    return sim.mols->spname[i];
}

// Free an array that libsmoldyn allocated with calloc (smolGetOutputData).
inline void smolrs_free_doubles(double *array) { free(array); }
