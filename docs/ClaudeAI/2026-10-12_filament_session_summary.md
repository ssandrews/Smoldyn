# Claude Code session summary: Smoldyn filaments, October 12–30, 2026

This file summarizes two long Claude Code sessions (Claude Opus 5.5) with the
Smoldyn maintainer, on October 12–20 (parts 1–4) and October 23–30 (parts 5–16). Most of the work was on the filament code in
`source/Smoldyn/smolfilament.c`. There were also small changes to
`smoldyn.h`, `smolboxes.c`, `smolsurface.c`, `smolcomparts.c`, `smolsim.cpp`, `smolcmd.c`, `libSteve/RnSparse.c`, `libSteve/Geometry.c`, the user's manual
(`docs/Smoldyn/SmoldynManual.tex`), the code documentation
(`docs/Smoldyn/SmoldynCodeDoc.tex`), the generated help
(`source/Smoldyn/smolhelp_data.h`), and the `examples/S13_filaments` examples.
Everything listed below was committed and pushed to `master` on
github.com/ssandrews/Smoldyn.

## Working conventions used

- Claude proposed a design first, and the user then said "go ahead with your
  recommendations".
- Commits were made only when asked. Claude's commits end with
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. The user's own
  edits were committed separately, and only when the user said to include
  them.
- Generated files were never committed: `build/`, LaTeX `.aux`/`.log`
  files, example output, `.so` files, doxygen output, and `.rst` manuals.
  The manual and code documentation PDFs were regenerated and committed only
  when the user asked
  (pdflatex, bibtex, then pdflatex twice, using `/Library/TeX/texbin/pdflatex`;
  the MacPorts `pdflatex` that comes first in the PATH lacks `scrbook.cls`).
- Every change updated the manual and the code documentation. The help text
  was regenerated with `python3 scripts/make_smolhelp.py`.
- Verification:
  - Regression of all S13 examples against a build of HEAD, made in a
    scratchpad git worktree, comparing the `printFilaments` output.
  - AddressSanitizer sweeps over every example and integrator.
  - Targeted physics tests: energy dissipation, Boltzmann variances,
    diffusion ratios, and fixed nodes.
  - Changes to molecule code were checked by running every example with
    surface-bound molecules on both builds and comparing the full output,
    with a molecule listing at the end of each run.
  - The HEAD build has to be rebuilt after each commit, or the regression
    compares against an old binary (see the mistakes section).
  - Unseeded examples need a fixed seed before their outputs can be
    compared. macOS has no `timeout`, so runs were limited with
    `perl -e "alarm N; exec @ARGV"`.
- `/usr/local/bin/smoldyn` can lag behind `build/` until `sudo make install`
  is run. Examples were run with `build/smoldyn`.

## Part 1: naming, branching, end states, removal (Oct 12–15)

