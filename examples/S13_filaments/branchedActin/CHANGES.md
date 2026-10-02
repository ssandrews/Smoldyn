# Akamatsu Lab — filament branching, elongation and capping

Shared spec for the filament work on branch **`akamatsu/filament-capabilities`**. Written so
you can see what we added and *why we chose it that way* — the rationale sections are the ones
worth arguing with. Edit this file directly if you want something different; we will follow it.

**Status, 2026-10-02.** The whole branch was merged into `master` on 2026-09-29 (PR #176),
so every hash and tag below is now an ancestor of `master`, and statements such as "nothing
touches `master`" describe the branch as it was when each section was written. This file was
shared out-of-band until now; it lives in the repository from this date, beside the
compression-model configs (see `README.md` in this folder). Three things changed after the
merge and are not reflected in the sections below: the filament examples moved into
subfolders of `examples/S13_filaments/` (`branching/`, `elongation/`, `dynamics/`, `simple/`),
so the example paths below lack the subfolder; the `capped` field is a plain 0/1 flag (the
`FILCAPPLUS` bitmask described under unit 2 is gone); and `branch_rate` parses with units of
per length per time. None of these changes a result: the eight seeded examples and the three
compression configs give byte-identical output before and after.

**Guided entry point: `filament-capabilities-overview.html`** (shared separately because of
its size; self-contained, figures embedded) — a sequential tour of all five units with code snippets, permalinks into
the public branch, and output movies/snapshots. This file (CHANGES.md) remains the detailed
manifest behind it: full parameter tables, validation methods and numbers, the review
record, and the open questions. Share the two together.

Supporting detail, same folder: `actin-branching-proposal.md` (branching design),
`elongation-capping-proposal.md` (the elongation + capping design, including how Cytosim and
MEDYAN solve the same problem), `elongation-capping-report.md` (audit of what the filament
engine could and could not do about growth), `branching-figure.html` /
`branching-timelapse.html` / `branch_geometry_70deg.png` (visualizations, shared separately).

---

## Where the code is

Branch `akamatsu/filament-capabilities`, rebased onto `master` @ `dd9fd22` (2026-08-20).
Nothing touches `master`. Two units, each an annotated tag:

| Unit | Tag | Commits |
|---|---|---|
| 1. Arp2/3-style branching | `filament-branching-v1` | `93cb3f6` `86a0651` `8aef461` `e5dd1cf` `99513ec` |
| 2. Plus-end elongation + capping | `filament-elongation-capping-v1` | `912fea7` `2f97b76` `a672963` `4fd3367` `b5e6a78` |

```
git diff master..akamatsu/filament-capabilities                  # everything
git diff filament-branching-v1..filament-elongation-capping-v1   # just unit 2
git cherry-pick <hash>                                           # take a single commit
```

Commits are ordered fix → tool → feature → examples → review so the low-risk pieces stand
alone. `93cb3f6`, `86a0651` and `912fea7` compile against stock `master` and cherry-pick
cleanly (verified). Each tag builds on its own.

The trailing **review pass** in each unit is a separate commit rather than a rewrite of the
feature commit, so you can read what we changed on a second look and why. Everything in
those two commits is described in its message; the parts that change behaviour are called
out under *Design decisions* and *Known limitations* below.

---

## What's new

All parameters are opt-in and inert at their defaults. Filament-type parameters, inside
`start_filament_type` … `end_filament_type`:

| Keyword | Units | Default | Meaning |
|---|---|---|---|
| `branch_rate` | 1/(length·time) | `0` (off) | Arp2/3 nucleation per unit mother length |
| `branch_angle` | radians | ~70° | mean daughter angle off the mother (one value) |
| `branch_spread` | radians | `0` | Gaussian jitter on the branch angle |
| `branch_segments` | count | `1` | segments a daughter is born with |
| `plus_end` | `back` \| `front` | `back` | which geometric end is the barbed/plus end |
| `elongation_rate` | length/time | `0` (off) | plus-end growth **velocity** |
| `elongation_max_length` | length | `0` (unbounded) | stop growing past this contour length |
| `capping_rate` | 1/time | `0` (off) | plus-end capping rate, per filament |
| `uncapping_rate` | 1/time | `0` (off) | plus-end uncapping rate, per filament |

Commands: `branch <daughter> <segment> [yaw pitch roll]` (inside a filament block);
`printFilaments <file>` (runtime), which dumps

```
FIL <time> <type>:<name> <nseg> <parent-or-"-"> <capped> x0 y0 [z0] x1 y1 [z1] ...
```

New functions in `smolfilament.c`: `filAddBranch`, `filBranchDynamics`, `filPinBranches`,
`filContourLength`, `filEndIsCapped`, `filElongate`, `filElongationDynamics`,
`filCappingDynamics`, `filtypeSetPlusEnd`.

The struct fields `nbranch` / `branchspots` / `branches` / `frontend` / `backend` already
existed in `master` as unused scaffolding; this work populates and uses them.

**One change to a function of yours.** `filArrayShift` now shifts `branchspots[]` along
with the segments it renumbers, dropping any branch whose segment is gone (and clearing the
daughter's end pointer). Without it, a single front-end add or remove left every recorded
branch pointing at the wrong segment, and `filPinBranches` would then move the daughter to
it. This cannot affect anyone who is not branching: nothing in stock `master` ever populates
`branchspots`. `nodemobility[]` has the same gap and we did **not** touch it — that one
affects treadmilling users today and is yours to call.

Six new files in `examples/S13_filaments/` — `branching2D`, `branchingMovie2D`,
`branchingDynamics2D`, `elongation2D`, `cappedElongation2D`, `dendriticNetwork2D`. They
follow the directory's conventions: a short header, and a `random_seed`, as 14 of the 17
existing examples have (branching grows exponentially, so an unseeded run has wildly
variable cost). `branchingDynamics2D` is the one that runs branching with `dynamics euler`,
so the branch constraint is exercised against a mother that actually moves.

---

## Design decisions — the ones worth arguing with

**Elongation is a velocity, not an event rate.** `standard_length` is a discretization choice,
not physics. If elongation were segments/time, halving `standard_length` to resolve a filament
better would silently halve its growth speed. A velocity is also the unit the literature
reports (`k_on·[G]·rise` lands in µm/s). Cytosim's `growing_speed` and MEDYAN's `k_on·[G]`
make the same choice.

**Growth is banked per filament.** `growbank += rate*dt`, and a segment is emitted whenever
the bank can afford one, remainder carried forward. Because the bank is debited by the length
actually drawn, contour length tracks `rate·t` with a *bounded* error rather than an
accumulating one, whatever the segment-length distribution. Cost: the tip advances as a
staircase, not a ramp — invisible to population statistics, but relevant if a later
load-dependent model reads an instantaneous tip force.

**A blocked plus end loses the growth; it does not bank it.** `filAddOneRandomSegment` with
`constraints=1` already retries then fails against a surface. On failure we undo the step's
bank increment and clamp the bank to one segment, so a stalled end sits at true zero velocity
instead of discharging a burst when the obstruction clears. This is the correct `f→∞` limit of
a Brownian ratchet, so a later force model refines it rather than undoing it.

**Capping blocks addition; it does not make filaments shrink.** A capped plus end stops
accepting segments from elongation *and* treadmilling, so a capped treadmilling filament
freezes rather than depolymerizing away. This is a real modelling compromise: the engine has
no filament-destruction path, and a mother that vanished would leave its daughters holding
stale branch records. Making capped filaments disappear needs a removal design first. `capped`
is a bitmask, bit 1 reserved for the pointed end.

**Rate → probability is `1 - exp(-rate*dt)`**, matching the unimolecular convention at
`smolreact.c:1323` rather than a linearized `rate*dt`. This is what makes the measured capping
rate independent of the timestep (validated directly — see V5).

**One tag for elongation + capping, not two.** They share a struct block, a parser section and
a `filDynamics` hook, and capping is only meaningful once something grows. Split, neither
compiles nor validates alone.

---

## What did not change

No existing behaviour is altered. `filTreadmill` is untouched except for one
`if(fil->capped & FILCAPPLUS) continue;` guard placed *before* the Poisson draw, and `capped`
is identically 0 whenever `capping_rate` is 0. Both new blocks in `filDynamics` return before
drawing any random number when their rate is zero, and they are appended after the existing
blocks rather than interleaved — so the RNG draw sequence of any model that does not use the
new parameters is untouched.

The two edits that do touch your code are inert at defaults: the `filArrayShift` branchspots
shift is guarded by `if(fil->nbranch)`, which is 0 unless something branched, and the
treadmilling guard is `filEndIsCapped`, which returns 0 unless something capped.

Verified on fixed seeds, against a stock `master` binary built from `dd9fd22`:

| Check | Result |
|---|---|
| `molcount`, molecule/reaction model, no filaments, 200 s | **byte-identical** to stock `master` (201 lines) |
| filament trajectory, treadmilling + Euler + surface collisions, 40 segments, 50 s | **byte-identical** to stock `master` (451 lines) |

---

## Verification

Built headless via CMake; clean, no new warnings.

| # | Test | Result |
|---|---|---|
| V1 | contour length vs *t*, `standard_length` 1× / 2× / 4× | velocity 0.4991 / 0.5000 / 0.4998 vs 0.5 set — unmoved by a 4× discretization change |
| V2 | residual `L(t) − v·t` | bounded at ~1.05 segment lengths, no drift — the bank self-corrects |
| V2b | segment draw pushed in and out of its truncated regime (σ = 1.3× vs 0.13× `standard_length`) | velocity 0.99998 in both — only the constant offset moves |
| V4 | 2000 filaments, capping only, interval-censored MLE | 0.5036 ± 0.0113 vs 0.5 set; `S(t)` matches `exp(−kt)` at every sampled time |
| V5 | elongation `v=1` + capping `k=0.1`, cohort run to full capping | added length 9.79 ± 0.11, sd 9.77 (exponential has sd = mean), KS consistent with Exponential(v/k); at `dt`/10 the mean moves −1.3 s.e. — i.e. not at all |
| V6 | fixed-seed regression | byte-identical (table above) |
| V7 | all 21 files in `examples/S13_filaments` | 20/20 load and simulate; `integration2d.txt` exceeds a 60 s cap, but does so on stock `master` too (61 s) and is unmodified by us |
| V8 | branched network, elongation-grown, fixed seed | junction angle **70.2° ± 5.3°**, 94.2% within 70±10°, n=1060 junctions |
| V9 | official `tests/*.py` and the CTest `examples` target | **103 pytest assertions pass**; CTest 33/44, with the same 11 failures on stock `master` (missing `pytest`/`flaky` in the CMake-selected interpreter, plus a matplotlib import) |

**On V8 and treadmilling.** The earlier version of this document compared elongation
(70.1° ± 5.1°) against treadmilling (91.6° ± 50.3°) and concluded that treadmilling's
segment renumbering corrupted the junctions. That was the right diagnosis, and the review
pass fixed the cause in `filArrayShift` rather than steering the examples around it. With
the fix, branching plus treadmilling no longer produces wandering junctions — it produces
**no persistent junctions at all**, because treadmilling removes from the front and the
branch is dropped when the segment carrying it is removed. That is the physically right
answer (depolymerizing past a junction debranches it), so the old comparison number no
longer has anything to describe. Elongation remains the growth mechanism a dendritic network
wants, now for a stated reason rather than a numerical one.

Behavioural checks beyond the numbered plan:

- `elongation_max_length 20` plateaus at 20.17 and holds for the remaining 80% of the run.
- `capping_rate 1.5` + `uncapping_rate 0.5` → steady capped fraction 0.7486 ± 0.0013 vs the analytic `k/(k+k') = 0.75`.
- A filament grown into a surface stalls at the wall (furthest approach 24.9998 against a wall at 25, never crossed); largest segment gain between samples is 1 — no surge, confirming denied growth is dropped. Its length stays permanently behind `v·t` (26.1 vs 199) rather than catching up.
- `treadmill_rate 5` + `capping_rate 0.15`: 198/198 filaments that capped froze on capping and never moved again, while 160/200 were demonstrably treadmilling beforehand.
- `plus_end front` grows at node 0 at the same velocity as `plus_end back` (0.5009 both).
- Parameter validation: negative rates and a bad `plus_end` value are parse errors; `elongation_rate > 0` with `standard_length <= 0` is a `filCheckParams` error; `branch_rate > 0` with `plus_end front` is an error (branching always builds daughters back-to-front); `treadmill_rate` together with `elongation_rate` is a warning, since composing turnover with net growth is legitimate, as is treadmilling at the minus end while plus-end capping is on.

**Not run:** nothing. The pytest suite and CTest now run against this branch — see V9. The
11 CTest failures are environmental and reproduce identically on stock `master`.

---

## Calibration of the example values

Example values are scale-free like every other filament example — no `units` statement is
introduced — but chosen so that reading length as µm and time as s puts them at the endocytic
actin scale. `elongation_rate 0.32` is `k_on` 11.6 µM⁻¹s⁻¹ (Pollard 1986) × 10 µM × 2.75 nm
rise per subunit. Note `k_on` has a real ~35% spread across methods, and profilin-actin
elongates barbed ends ~30% slower.

`capping_rate 4.3` is set **geometrically**, as `elongation_rate` / target length, not from
capping-protein kinetics — the kinetic route (`k_on,CP × [CP]` ≈ 0.5 s⁻¹) predicts ~600 nm
filaments against the 59–108 nm measured in situ by cryo-ET (Serwas 2022); capping there runs
~10× faster than bulk concentration predicts (Berro 2010). Conveniently the model's mean
length *is* `v/k`, so the calibration and validation V5 are the same measurement.

---

## Known limitations, carried forward

- **Branch junctions are position-pinned, not angle-held.** `filPinBranches` holds the branch
  point but not the ~70° orientation against bending. An angle-holding junction force is the
  highest-value remaining item for branch mechanics — and would be the first cross-filament
  force term in the module, which is why we did not add it unilaterally.
  (Since built: `branch_force_angle`, unit 3.)
- **Branch sites are node-quantized, with no occupancy rule** (noted 2026-08-28).
  `filBranchDynamics` picks a uniform random segment and anchors the daughter at that
  segment's back node, so branch positions land only on nodes — inter-branch spacing along
  a mother comes in multiples of `standard_length` — and nothing prevents two or more
  branches from occupying the *same* node: at high `branch_rate`·length·dt (e.g. the
  compression proof-of-concept) junctions stack at a point with no steric spacing, and
  co-located daughters get arbitrary relative azimuths. Real Arp2/3 has a ~5-subunit
  (≈13.75 nm) footprint per side, so both the spacing comb and the unlimited occupancy are
  artifacts of the discretization. Options for a spacing/occupancy rule — a per-node cap,
  a contour-length exclusion (our recommendation), and whether an opposite-facing pair at
  one site should ever be allowed — are specced in `branch-occupancy-proposal.md`; see
  also open question 6.
- **Growth is deterministic.** An honest length-*variance* model needs Poisson monomer arrival
  (`bank += δ·poisrandD(v·dt/δ)`, giving `D_L = vδ/2`) at δ ≈ 2.75 nm. One parameter, one line,
  fully separable; v1's deliverable is the length *distribution*, which comes from capping.
- **No monomer pool.** Growth does not deplete a G-actin species, exactly as nucleation is
  already phenomenological. Cytosim's non-spatial stand-in (a global `free_polymer` scalar
  multiplying the velocity) is a known option we deliberately did not take.
- **Capped filaments cannot shrink or disappear** (above).
- **`plus_end front` works but is the slow end**: front addition calls `filArrayShift` on
  every segment, so it is O(nseg) rather than amortized O(1). A generality knob, not a
  performance-equal option. It is also rejected outright in combination with `branch_rate`,
  since `filAddBranch` always builds daughters back-to-front and `filPinBranches` always
  anchors their front — teaching branching about polarity is ~20 lines we did not write.
- **`filWrite` is still a stub**, so the new per-filament state (`growbank`, `capped`) is not
  serialized by `savesim` — the same position the module was already in. `printFilaments` is
  the observability channel.

---

## Open questions for you

1. **Is `printFilaments` the right home for this?** We added a bespoke dump command because
   `filWrite` is a stub — and, we found, is never called from anywhere: `cmdsavesim` does not
   reference it and it is not exported in `smoldynfuncs.h`. Making `savesim` round-trip
   filaments means writing the body, exporting it, calling it, *and* fixing a config-file
   format for filament types, segment quaternions and branch topology. That is your design
   call, so we left it alone. `printFilaments` also serves a different purpose — a time-tagged
   `cmd i` observation stream rather than a config snapshot.
2. **Do you want the angle-holding junction force?** It is the first cross-filament force term
   in `filComputeForces` and therefore a structural decision about the module, not a local one.
   The cheapest version that needs no global solver is a one-way restoring torque on the
   daughter, treating the mother segment's `qabs` as an external field: O(1) per branch, and
   it violates Newton's third law, which is exactly why we are asking rather than doing.
3. **`nodemobility` renumbering.** We fixed `branchspots` in `filArrayShift` (see above) but
   left `nodemobility[]`, which has the identical gap and, unlike `branchspots`, affects
   treadmilling users on stock `master` today. Want that one too?
4. **Naming.** `plus_end`, `elongation_rate`, `capping_rate` follow the actin literature rather
   than your existing `treadmill_rate` register. Happy to rename.
5. **`filAddFilament` escalates the sim condition on every branch nucleation.** It calls
   `filSetCondition(...,SClists,0)`, which knocks `sim->condition` off `SCok`, so a branching
   run re-enters the full `simupdate()` — including `reassignmolecs` over every molecule —
   on most timesteps. Invisible in our filament-only examples; a real cost for a network
   coexisting with molecules. Fixing it properly means letting the dynamics path set
   `filss->condition` without escalating to the sim, which is a change to your contract for
   `filAddFilament`, so we did not make it.
6. **Do you want a branch occupancy/spacing rule, and at which layer?** (added 2026-08-28)
   Nothing today limits how many daughters share one mother node (see Known limitations).
   `branch-occupancy-proposal.md` specs two thinning-style options — a per-node cap and a
   contour-length exclusion (`branch_exclusion`, ≈ the 13.75 nm Arp2/3 footprint; our
   recommendation, because its physics don't change with `standard_length`) — both
   deterministic accept tests in the same style as `branch_surface`, inert by default.
   But if your fiber–molecule mechanism gives branch sites an explicit bound nucleator,
   occupancy becomes emergent and the spacing rule may belong in that binding design
   instead. Your call which layer owns it; we'd build the stopgap in the interim either
   way, for the same reason the region gate exists.

---

## One gotcha for re-running these

`SMOLDYN_NO_PROMPT` suppresses only the quit-at-end prompt. The *output-file overwrite* prompt
in `SimCommand.c` is unconditional, so a stale output file from a previous run makes a headless
batch run block on stdin forever. Delete declared output files before re-running, or redirect
stdin from `/dev/null`.

---

# Unit 3 addendum — thermally constrained branch junctions (2026-08-27)

| Unit | Tag | Commits |
|---|---|---|
| 3. Junction mechanics + FDT thermal fix | `filament-thermal-junction-v1` | `36f3a45` `7fd9cb6` `ee84c95` `c4726c8` `dacd8fb` `84eb2f0` `0247575` `8f6d277` |

Full design rationale, measured validation (equipartition, dt/mobility invariance,
network-scale 77.7 ± 6.8° vs the predicted 77 ± 6.6), and the review history live in
`thermal-junction-proposal.md` — this addendum is just the front door.

**What's new (all opt-in, inert defaults):**

| Parameter | Units | Default | Meaning |
|---|---|---|---|
| `branch_force_angle` | energy/rad² | 0 = off | junction torsional spring, E = ½k(θ−θ₀)², two half-couples (daughter nodes 0/1, mother nodes spot/spot+1), no cross-filament writes |
| `branch_azimuth` | rad or `random` | random | birth azimuth about the mother axis, in the mother segment's material frame; setting it also makes birth spin deterministic |
| `branch_azimuth_fix` | 0/1 | 0 | rigid azimuth restore in `filPinBranches`; for static/pinned-mother models — warned under thermal dynamics (it pumps energy into dense networks; measured) |
| `branch_force_azimuth` | energy/rad² | 0 = off | azimuthal spring toward the recorded birth azimuth; the recommended azimuth hold for thermal networks; one-way (daughter side), force capped below sinθ = 0.1 |

Plus: `branch_spread` unset now derives as sqrt(kT/`branch_force_angle`) at the
SCparams update stage (`filUpdateParams` — previously a stub), and **one change to a
function of yours that is not inert**: `filAddThermalForces`' amplitude is replaced by
the fluctuation–dissipation form sqrt(2kT/(mobility·nodemobility·dt)) — your own
`??`-flagged equation. Draw count and order are unchanged, so `kT 0` models are
byte-identical; `kT > 0` trajectories change by design. That one is discussed in the
proposal's §3.5 and is the headline question for you.

**What did not change:** fixed-seed byte-identity to `filament-elongation-capping-v1`
verified for a kT 0 branching+elongation+capping model and a molecules+reactions
model; all 22 S13 examples pass; pytest 114-passed/3-failed and CTest 33/44 with
failure sets identical to a stock baseline built at the branch point.

**Reported, not fixed** (proposal §7.5): explicit bend-force dt ceiling
(`dt ≲ standard_length²/(4·mobility·force_angle)`); `FDimplicit` unreachable by parser
prefix-shadowing AND explodes where ImplicitOld works (we tried the reorder, measured
the regression on `test3D.txt`, and reverted it — your call); `relax2D/3D.txt`'s
`dynamics implicit2` silently parses as dynamics none.

# Unit 4 addendum — location-gated branching (interim) + 3D birth-cone fix (2026-08-27)

| Unit | Tag | Commits |
|---|---|---|
| 4. Location gate + cone fix | `filament-branch-region-v1` | `b846c04` (fix) `35f73ad` (feature) `8669952` (example) |

Design rationale and the option analysis behind this unit live in
`surface-branching-proposal.md`; this addendum is the front door.

**The fix comes first because it corrects something already handed off.** The 3D
branch birth cone did not hold the polar angle: `Sph_Eax2Ypr` parametrizes the body
z axis on a cone about the reference z axis, but a segment's direction is its body
x axis. Measured symptoms (spread 0): daughters at 86.7° ± 30.3° off the mother
instead of exactly 70°, and a defined `branch_azimuth` acting as an in-plane polar
angle (`branch_azimuth 0` = daughters parallel to mothers). The birth orientation is
now built directly in the mother segment frame — direction (cos θ, sin θ cos φ,
sin θ sin φ), azimuth from +y toward +z, matching `filBranchAzimuth` — and measures
70.00° with zero variance for random and defined azimuths, with realized azimuth
equal to the keyword. Why our own validation missed it: every birth-angle number we
published came from 2D models (different code path), and the 3D junction networks
were validated with `branch_force_angle` on, whose spring pulls junctions to θ₀
after birth and masked the wrong birth geometry. Re-measured after the fix,
`branchingJunction3D` holds 78.8° ± 8.4° (n = 25) against the spring's predicted
77° ± 6.6° — the network-level claims stand; the birth-geometry claims were wrong
until now. 3D branching trajectories change by design (and one fewer RNG draw per
nucleation); 2D and non-branching models are byte-identical.

**What's new (all opt-in, inert defaults):**

| Parameter | Units | Default | Meaning |
|---|---|---|---|
| `branch_surface <name> <dist>` | length | unset = off | accept a nucleation event only if its branch point lies within `dist` of any panel of the named surface |
| `branch_compartment <name>` | — | unset = off | accept only branch points inside the named compartment |

Both thin the existing Poisson draw: events are drawn exactly as before and rejected
by a deterministic location test (`closestpanelpt` over the named surface's panels,
or `posincompart`), so the realized process is exactly inhomogeneous Poisson —
`branch_rate` per unit mother length inside the region, zero outside, at any dt —
and cost scales with drawn events, not with segments per step. Names bind at parse
time like `reaction_cmpt` (define the surface/compartment first). Setting both is a
`filCheckParams` error; a gate with `branch_rate` 0 warns.

**Stated scope — this one is designed to be retired.** The gate is an interim
geometric proxy for membrane-bound branching nucleators (Arp2/3 recruited by
membrane NPFs): it localizes where branching can happen but models no nucleator
molecules, so there is no depletion or saturation. The real mechanism is
filament–molecule binding — a nucleator species with a capture radius, consumed on
nucleation — which couples your chemistry engine to the filament module and is
yours to design. `surface-branching-proposal.md` §5–6 holds our worked sketch
(Doi-convention rate, box-scan capture, per-molecule dedup, mass conservation) as
input, not as a request to merge anything of ours. When that exists, this gate and
its two keywords can be retired; the example file and every doc for this unit say
so explicitly.

**What did not change:** fixed-seed byte-identity to the branch point (`8f6d277`)
for a kT-0 2D branching+elongation+capping model and a molecules+reactions model;
all 23 S13 examples (22 prior + `branchZone3D`) load and simulate.

**Verification** (configs in the lab notebook, `validation/branch-region/`):

| # | Test | Result |
|---|---|---|
| 1 | cone: spread-0 polar angle, random / defined azimuth (stock) | 86.7° ± 30.3°; azimuth knob acts as in-plane polar |
| 2 | cone: same, after fix | 70.00°, zero variance, all modes; azimuth = keyword |
| 3 | gate: static mother, one gated node, dt 0.005 | 2.434 ± 0.076 /time vs 2.500 predicted |
| 4 | gate: dt 0.0025 | 2.496 ± 0.110 — no dt dependence |
| 5 | gate: distance bracket at node spacing (0.049 / 0.051) | 2.494 (1 node) / 7.08 ± 0.82 vs 7.5 (3 nodes) — sharp cutoff |
| 6 | compartment-slab variant | 2.456 ± 0.109 vs 2.5 |
| 7 | out-of-zone mother; grandchildren in single-node geometry | exactly 0; exactly 0 |
| 8 | fixed-seed regression, gate off | byte-identical to `8f6d277`, both models |
| 9 | official suites | pytest 114 passed / 3 failed, CTest 33/44 — both failure sets identical to the stock baseline (zero new failures) |
| 10 | `branchingJunction3D` junction angles, post-fix | 78.8° ± 8.4° (n=25) vs the spring's predicted 77° ± 6.6° |

**One gotcha for you:** building with `OPTION_PYTHON=ON` at a commit that carries
one of our capability tags breaks the wheel version — `git tag --points-at HEAD`
finds e.g. `filament-thermal-junction-v1` and the `string(SUBSTRING ${SMOLDYN_TAG}
1 -1 ...)` "drop the v" step yields a non-PEP440 string. We work around it with an
explicit `-DSMOLDYN_VERSION=...` at configure time; a guard that only strips a
leading `v` (or ignores non-`v` tags) would make any tag name safe. Your call —
we did not touch the version logic.

# Unit 5 addendum — harmonic surface confinement (2026-08-27)

| Unit | Tag | Commits |
|---|---|---|
| 5. Filament confinement | `filament-confinement-v1` | `aab2ae3` (feature) `9332b94` (example) |

**What's new (opt-in, inert default):**

| Parameter | Units | Default | Meaning |
|---|---|---|---|
| `confine_surface <surface> <k> [front\|back]` | energy/length² | unset = off | per-node harmonic penalty ½k·d² for nodes on the side opposite the confined face (front default); force k·d toward the nearest point of each violated panel |

This is the first force on filaments "from external influences, such as surfaces"
— the item your code documentation marks as not included yet — done in the spirit
of Cytosim's per-model-point `confine` stiffness, and deliberately minimal: a soft
steric wall for filament mechanics only. It lives in `filAddConfineForces`, called
from `filComputeForces` after the junction forces; it draws no random numbers and
returns immediately when off, so regression is byte-identical for models without
the keyword. Summing over violated panels makes box corners behave; a surface
whose panels all face inward confines on every side. `filCheckParams` warns for
dynamics none (never applied), eulermat/implicitold (the analytic force matrix
carries stretch and bend only), and mobility·k·dt > 0.5 (wall spring
under-resolved by explicit integration). The constant is per NODE like the other
per-element constants, so refining `standard_length` stiffens the wall per unit
contour length — stated in the function comment.

**Verification** (configs in the lab notebook, `validation/confinement/`):

| # | Test | Result |
|---|---|---|
| 1 | zero-gap trap (two coincident rect panels, opposite fronts → exact harmonic trap ½k·z² on every node); single stiff segment, node heights independent Gaussians sd √(kT/k) = 0.00500 | sd 0.00506 (dt 1e-5), 0.00495 (dt 2e-5) — both in 95% CI, no dt dependence |
| 2 | fixed-seed regression, feature off | byte-identical to the branch point, kT-0 filament model and molecules+reactions model |
| 3 | `branchZone3D` payoff: below-membrane node samples over 1 s | unconfined 53.7% (deepest 0.25); confined k=400: 13.3%, max penetration 0.026, rms 0.005; network z-extent 0.54 → 0.22 |
| 4 | official suites | run at the branch tip — results recorded in the proposal/log (same environmental failure sets as stock baseline expected; verified before tagging) |

**Known interactions, stated:** branch *birth* does not surface-check (a daughter
born pointing at the membrane crosses it at birth and is then pushed back by the
wall over a few relaxation times) — making birth orientation surface-aware would
change the azimuth distribution and is a modeling decision we deliberately did not
take unilaterally. Elongation already cannot cross surfaces
(`filAddOneRandomSegment` constraints), so growth and confinement compose cleanly.

## Units 4+5 review record (2026-08-28, pre-push)

Two review passes ran before pushing: a four-angle cleanup review (reuse /
simplification / efficiency / altitude) and a ten-angle correctness review, both
multi-agent with independent verification. Commits `b168d0d` (cleanup) and
`6927b20` (correctness) carry the itemized results; both are behavior-neutral for
valid configurations (validation outputs byte-identical across each). Updated
commit list for the ship: `b846c04` `35f73ad` `8669952` (unit 4, tag
`filament-branch-region-v1`) then `aab2ae3` `9332b94` `b168d0d` `6927b20`
(unit 5, tag `filament-confinement-v1`).

**Hardened by the review:** bare-keyword parse lines no longer reach
`sscanf(NULL)` (a glibc crash; Apple's libc masked it); a failed runtime
`set filament_type ... confine_surface` no longer half-applies (fields commit
only after full validation); `filCheckParams` warns on a gate or confinement
surface with no panels and on a gate compartment with no inside-defining points
(previously those silently disabled branching); the confinement stiffness warning
names the per-node mobility caveat; `filAddConfineForces` carries the same
defensive node-range clamp as its siblings; the gate delegates its distance test
to your `closestsurfacept`; face tokens go through `surfstring2face` (so prefix
abbreviations work like every other face keyword).

**Known limitations, stated rather than fixed (design calls we did not make for
you):**
- The confinement side test is `panelside`'s, which classifies rect/tri/disk
  panels against their INFINITE plane; a confining surface should span or
  enclose the region filaments occupy. A partial patch penalizes nodes laterally
  beyond it, and coplanar tessellations stack (the constant is per violated
  panel). Both stated in the function banner.
- Daughter birth roll is deterministic (0) after the cone fix; anisotropic-
  bending or intrinsic-twist types get a fixed material-frame orientation per
  daughter where a random roll would decorrelate it. Isotropic types (all
  shipped examples) are unaffected.
- With `branch_spread` comparable to `branch_angle`, the truncated-Gaussian
  jitter can realize a defined `branch_azimuth` at phi+pi (negative theta).
  Clamping the jitter would change the RNG draw sequence of every existing
  spread model, so we left it; unreachable at physiological spread/angle ratios.
- The gate's per-length exactness holds for equal segment lengths (the spot draw
  is uniform per segment, the same sampling as the ungated draw).
- `filCheckParams` runs only at load, so runtime `set` / `settimestep` bypasses
  the new cross-parameter checks -- consistent with every existing filament
  parameter.
- `confine_surface`'s unit tag copies `force_length`'s `|E/L` although both
  constants are dimensionally energy/length^2 -- flagging the shared convention
  as a units question for you rather than diverging from it.
- `branch_compartment` has no in-repo example (the two gate keywords are
  mutually exclusive, so `branchZone3D` cannot carry both); it is exercised by
  our out-of-repo validation. Happy to add a small example if you want one.

**Found in your stock code during the review, reported not fixed:** the 2D
`modify_segment ... front_position` path in `filReadString` (~line 1904) uses
format string `"%mlg|L $mlg|L"` -- the `$` typo means `itct` is always 1 and the
statement can never parse in 2D.

**One more build note:** the users-manual filament statement list will need
entries for the six new keywords at merge time; per this project's rules we
don't edit your docs.
