# Proposal: thermally constrained branch junctions

Branch: `akamatsu/filament-capabilities` (continues after tag `filament-elongation-capping-v1`)
Files to touch: `source/Smoldyn/smoldyn.h`, `source/Smoldyn/smolfilament.c`
New example: `examples/S13_filaments/branchingJunction3D.txt`

Status: **implemented, validated, and reviewed** (eight commits off
`filament-elongation-capping-v1`, including two review passes — a four-angle cleanup
review and an eight-angle correctness review whose findings and fixes are in the §9
commit list; tagged `filament-thermal-junction-v1` on `akamatsu/filament-capabilities`). Validation
results are inline in §7; regression: byte-identical to the previous tag for kT 0
filament models and molecule models, all 22 S13 examples pass. **One design revision
made during validation:** the junction couple is two-sided, not one-way — measurement
showed the one-way version overheats the junction angle when the mother is mobile
(§3.2 tells the story). Three stock-engine findings surfaced along the way (§7.5).
Official suites run 2026-08-27: pytest 114 passed / 3 failed and CTest 33/44 — with
**failure sets identical on a stock baseline build**, so the branch introduces zero
new failures. Remaining: curate onto the accumulating branch + tag, and the Steve
conversation (§9 questions). This
is roadmap item 1 from `actin-branching-proposal.md` §6 ("angle-holding junction, v2
mechanics"), with a measured parameter set, an exact energy form, and the
thermal-force correction the junction's validation depends on.

---

## 1. TL;DR

v1 branching pins the branch **point** but not the branch **angle**: `filPinBranches`
translates each daughter so its node 0 sits on the mother's branch point, and nothing
restores the ~70–77° junction geometry once `dynamics` flexes the network
(`actin-branching-proposal.md` §5.2). This unit makes the junction mechanically real, and
does it with a measured stiffness rather than an invented one:

1. **A junction torsional spring** with energy `E = ½·κ·(θ − θ₀)²`, where θ is the angle
   between the mother segment's tangent and the daughter's first-segment tangent. One new
   filament-type parameter, `branch_force_angle` (= κ, energy·rad⁻²; default 0 = off, so
   existing behavior is untouched). The positional pin stays exactly as it is.
2. **The measured value.** Blanchoin et al. 2000 (Nature 404:1007–1011, Fig 2e–n) measured
   the Y-junction's rotational spring constant by equipartition from thermal angular
   fluctuations: **κ = 7.6 × 10⁻²⁰ J·rad⁻² = 76 pN·nm·rad⁻²** for bovine Arp2/3
   (1.3 × 10⁻¹⁹ J·rad⁻² for *Acanthamoeba*), with mean branch angle **77 ± 13°** (bovine,
   in vitro). Equipartition ties the two numbers together: `κ·⟨δθ²⟩ = k_BT` reproduces the
   13.1° spread exactly. **Stiffness and angular spread are one measurement, not two** —
   which fixes how `branch_spread` should be set (§3.4).
3. **A fixed branch azimuth.** The molecular model is a stereospecific slot: Arp2/3
   docks a defined footprint on the helical mother, so the daughter's direction *around*
   the mother axis is set by the docking site, not free to diffuse. Smoldyn can honor
   this where Cytosim could not — Cytosim fibers are point-chains with tangents only,
   while Smoldyn segments carry a full material frame (quaternions plus an integrated
   per-segment roll state). v2 records each junction's birth azimuth and holds it by a
   **rigid constraint** folded into `filPinBranches` (§3.6); a defined-at-birth azimuth
   parameter comes along nearly for free, while helical-registration of birth azimuth is
   explicitly deferred (§8).
4. **A fluctuation–dissipation fix to `filAddThermalForces`.** The current amplitude
   (`frms = sqrt(kypr[0]*kT)/stdlen`) carries the in-code comment *"This equation is
   almost certainly incorrect"* — it has no `dt` and no mobility in it, so the sampled
   ensemble is not at temperature `kT`. Since everything this unit promises (the ±13°
   spread, and any later persistence-length claim) is a *fluctuation* quantity, the fix is
   a prerequisite, not an optional cleanup (§3.5).

The junction spring is deliberately **one-way** (torque on the daughter, mother treated as
an external field) and enters through `filComputeForces`, so every integrator that
differentiates forces numerically — euler, RK2, RK4, implicit — gets it for free with no
cross-filament solver (§3.2).

---

## 2. The physics: exact energy form and its single source

### 2.1 Energy

For each recorded junction (mother `m`, branch spot segment `s`, daughter `d`):

```
E_junction = ½ · κ · (θ − θ₀)²

θ  = angle between t_m and t_d, in [0, π]
t_m = unit tangent of mother segment s        (xyzback − xyzfront)/len
t_d = unit tangent of daughter segment 0      (node 1 − node 0)/|node 1 − node 0|
θ₀ = branch_angle   (existing parameter)
κ  = branch_force_angle   (new parameter, energy·rad⁻²)
```

Using the unsigned angle handles 2D's ±θ₀ branches automatically (a daughter on either
side is restored toward its own side). In 3D this energy constrains the **polar** angle
of the dendritic cone; the **azimuth** φ — the daughter's direction around the mother
axis, measured in the mother segment's material frame — is handled separately, as a
constraint rather than an energy (§3.6), because it is molecularly a slot, not a
measured compliance.