| Commit | Change |
|---|---|
| 593fe52 | Filament ends renamed from front/back to minus/plus (`xyzminus`/`xyzplus`, `'m'`/`'p'`, `minus_position`). `plus_end` statement removed. Fixed an index bug in `filArrayShift` and a 2D format typo. |
| 5023b5a | `branchspots` renamed to `branchnodes`. Branches record the mother node (1..nseg) rather than a segment. Added `filShiftBranchNodes`. Fixed `filRemoveSegment` leaving a branch on a removed node. |
| a78e3b2 | Branches can attach by either end. Added the `branch_end` statement. The `branch` statement gained optional end, length, and angle arguments. `\|` replaced by `/` in usage messages, because the log treated `\|` as a units tag. |
| 77a97f0 | Same `\|` fix for the `confine_surface` messages. |
| 6bc25b4, 4181e0a | Example reorganization (static/, dynamics/, treadmilling/), new minimal and branch examples, and the `endstates2D` example (included the user's changes). |
| 5c4eed5 | Mathematica notebooks for the Sphere library documentation. |
| 47bca69 | CMake accepts any Python ≥ 3.10. Generated `.rst` manuals added to `.gitignore`. |
| a28252f | General end states. Each end is open, capped, or shrinking, with per-end, per-state rates and stochastic transitions. New statements: `elongation_rate end state value`, `end_transition`, `end_state`. Type-level `end_state` affects only new filaments, as the user wanted. Fixed stale node pointers in `filArrayShift`. |
| 88ace63 | Filament removal: the `remove_filament` statement, plus automatic removal when the last segment shrinks away (committed by the user). |

## Part 2: filament networks with loops (Oct 15–17)

**55a6dbe: networks with loops.** Branch fields were replaced with general
junction records. Each record holds the filament, end, target, node,
rigid/bend flags, and azimuth.

- `filClassifyJunctions` splits junctions into two kinds:
  - rigid junctions that form a spanning tree;
  - loop-closing junctions, which are modeled as springs.
- New statements:
  - `attach end target node` and `detach end` in the filament block;
  - `junction_force` for filament types.
- Branch targets can now be node 0..nseg.
- `printFilaments` prints the attachments of both ends.
- New examples: loop networks, plus a spider-web network (`spiderWeb2D`).
- Known behavior change: in `BranchAndTreadmill2D`, a daughter attached at
  node 0 is now kept rather than released.

The roll and thermal fixes that followed:

| Commit | Change |
|---|---|
| f45fc56 | Fixed `filCopyYpr2ToRoll`/`filCopyRollToYpr2`, which reset the roll angles every time step. |
| d5f943e | The matrix integrators (EulerMat, ImplicitOld) evolve roll correctly. |
| 4d01a3b | Thermal noise on roll, with rms sqrt(2kT/(μ·m·dt)). |
| 12541ab | Thermal noise in EulerMat and ImplicitOld. |
| 6ec3c00 | Junction and confinement forces in EulerMat and ImplicitOld. |

6ec3c00 also fixed three memory bugs found with ASan:

- uninitialized `fil->filwork`;
- out-of-bounds column reads in `sparseMultiplyMV` and
  `sparseMultiplyScalarM`;
- an out-of-bounds node read in `filAddBendForceMat`.

## Part 3: stability, correct torques, frames, constraints (Oct 18–19)

| Commit | Change |
|---|---|
| 8509a21 | `filUnstable` stops the simulation with a message when the mechanics blow up. It uses NaN, coordinate-size, and spring-based limits. This fixed the hang in `branchingMovie2D`. |
| 9894890 | The Implicit integrator now uses an analytic Gauss–Newton Jacobian (−k∇r∇rᵀ) instead of a finite-difference one. The finite-difference Jacobian had positive eigenvalues and discontinuities. Stretching includes a definiteness fix. The 3D bending and twist gradients come from the inverse Euler-rate relations, with cos p bounded by `FILCOSPMIN`. Implicit now solves for the displacement. |
| 15133aa | The 3D bending torque is now the true energy gradient, τ = −M⁻ᵀΔ. |
| 94ef70a | Wrote the manual section "Filament dynamics integration" (`sec:FilDynamicsIntegration`). It gives the equation and algorithm for each method (Euler, RK2, RK4, EulerMat, ImplicitOld, Implicit). `twist2` `time_stop` set to 45000. |
| c45768b | Segment frames move by minimal rotation from the old direction to the new one, followed by the roll increment. This fixes the geometric twist coupling along the chain. |
| 28a8678 | Faster frame transport, using the `wqabs` frame cache. |
| 5691281 | Negative force constants act as rigid constraints during dynamics. They use SHAKE-style projection along start-of-step gradients, which samples the rigid-constraint ensemble. |

## Part 4: consistent thickness scaling and drag (Oct 19)

| Commit | Change |
|---|---|
| 106b2e9 | Stretching: thk·klen |
| 443b4c7 | Bending: average thk·kypr |
| 050c860 | Junction springs: thickness of the end segment times kj |
| 70475ae | Branch springs: average thk; the derived branch spread is divided by sqrt(thk) |
| 03721ed | Confinement: node thickness |
| bf558e9 | New `thickness_drag` statement, on by default: effective mobility = `node_mobility` / node thickness. Thermal amplitude is consistent with this. Implicit honors node mobility through row weights W, solving (I − μΔt W J)Δx = μΔt W f. |
| 395b4a9 | EulerMat and ImplicitOld also honor node mobility. |
| 18c98a1 | `filCheckParams` warns that EulerMat and ImplicitOld are test methods. EulerMat matches Euler but is slower. ImplicitOld can be unstable even at small time steps, because it uses the factored force matrix rather than the Jacobian. They are kept for teaching and testing. |

## Part 5: minimum branch spacing (Oct 23)

**b1b42e9.** New `branch_min_spacing` statement for filament types (the
user preferred this name). A branch from `branch_rate` isn't created if it is
closer than this distance, measured along the mother's contour, to an existing
branch on the same mother (`filBranchNodeSpaced`). Branches from the `branch`
statement and `attach` aren't affected, but Smoldyn warns if they are too
close. New example: `branching/branchMinSpacing2D.txt`. The user's own
changes were committed separately (a93d55d, 9097b8f) and the manual PDF was
regenerated (b3a54a1).

## Part 6: filament-surface interactions (Oct 23)

**710c725.** The stub `filsurfinteractstruct` was completed, and filament
types now hold lists of surface, molecule, and filament interactions
(`surfint`, `molint`, `filint`; the molecule and filament lists are stubs that
are always empty).

- New statement: `surface_interaction surface face parts action [parameters]`.
  - `face` is the side of the surface that the filament is on: front, back,
    or both.
  - `parts` is plus, minus, side, all, or a list such as `plus,side`.
  - `action` is `force` (a soft harmonic wall with force constant k, so the
    force is t·k·d) or `block`.
- As the user asked, there's no backward compatibility: without listed
  interactions, filaments and surfaces don't interact at all.
  `confine_surface` remains as shorthand for a force interaction on all parts.
  The old `confinesrf`, `confineforce`, and `confineface` fields were removed.
- `filAddSurfForces` replaced `filAddConfineForces`, and
  `filSurfBlocksSegment` replaced `filSegmentXSurface`.
- New example: `elongation/surfaceInteraction2D.txt`. The treadmill and
  `branchZone3D` examples gained block interactions, chosen so that their
  results matched HEAD.

## Part 7: filaments in virtual boxes (Oct 23)

| Commit | Change |
|---|---|
| de70566 | Fixed an off-by-one heap overflow in `filSetSequence`, found by ASan in `sequence.txt`. |
| 3446427 | Each virtual box can list the filament segments that pass through it and the filament ends in it (`boxaddfilseg`, `boxaddfilend`, `filBoxesUpdate`). The lists are only maintained when `filSetBoxLists` turns them on, which nothing does yet, because rebuilding them every step slowed simulations without dynamics by up to 4.6×. Compiling with `FILBOXCHECK` forces them on and checks them every step with `filBoxesCheck`. Blocking now walks the boxes along a new segment instead of testing every panel. Smoldyn warns if filaments have surface interactions and there is only one box. |

The commit messages of de70566 and 3446427 lost their subject lines, because
of a shell quoting mistake. The user chose to leave them as they are.

## Part 8: block interactions as walls (Oct 23)

(In part 9, `block` was renamed `reflect` and `force` was renamed `softwall`,
and the `filSurfBlock...` functions became `filSurfReflect...`.)

| Commit | Change |
|---|---|
| b843d51 | Block interactions act as hard walls for filament dynamics, as the user designed. Node positions are saved before the integrator (`filSaveStartNodes`). Afterward, a node whose path crossed a blocking surface from the filament's side is reflected back to the same distance on the allowed side (`filSurfBlockNodes`, `filSurfBlockCrossing`). The resulting stretching and bending forces act in the next step. Flat panels reflect across the panel plane. Curved panels reflect through the closest panel point, because tangent-plane reflection let a few nodes stay outside spheres. Block interactions now accept the side part, and `all` means every node. `surfaceInteraction2D` uses only a block interaction for its ceiling. |
| f8abade | New branches from `branch_rate` are tested with `filSurfBlocksBranch`. A blocked daughter is removed, its automatic name is reused, and the branch is retried with new random angles, up to `FILMAXTRIES` times. Node reflection was moved after `filPinJunctions`, because pinning moved daughters across the membrane in `branchZone3D`. |
| 8b5f945 | `boxsize 0.25` added to `branchZone3D`, which removes the one-box warning without changing the results. |
| c9fbbe3, 705511d | Manual PDF regenerated. |

Tests:

- Block-only walls with high kT: 0 of 141,146 node positions crossed.
- Filaments inside and outside a sphere: none crossed.
- Cube of six rectangles: the only nodes outside were ones that
  `random_filament` placed outside at the start.
- `branchZone3D` with 10 seeds: no nodes ended below the membrane, compared
  with 2–28 before.
- Branches off a mother just above a blocking floor: none crossed it, compared
  with 3 of 12 without the block.

## Part 9: renamed interactions and the repel interaction (Oct 28)

**7dd5146.** As the user asked, the `force` interaction is now `softwall`
(`FSAsoftwall`) and `block` is now `reflect` (`FSAreflect`), with no backward
compatibility for the old keywords. The internal reflect functions were
renamed to match, but `filSurfBlocksSegment` and `filSurfBlocksBranch` kept
their names, since they test whether growth is blocked.

- New `repel` interaction: `surface_interaction srf face parts repel k
  repeldist`. It's a softwall whose zero-force point is moved `repeldist`
  away from the surface, on the filament's side, so a node closer than that
  feels t·k·(repeldist − s). With `repeldist` 0, its output is identical to
  softwall.
- `surfaceInteraction2D` gained a green type that is repelled from the
  ceiling, so it shows all three interactions.
- The commit also included the user's own documentation edits and new
  `force` fields in the panel and surface structures, as the user asked.

## Part 10: adsorbing filament nodes to surfaces (Oct 28)

**2eb39e4.** Claude proposed the design and the user approved it.

- New per-node arrays in the filament structure: `nodepnl` (the panel, or
  NULL), `nodeface`, and `nodesrfmob` (the lateral mobility factor).
- New statements: `adsorb_node node surface [face [mobility]]` and
  `desorb_node node`, which also work at runtime with `set filament`.
- An adsorbed node's mobility is multiplied by its lateral factor (in
  `filNodeMobility`, so every integrator and the thermal noise use it).
  After the step, `filSurfAdsorbedNodes` moves it back onto its surface,
  which removes the normal part of its motion.
