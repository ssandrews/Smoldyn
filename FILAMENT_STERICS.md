# Filament excluded volume

This implementation gives straight filament segments finite capsule volume
(a cylinder with hemispherical ends). It adds overdamped contact forces to the
existing filament mechanics. It is an initial mechanical model, not a calibrated
description of actin packing or a hard nonpenetration solver.

## Enable it

Add these lines inside `start_filament_type ... end_filament_type` for each
filament type that should participate in contacts. The values below assume
lengths in micrometres and the existing model energy convention:

```text
dynamics euler
steric_radius 0.0035
steric_stiffness 4000
steric_skin 0.002
steric_substeps 4
```

| Parameter | Meaning | Default |
| --- | --- | --- |
| `steric_radius` | Physical capsule radius, length units; 0 disables contacts for this type | 0 |
| `steric_stiffness` | Spring penalty for a segment pair, energy/length squared | 0 |
| `steric_skin` | Extra distance retained in the cached neighbor list, length units | 0 |
| `steric_substeps` | Number of mechanical steps within each chemistry timestep | 1 |
| `steric_growth_check` | Reject overlapping elongation/treadmilling; 0 permits insertion while contact forces remain active | 1 |

The example radius gives a 7 nm diameter. The stiffness and skin are numerical
choices. A nonzero radius requires positive stiffness and skin. Display
`thickness` and the existing internal `filradius` field do not enable collision
volume. Types with radius 0 do not repel or obstruct types with nonzero radius.

The largest `steric_substeps` value across filament types is used for the whole
coupled network. Four is a cautious starting value for the current 10 nm node
spacing and `time_step 0.00001`, not a proof of convergence. Chemistry, capping,
branch attempts and output still use the original timestep. Mechanical forces
and independent thermal increments are recomputed at each smaller step.
Mobility is not changed by this feature.

From the project folder in PowerShell:

```powershell
.\build-ninja-rtools\smoldyn.exe .\Free_growth_experiment_1_steric.txt -t
```

This separately prepared model writes `free_growth_steric_frames.txt`. Its
original 30 s duration can be expensive as the network grows. Start with a short
duration when choosing parameters. The original free-growth input is unchanged.
The coordinate dump format is unchanged, so existing analysis and Simularium
conversion scripts can read it.

A small deterministic example is in
`examples/S13_filaments/sterics/capsule_contact.txt`. It starts two perpendicular
capsules 6 nm apart and lets them relax toward their 7 nm contact separation.

## Mechanics and neighbor search

For closest centerline distance `d`, combined radius `R = ra + rb`, and overlap
`delta = max(0, R - d)`, the contact potential is `U = k * delta^2 / 2` and the
repulsive force magnitude is `k * delta`. Different types use the harmonic mean
of their two stiffnesses. Closest-point interpolation weights distribute equal
and opposite forces to the segment endpoints. There are no inertial impulses,
friction, adhesion or new crosslinks.

Every segment enters each occupied spatial cell touched by its expanded bounding
box. Only pairs sharing a cell become candidates; exact segment distance filters
them. The lowest shared cell owns each pair, avoiding repeated distance checks
across shared cells; final pair sorting ensures stable order. Compact cell
entries use validated integer coordinates. Cell size follows the largest current
segment length and contact extent. The index is sparse, so nanometre resolution
does not allocate a dense grid over the whole chamber.

This follows the ownership approach Steven described: cells hold segment
references; segments do not own lists of cells. The molecule and filament grids
are **parallel instances of a shared `BoxGrid` interface in `smolboxes.c`**.
`boxsuperstruct.grid` uses the original dense molecule boxes;
`filamentstericstruct.grid` specializes that interface with sparse sorted
occupied-cell entries. Their resolutions remain independent. The molecule grid
may have only one cell in a filament-only run and is often too coarse for contacts.

The shared implementation provides position-to-cell conversion, expanded segment
search bounds, cell lookup, sorting, sparse entry allocation and freeing. A
`BoxGridCell` view returns the original dense `boxptr` or a contiguous range of
sparse object IDs, without allocating a cell object. IDs refer to the existing
filament segment array. Sparse entry layout and pair ordering are preserved;
there are no new per-cell molecule lists, empty cells, or persistent mappings
between the two grids. A molecule's coordinates can directly query the filament
grid. Existing `pos2box(sim, ...)` and `box2pos(sim, ...)` remain wrappers around
the explicit-grid dense utilities, preserving existing callers and clamping.