### 2.2 The measured parameter set (Blanchoin et al. 2000)

| Quantity | Bovine Arp2/3 | *Acanthamoeba* | How measured |
|---|---|---|---|
| Mean branch angle θ₀ | **77°** (1.344 rad) | — | thermal fluctuations of rhodamine-phalloidin-labelled branches, in vitro |
| Angular s.d. σ | **13.1°** (0.229 rad) | 10.1° | same image series |
| Rotational spring κ | **7.6 × 10⁻²⁰ J·rad⁻²** = 76 pN·nm·rad⁻² | 1.3 × 10⁻¹⁹ J·rad⁻² | equipartition, `κ⟨δθ²⟩ = k_BT`, with k_BT = 4 × 10⁻²¹ J |

Self-consistency: `σ = sqrt(k_BT/κ)` = 13.1° (bovine) and 10.1° (amoeba) — both reported
standard deviations are recovered from the two spring constants, i.e. the spread **is**
the stiffness read through equipartition. The authors' own conclusion is the relevant one
for us: the junction is stiff enough to be compatible with the elastic Brownian ratchet.

**The portable, unit-free form.** Smoldyn is unit-agnostic, so the number to preserve is
the dimensionless ratio

```
κ / k_BT ≈ 19 rad⁻²        (bovine; 32.5 for amoeba)
```

In any consistent unit system: `branch_force_angle = 19 × kT`. In pN·µm units
(kT = 4.1 × 10⁻³ pN·µm at 25 °C): κ = 7.6 × 10⁻² pN·µm·rad⁻².

### 2.3 Why 77° and not the engine's 70° default

The engine default `branch_angle` = 70° matches the textbook number and should stay (it
is a default, not a claim). For the in vitro branched-network application this branch is
aimed at (WAVE1/bovine-Arp2/3 reconstitution assays), the defensible number is the
**in vitro bovine** measurement — 77° — from the *same* measurement that supplies κ.
Cellular cryo-ET values run lower (68–71°), but they are a different context. This is a
config-file choice, not an engine change.

### 2.4 The junction spring is the *soft* element — no new timestep restriction

Per-joint bending stiffness at the discretizations we use is much stiffer than the
junction. For a discrete worm-like chain, `force_angle` per joint ≈ L_p·kT/standard_length;
with L_p = 10 µm and 5 nm segments that is ≈ 2000·kT·rad⁻², i.e. **~100× stiffer** than
κ = 19·kT. Two consequences:

- The junction adds **no new stability constraint**: any `dt` stable for the bending
  forces is stable for the junction (relaxation rate μ·κ/L² vs μ·force_angle/L²).
- The junction is the compliant element at the node scale, exactly as in the source data
  (there, µm-long arms made the *filaments* the floppy element; at nm discretization the
  joint stiffnesses invert, but the junction κ is the same physical object either way).

---

## 3. Design decisions, and why

### 3.1 Keep the rigid positional pin; add only the torsional spring

The roadmap sketch said "stiff harmonic tether to the mother branch point plus a
torsional spring." This spec **drops the tether half**: `filPinBranches` already holds
the branch point *exactly* (it translates the daughter after the mechanical step), it is
validated, and it costs nothing. Replacing it with a stiff spring would introduce a large
spring constant whose only job is to approximate the constraint we already enforce
exactly — and *that* spring, unlike κ, **would** set the timestep. So: position by
constraint (unchanged), angle by force (new). The two compose because the pin is a pure
translation, which does not change θ.

### 3.2 Two-sided half-couples: each filament applies its own share, no cross-filament writes

`filComputeForces` **clears the filament's whole force array at the start of each
per-filament evaluation** (smolfilament.c, filComputeForces). A reaction force written
into the mother's array from the daughter's pass would be wiped whenever the mother is
(re)evaluated after the daughter — and the numerical-Jacobian path
(`filComputeDerivForceMat`) re-evaluates single filaments constantly. So the junction
force must not write across filaments.

The original draft concluded from that constraint that the coupling should be
**one-way** (daughter feels the mother as an external field; mother feels nothing),
arguing the equilibrium distribution of θ would still be exact. **That argument is
wrong for a mobile mother, and validation caught it:** with the mother pinned, the
one-way spring gave exact equipartition, but with a free mother the junction angle
receives thermal noise through *both* partners' tangents while feeling drag through
only the daughter's — its stationary variance is `kT/κ · (1 + μ_mother/μ_daughter)`,
measured at ~3× kT/κ for a short free mother, and worse in a young network where the
inflation cascades down the branch tree.

The implemented design keeps per-filament isolation **and** action–reaction: each
filament applies **its own half of the couple during its own force evaluation**,
reading the partner's current geometry as an external field. A daughter applies the
exact gradient of E with respect to its nodes 0 and 1; a mother loops over its
branches and applies the exact gradient with respect to its nodes `spot` and `spot+1`.
Both halves are pure torques about the branch point and sum to zero total torque, so
the shared energy enters both partners' dynamics and θ equilibrates at Boltzmann — the
free-mother test then lands on kT/κ within sampling error (§7). No cross-filament
force writes occur, the clearing hazard never arises, and the Jacobian sparsity
assumption (force at a node depends only on nearby nodes *of the same filament*) still
holds, since each half-couple depends only on its own filament's nodes plus external
data.