- Crossing onto neighboring panels reuses the molecule code. The core of
  `movemol2closepanel` was split out into the point-based
  `movept2closepanel`. It writes the panel through a pointer only where the
  original assigned `mptr->pnl`, which kept molecule results identical.
- Softwall and repel with the same surface skip adsorbed nodes, and reflect
  skips all adsorbed nodes. A rigidly pinned branch end can't be adsorbed.
  Adsorbed daughter nodes are projected again after `filPinJunctions`.
- Fixed a pre-existing bug: `filArrayShift` didn't shift node mobility
  factors, so they stayed at their array index when the minus end grew or
  shrank. Per-node state now moves with the nodes, and new or removed nodes
  are reset with `filNodeStateClear`.
- New example: `dynamics/adsorbedNodes3D.txt`.

Tests:

- 200 fully adsorbed filaments on a plane: the center-of-mass diffusion was
  1.05 times the prediction kT·μ·f/N over 4 seeds. Nodes stayed on the plane
  to 2×10⁻¹⁶.
- A floor of 16 tiles with neighbors: nodes crossed tiles, and every
  recorded panel matched the node's position.
- On a sphere, radii stayed within 6.5×10⁻⁵ of the sphere's.
- Adsorption followed nodes through minus-end growth and shrinking.
- Molecule regression after the refactor: 67 completed examples identical.

