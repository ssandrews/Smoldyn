# Claude Code session summary: Smoldyn filaments, October 12–20, 2026

This file summarizes a long Claude Code session (Claude Opus 5.5) with the
Smoldyn maintainer. Most of the work was on the filament code in
`source/Smoldyn/smolfilament.c`. There were also small changes to
`smoldyn.h`, `smolcmd.c`, `libSteve/RnSparse.c`, the user's manual
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
  files, example output, `.so` files, doxygen output, `.rst` manuals, and
  the rebuilt PDFs.
- Every change updated the manual and the code documentation. The help text
  was regenerated with `python3 scripts/make_smolhelp.py`.
- Verification:
  - Regression of all S13 examples against a build of HEAD, made in a
    scratchpad git worktree, comparing the `printFilaments` output.
  - AddressSanitizer sweeps over every example and integrator.
  - Targeted physics tests: energy dissipation, Boltzmann variances,
    diffusion ratios, and fixed nodes.
- `/usr/local/bin/smoldyn` can lag behind `build/` until `sudo make install`
  is run.

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

## Known limitations (documented in the manual)

- Rigid constraints give rigid-ensemble statistics. Bending variance is about
  5–10% above kT/k, compared with a stiff-spring model.
- `betterGraphics3D` (negative constants) is still too stiff for the
  explicit methods. Use Implicit.
- ImplicitOld can diverge in stiff, thermal 3D systems.
- Loop-closing junctions have small gaps under load. These shrink as
  `junction_force` is increased.
- Frame transport still has some `atan2` roll overhead.

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