The residual approximation is the integrators' sequential (Gauss–Seidel) sweep: a
filament evaluated later in a substep reads its partner's already-moved geometry — an
O(dt) splitting error of the same class as the `filPinBranches` ordering already
documented in `actin-branching-proposal.md` §5.3.

### 3.3 Exact force: a couple on daughter nodes 0 and 1

With `t_d = (r₁ − r₀)/L` and `θ = acos(t_m·t_d)`, define the in-plane unit vector

```
p̂ = (t_m − cosθ · t_d) / sinθ          (⊥ t_d, pointing toward t_m)
```

Then the exact gradient of E with respect to the daughter node positions gives

```
F₁ = + κ (θ − θ₀) / L · p̂       applied to daughter node 1
F₀ = − F₁                        applied to daughter node 0
```

(derivation: ∂θ/∂r₁ = −p̂/L since t_m − cosθ·t_d is already perpendicular to t_d).
This is a **pure couple**: zero net force on the daughter, torque −κ(θ−θ₀) about the
branch point, sign such that θ > θ₀ is pulled closed and θ < θ₀ pushed open. Node 0's
share is absorbed by the pin, harmlessly. Longer daughters need no extra terms — the
angle is defined by segment 0 only, and bending forces propagate the correction down the
chain, which is the same locality assumption the engine already makes for `force_angle`.

The mother's half is the mirror image: with `q̂ = (t_d − cosθ·t_m)/sinθ`,

```
F_back  = + κ (θ − θ₀) / L_m · q̂     applied to mother node spot+1
F_front = − F_back                    applied to mother node spot
```

— again a pure couple, and the two couples' torques cancel exactly (the energy is
invariant under a global rotation of the pair).

**Guard:** if `|sinθ| < ε` (θ near 0 or π) the plane is undefined; skip the force that
evaluation. With θ₀ = 77° and σ = 13°, θ near 0/π is a >5σ excursion, so the guard is a
numerical safety, not a modeling decision.

**Degenerate junctions:** skip if the branch spot is no longer a valid segment index or
the daughter has fewer than 1 segment (the `filArrayShift` branch-drop path already
removes junctions the mother has turned over past).

### 3.4 Birth sampling must match the equilibrium the spring maintains

`branch_spread` currently jitters θ at birth, independently of any mechanics. Once κ
exists, the birth distribution and the equilibrium distribution should be the same
Boltzmann distribution, or every branch is born displaced from its own steady state and
relaxes visibly during its first ~ζ/κ:

```
branch_spread = sqrt(kT / branch_force_angle)        (= σ from equipartition)
```

Proposed behavior: when `branch_force_angle > 0` **and** the user has not set
`branch_spread` explicitly, derive it as above at `filCheckParams` time (log at display
level). If the user set both and they disagree by more than ~2×, warn — it almost
certainly means the config mixes two sources. No hard error: deliberately mismatched
values are a legitimate numerical experiment (e.g. relaxation-time measurement).

### 3.5 The thermal-force fix is a prerequisite, not a companion

The current amplitude in `filAddThermalForces`:

```c
frms = sqrt(kypr[0]*kT)/stdlen;      //?? This equation is almost certainly incorrect
/* components drawn as 2*frms*gaussrandD() each */
```

has no `dt` and no mobility, so it cannot satisfy fluctuation–dissipation for the update
rule `x += dt·mobility·nodemobility[n]·F` (`filStepDynamics`). The realized temperature
works out to `kT_eff = kT · (2·mobility·dt·kypr[0]/stdlen²)` — correct only on a
constraint surface linking four parameters that the user is free to choose independently.

The FDT-correct amplitude for an overdamped Euler step is, per Cartesian component of
node n:

```
σ_F = sqrt( 2·kT / (mobility·nodemobility[n]·dt) )
```

(equivalently: displacement noise of std `sqrt(2·kT·mobility·nodemobility[n]·dt)`).
Notes for implementation:

- The per-**node** mobility belongs inside the per-node amplitude (nodes with mobility 0
  — pinned nodes — correctly receive no thermal kick).
- The existing `thermtime` freshness logic (recompute once per `sim->time`, reuse across
  sub-evaluations within a step) is exactly right for RK and Jacobian re-evaluations and
  is kept.
- For RK2/RK4 this makes the noise enter as a constant force over the step — standard
  practice for weak-order-1 stochastic integration; not claiming higher weak order.
- **This changes trajectories for every existing config with `kT > 0`.** It cannot be
  byte-identical to master. It is packaged as its own risk-ordered commit at the front of
  the unit, framed as the answer to the in-code `??` comment, with its own validation
  (§7, T2). Whether upstream wants it gated behind a legacy toggle is a maintainer call —
  question 1 in §9.

Without this fix, "thermally constrained" is not a claim the engine can make: the ±13°
target would be validated against an ensemble at an accidental temperature.

### 3.6 Azimuth: fixed by constraint, not by spring

**The molecular case.** The Arp2/3 branch junction is stereospecific — the complex
docks a defined footprint spanning adjacent subunits on the mother's helical lattice
(the branch-junction subtomogram average, Fäßler 2020, and subsequent junction
structures fix the full geometry, not just the 70° polar angle). So the daughter's
azimuth around the mother axis is set by *which slot* Arp2/3 occupies, and once formed
it should not diffuse. Blanchoin 2000 could not see this — the measurement is of
surface-confined, in-plane fluctuations — which is why κ constrains θ only.