## Part 11: adsorb and desorb interactions (Oct 28)

**5d46211.** New surface interactions:

- `surface_interaction srf face parts adsorb rate capture_distance
  [mobility]`: a free node within the capture distance, on the face, adsorbs
  with probability 1−exp(−rate·dt) per step.
- `surface_interaction srf face parts desorb rate [offset]`: an adsorbed node
  desorbs with probability 1−exp(−rate·dt) and is placed `offset` from the
  surface.
- These run after the mechanics in each step (`filSurfAdsorbDesorb`), also
  for filaments without dynamics. Random numbers are only drawn for nodes
  that could change state.
- Fixed: a node exactly on a surface was classified as being on its back
  face by `panelside`, so `adsorb_node` without a face put it on the back.
  It now goes to the front, or to the interaction's face.
- New example: `dynamics/adsorbDesorb3D.txt`.

Tests: with 1,000 static nodes, k_ads = 2 and k_des = 1, the mean adsorbed
count over 7 seeds was 519.9 at t = 0.5 (predicted 518.6) and 667.6 at
t = 10 (predicted 665.6). Only listed parts adsorbed, pinned branch ends
never did, desorbed nodes landed exactly at the offset, and face `both`
adsorbed nodes to whichever face they were on.

## Part 12: reaction forces on surfaces (Oct 28)

**1275f10.** As the user asked, softwall and repel interactions now apply the
equal and opposite force to the panel that pushes on the node, in the panel's
`force` element, and each surface's `force` element is the sum of its panels'.

- The force loop moved into `filSurfInteractForces(fil, nodemin, nodemax,
  reaction)`, which adds forces to the nodes (reaction 0) or the opposite
  forces to the panels (reaction 1). `filAddSurfForces` calls it with 0.
- `filSurfReactionForces` clears all panel and surface forces, adds the
  reaction forces of every filament whose type has dynamics, and sums each
  surface's panels. `filDynamics` calls it once per step, at the end, from
  the final node positions. It can't be done inside `filAddSurfForces`,
  because RK2, RK4, and Implicit call that several times per step.