The neighbor list is reused until an endpoint has moved by half the skin, a
segment is added/removed/reordered, or contact parameters change. Inflated bounds
and the skin prevent missed pairs between rebuilds. Typical rebuild cost is
sorting occupied entries plus local pair checks. Each force evaluation scans
segments and cached pairs; dense packing or unusually long segments can still
make the local pair count large.

## Junctions, growth and walls

- Same-segment and adjacent-segment contacts within a filament are excluded.
  Nonadjacent self-contact is included.
- A directly bonded mother/daughter pair has a bounded junction exemption:
  the daughter's first `2 * (ra + rb)` of contour is trimmed for contacts only
  against nearby mother segments. Distant mother/daughter segments still collide.
  This is an approximate junction model, not a resolved Arp2/3 structure.
- Branch node 0 and its mother's attachment node share one displacement. Member
  forces are summed and divided by summed drag; a fixed member fixes the junction.
  Contact loads can therefore reach the mother. The old post-step translation of
  the entire daughter is bypassed while sterics is enabled. Connected nodes must
  start coincident and have the same temperature.
- Elongation and treadmilling reject a newly inserted segment whose complete
  capsule intersects another participating segment. Branch births are similarly
  rejected and rolled back. Chemistry starts with a valid box snapshot; growth
  queries only nearby occupied cells. Newly accepted segments are also checked
  through a small pending list until the next rebuild. Removed/reused slots are
  checked for current membership, so they cannot become ghost obstacles. This
  avoids a full-network scan on every retry by a blocked tip.
  Rejected growth follows the existing blocked-growth bank behavior; it does not
  implement a calibrated force-dependent polymerization law.
  `steric_growth_check 0` disables this rejection for elongation and treadmilling
  only. Accepted unchecked segments still enter the pending list immediately so
  other queries see them. It does not disable forces or branch-birth checks.
- Rectangular `confine_surface` panels repel at a centerline distance of one
  radius from the wall, rather than waiting for the centerline to cross it.
  They act as supporting planes for confinement. The existing confinement spring
  remains a soft penalty. Simulation domain `boundaries` do not automatically
  become filament walls; use explicit confinement surfaces.

## Stability and limitations

The integrator remains explicit overdamped Euler. Contact stiffness, mobility,
node mobility factors, accumulated contacts, stretching, bending and junction
springs all matter. Before moving nodes, a conservative row-sum estimate of the
**normal contact stiffness** is multiplied by mobility and the mechanical
timestep. A value above 0.5 stops the run with a request to reduce `time_step` or
increase `steric_substeps`. This guard does not bound every elastic or geometrical
mode and does not prove full-system stability. Substep failures stop rather than
retrying a Brownian increment.

Small overlaps remain possible. For an isolated contact coordinate, thermal
overlap scales roughly as `sqrt(kT / k)`; the example values give about 0.5 nm.
Increase stiffness only together with a timestep/convergence check. The penalty
is per segment pair, so changing node spacing also changes the effective contact
response per unit length and requires retuning and validation.
The current same-filament exemption covers adjacent segments; node spacing
shorter than the diameter needs a broader local contour exemption to prevent
the overlapping endcaps of nearby segments from repelling their own filament.
The provided 10 nm spacing is larger than the 7 nm diameter.

There is no continuous collision detection: very large mechanical displacements
can pass through a thin obstacle between force evaluations. Growth insertion is
checked along its complete new segment, but this does not solve tunneling during
motion. Arbitrary initial overlaps are relaxed, not rejected automatically.

Supported mechanics: 2D and 3D, `dynamics euler` or `none` on every filament type.
Periodic domains, rigid `branch_azimuth_fix`, other filament integrators, and
nonrectangular confinement panels are rejected with explicit errors. The existing
one-way `branch_force_azimuth` approximation remains. Molecule–filament exclusion,
NPF occupancy, monomer depletion and chemical rate calibration are not added.

At termination, the log reports neighbor count, active contacts, maximum overlap,
penalty energy, rebuilds and rejected births/growth. Contact values refer to the
last force evaluation, just before the last mechanical move; they are not maxima
over the run.

## Grid and filament query APIs

The declarations are in `smoldynfuncs.h`:

```c
BoxGridCell cell;
boxGridFindPoint(&sim->boxs->grid, point, &cell);       /* molecule grid */
boxGridFindPoint(&sim->filss->steric->grid, point, &cell); /* prepared filament grid */

segmentptr nearest = NULL;
double clearance;
segmentptr hit = filPointInFilament(sim, point, &clearance, &nearest);
hit = filLineXFilament(sim, endpoint_a, endpoint_b, probe_radius,
                      &clearance, &nearest);
```