**Why the engine can do this (and Cytosim couldn't).** Cytosim fibers have no material
frame — points and tangents only — so "azimuth relative to the filament" is not even
representable there; leaving it free was forced. Smoldyn segments carry full
orientation quaternions (`qrel`/`qabs`), a per-segment integrated **roll** state
(`fil->roll[]`, stepped from torques in `filStepDynamics`), and a base-frame up-vector
(`seg0up`). The mother's `qabs` at the branch segment is therefore a well-defined
material frame, and φ (azimuth of the daughter tangent around the mother tangent,
measured against that frame) is well-defined at every step.

**Three options considered — and the choice reversed by network-scale measurement:**

1. **Rigid azimuthal pin** (`branch_azimuth_fix`): record φ₀ at birth; each step,
   rotate the daughter rigidly about the mother-tangent axis back to φ₀. Exact and
   θ-preserving per rotation — and it validates perfectly on a single junction with a
   pinned mother (zero drift, clean equipartition). **But in a thermal network it is
   pathological:** the mother's tangent is itself a thermalized degree of freedom, so
   the pin is an infinitely stiff constraint slaved to thermal wobble — every
   fluctuation rigidly swings the entire downstream subtree, pumping energy into the
   network. Measured: junction polar spreads inflate from the equilibrium 6.6° to
   13.5° in a 147-junction 1 s network, and get *worse* (26.8°) at half the timestep —
   the signature of constraint-driven pumping, not integration error — while turning
   azimuth handling off restores 7.0°. Retained for static / pinned-mother uses;
   `filCheckParams` warns when combined with thermal dynamics.
2. **Azimuthal spring (chosen for thermal networks)** — `branch_force_azimuth`,
   E = ½κ_az(φ−φ₀)², exact-gradient couple on daughter nodes 0/1
   (∂φ/∂r₁ = (t_m×t_d)/(L sin²θ)). Originally rejected because κ_az is unmeasured;
   chosen now for a better reason: **a bounded spring torque cannot pump the way a
   rigid constraint can.** One-way by design (a mother-side gradient would require
   differentiating the recursively built material frame); the cost is a bounded
   inflation of the azimuth spread only — acceptable for a DOF with no measured
   stiffness. At κ_az = 10× the polar κ ("slot-stiff", σ_az ≈ 2°), the 1 s dense
   network holds polar angles at **77.7 ± 6.8°** against the 77 ± 6.6 prediction.
3. **Free azimuth (Cytosim parity).** The default, for backward compatibility and as
   the explicit comparison arm against prior art.

**What "fixed" means for fluctuations.** The pinned azimuth inherits only the mother's
twist fluctuations — and since `filAddThermalForces` applies no thermal torques to the
roll degree of freedom (that path is commented out upstream), pinned azimuths are
essentially noiseless: the rigid-slot limit. That is consistent with the molecular
model. If roll is thermalized later, azimuth correctly inherits mother-twist noise, and
`kypr[2]` acquires a physical anchor (F-actin torsional rigidity is measured, ~10⁻²⁶
N·m² scale).

**Consistency for grand-daughters.** Daughters are themselves mothers, and their
junction pins reference *their* material frames — so the rigid rotation must rotate the
daughter's node positions **and its `seg0up` vector together**, then `filNodes2Angles`,
so the daughter's own frame co-rotates and deeper pins stay consistent.

### 3.7 What stays out of the energy (and why)

- **No mother-side bending compliance at the branch spot.** The mother's own `force_angle`
  already governs its local bending; the junction κ is a *relative-angle* spring only.
- **No force-dependent debranching.** The junction force now computes −κ(θ−θ₀) every
  step, which is exactly the input a future load-dependent debranching rate needs
  (Pandit-style force sensitivity) — noted as a hook, not implemented.

---

## 4. Proposed configuration surface

```
branch_force_angle <kappa>    # junction torsional spring constant, energy/rad^2.
                              # 0 (default) = off: junctions are position-pinned only,
                              # byte-identical to filament-branching-v1 behavior.
branch_azimuth <phi>          # birth azimuth, radians, in the mother segment's material
                              # frame. Unset (default) = uniform random on [0,2pi),
                              # exactly the current behavior.
branch_azimuth_fix <0|1>      # 1 = hold each junction's azimuth at its recorded birth
                              # value by rigid constraint (§3.6). 0 (default) = free.
                              # For static / pinned-mother models; warned under
                              # thermal dynamics (energy-pumping, §3.6 option 1).
branch_force_azimuth <k_az>   # azimuthal spring toward the recorded birth azimuth,
                              # energy/rad^2; 0 (default) = off. The recommended way
                              # to hold azimuth in thermal networks; suggested value
                              # ~10x branch_force_angle ("slot-stiff").
```

Semantics and checks:

- All three apply to **every recorded junction** of the type (Poisson-nucleated and
  manual `branch`-command junctions alike); the spring and pin are gated on
  `nbranch > 0`, not on `branch_rate > 0`.
- `branch_azimuth_fix` holds whatever azimuth the junction was *born with* — so it
  composes with either random or defined birth azimuth. The two knobs are independent:
  random-birth + fixed (marginalizing over unresolved subunit register, then slot-rigid)
  is the recommended actin configuration until helical registration exists (§8).
- When `branch_azimuth` is set, the daughter's birth **spin** (its own roll about its
  axis) is also made deterministic (0) rather than random — a stereospecific slot
  defines both. Note this removes two RNG draws per nucleation, changing the draw
  sequence only for models that opt in.
