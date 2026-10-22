# Filament performance profile — 2026-10-10

## Measured result

The current free-growth experiment spends almost all its time on mechanics.
Measurements used its actual parameters: chemistry timestep 0.0001 s, 40 mechanical
substeps, mechanical timestep 0.0000025 s, Brownian motion and sterics enabled.
No model parameters or algorithms were changed for profiling.

| Recorded geometry | Filaments | Segments | Ordinary forces | Movement | Sterics/cache | Branching + capping + elongation |
|---|---:|---:|---:|---:|---:|---:|
| 10.0001 s | 37 | 346 | 47.8% | 41.7% | 10.4% | 0.04% |
| 20.0001 s | 133 | 1,553 | 45.5% | 40.8% | 13.7% | 0.03% |
| 29.9001 s | 505 | 6,214 | 42.8% | 39.2% | 18.0% | 0.03% |

Percentages are mutually exclusive phase totals from coarse timers. Each replay
measured 100 outer timesteps (0.01 simulated seconds), after 10 warmup timesteps.
The untimed largest replay took 33.10 s wall time. Do not use this to predict a
whole run: network size and packing keep changing.

Detailed timers identify these costs near 30 s (approximate shares of that
**separate detailed run**, not percentages to add to the coarse table):

| Operation | Share of detailed loop | Function/location |
|---|---:|---|
| Node coordinates → angles/material frames | 28.5% | `filNodes2Angles`, smolfilament.c |
| Brownian force generation/application | 18.5% | `filAddThermalForces`, smolfilament.c; `gaussrandD`, libSteve/random2.c |
| Bending forces | 15.1% | `filAddBendForces`, smolfilament.c |
| Contact-list rebuild, excluding membership | 7.2% | `filBoxesBuild`, smolboxes.c |
| Shared-node movement bookkeeping | 6.9% | `filStericMove`, smolfilamentsteric.c; excludes its timed children |
| Contact-cache validation, excluding rebuild | 5.0% | `filStericPrepare`, smolfilamentsteric.c |
| Actual contact force pass and row bounds | 4.4% | `filStericForces`, smolfilamentsteric.c; excludes cache preparation |
| Branch junction forces | 4.4% | `filAddJunctionForces`, smolfilament.c |
| Filament step/roll update | 2.7% | `filStepDynamics`, smolfilament.c |
| Stretching | 2.2% | `filAddStretchForces`, smolfilament.c |
| Box segment membership | 1.1% | `boxesupdatesegments`, smolboxes.c |
| Wall confinement | 0.9% | `filAddConfineForces`, smolfilament.c |

Late capping and elongation scans each took less than 1 ms across the entire
100-timestep window; branching took about 9 ms. There was one new branch in the
late window, with unchanged segment count elsewhere. Most of these timings are
routine scans, not averages over many successful chemistry events.

A separate run from the original seed definitions measured 10,000 timesteps
(1 simulated second), taking 8.16 s. It grew from 3 to 4 filaments and 15 to 31
segments. Branching/capping/elongation together cost 0.115% of that loop. It
exercised 16 growth/branch collision queries; no capping event occurred.

Writing a late frame took about 19 ms locally. At one frame per 0.1 simulated
seconds, this corresponds to roughly 0.006% extra wall time in the late replay.
This is a formatting/flush measurement with local filesystem caching, not a
long-run disk throughput guarantee.

## What the measurements imply

1. **Geometry and movement first.** `filNodes2Angles` traverses every filament
   after every mechanical substep, computing trigonometric functions and
   quaternions. Look for redundant conversions and shared intermediates with
   bending/junction calculations. These frames are needed for subsequent forces,
   so simply skipping their refresh would change the mechanics.
2. **Remove repeated movement bookkeeping.** `filStericMove` reconstructs node
   identities and branch unions each substep even when topology is unchanged.
   Cache topology separately from changing positions, forces and mobilities,
   invalidating it on branch/growth/removal edits. It also calls the ordinary
   `filStepDynamics`, whose independent Cartesian movement is immediately
   overwritten by the coupled positions. A dedicated roll/material update
   could remove that redundant work while retaining the current torque handling.
