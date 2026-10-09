# Filament sterics validation, 2026-10-08

Implementation and usage: [FILAMENT_STERICS.md](FILAMENT_STERICS.md).

## Build and native checks

Windows, Release build, Rtools44 GCC, Ninja. Headless and OpenGL targets compile.
Native checks are registered as the CTest test `filament_steric`.

The final suite passed through CTest (1 test, 0 failures, about 0.1 s). It covers:

- Finite-segment closest points at nanometre scales; parallel, crossing and
  zero-length geometry.
- Spatial neighbors against exhaustive enumeration for 250 randomly positioned
  segments; no missing or duplicate pairs; cache invalidation by movement and
  growth.
- Contact force as an energy gradient; zero net contact force and torque;
  relaxation of a 1 nm overlap toward the 7 nm contact separation.
- Contact force transmission across a branch junction; exact common-node
  attachment over repeated mechanical steps; local junction exemption.
- Growth blocked by an obstacle along the new segment; branch-birth rollback;
  protection of an existing named daughter. Growth queries are checked against
  exhaustive enumeration for 400 random trials, with movement inside the skin,
  newly accepted uncached segments, removed/reused storage slots, and front-end
  growth and removal.
- Brownian center-of-mass diffusion with one and four mechanical substeps, each
  measured over 12,000 steps.
- Floor and ceiling confinement beginning one radius inside each plane.
- Rejection of excessive contact stiffness and unsupported integrators/periodic
  boundaries before invalid motion occurs.
- First-order timestep convergence to the analytical contact-relaxation solution
  in both 2D and 3D.

These checks validate the implemented numerical mechanics. They do not validate
actin packing density, effective drag, force-dependent growth, height saturation
or agreement with Bieling 2016.

## Model runs

The deterministic `capsule_contact.txt` example completed through 0.00501 s
(501 mechanical evaluations). Its last measured overlap was about 1.37e-12
micrometres, with one neighbor pair and one neighbor-list rebuild.

A one-second copy of `Free_growth_experiment_1.txt`, using radius 0.0035,
stiffness 4000, skin 0.002 and four mechanical substeps, completed through
1.00001 s with no parameter errors or warnings. Both the intermediate OpenGL
build in text mode and the final headless build completed it. It took about 7 s, ended with 34 segments and
two cached neighbor pairs, and performed 400,004 mechanical evaluations and
12,484 neighbor rebuilds. The final force evaluation had no active contacts;
this short sparse run is an integration check, not a dense-contact validation.

The complete 3.5 s compression baseline also passed with those parameters:
3,443 segments, 406 active contacts in the last force evaluation, maximum
penetration 0.00165076 micrometres (1.65 nm), 240 blocked branches and 325,449
blocked-growth attempts. That maximum overlap is about 24% of the nominal 7 nm
diameter: these penalty parameters are a starting point and do not meet a strict
hard-volume interpretation. The run used four substeps, 1,400,000 mechanical
evaluations and 72,869 neighbor rebuilds.

This full run took 1,540 s before the final growth-query optimization. It exposed
the high cost of repeated exhaustive growth scans, which have since been replaced
by box queries plus a pending-insertion list. That runtime is **not a timing of
the final implementation**. The final query path is checked independently against
the exhaustive oracle; the full 3.5 s trajectory has not been rerun with it.

The final optimized code completed a 2 s copy of the compression baseline in
110 s, ending with 470 segments, six contacts, maximum penetration 0.78 nm and
5,624 blocked-growth attempts. Every one of its 4,294 output lines matches the
corresponding prefix of the earlier full trajectory exactly. The cell and
growth-query optimizations therefore preserve this recorded trajectory through
2 s. This check does not establish the final full-run runtime.

Detailed local logs and coordinate outputs are in the sibling
`build-ninja-rtools` directory. The prepared 30-second model is the sibling file
`Free_growth_experiment_1_steric.txt`; the full 30-second free-growth experiment
has not been run as part of this implementation.

## Performance

The `--benchmark-thermal` measurement after cell optimization uses straight arrays of 10-segment filaments,
`kT 0.001`, bending constants 1, segment length 0.01, `force_length 10000`,
mobility 1, contact stiffness 1000, radius 0.0035, skin 0.002, timestep 1e-5,
one mechanical substep and 100 timesteps. Chemistry and output are absent.
Sparse arrays are spaced 20 nm apart; dense arrays begin 6 nm apart, with
initial overlaps. Each mode uses a separate simulation; identical thermal
trajectories are not asserted. Timings are single measurements on this machine,
with other work running, so ratios are approximate.

| Segments | Arrangement | Contacts disabled (s) | Contacts enabled (s) | Ratio | Enabled rebuilds |
| --- | --- | ---: | ---: | ---: | ---: |
| 1,000 | Sparse | 0.072 | 0.129 | 1.8x | 26 |
| 5,000 | Sparse | 0.393 | 0.714 | 1.8x | 33 |
| 15,000 | Sparse | 1.278 | 2.834 | 2.2x | 36 |
| 1,000 | Dense | 0.071 | 0.158 | 2.2x | 27 |
| 5,000 | Dense | 0.371 | 1.079 | 2.9x | 32 |
| 15,000 | Dense | 1.300 | 5.849 | 4.5x | 38 |