- The `force` fields, which the user had added, were never initialized; they
  are now zeroed in `panelsalloc` and `surfacealloc`.
- `surfaceoutput` (for example, `diagnostics surface`) lists the force that
  filaments exert on a surface when it isn't zero.

**5f345ef.** A floor made of two coplanar rectangles exposed an older
problem: each panel acted as an infinite plane, so a node behind the floor was
pushed by both, and the panel it wasn't over pulled it sideways (a surface
force of (17.3, 0, −10.5)). The user asked that each node interact only with
the closest panel of each surface, and to fall back to the next panel if the
closest one gives no force. Claude pointed out that the fallback would let the
side faces of a convex obstacle pull on a node just outside it, and the user
chose "closest panel decides": a node that's on the allowed side of its
closest panel (from `closestsurfacept`) gets no force from that surface.

Tests: a sphere with repel gave the same surface force as an independent
calculation from the node positions. The two-tile floor now gives (0, 0,
−7.217), the same as one panel of the same size, with identical filament
output. All 37 seeded S13 examples matched HEAD, and ASan was clean.

## Part 13: surfaces that move from filament forces (Oct 28)

The user asked for surfaces to move in proportion to the applied forces with
a mobility value, ignoring the dependence on cross-section for now, with
adsorbed molecules and filaments moving with the surface, and with molecules
that the surface collides with pushed along as in `comparttranslate`. The
user suggested that surfaces might need a list of their adsorbed filaments.
Claude proposed two phases, and the user said to go ahead.

**61b1b93 (phase 1: surfaces and molecules).**

- A new surface statement, `mobility μ`, sets `srf->mobility` (default 0).
  `surfmovebyforces`, called in `simulatetimestep` right after `filDynamics`,
  translates each mobile surface by μ·F·Δt using the force from
  `filSurfReactionForces`. Surfaces only translate, with no rotation or
  thermal motion. `srf->displace` accumulates the displacement, and
  `diagnostics surface` lists it.
- The code&2 block of `comparttranslate` became `surftranslatebound`, which
  both use to carry surface-bound molecules.
- `surfsweepmols` pushes solution molecules that the surface runs into,
  using the code&8 method: the line from each molecule's position to its
  position minus the move, tested against the panels at their old positions.
  Each molecule is moved at most once, and only molecules inside the bounding
  box of the old and new panel positions (`panelboundingbox`) are tested.
- `boxesupdatesurface` (in `smolboxes.c`) updates the box panel lists for
  just the moved surface, instead of `surftranslatesurf`, which would trigger
  a full box rebuild. Molecules are reassigned to boxes only if some moved.
- A compartment's inside-defining points move only if all of its surfaces
  moved together. `checksurfaceparams` warns if a mobile surface has
  unbounded emitters, is part of a port, or bounds a compartment.
- New example `dynamics/movingSurface2D.txt`: filaments grow up from a floor
  and push a lid, carrying bound molecules and pushing others.

Tests: a lid pushed through several layers of boxes kept every diffusing
molecule on its own side, bound molecules exactly on the lid, and pushed
molecules just ahead of it. A temporary check against `panelinbox` found no
box-list mismatches for a lid and for a sphere moving 4.3 through boxes of
size 2. All 76 seeded molecule examples (including `translatecmpt` and
`diffusecmpt`) and all 37 S13 examples matched HEAD, and ASan was clean.

**46cd510 (phase 2: filaments).**

