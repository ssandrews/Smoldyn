# Cached movement and optional filament workers

## Run choices

Add **one global line**, outside `start_filament_type`, to the model:

```text
# Mechanical CPU workers. Chemistry and Brownian draws retain their serial order.
filament_workers 4
```

Choices:

| Value | Meaning |
|---|---|
| `1` | Serial mechanics; default when the keyword is absent |
| `2`, `4`, `8`, etc. | Requested maximum worker count; accepted range 1–256 |
| `0` | Automatic selection, bounded by the OpenMP maximum and about 1024 nodes per worker |

Networks with fewer than 1024 nodes or eight filaments use one worker to avoid
parallel overhead. Explicit counts are also bounded by the number of filaments.
OpenMP runtime/environment limits may restrict the thread team further. The
end-of-run report records the selected worker budget and topology rebuild count.

This setting accelerates the existing **Euler mechanics with filament sterics
enabled**. Chemistry, contacts and the other filament integrators retain their
existing serial paths. The shared molecule/panel/filament box system is unchanged.
Worker count does not change timestep, substeps, rates, mobility or forces.

`brownian_motion 0` inside a filament-type block disables its mechanical thermal
forces; `1` restores them (the default). It leaves `kT` and its use for random
seed/growth geometry and derived branch-angle spread intact. Elastic, wall and
contact forces still move nodes. Gaussian draws are retained to preserve the
existing RNG consumption order, so this switch is not a noise-generation speed
optimization. Runtime changes invalidate the cached thermal forces.

From the project folder, use the updated headless executable:

```powershell
.\build-ninja-rtools\smoldyn.exe .\Free_growth_experiment_1_steric.txt -t
```

The OpenGL executable in `build-ninja-rtools-gl` was rebuilt as well. Experimental
model files are left at their existing settings; add the keyword when choosing
a parallel run.
The earlier `build-ninja-rtools/smoldyn_unified.exe` alias was updated too.

## Changes and organization

The implementation follows the existing division between simulator input,
filament parameters/helpers, coupled mechanics and owned workspaces:

| File | Responsibility |
|---|---|
| `source/Smoldyn/smoldyn.h` | `workers` setting belongs to the filament superstructure, initialized to one |
| `source/Smoldyn/smoldynfuncs.h` | Public `filSetWorkers` and support-query declarations |
| `source/Smoldyn/smolsim.cpp` | Global `filament_workers` parsing and configuration serialization |
| `source/Smoldyn/smolfilament.c` | Setting/default/report; thermal preparation and material-frame helpers reused by the coupled path |
| `source/Smoldyn/smolfilamentsteric.h` | Borrowed filament/branch topology snapshots in the existing owned contact workspace |
| `source/Smoldyn/smolfilamentsteric.c` | Cache validation/rebuild, coupled motion, worker selection and two parallel filament loops |
| `source/Smoldyn/smolprofile.*` | Outer wall-time scopes remain usable; overlapping fine worker scopes are suppressed |
| `CMakeLists.txt` | Optional OpenMP dependency, propagated to static/shared libraries |
| `tests/test_filament_steric.c` | Cache lifecycle, serial fallback and exact deterministic worker comparisons |

### Movement cache

Previously each mechanical substep reconstructed the node identities, branch
unions and accumulated drag, then repeatedly searched those unions for roots.
Now:

1. At the beginning of an outer step, compare the live filament/branch lists with
   cached identities. This is a small filament/branch scan, not a node scan at
   every mechanical substep.
2. Rebuild node identities and flatten shared-point roots only if topology
   changes. Checks include filament order/pointers, segment and branch counts,
   attachment pointers, branch spots and daughter pointers. A same-count branch
   edit therefore invalidates the cache too.
3. Refresh mobility, fixed-state, temperature compatibility and group drag once
   per outer step; these numerical properties are not assumed immutable.
4. Each mechanical substep clears root forces, sums member forces in the
   original order, checks attachment, and computes the common displacement.

The old `filStepDynamics` call also computed independent Cartesian displacements
that the coupled movement code immediately overwrote. `filStepMaterialFrame`
now performs just its material/roll part, using the old segment axis. Ordinary
filament integrators reuse the same helper and keep their Cartesian step.
The existing roll convention and force summation order are retained.

