# Optional mechanochemical filament growth

> Updated for upstream integration on 10 November 2026: see [compatibility choices and validation](UPSTREAM_INTEGRATION_2026-11-10.md). Native junction/end-state names and shared box storage supersede older implementation descriptions below. Historical timing tables describe their original revisions, not a new benchmark of the merged engine.

## Prescribed growth into obstacles (10 November 2026)

Select `elongation_model overlap` before creating filaments to disable
force-dependent elongation and use the original geometric segment-addition
path without collision rejection. The configured `elongation_rate` accumulates
in the normal pending-length bank; whole segments are added when that bank
reaches `standard_length`. Newly added segments can cross surface panels and
overlap existing filament capsules. They are registered in the existing
chemistry pending list, so subsequent collision-checked growth/branch births
can see them; mechanics refreshes the ordinary shared box index as usual.

Existing capsule repulsion, surface confinement, junction forces, implicit
integration and Brownian movement are unchanged. They act on the new geometry
in the following mechanical phase. No contact force is converted to a growth
rate correction, and no finite-work or stall-force bound is applied. This is a
prescribed-growth model; it does not supply a thermodynamic polymerization law.
Whole-segment jumps and soft forces permit penetration/crossing and do not
guarantee noncrossing topology or instantaneous removal of overlaps. Solver
convergence still limits practical timesteps and node spacing. A surface needs
an active `confine_surface` force to mechanically oppose growth through it;
display-only panels provide no force.

With `force_length -1`, all segments are constrained, including the terminal
segment. Positive `force_length` retains ordinary elastic segments. The
mechanochemical parameters `elongation_step`, `elongation_energy` and
`elongation_stiffness` are unused in overlap mode. Capping and the maximum-length
setting retain their existing behavior. Branch births and treadmilling retain
their existing collision checks; this change applies to elongation only.

`../Free_growth_hybrid_brownian_30s.txt` now selects overlap growth and writes
`free_growth_overlap_brownian_frames.txt`. Its existing end time and all
branching/capping, Brownian and mechanical settings are retained; the filename
is historical. Ordinary FIL output works with the existing viewer converter
and analysis scripts; continuous-rest-length LENGTH metadata is unnecessary.

The `legacy` and `mechanochemical` choices remain available for comparison.
The following sections document the latter, not the current free-growth input.

Implemented 6 November 2026. The default remains `elongation_model legacy`.
The optional method couples polymerization to the existing axial energy and
implicit network mechanics. It addresses whole-segment insertion arrest; it
does not establish agreement with Bieling 2016 or guarantee sustained vertical
growth of an arbitrary branched network.

## Run the new experiment

From `Smoldyn_project`:

```powershell
.\build-filament-workers\smoldyn.exe .\Standard_experiment_mechanochemical_60s.txt -t
```

This separate input retains the current 40 pN total load on a 1 um² patch,
20 s free growth followed by 40 s of force-clamped growth, seeds, branching,
capping, mobility, steric radius and timestep. It selects the verified cached
Jacobian to reduce force evaluations. Outputs have a separate
`mechanochemical_60s_40pN` prefix. The previous inputs and trajectories are
preserved. Edit `LOAD_STRESS` and the output filenames together for other loads.

Inside the filament-type block the essential settings are:

```text
dynamics implicit_network
force_length 10000
elongation_model mechanochemical
elongation_step 0.0027
elongation_energy 0.00318
elongation_rate 0.06
```

`elongation_step` is a 2.7 nm chemical step, independent of the 10 nm mechanical
node spacing. `elongation_energy` is the available free energy per step in
model energy units, here 3.18 kT when kT=0.001. These two parameters must be set
explicitly; their generic defaults are zero/unconfigured. In this project's
unit convention the corresponding growth-conjugate stall force is about
4.85 pN per actively growing tip. It is not the total network force or a
claim that experimental tips all carry that force.

Select `elongation_model legacy` **before creating filaments** for the original
whole-segment insertion algorithm. Model switching on live filaments is
rejected because it would reinterpret pending growth as incorporated polymer.
This implementation requires positive kT, back-end growth, a straight untwisted
rest shape, and no treadmilling or attached back end. Positive `force_length`
selects the elastic version. For fixed completed segments, explicitly provide
a positive growing-tip stiffness as described below; negative `force_length`
without that extra setting is rejected. Both preconditioners and the cached and
finite-difference Jacobians remain available.

### Fixed completed segments with a compliant growing tip

The supported mechanochemical combination (used by that input before
10 November 2026) is:

```text
force_length -1
elongation_model mechanochemical
elongation_stiffness 10000
brownian_motion 1
```