- Each surface has a list of filaments that may have nodes adsorbed to it
  (`maxfil`, `nfil`, `fillist`), managed by `surfaddfilament` and
  `surfremovefilament`. Filaments are listed instead of nodes, because node
  indices change when the minus end grows or shrinks. The list may include
  filaments with no nodes left on the surface: `filNodeAdsorb` and
  `filSurfAdsorbedNodes` (when a node walks onto another surface's panel) add
  to it, `filNodeDesorb` removes a filament when its last node on the surface
  desorbs, `filRemoveFilament` removes it from every surface, and
  `filSurfMove` prunes the rest.
- `filSurfMove` moves the adsorbed nodes with the surface, and pushes free
  nodes of types with dynamics and a reflect interaction with the surface,
  using the same sweep test as for molecules. `filSurfMoveDone` then re-pins
  rigid junctions and updates the filament box lists.
- New example `dynamics/movingSurfaceFilaments2D.txt`: blue filaments hang
  from the lid by anchored adsorbed ends, and green filaments above the lid
  are pushed up by it.

Tests: adsorbed nodes stayed on the lid within 10⁻⁵ as it rose 14.6, a branch
attached to a carried filament stayed on its node, a free filament above the
lid never got behind it, and desorbing or removing filaments took them off
the lid's list. A temporary every-step check that every adsorbed filament is
listed passed on the new tests and on all S13 examples with surfaces. All 38
seeded S13 examples matched HEAD, and ASan was clean.

## Part 14: the applysurfaceforce command (Oct 29)

**0891948.** The user asked for a runtime command that applies a force to a
surface. `applysurfaceforce surface Fx [Fy [Fz]]`, where the surface can be
`all`, adds to a new surface element, `extforce`, through `surfapplyforce`.
`surfmovebyforces` adds this to `force` and then clears it for all surfaces,
so it lasts one time step, like `longrangeforce`, and `cmd E` gives a constant
load. Claude chose this one-step behavior and told the user that a persistent
force would be a small change. It's kept apart from `force` because
`filSurfReactionForces` recomputes that each step, and only if there are
filaments, so a force added to it would persist without filaments.

An undefined name such as `x` was read as NaN with no error from
`strmathsscanf`, so the command now requires finite values.

The user asked to run "the example file", though there wasn't one, and chose
a new one when asked. `dynamics/surfaceLoad2D.txt` has three lanes with lids
under loads of 0, 1.5, and 3. The first version, with two long filaments in
each lane, gave a smaller rise for the 0.5 load than for the 1.5 load, because
buckling dominated. Three stiffer filaments with Implicit dynamics and a lid
at y = 22 gave rises of 35.8, 16.6, and 8.7.

Tests: a lid with mobility 0.5 and force 2 moved exactly 10.01 in 1001 steps,
a force on every 10th step gave exactly 1/10 the motion, and a surface
without mobility didn't move. All 39 S13 examples matched HEAD, and ASan was
clean.

## Part 15: barbed filament ends (Oct 30)

**a988ef1.** The user asked for a new end state, `barbed`, for either end, with the
elongation rate v = r + a(exp(−kF) − 1), where r is the existing `endrate`
value, a and k are entered after the other values of `elongation_rate` and
stored in a new filament type element, `barbedparameters`, shared by both ends,
and F is the force pushing against the end node, parallel to the end segment.

- `FESbarbed` was added to `enum FilEndState`, and `FESMAX` is now 4. The
  statement is `elongation_rate end barbed r a k`.
- **The choice of F.** The user's description didn't say which forces count.
  `filEndPushForce` uses only the softwall and repel forces on the end node,
  not the total force. These dynamics are overdamped, so the total on a node
  pressed against a surface is close to zero (stretching balances the surface
  force), and it includes thermal noise. F is positive when it pushes the node
  back toward the filament. Reflect interactions exert no forces, so they
  don't count, and Smoldyn warns if a barbed type has no softwall or repel
  interactions.
- `filSurfInteractForces` has a new `fsum` argument, which adds the forces to a
  vector, without `filwork`, so barbed ends work for filaments without
  dynamics. `filEndVelocity` computes the rate for `filEndDynamics`.
- F isn't limited to positive values, as the formula was specified, so an end
  growing away from a repelling surface, within the repel distance, can grow
  much faster than r (about 14 times in one test).
- Treadmilling still only applies to open ends.
- New example `dynamics/barbedEnd2D.txt`: two lanes with equal loads, open and
  barbed. At seed 1 the lids rose 30.8 and 12.9, with about 60 and 33 segments.
  Claude's first header comment said that barbed lids rise steadily without
  buckling, but runs at other seeds showed that this isn't reliable, so
  the comment was reworded.

Tests: a straight filament without dynamics or thermal noise, growing into a
repelling wall, matched the integrated formula to within one segment (0.05)
for approach to stall, growth past a weak force, starting beyond stall (where
the end shrinks), and negative force, and the minus end mirrored the plus end.
The 40 S13 examples that were in the previous commit matched it, and ASan was
clean.

## Part 16: contact forces between filaments (Oct 30)

**2dd415f.** The user asked for softwall repulsion between filaments, including
a filament looping onto itself, turning on when segment centerlines are closer
than the sum of the radii, and asked how Claude would do it. Claude proposed a
design, and the user agreed to all of it, including its three suggestions:
whole filaments with no `parts` option, barbed ends responding to contact
forces, and a default radius of 1.

- New statements: `filament_interaction type softwall k`, which is symmetric,
  so one interaction is stored however it's entered, and `radius`, because
  `filradius` could not be set before. `FFAforce` became `FFAsoftwall`.
- The force is k_eff(D − d) between the closest points of two segments, from
  the energy ½k_eff(D − d)², where k_eff is k times the harmonic mean of the
  thicknesses. It's split between each segment's nodes, which conserves force
  and torque. Ends need no separate code, since segments are finite.
  `Geo_NearestSeg2SegPts` is new, and uses the middle of the overlap for
  parallel segments.
- `filContactPair` skips pairs in three cases: centerlines that touch; pairs
  closer along the filaments than a hairpin, πD/2 (`filContactGap` also follows
  junctions); and a contact at a shared node that a neighboring segment
  counts instead, which gives one force per contact.
- `filContactForces` runs once per step, after end dynamics and before the
  integrators, and the forces stay fixed through the step. They can't be
  computed in the integrators, because forces are cleared one filament at a time
  and filaments are stepped one at a time. `filDynamics` was split into
  `filDynamicsEnds` and `filDynamicsIntegrate`, and runs all end dynamics first
  only when contacts exist, so other simulations draw the same random numbers.
- This is the first use of the filament box lists. Each segment searches the
  boxes that overlap its bounding box plus the contact distance, rather than
  the `neigh` lists, which depend on simulation accuracy and fail for boxes
  smaller than the contact distance.
- New example `dynamics/filamentContact2D.txt`.

Tests: two parallel segments followed the exact decay to within 0.0004; a
head-on pair turned on between gaps of 2.02 and 1.98; the sum of node positions
was conserved in a skew 3D pair; a straight filament felt no self-force;
hairpin arms separated; a straight branch felt no force at its junction; a
barbed end stalled against a static filament at 18.7 (predicted 18.69); and box
sizes from 0.5 to 100 gave identical results. All 41 shared S13 examples
matched HEAD, and ASan was clean. In the example, over seeds 1–8, contact
filaments never crossed, and plain ones did in seven seeds.

## Known limitations (documented in the manual)

- Rigid constraints give rigid-ensemble statistics. Bending variance is about
  5–10% above kT/k, compared with a stiff-spring model.
- `betterGraphics3D` (negative constants) is still too stiff for the
  explicit methods. Use Implicit.
- ImplicitOld can diverge in stiff, thermal 3D systems.
- Loop-closing junctions have small gaps under load. These shrink as
  `junction_force` is increased.
- Frame transport still has some `atan2` roll overhead.
- Reflect only tests node paths. A segment can still cut across a surface's
  edge or corner while both of its nodes stay on the allowed side.
- A node that starts on the wrong side of a reflecting surface isn't moved. With
  face `both`, it stays on that side.
- Branches from the `branch` statement, and filaments defined directly, aren't
  checked against reflect interactions.
- The softwall and repel side tests use the infinite plane of each node's
  closest flat panel, so a node beyond the edge of a partial patch, and behind
  its plane, is pulled toward the edge. In a concave corner, repel only pushes
  a node away from the nearer wall.
- Moving surfaces only translate, with a mobility that doesn't depend on
  their size or shape, and no thermal motion. Their motion is explicit, so a
  large mobility pushed by stiff interactions can overshoot and oscillate.
- Moving surfaces don't update compartment volumes or box lists, emitter
  absorption, or ports. Molecules and filament nodes squeezed between a
  moving surface and another surface can end up on the wrong side of it;
  pushed filament nodes aren't checked against other surfaces.
- `comparttranslate` still has the round-off problem that `surfsweepmols`
  fixes with `SWEEPCROSSTOL` (see the mistakes section); it was left unchanged
  so `translatecmpt` results stay the same.
- Contact forces are explicit, so they're limited by mobility × k × time step
  even with Implicit dynamics. Their maximum is k_eff times the contact
  distance, so a hard push can force one filament through another. Filaments
  crossed in 2D stay crossed, and contacts aren't found across periodic
  boundaries.
- Free nodes can get past a finite surface by going around its edge, which
  reflect correctly allows. Examples need surfaces larger than the region
  the filaments can reach.
- Adsorbed nodes aren't stopped by reflect interactions with other surfaces
  as they move within their own surface.
- Adsorbed nodes and rigid constraints (negative force constants) can
  conflict. Smoldyn warns about this.
- Adsorbed nodes move with surfaces that move from forces, but not with
  surfaces translated by commands such as `translatecmpt`. They don't exert
  forces on surfaces; only softwall and repel do.
- The adsorb interaction checks all panels of its surface for each free node,
  rather than using the virtual boxes.
- `filCopyFilament` doesn't copy node mobility factors or adsorption, and the
  FIL lines from `printFilaments` don't show adsorption.

## Notable mistakes during the session

- A scripted edit matched a function prototype instead of its definition and
  deleted much of `smolfilament.c`. It was restored from a backup identical
  to HEAD, and the edit was reapplied. Lesson: anchor scripted edits on
  unique text, and diff the list of functions against HEAD afterwards.
- The first constraint projection used gradients at the projected point,
  which biased the ensemble. It was switched to start-of-step (SHAKE)
  gradients.
- The help generator does not render `\sqrt`, so the formulas in statement
  descriptions are written in plain text.
- Claude first said that rebuilding the box lists every step would cost
  little compared with the dynamics. That was wrong for simulations with
  dynamics `none`, which is why the lists are now only built when needed.
- A commit command chained several heredocs, and the shell ran two commit
  subject lines as commands, so those commits have no subjects. Lesson:
  write each commit message with its own command, or use `-m` arguments.
- The HEAD build used for regression wasn't rebuilt after the rename commit,
  so it couldn't read the new `reflect` keyword, and four examples showed false
  differences. Lesson: rebuild the HEAD binary at the current commit before
  each regression.
- Midway through the session, many scratchpad files (scripts, test inputs,
  and the HEAD and ASan builds) were deleted, and one regression run then
  compared against a missing binary. Everything was rebuilt before results
  were reported. Lesson: check that the binaries exist before trusting a
  regression run.
- A test filter used `\|` with macOS `sed`, which doesn't support it, so the
  filter silently did nothing. `grep -E` was used instead.
- The adsorb test first found nodes below a membrane and looked like a reflect
  bug; the nodes had gone around the edge of the finite membrane.
- A scripted edit of a comment block found its end marker at the function's
  prototype, which comes before the comment. The slice came out empty, and
  replacing an empty string inserted the new text between every character of
  `smolfilament.c`. The file was restored with `git checkout`, since the
  previous work was committed, and the edit was redone with asserts that each
  match is unique and in order. This is the same lesson as the first mistake.
- The code documentation has an older undefined reference,
  `sect:FilMathBending`, in the `filFlatToWnodes` entry. It wasn't fixed.
- In the first moving-lid tests with larger mobilities, nearly every
  molecule the lid pushed ended up behind it. With moves of about 10⁻⁵,
  round-off made `checksurfaces1mol` count the moved panel itself as a new
  crossing, so the molecule reflected back through it. `surfsweepmols` now
  starts that check `SWEEPCROSSTOL` (10⁻⁶ of the move) past the panel.
- One regression run overlapped a rebuild of the binary it was testing, so
  its results were thrown away and it was rerun. Lesson: don't rebuild while
  a regression is running.
- A test run was given Smoldyn's `-w` flag, which suppresses warnings, so it
  briefly looked warning-free. It was rerun without the flag.
- The first draft of `movingSurface2D` placed bound molecules along the whole
  lid, which is wider than the system, giving 10 "outside system volume"
  warnings. The lid was split into three panels and the molecules put on the
  middle one.
- The first `barbed` test compared with the formula using the default kT, which
  gives new segments random lengths and angles, so the filament grew about 5
  times too slowly in height. Setting `kT 0` fixed the test. The minus-end
  comparison also had a sign error in the test script, not in Smoldyn.
- One regression run stalled because stdin stayed open, so examples with `cmd B
  pause` waited for Enter until their 120-second limits. Lesson: run these with
  `</dev/null`, and check CPU use when a run seems slow. Earlier runs had
  finished with status 0, so they were unaffected.
- The contact design put forces in `filwork`, which filaments without
  dynamics don't have, though they need to act as obstacles. The forces moved
  to the filament structure.
- A long scripted edit stopped at an anchor that didn't match, after applying its
  earlier edits. Saving the script to a file made the rest easy to apply.
- Results at the two smallest box sizes differed in 2D. A temporary brute-force
  check found no missed pairs. The cause was crossed segments, whose separation
  is zero only up to round-off, so the `dist<=0` test missed them and they got
  a full force in a random direction. `FILCONTACTMIN` fixed it. Lesson: don't
  dismiss a difference as round-off without measuring it.
- `cmd e` runs every step, not at the end, which printed about 600 lines of
  test output. `cmd A` was the intended command.
- The first `filamentContact2D` run never reached the contact distance, so it
  didn't show the force. The filaments were moved closer together.
- `SmoldynManual.pdf` showed as modified at 13:08 on Oct 30, after it had been
  committed, and the cause wasn't found. The next regeneration replaced it.