- `filCheckParams`: `branch_force_angle` ≥ 0; if > 0 and `branch_spread` unset → derive
  per §3.4; if > 0 and `branch_spread` set inconsistently → warn. In **2D**, azimuth is
  the ± side, already fixed by birth and topology: `branch_azimuth`/`branch_azimuth_fix`
  draw a warning and are ignored.
- `simLog` display block: print κ, azimuth mode, and fix flag alongside branch
  rate/angle/spread, plus the derived σ.

Task 3.1 reference values (pN·µm·s units): `kT 4.1e-3`, `branch_angle 1.344`,
`branch_force_angle 7.6e-2` (giving derived `branch_spread 0.229`).

---

## 5. Proposed implementation

### 5.1 New state

`filamenttypestruct`: three new fields — `double branchforceangle;` (default 0),
`double branchazimuth;` (default flagged-unset = random), `int branchazimuthfix;`
(default 0) — next to the existing `branchrate/branchangle/branchspread/branchsegments`
block.

The polar spring needs no per-junction state (mother, spot, daughter are already
recorded via `branchspots[]`, `branches[]`, `frontend`). The azimuth pin does: **a new
per-junction array `double *branchazim0` on `filamentstruct`**, parallel to
`branchspots[]`, recording each junction's realized birth azimuth φ₀ (whether drawn or
set). It must be maintained everywhere `branchspots[]` is: the `filAlloc` growth path
(**reassign the new pointer** — this exact path has produced two latent bugs before),
`filArrayShift` (shift in lockstep; drop with dropped branches), `filCopyFilament`, and
`filAddBranch` (record at creation). This is the first per-junction `double` state in
the module and the real cost of the azimuth feature.

### 5.2 New function: `filAddJunctionForces(fil, nodemin, nodemax)`

Called from `filComputeForces` after `filAddThermalForces`; both roles of the current
filament are handled in one pass (a filament can be daughter and mother at once):

```
if branchforceangle == 0: return
# daughter side (fil->frontend set): find own record on the mother
#   (linear scan of mother->branches; nbranch is small), guard the spot,
#   compute shared geometry (filJunctionGeometry), apply the couple to
#   own nodes 0 and 1 per §3.3, subject to the node window.
# mother side: for each of fil's branches with a valid spot and daughter,
#   compute the same geometry and apply the reaction couple to own nodes
#   spot and spot+1, subject to the node window.
```

Neither side writes to the partner's force array (§3.2), and the `nodemin/nodemax`
windowing keeps `filComputeDerivForceMat`'s node-local re-evaluations correct: each
half-couple contributes to its own filament's numerical Jacobian automatically, so
Jacobian-based integrators would support the junction with zero extra work (both stock
implicit integrators currently NaN on plain filaments — §7.5 — so this is future-proofing,
not a tested path). The one integrator that structurally cannot see it is `eulermat`
(analytic force matrix has only stretch + bend) — same status as the thermal force
there; documented, not fixed.

Ordering caveat (accepted): integrators sweep filaments sequentially, so a filament
evaluated later in a substep reads its partner's already-moved geometry — O(dt)
splitting, same class as the `filPinBranches` ordering caveat.

### 5.3 The azimuth pin: birth and hold

**Birth** (`filBranchDynamics` / manual `branch`): if `branchazimuth` is set, use it for
the cone azimuth and 0 for the spin instead of the two `unirandCOD(0,2π)` draws;
either way, record the realized azimuth into `mother->branchazim0[br]` in
`filAddBranch`.

**Hold** (`filPinBranches`, after the existing position pin, gated on
`branchazimuthfix` and 3D): for each junction,

```
t_m from mother->segments[spot]; (ŷ_m, ẑ_m) from mseg->qabs   # material frame
e  = t_d − (t_d·t_m)·t_m;   guard |e| > ε                     # daughter tangent ⊥ t_m
φ  = atan2(e·ẑ_m, e·ŷ_m)
rotate daughter rigidly by (φ₀ − φ) about the axis (branch point, t_m):
    node positions AND seg0up, then filNodes2Angles(daughter)
```

Rotating `seg0up` with the nodes keeps the daughter's own material frame consistent, so
grand-daughter junctions pinned to *that* frame stay valid (§3.6). The rotation
preserves θ exactly, so it composes with the polar spring without coupling. Same
arbitrary-order caveat for deep trees as the position pin (settles over a few steps
after large moves).

### 5.4 The thermal-force fix

In `filAddThermalForces`, replace the amplitude with the per-node FDT form (§3.5):
`sigmaF_n = sqrt(2·kT/(filtype->mobility · fil->nodemobility[n] · sim->dt))`, drawn once
per node per step under the existing `thermtime` gate. `kypr` and `stdlen` drop out of
the expression entirely. Guard `nodemobility[n] == 0` by zeroing the *amplitude*, not by
skipping the draw.