Queries inspect types with positive `steric_radius`. Points are zero-radius
probes; lines are **finite** endpoint-to-endpoint probes with optional radius.
`filSegmentXFilament` now uses the accelerated physical capsule query while
sterics is enabled, including adjacent-segment and local-junction exemptions;
its old `thk`-based behavior is retained when sterics is disabled.

`hit` is NULL if there is no contact. Optional `clearance` is the minimum signed
surface separation to an individual capsule: negative for overlap, zero for
touching, positive for separation. Optional `nearest` identifies that capsule
even without contact; any tied nearest capsule is valid. With no participating
segments the results are NULL and `DBL_MAX`. Invalid inputs/preparation errors
return NULL and `NAN` when a distance output was requested.

With both optional outputs NULL, hit detection uses nearby cells and stops at
the first hit. Requesting distance or nearest performs a global bounds pass
with exact-distance pruning, so a distant closest segment is not missed. Very
long diagonal probes also use this pass instead of enumerating a huge cell
rectangle. These optional queries are not run in the mechanics hot loop and
do not add trial segments to the pending list. Cache validation outside an
existing chemistry snapshot scans current segments before querying; inside the
snapshot, queries reuse its entries and include accepted pending growth.

## Changed code

| File | Change |
| --- | --- |
| `source/Smoldyn/smolfilamentsteric.c` (new) | Capsule contacts, neighbor-cache validity, trial growth checks, shared-node constraints, mechanical substeps, stability guard and diagnostics |
| `source/Smoldyn/smolfilamentsteric.h` (new) | Internal workspace, segment/pair/node and sparse cell structures |
| `source/Smoldyn/smolboxes.c` | Shared dense/sparse `BoxGrid` interface, explicit-grid compatibility utilities, sparse construction and unique candidate pairs |
| `source/libSteve/Geometry.c`, `Geometry.h` | Closest points for finite segments, including parallel, crossing and degenerate cases with micrometre/nanometre scale tolerances |
| `source/Smoldyn/smolfilament.c` | Input parsing/defaults/validation, physical query wrappers, independent growth check, integration dispatch, radius-aware confinement, rollback, initialization and freeing |
| `source/Smoldyn/smoldyn.h`, `smoldynfuncs.h` | Shared grid/cell types, query declarations, parameters, cache ownership and report declaration |
| `source/Smoldyn/smolsim.cpp` | End-of-run steric report |
| `CMakeLists.txt` | Compile the new C source and offer native regression tests |
| `tests/test_filament_steric.c` (new) | Mechanics, spatial-index, diffusion, wall, convergence and rejection tests; synthetic timing modes |

The workspace fixes initialize `fil->filwork` to NULL, free it when a filament is
destroyed, and free its individual thermal-force arrays when resized/destroyed.
The uninitialized pointer caused a native fixture crash before the first contact
test. No contact forces or coupled integration are enabled when all radii are 0.

## Build and verification

```powershell
cmake -S .\Smoldyn -B .\build-ninja-rtools -DOPTION_FILAMENT_STERIC_TESTS=ON
cmake --build .\build-ninja-rtools --target smoldyn test_filament_steric
ctest --test-dir .\build-ninja-rtools -R filament_steric --output-on-failure
```

Use the configured CMake/compiler paths if they are not on PATH. Both the
headless and OpenGL executable builds have been rebuilt. The native tests verify
closest-point cases, sparse boxes against exhaustive pair enumeration, unique
pairs, movement/topology invalidation, the contact energy gradient, force/torque
balance, overlap relaxation, mother load transmission and exact attachment,
blocked growth, Brownian diffusion with one/four substeps, radius-aware floor and
ceiling forces, unsafe-step rejection and 2D/3D convergence to an analytic rod
relaxation solution.

The native test's deliberate invalid-input cases print ERROR lines before passing.
Mechanical benchmark modes are `--benchmark` and `--benchmark-thermal`; they use 1,000,
5,000 and 15,000 segments, sparse/dense parallel rod arrays, one substep, no
chemistry or output, and a contact penalty of 1000. These synthetic tests do not
establish performance or biological correctness for a branching actin network.
`--benchmark-growth` compares 10,000 blocked-growth queries using boxes versus
exhaustive scans at the same three segment counts.

Validation results and timing measurements are recorded in
`FILAMENT_STERICS_VALIDATION.md`.