### Worker phases

Each mechanical substep follows this sequence:

```text
serial Brownian noise preparation in original filament/node order
parallel ordinary forces, one filament owned by one worker
serial contact preparation and contact-force accumulation
serial shared-node force accumulation and common positions
parallel material update → apply common positions → reconstruct geometry
```

Junction helpers read neighboring positions but write only their own filament's
force array. Worker force computation therefore uses a frozen geometric state.
Parallel regions end before contacts or the next mechanical step begin.

Filaments are assigned cyclically (`schedule(static,1)`) to spread older, longer
filaments among workers. There are two parallel regions per mechanical substep;
OpenMP reuses its worker pool. The serial path does not enter those regions.
This is not a separate persistent team around the entire outer step.

Brownian generation remains serial because the original Gaussian generator has
shared state. Contact additions and shared-node sums remain serial to avoid
competing writes and preserve floating-point accumulation order. Full-precision
checks verify that worker choices preserve coordinates, material frames,
growth/capping state and subsequent random draws.

## Build support

`OPTION_FILAMENT_WORKERS=ON` is the CMake default. If OpenMP is available, it is
linked; otherwise the build remains serial. In a serial-only build, `0` selects
one worker and an explicit count above one is rejected with a clear error.

To disable OpenMP at build time:

```powershell
cmake -S .\Smoldyn -B .\build-ninja-rtools -DOPTION_FILAMENT_WORKERS=OFF
cmake --build .\build-ninja-rtools --target smoldyn
```

Re-enable with `-DOPTION_FILAMENT_WORKERS=ON`. Runtime defaults remain serial
regardless of the build option. The local headless and graphics builds currently
have OpenMP support enabled.

## Validation and measurements

Dense geometry replay near 29.9 s (505 filaments, 6214 segments), 100 measured
outer steps after 10 warmup steps, timers disabled:

| Choice | Selected worker budget | Loop wall time | Speedup versus current serial |
|---|---:|---:|---:|
| Serial | 1 | 26.55 s | 1.00× |
| 2 workers | 2 | 20.69 s | 1.28× |
| 4 workers | 4 | 17.30 s | 1.53× |
| 8 workers | 8 | 14.57 s | 1.82× |
| Automatic | 6 | 15.18 s | 1.75× |

The earlier executable took 25.82 s in this sweep and 31.29 s in an earlier
sweep. Movement-cache removal alone was within timing variation; it is not
claimed as a measured serial speedup here. The new topology was rebuilt twice,
rather than reconstructed in all 4400 warmup/measured mechanical substeps.
Raw results and provenance: `../profiling/workers_2026-10-12/REPORT.md`.

- Native mechanics, geometry, diffusion, confinement and growth tests pass with
  OpenMP enabled and disabled.
- Cache tests cover unchanged-topology reuse, changed mobility, a branch-spot
  edit with unchanged counts, segment removal and changed attachment.
- Native 1/4-worker mechanics tests compare coordinates and material frames
  exactly. Brownian worker comparisons run in separate processes, avoiding
  ambiguity from the original generator's process-global cached Gaussian value.
- `tools/profile_filaments.c` writes `final_state.bin`: exact doubles and
  subsequent RNG probes. The worker benchmark compares hashes of this file,
  rather than relying on rounded text coordinates.
- `tools/benchmark_filament_workers.py` compares 1/2/4/8/automatic worker choices
  on a dense recorded geometry. Parameters are grouped at the top for VS Code.
  Raw timing/state results are under `../profiling/workers_2026-10-12/`.
- The earlier executable is retained as
  `../build-filament-profile/profile_filaments_before_workers.exe` for the
  movement baseline. It supports rounded frame comparison, not the new exact
  state dump.
- The executable used for the timing table is retained as
  `../build-filament-workers/profile_filaments_benchmarked.exe`; the final driver
  also skips profiling thread checks when timers are disabled. A timed four-worker
  check matched the untimed full-precision state exactly.

The benchmark uses short geometry replays; these are not exact biological
checkpoints. Timings vary with scheduling and processor frequency. A larger
worker count can cease to help because noise generation, contacts and shared
movement remain serial. Use measured results for the intended network size.