Only the uncapped terminal segment has an axial spring. The existing SHAKE
solver constrains completed segments to standard_length; the tip spring uses
elongation_stiffness and its current incorporated rest length. When capping
occurs, the terminal segment becomes constrained at its entire incorporated
length, including its partial growth fraction. Uncapping releases that terminal
constraint and restores the tip spring. Constraint membership is checked even
if simultaneous capping/uncapping leaves the total edge count unchanged.

On refinement, the completed half becomes constrained and the new terminal
half remains compliant. The ordinary constraint solve restores the completed
half to its nominal length. This deliberately changes axial compliance and is
not the compliance-preserving elastic refinement described below. The elastic
work diagnostic counts rest-length changes of the compliant tip; it does not
include energy changes at rigidification/capping/uncapping. It is not a complete
thermodynamic accounting of those transitions. This is a hybrid axial model,
not an assertion that the growing tip is itself inextensible.

The new example runs 30 s with nominal unloaded elongation 0.15 um/s = 9 um/min,
Brownian motion and excluded volume enabled, no piston, and no top confinement
panel. Mechanical and chemical timesteps are 0.0001 s. All physical randomness
and existing branching/capping remain active. These choices require timestep
and node-spacing checks for a production interpretation.

The initial condition is 16 five-segment seeds on a 4 x 4 basal grid, with
alternating in-plane directions and front nodes 10 nm above the membrane.
This population was explicitly selected after the three-seed Brownian smoke
test capped out by 2 s. More seeds reduce early stochastic extinction risk;
they do not guarantee sustained growth or represent a measured NPF-site density.
Native tests cover hybrid Brownian movement with one/multiple segments,
refinement, shared branch-node attachment, capsule force transmission, and
capping/uncapping exchanges that preserve the total constraint count. Completed
segments meet constraint_tolerance; the active terminal segment is excluded
from that constraint error by design.

## What changes in one timestep

1. Existing branching and capping retain their order and stochastic rules.
2. For each uncapped tip, elongation increases its terminal segment's **rest
   length**, continuously. It does not translate nodes or append a 10 nm rod
   through existing material. Opposing elastic work reduces the rate.
3. The existing coupled mechanical solve sees the new rest length. Its axial
   spring exerts equal and opposite forces at the segment ends; wall and capsule
   forces transfer that stress to surfaces and other filaments. The force clamp
   can retreat as polymerization pushes it upward.
4. When terminal rest length reaches twice the normal spacing, that existing
   straight segment is bisected without moving its endpoints or changing its
   capsule union. The added midpoint has zero bend/twist. Tip mobility and any
   daughter junction at the old tip are reassigned to that same physical tip.
   Ordinary box/movement bookkeeping then handles the changed topology.

Refinement is not a collision-rejected birth. A capsule already in the same
place is being represented by two adjacent capsules. Branch births still use
the original insertion/rejection checks and are not given free mechanical work.
Mechanical Brownian motion is not enabled by this feature.

## Mechanical work and the rate law

Let r be terminal rest length, l its current geometric length, and
EA = force_length * standard_length * thickness. In this mode:

```text
k(r) = EA / r
E(l,r) = EA * (l-r)^2 / (2*r)
g = max(0, dE/dr) = max(0, EA/2 * [1-(l/r)^2])
g_stall = elongation_energy / elongation_step

v(g) = v0 * max(0, exp(-g*delta/kT) - exp(-Delta_mu/kT))
             / (1 - exp(-Delta_mu/kT))
```

Here v0 is the configured unloaded elongation rate. The exponential rate
factor is an effective, mean-field ratchet-inspired kinetic law. It represents
unresolved chemical/gap fluctuations rather than explicitly simulating every
monomer and thermal opening. Reverse depolymerization is not implemented:
negative net velocity is clipped to zero. It should not be described as an
exact microscopic Brownian ratchet or a full detailed-balance sampler.

The rate uses the work conjugate to increasing **rest length**, rather than
the tip's net Cartesian force (which can be almost zero in force balance).
For small axial strains g is close to the compressive axial force. They differ
at finite strain. At frozen geometry g(r) increases monotonically. A step is
bounded so its endpoint cannot exceed g_stall, ensuring:

```text
Delta E_stretch <= (Delta_mu / delta) * incorporated_rest_length
```

Scaling k with 1/r makes axial energy and compliance continuous across
refinement. This consistent thickness factor is used only in the new mode;
the legacy force convention is preserved. Bending stiffness follows the
existing discrete angle model. During growth terminal spacing ranges from one
to two standard lengths, so node-spacing convergence remains necessary.