The 15,000-segment dense case ends with about 79,759 cached neighbor pairs.
Thermal rebuilds are a substantial part of the overhead. Compact cell entries,
a cell width of at least twice the longest segment, and one owning cell per
candidate pair reduced enabled runtime in this case from an earlier 15.4 s to
5.85 s. The earlier implementation repeated distance checks across shared cells.

The mechanical timing table precedes the last growth-query change, which adds a
cache-validity check at chemistry entry. Four substeps incur extra force
evaluations beyond the one-substep benchmark.
Do not interpret these numbers as negligible overhead or as a runtime prediction
for the 30-second free-growth model. Increasing the skin can reduce rebuilds but
also increases candidate pairs; this should be profiled for the actual network.
Growing/branching networks are not represented by this timing table.

The final growth benchmark deliberately repeats 10,000 blocked insertions:

| Existing segments | Box queries (s) | Exhaustive scans (s) |
| --- | ---: | ---: |
| 1,000 | 0.003 | 0.138 |
| 5,000 | 0.008 | 0.838 |
| 15,000 | 0.009 | 1.803 |

These queries run against an already prepared index. The measurements exclude
index creation and mechanical integration, and are too short for precise ratio
claims. They demonstrate that blocked-growth query cost no longer grows as a
full scan over all existing segments. Overall simulation speed still depends on
mechanical substeps, thermal rebuilds and contact density.

## Shared box-grid refactor, 2026-10-09

Molecule and filament grids now instantiate the same `BoxGrid` interface with
dense and sparse backends. Sparse entries remain four integers each; there are
no added allocations for empty cells or per-cell molecule lists. Cached neighbor
pairs and their ordering are retained.

The expanded native suite passed in about 0.14 s. Additional checks cover:

- Dense/sparse coordinate lookup, negative sparse coordinates, dense boundary
  clamping, cell payloads and dense-grid resize/free lifecycle in 2D and 3D.
- 300 random point and finite-line queries per dimension against an exhaustive
  distance oracle, including contact-only and global-nearest variants.
- Far-away nearest queries, very long diagonal probes, capsule endcaps,
  different radii, empty networks and invalid coordinates.
- Pending accepted growth visibility without query-induced insertions, removed
  segment liveness, and the physical-radius `filSegmentXFilament` API.
- `steric_growth_check` parsing, unchecked insertion registration and contact
  forces continuing to act when growth rejection is disabled.

Fixtures now seed their random generator explicitly. The old front-growth
location assertion also uses nonzero bending stiffness: zero stiffness samples
arbitrary birth angles even at zero temperature, making that location assertion
depend on an uncontrolled random angle.

The seeded one-second free-growth smoke output retained its exact SHA-256:
`3DAF53CAAFD96C50DD4F7CEA720C83C63BB20419E6575B5568E7BCB1EEBDB22A`.
This compares all dumped filament coordinates, topology, times and cap states.

Final prepared-index growth timings for 10,000 blocked queries:

| Segments | Shared boxes | Exhaustive scan |
| --- | --- | --- |
| 1,000 | 0.004 s | 0.088 s |
| 5,000 | 0.008 s | 0.821 s |
| 15,000 | 0.012 s | 1.776 s |

The immediately preceding implementation measured 0.004, 0.009 and 0.011 s
for the same query counts and network sizes. These millisecond measurements
exclude index construction and do not establish a precise speed ratio.

Thermal mechanics checks at 15,000 segments and 100 outer steps, before the final
one-pass bounds optimization, measured 2.917 s sparse and 5.440 s dense in the
repeat run, versus 2.728 s and 5.230 s immediately before this refactor. Another
post-refactor run measured 3.744 s and 7.148 s under varying system load; its
dense no-contact control also increased from 1.189 to 1.645 s. Dense contact
overhead relative to its own no-contact control remained about 4.35–4.46 times,
versus 4.40 before. These are synthetic timing checks, not a guarantee for every
branching network. The new fixed fixture seed also changes rebuild counts, so
small before/after differences cannot be attributed solely to the interface.

Global nearest queries intentionally make a bounds pass over all participating
segments; they are optional and are not inserted into the mechanics/growth hot
loop. Hit-only local queries reuse sparse cells, with cache validation when
called outside a chemistry snapshot.

## Remaining scientific checks

Before interpreting packing or force measurements, compare trajectories and
observables at successively smaller mechanical timesteps, test contact stiffness
and node-spacing sensitivity, and compare the original and volume-enabled
network over several seeds. Track overlap relative to the 7 nm diameter and
avoid treating a stable run as proof of biological accuracy. The contact guard
bounds only normal contact stiffness; the coupled elastic system still needs
convergence checks at the intended network density.