**RNG-sequence rule:** keep the draw structure exactly as it is — always call
`gaussrandD()` per component per node, multiply by the (possibly zero) amplitude. The
current code already draws even when `frms = 0`, so this preserves the global draw
sequence and keeps every `kT 0` model byte-identical across the change (the regression
anchor in T3). Only amplitudes change, never the number or order of draws.

### 5.5 Wiring checklist

- `smoldyn.h`: three type fields + the per-junction `branchazim0[]` array.
- `filtypeSetParam` entries; parser words `branch_force_angle`, `branch_azimuth`,
  `branch_azimuth_fix`; `filCheckParams` rules; `simLog` output (mirror the
  `branch_rate` block).
- `filAlloc` / `filArrayShift` / `filCopyFilament`: maintain `branchazim0[]` in
  lockstep with `branchspots[]` (reassign the grown pointer!).
- `filComputeForces`: add the `filAddJunctionForces` call.
- `filPinBranches`: azimuth-hold pass (§5.3).
- `filBranchDynamics` / `filAddBranch`: birth azimuth/spin handling + recording.
- `filAddThermalForces`: new amplitude.
- New example `branchingJunction3D.txt`: small dendritic network, `dynamics euler`,
  junction spring on, `random_seed` set (directory convention — branching models grow
  exponentially), `printFilaments` dump for the angle-statistics script.
- `CHANGES.md` (handoff): validation numbers when they exist.

---

## 6. Physical calibration summary

| Parameter | Physical value | pN·µm·s units | Source |
|---|---|---|---|
| `branch_angle` | 77° (in vitro, bovine) | 1.344 rad | Blanchoin 2000 |
| `branch_force_angle` | 76 pN·nm·rad⁻² | 7.6 × 10⁻² | Blanchoin 2000, equipartition |
| `branch_spread` | 13.1° | 0.229 rad (derived, §3.4) | same measurement |
| `kT` | 4.1 pN·nm (25 °C) | 4.1 × 10⁻³ | — |
| dimensionless invariant | κ/kT ≈ 19 rad⁻² | — | portable across unit systems |

Amoeba alternative: κ = 130 pN·nm·rad⁻² (κ/kT ≈ 32.5, σ = 10.1°) — useful as a
stiffness-sensitivity arm in any sweep.

Azimuth settings for actin configs: `branch_azimuth` unset (random birth = correct
marginal over subunit register, §8.2) + `branch_azimuth_fix 1` (slot-rigid thereafter,
§3.6). No literature number is needed — the hold is a constraint, not a stiffness.

---

## 7. Validation plan

**T1 — junction equipartition (the headline test).** One mother (held by
`nodemobility 0` on its ends), one daughter, no growth, `dynamics euler`, junction spring
on. Long run; measure θ(t) from `printFilaments` dumps. Pass criteria:
⟨θ⟩ = θ₀ (3D: expect a small +σ²·cotθ₀ ≈ +0.7° shift from the sinθ measure — predicted,
not a bug) and Var(θ) = kT/κ within sampling error. Then the FDT cross-check: repeat at
2× and ½× `dt`, 10× mobility, and 2× `standard_length` — **Var(θ) must not move.** Under
the current thermal force it would scale linearly in `mobility·dt/stdlen²`, which is the
before/after demonstration for §3.5.

**T2 — thermal fix on a plain filament (no branching).** Free filament,
equipartition of bend fluctuations / tangent-correlation decay against the discrete-WLC
prediction L_p = force_angle·standard_length/kT, across the same dt/mobility sweep.
This validates the fix independently of the junction, on the observable (persistence
length) that the actin configs actually care about.

**T3 — regression.** `branch_force_angle 0` **and** `kT 0`: byte-identical to
`filament-elongation-capping-v1` (thermal force is zero in both, junction force off).
With `kT > 0` byte-identity is impossible by design (§3.5); instead re-run the standard
examples and confirm statistical observables (mean lengths, molcounts) are unchanged
where they should be, and report the ones that legitimately move.

**T4 — dendritic network under dynamics (the v1 gap, closed).** Re-run the
branching + elongation + `dynamics` protocol that motivated this unit: junction-angle
distribution over the whole network, fixed seed. v1 measured junctions decorrelating
under flexing; pass = distribution holds at θ₀ ± sqrt(kT/κ) (77 ± 13° at the reference
parameters) at steady state, n ≥ ~1000 junctions.