3. **Brownian and bending arithmetic.** Thermal work includes three Gaussian
   draws per node per substep, scale computation and force-array traffic.
   The current Gaussian generator already caches the second draw of a polar
   Box–Muller pair. Investigate batching and invariant scale reuse; retain the
   required independent increments and drag weighting. Replacing the generator
   changes seeded trajectories and requires statistical validation.
4. **Steric cache next.** At the late snapshot, 305 rebuilds occurred in 4,000
   force passes: approximately one rebuild every 13 substeps. The final list
   retained 2,089 pairs, of which 111 were active contacts. Profile a skin-margin
   sweep to balance rebuild cost against longer candidate lists. Investigate
   recording displacement bounds during movement rather than rescanning all
   segment endpoints at every cache validation; preserve topology/parameter
   invalidation and the safe skin/2 bound.
5. **Chemistry and output have little payoff at this configuration.** Branching
   still scans mother segments, and crowded growing ends may repeatedly test
   insertions. Revisit their cost if branching rates or stall fractions change.

These are optimization candidates; no performance refactor was implemented in
this profiling change. A twofold improvement in a 28.5% phase would improve the
whole detailed loop by only about 1.17×, not 2×.

The current mechanical step is the same as 1e-5/4. Increasing the outer timestep
while increasing substeps proportionally mainly reduces the already cheap
chemistry scans; it does not reduce force/movement evaluations per simulated
second. Reducing mechanical substeps requires stability and convergence checks.

## Reproduce

New files:

- `source/Smoldyn/smolprofile.h` and `.c`: optional nested wall-clock timers.
- `tools/profile_filaments.c`: native model/snapshot replay driver.
- `tools/profile_filaments.py`: parameters at the top; selects real frames,
  creates private run directories, runs replays and controls, writes CSV/JSON
  and a Markdown report. Uses only Python's standard library.

Configure a **separate Release build** with `OPTION_FILAMENT_PROFILE=ON` and the
same compiler and feature settings as the production build. The local build
directory for the current worker implementation is `../build-filament-workers`;
the original measurements used `../build-filament-profile`. Profiling defaults OFF.
After initial configuration, from the Smoldyn repository:

```powershell
cmake --build ../build-filament-workers --target profile_filaments
python tools/profile_filaments.py
```

The script can also be run directly in VS Code. Its default source is
`../Free_growth_experiment_1_steric.txt`, with
`../free_growth_steric_frames_2.txt`. Results are written under
`../profiling/free_growth_current/`; the original experiment files are untouched.
Historical measurements above remain under `../profiling/free_growth_2026-10-10/`.
Change the grouped parameters to profile another experiment.

Native driver arguments are `model snapshot steps warmup timers output_repeats`.
Use timer mode 0 for an untimed control, 1 for detailed timers, 2 for coarse
timers. A snapshot path of `-` keeps the original model seeds. The driver expects
private copies with scheduled commands removed and output named
`profile_frames.txt`; the Python script prepares these copies.

## Verification and limits

- Release builds compiled with profiling both ON and OFF.
- Native steric regression checks passed in both builds.
- Timed, coarse-timed and untimed replays at 20 s and 29.9 s produced identical
  final frame hashes. Timing hooks do not add random draws or change physics.
- Fine timers added approximately 15% wall time to the largest sample. Coarse
  timing differed from untimed runs by -6% to +4%, including scheduling noise.
  Treat the fine breakdown as approximate and use coarse totals for rankings.
- Geometry replays are **not exact biological restarts**. Frame coordinates are
  rounded; dumps omit material rolls/frames, birth azimuth, RNG state and growth
  bank. The replay reconstructs material frames, initializes birth azimuth to
  current geometry, infers growth bank from recent observed elongation, and
  resets the RNG to the model seed. Warmup reduces reconstruction effects, but
  exact late-trajectory timing would need in-process profiling or full checkpoints.
- These short dense windows do not characterize frequent growth rejection or
  late conditions beyond the available frames. Actual chemistry event
  frequencies can require longer sampling windows.
- Graphics and progress display are excluded. The model has no explicit
  molecular species: its capping/branching are filament-level stochastic rules.
  These numbers do not characterize molecule-rich Smoldyn models.
- `time_stop` is 90 s but the current `printFilaments` command stops at 30 s.
  The existing frame file cannot characterize the remaining 60 s.