Chemistry is updated once per outer timestep. Mechanical substeps do not
subdivide this chemical update. The parser rejects a requested growth increment
larger than one quarter of the nominal spacing. This is a guard, not an
accuracy guarantee. Halving the outer timestep remains the appropriate check.
Existing failure behavior is retained: a failed mechanical interval restores
coordinates, while preceding chemistry/rest-length incorporation is retained
and the run stops. No hidden retries or additional Brownian draws are introduced.

## Output and analysis

`printFilaments` keeps each existing FIL geometry line unchanged. In the new
mode it follows that line with:

```text
LENGTH time type:name incorporated_rest_length
```

This includes the growing terminal fraction. `free_growth_test.py` and
`compression_test.py` now use these optional records for actin incorporation
and mean nominal length, with their old nseg*spacing fallback for legacy data.
Their damage recovery treats malformed/mismatched length metadata as a damaged
frame. Simularium conversion continues using the unchanged geometry lines.
Old analysis programs that ignore LENGTH will miss partial terminal growth.

The final runtime report lists attempted and incorporated elongation, stalled
calls, refinements, elastic work, and available chemical work. These diagnostics
exclude the initial polymer and energy created by branch births. They therefore
support a clearer elongation balance than counting newly created segments alone.

## Validation results

Native regressions use an anchored straight filament and the existing ideal
clamp for 0.4 s of growth, Brownian movement off, dt=0.0001 s:

| Applied total load | New incorporated length | Plate advance |
| --- | --- | --- |
| 0.1 model units (0.4114 pN) | 0.0161107 um | 0.0160801 um |
| 0.5 model units (2.057 pN) | 0.00524804 um | 0.00521851 um |
| 1.5 model units (6.171 pN; above stall) | 0 | 0 |
| Legacy insertion, 0.4114 pN | no segment accepted | 0 |

Halving dt changed incorporated length from 0.0161107 to 0.0161125 um
(about 0.011%). Cached and finite-difference solvers agreed within the native
test tolerance. A separate mobile-filament obstacle was pushed upward by
0.00970827 um through capsule contact forces. Other checks cover capping,
maximum length, finite-energy stalling, work bounds, thickness/force energy
gradients, and refinement with a daughter attached at the old tip.

All 14 native regression tests and 7 Python analysis tests passed. A separate
four-second seeded, branched network with loading at 2 s completed with no
solver failures. It ended with 161 segments, 125 refinements, and 4 rejected
branch births. Elongation incorporated 1.34802 um of 2.01932 um attempted, with
0.148559 model energy of signed elastic work versus 1.58767 available chemical
energy. The last maximum capsule penetration was 0.00145312 um: contacts remain
soft, and finite overlap is not eliminated by the growth mechanism.

The same new-network input took approximately 26 s with residual-difference
Jacobians and 16 s with cached derivatives in short local runs. The legacy
network took about 9 s, but produced only 70 segments and a different trajectory;
those are **not** matched-size speed comparisons. Cached mode reduced force
evaluations from 237,536 to 194,094 while reproducing the same final network
summary. These short runs do not validate the full 60 s trajectory, dense-network
performance, or the previously estimated 13% incorporation fraction at late times.

## Implementation map and scope

- `smolfilamentgrowth.c`: rest lengths, energy-consistent stiffness, kinetic
  slowdown, work cap, geometry-preserving refinement and diagnostics.
- `smolfilament.c`: optional parser/defaults/validation and growth dispatch;
  rest-length-aware axial forces and stretching energy.
- `smolfilamentprecond.c`, `smolfilamentoperator.c`: matching axial stiffness
  and rest lengths for banded factors and cached force derivatives.
- `smoldyn.h`, `smoldynfuncs.h`: type-owned parameters and small helper API.
- `smolcmd.c`: optional LENGTH output; `smolfilamentsteric.c`: validation/report.
- `tests/test_filament_growth.c`: controlled physical and compatibility checks.
- `CMakeLists.txt`: new source and test under the existing filament-test option.

No new collision grid, integrator, wall geometry, parallel worker pool or
external numerical library is introduced. The original boxes, contact law,
surfaces, clamp and shared-node mechanics provide the force transmission.
Filament orientation selection at nucleation, branching-surface activation,
capping under load, monomer depletion, and hydrolysis remain separate modeling
questions. This addition does not by itself repair those assumptions.

The kinetic motivation is [Peskin, Odell and Oster 1993](https://doi.org/10.1016/S0006-3495(93)81035-X).
The particular growing elastic element and positive-growth-only closure above
are this implementation's modeling choices, rather than formulas copied from
that paper. Experimental comparison remains with
[Bieling et al. 2016](https://doi.org/10.1016/j.cell.2015.11.057).
Machine logs, smoke inputs and analysis outputs are in
`../profiling/mechanochemical_growth/`.