**T5 — stiff-limit sanity.** κ × 100: σ shrinks 10×, no instability at the `dt` set by
the bending forces (per §2.4 the junction should never be the binding constraint —
verify, don't assume).

**T6 — azimuth hold.** Dendritic network under `dynamics`, fixed seed, azimuth pin on:
per-junction |φ − φ₀| stays at numerical tolerance over the whole run, θ statistics
unchanged from T4 (the pin must not perturb the polar ensemble — this is the
orthogonality claim of §3.6 made checkable), and grand-daughter junctions hold too
(frame consistency). With the pin off, the same run reproduces azimuthal diffusion
around the cone — the Cytosim-parity arm, and the before/after figure.

### 7.1 Results (2026-08-27, configs in the notebook's `validation/thermal-junction/`)

Expected equilibrium: Var(θ) = kT/κ = 0.001/0.076 = **0.013158 rad²** (σ = 0.1147);
sampling SE ≈ ±4.5% at n ≈ 990.

| Test | Setup | Result | Verdict |
|---|---|---|---|
| T-relax (kT 0) | euler, dt 2e-6, free mother | θ: 1.9 → **1.3440 exactly**, monotone, stays | pass |
| T1 pinned | euler, dt 5e-5 | Var 0.013613 | pass (+3.5%, ≈ predicted Euler bias λdt/2) |
| T1 pinned, dt/2 | euler, dt 2.5e-5 | Var 0.012875 | pass — **variance invariant in dt** (old thermal amplitude would halve it) |
| T1 pinned, 10× mobility | euler, dt 5e-6 | Var 0.014236 | pass — invariant in mobility |
| T1 free mother | euler, dt 2e-6, two-sided couple | Var **0.012576**, mean 1.3455 | pass — Boltzmann restored (one-way gave 0.0405) |
| T6 azimuth fixed | 3D, pinned mother, 100k steps | drift **0.00000 rad**; Var(θ) 0.013698; mean 1.3511 (matches +σ²·cotθ₀ 3D measure shift) | pass |
| T6 azimuth free | same, fix off | azimuth circular sd 1.65 rad (full cone); Var(θ) 0.013362 | pass — θ ensemble unperturbed |
| T4 network, 0.5 s | example config, 17 filaments, 14 junctions, full dynamics, azimuth pin | **79.5° ± 6.2°** vs predicted 77° ± 6.6° | pass (v1 position-only pin art: 91.6° ± 50.3°) |
| T4 network, 1 s, azimuth **pin** | 147 junctions | 81.9° ± **13.5°**; ±**26.8°** at dt/2 | **FAIL — exposed the pin pathology (§3.6)** |
| T4 network, 1 s, azimuth free | 143 junctions | 78.1° ± 7.0° | pass — isolates the pin as the cause |
| T4 network, 1 s, azimuth **spring** κ_az=0.76 | 136 junctions | **77.7° ± 6.8°** | pass — the shipped configuration |
| T6-spring, pinned mother | κ_az = 0.76, dt 5e-5 | azimuth sd 0.049 rad = √(kT/κ_az) × the predicted Euler-bias factor (λ_az·dt≈0.8); Var(θ) 0.01353 | pass |

Regression: fixed-seed **byte-identical** to `filament-elongation-capping-v1` for a
kT 0 branching + elongation + capping model and for a molecules + reactions model;
all 22 `S13_filaments` examples load and simulate. kT > 0 filament trajectories change
by design (the thermal fix). Official suites (2026-08-27, x86_64 Python 3.12 venv per
the Rosetta gotcha): **pytest 114 passed / 3 failed; CTest 33/44 — both failure sets
byte-for-byte identical to a stock baseline built at the branch point**
(test_connect_var ×2 and test_data_outputfile in pytest; the 11 documented
environmental CTest failures). Zero new failures from this branch.

### 7.5 Stock-engine findings surfaced by this validation

1. **Explicit integration of the bending forces has a hard dt ceiling** at common
   parameter scales: with `force_angle` 5, `standard_length` 0.01, `mobility` 1, a
   plain 3-segment chain with a 0.1 rad kink (no branching, kT 0) blows up at
   dt 1e-4 and saturates into a period-2 zigzag; stability needs roughly
   `dt ≲ standard_length²/(4·mobility·force_angle)`. Prior examples ran at rest, at
   dynamics none, or briefly — the constraint was always there, unexercised. Every
   quantitative junction run here chooses dt accordingly, and the example documents
   the formula.
2. **The implicit integrators are fragile-to-broken.** At actin-scale parameters
   (10 nm segments, `force_length 500`, `force_angle 5`) *both* NaN on the first step
   for a plain 3-segment chain that euler handles. At `test3D.txt`'s O(1) scale,
   ImplicitOld runs fine — but `FDimplicit` still explodes there (coordinates ~1e27 by
   t = 0.4). Relevant because implicit is the natural escape from finding 1.
3. **`FDimplicit` is unreachable from the parser** — every prefix of "implicit" also
   prefixes "implicitold", which is tested first; `dynamics implicit` resolves to
   ImplicitOld, and `relax2D/relax3D.txt`'s `dynamics implicit2` silently parses as
   dynamics **none**. We tried the one-line dispatch reorder and **reverted it before
   push**: it silently switched `test3D.txt` (and any user config saying
   `dynamics implicit`) from the working ImplicitOld onto the exploding FDimplicit.
   Since FDimplicit was never reachable, it has plausibly never run — it likely needs
   finishing against the current node mechanics before being made selectable. Reported
   here for your call; nothing on the branch touches the dispatch.

---

## 8. Explicitly out of scope (and where each lands)

1. ~~Reaction force on the mother~~ — **no longer deferred**: implemented as the
   mother's own half-couple after validation showed the one-way version overheats
   mobile-mother junctions (§3.2). The cross-filament force *pass* is still future
   work for excluded volume / surface contact, but the junction no longer needs it.
2. **Helical registration of birth azimuth.** The full molecular picture is that the
   slot azimuth is set by *which subunit* Arp2/3 docks on, and the actin helix rotates
   ≈166° per 2.75 nm subunit — so φ₀ should advance with contour position. Two things
   block it today: branch anchor points are quantized to segment boundaries
   (`mseg->xyzback`), and at 5 nm segments the helical phase advances ≈300° per segment
   — aliased. Routes when wanted: subunit-scale segments (2.75 nm `standard_length`,
   with the §3.4/§5.4 machinery making that affordable to check), sub-segment branch
   anchoring (with the §3.5 fix making dt/discretization sweeps trustworthy), or the
   half-wired `facetwist` face machinery. Until then, random birth
   azimuth is the *correct marginal* over unresolved subunit register — and the fix
   flag makes it slot-rigid thereafter, which is the physically load-bearing part.
3. ~~A harmonic dihedral spring~~ — **no longer out of scope**: implemented as
   `branch_force_azimuth` after the rigid pin proved pathological under thermal
   dynamics (§3.6). Still open: a *measured* κ_az, and the mother-side reaction for
   the azimuthal term (requires the material-frame gradient).
4. **Force-dependent debranching** — the junction force computed here is its natural
   input; separate unit.
5. **Load application (obstacle/boundary force on barbed ends)** — roadmap item 4;
   nothing here blocks it, and T1's held-mother setup is a template for it.
6. **Higher-weak-order stochastic integration** — noise-as-constant-force over the step
   is the deliberate scope (§3.5).
7. **Thermalized roll / mother counter-torque about its axis** — the azimuth pin
   remains one-way (the mother's roll feels no reaction from pinned branches); it is a
   rigid constraint, not an energy, so the equilibrium-distribution argument that
   forced the polar spring two-sided does not apply. Revisit together with
   thermalizing the roll degree of freedom, at which point `kypr[2]` gets set from
   measured F-actin torsional rigidity.

---

## 9. Effort, packaging, and questions for review

**Effort.** Modest: three type fields plus one per-junction array (with its four
lifecycle touchpoints), one ~60-line force function, one ~40-line pin pass, one
amplitude change, parser/log plumbing, one example. The work is in the validation runs
(T1/T2 sweeps, T6), not the code; the risk is concentrated in the `branchazim0[]`
lifecycle (the `filAlloc`/`filArrayShift` class of bug).

**Packaging (as built).** Working branch `akamatsu/thermal-junction` off
`filament-elongation-capping-v1`, six commits, risk-ordered; to be curated onto the
accumulating branch as one annotated tag (`filament-thermal-junction-v1`) after the
official suites run:

1. `36f3a45` Fix thermal force amplitude to satisfy fluctuation-dissipation.
2. `7fd9cb6` Add `branch_force_angle`: torsional spring at branch junctions.
3. `ee84c95` Add `branch_azimuth` and `branch_azimuth_fix`: stereospecific direction.
4. `c4726c8` Junction spring: apply the reaction couple on the mother (§3.2 revision,
   with the measured rationale in the commit message).
5. `dacd8fb` Example: `branchingJunction3D.txt`.
6. `84eb2f0` Add `branch_force_azimuth` (azimuthal spring); warn on the rigid pin
   under thermal dynamics — the §3.6 revision, found by running the network 2× longer
   (the pathology scales with tree depth and density).
7. `0247575` Review pass 1 (four-angle cleanup: derivation moved to `filUpdateParams`
   so all entry paths reach it, unset sentinel for `branch_spread`, mother-loop gate,
   shared wrap/degeneracy conventions, thermal-loop hoist).
8. `8f6d277` Review pass 2 (eight-angle correctness review: azimuthal-force bound
   `FILJUNCTSINBOUND`, stale-`branches[]` frontend guards for `copy_to`/re-branch,
   eulermat/implicitold and azimuth-only warnings, runtime-`set` condition drop,
   sentinel display, sign-contract and O(dt) documentation). A `FDimplicit` parser
   reorder that briefly lived on this branch was dropped here after it was measured to
   regress `test3D.txt` (§7.5, finding 3).

Known limitations added by review, documented rather than fixed: a daughter's own
front-segment turnover under `treadmill_rate > 0` jumps its azimuth reference
discontinuously (per-event twist artifact if azimuth holding is on — treadmilling
branched types is already the degraded path), and the azimuthal force cap
(`FILJUNCTSINBOUND`) means the spring loses authority below sinθ = 0.1, where azimuth
is barely meaningful anyway.

**Questions for Steve.**

1. **Thermal fix: replace outright, or gate behind a legacy toggle?** The equation was
   flagged wrong in your own source comment, but the fix changes every `kT > 0`
   trajectory, so it is not inert for existing users — the one part of this unit to
   agree on before it lands. Recommendation: replace outright, with the invariance
   evidence in §7.1 attached; a knowingly wrong noise amplitude is not an interface
   worth preserving.
2. **The implicit integrators (§7.5, findings 2–3):** both NaN on a plain filament.
   Is `filImplicitDynamics` finished? The bending-stiffness dt ceiling (finding 1)
   makes a working implicit path valuable for exactly the actin-scale parameters this
   unit targets — happy to help debug it as a follow-up if useful.
3. **Derived `branch_spread` (§3.4): silent derive + warn-on-mismatch, or require the
   user to set it explicitly?** Silent-derive is friendlier; explicit is more Smoldyn-ish.
4. **Per-junction state (§5.1): is a parallel `branchazim0[]` array acceptable**, or
   would you rather see the mother-side junction records consolidated into a small
   junction struct (spot, daughter, rest azimuth) before more per-junction fields
   accumulate? The parallel array matches the existing `branchspots[]` pattern; the
   struct is the better long-term shape if debranching state is coming.
