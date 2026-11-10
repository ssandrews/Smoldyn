# Optional fixed-length filament segments

Implemented locally on 2026-10-30. This is an alternative axial mechanical
model, not simply a faster solver for the same elastic model.

## Choose stretching or fixed lengths

The project-level `../Standard_experiment.txt` groups the affected mechanical
settings and retains its original stretching default:

```text
define WITH_STRETCHING 1
define STRETCH_STIFFNESS 10000
```

Set `WITH_STRETCHING` to **0** to make every ordinary segment inextensible.
The filament-type block translates this selector into the upstream convention:

```text
standard_length 0.01
force_length WITH_STRETCHING*STRETCH_STIFFNESS-(1-WITH_STRETCHING)
```

With stretching this evaluates to 10000; without stretching it evaluates to -1.
A separate `../Standard_experiment_inextensible.txt` already selects 0 and
writes `standard_experiment_inextensible_frames.txt`, preserving distinct output
names. Changing the selector in the original file does not change its output name.

Without the project selector, an ordinary model can opt in directly:

```text
start_filament_type actin
dynamics implicit_network
standard_length 0.01
force_length -1
constraint_tolerance 1e-8
constraint_iterations 20
# Existing bending, mobility, chemistry, confinement and steric settings follow.
end_filament_type
```

- Positive `force_length`: the existing axial spring, unchanged.
- Negative `force_length`: constrain each segment to `standard_length`; its
  magnitude has no physical meaning. This matches upstream's negative-constant
  convention. Zero disables the spring and is **not** a length constraint.
- `constraint_tolerance`: dimensionless maximum relative length error. The
  library default is **1e-10**, matching upstream `FILCONSTRAINTTOL`. The Standard
  example explicitly uses **1e-8**. It is separate from the implicit displacement
  residual and the force-clamp force tolerance.
- `constraint_iterations`: maximum geometric Newton corrections per trial,
  default **20**, matching upstream's default iteration count. Each correction
  is damped if necessary to reduce length error.

All mobile types must continue to use `implicit_network`. Elastic and constrained
types may coexist in that network. Fixed (`dynamics none`) geometry is not moved
or repaired. This extension implements **length** constraints; negative angular
force constants in constrained mobile types are rejected, rather than treated as
upstream fixed-angle constraints. The older local integrators are unchanged.

The existing diagonal and optional banded preconditioners both work. Keep
`time_step`, mobility, bending, sterics, wall stiffness and chemical rates fixed
for initial comparisons. The Standard selector permits 30 outer Newton
corrections in fixed-length mode versus 12 in elastic mode; their residual target
is unchanged. A larger iteration cap does not force extra iterations.

An optional `implicit_method projected` now takes a linearly implicit predictor
with geometric restoration and a checked Newton fallback. The default is
`implicit_method newton`. See [FILAMENT_PROJECTED.md](FILAMENT_PROJECTED.md) for
its separate displacement-defect tolerance and validation; it is a different
integration scheme, not an alternative preconditioner.

## Physical and growth changes

Constrained segments still transmit axial tension/compression. They cannot store
elastic axial strain or axial spring energy. Network compression is accommodated
by bending, junction motion, sliding contacts and existing soft-wall/contact
overlaps. The clamp retains its specified total load; its position can change.

Existing seed, elongation and branch generation already use `filRandomLength`.
Its upstream-compatible negative-stiffness path returns exactly `standard_length`.
Consequently no thermal axial length variation is generated in this mode. Angle
sampling, kT, capping, growth banks and branching rules remain active as configured.
Whole-segment growth is retained by default; no automatic remeshing is introduced
in the legacy growth mode. An optional hybrid is now available with
`elongation_model mechanochemical` and positive `elongation_stiffness`: completed
segments use these constraints, while the actively growing terminal segment is
compliant. Capped terminal segments are constrained at their incorporated length.
See [FILAMENT_GROWTH.md](FILAMENT_GROWTH.md) for its distinct mechanics and limits.

The Gaussian draw sequence for length generation differs between axial models,
as upstream's convention also implies. Equal random seeds therefore do not
produce identical live growth trajectories. Chemical rates are not rescaled.

All segments of a constrained type target its standard length. Custom initial
segments of other lengths are projected, or the step fails if the constraints
cannot be satisfied. For reproducible comparisons use initially feasible
geometry. This is not a trajectory/checkpoint conversion mechanism.

## Coupling to the existing implicit solver

For a mechanical substep, save the unit segment gradients `G0` at its start.
The existing Newton unknown `u` forms a free proposal `q*=q_n+u`. Inside every
ordinary nonlinear residual trial, solve the SHAKE correction

```text
q = q* - M G0^T lambda
|q[b]-q[a]| = standard_length
```

with the existing shared-node mobility `M`. The correction direction remains
the start-of-step gradient throughout the substep. At each geometric Newton
iteration its multiplier increment solves

```text
(G(q) M G0^T) delta_lambda = current_length - standard_length
```

This follows upstream `filProjectConstraints` and its `filConstraintStart`
convention, extended across shared physical roots rather than independent
filaments. Projecting along current gradients instead would change the stochastic
scheme. The current fork's material-frame/torque conventions are preserved;
newer upstream frame transport and angular constraints are not copied wholesale.

After projection, refresh segment angles once and evaluate the original forces,
including capsule contacts and existing surface confinement. The ideal force
clamp solves its load equation on this projected geometry. Newton then solves

```text
u - h M F_other(q) = 0
```

plus the existing material-roll residuals. Thus accepted forces, lengths and clamp
load correspond to one configuration. There is no final position normalization
that could invalidate an already accepted contact or force balance.

Each physical branch point has one coordinate, using the original union roots
and summed drag. A fixed attached member fixes the point. Both constrained and
elastic members receive the same positional correction at a shared root.

## Efficient sparse constraint calculations

The constraint graph reuses existing segment endpoints and physical roots. Its
ordering is cached against the movement-topology generation and active types;
growth, removal and branching invalidate the cache through the existing lifecycle.
No new spatial partition is introduced.

For forest-shaped networks, leaf elimination solves the nonsymmetric constraint
matrix in linear time. A small mobility-response matrix at each parent node
represents all remaining incident constraints; branch cliques and a dense global
matrix are not stored. Loops or unsuitable leaf pivots use matrix-free BiCGSTAB,
as upstream also uses BiCGSTAB for its SHAKE system.

For GMRES derivative probes, factor once at the accepted Newton geometry and use
the analytic retraction derivative

```text
P = I - M G0^T (G M G0^T)^-1 G
delta_q = P delta_u
```

This replaces nested nonlinear projection on every Krylov probe with a cached
linear solve. The actual Newton/line-search trials still get full SHAKE and the
strict length check. Frozen positions reuse `filwork->flatnodes[0]`; importantly,
`wnodes[0]` aliases live positions and cannot hold that reference.

Preconditioning also accounts for this projection. For a diagonal primal
approximation B, the correction uses the sparse Schur matrix
`G B^-1 M G0^T` to invert `B P+(I-P)`. With banded primal blocks, the same Schur
approximation combines two existing banded inverse applications. The full
physical residual remains the convergence criterion.

Buffers grow and are reused under the existing implicit workspace. Shared-node
sums and forest ordering have fixed summation order; native checks compare one
and four mechanical workers bit for bit. The global constraint solve is currently
serial. Brownian forces are still drawn once per mechanical substep, in the
original order, and reused throughout Newton and rejected trials. No retries or
noise redraws are added. Finite-step thermal statistics still require convergence
checks; no equilibrium-accuracy claim is made from the Brownian increment tests.

Failures restore all mechanical substeps and the panel through the existing
rollback path. Chemistry already accepted in that outer interval remains. Fixed
anchors or singular configurations can make a constraint solve infeasible; the
code stops rather than silently accepting stretched segments.

## Validation and measured cost

See `../profiling/inextensible_2026-10-30/REPORT.md` for raw evidence and timings.
Tests include an independent dense SHAKE oracle for trees and loops, analytic
projection derivatives and preconditioner inverse checks, unequal/fixed mobility,
axial load transmission, bending, growth/removal/capping, sterics, Brownian
center-of-mass increments, worker reproducibility and whole-step rollback. The
complete existing force-clamp suite is also run in fixed-length mode.

At the current softened stretching stiffness, constrained mode was slower in
the tested replays. It removes an axial compliance assumption, but does not
remove bending/contact stiffness or guarantee a larger usable timestep. It
therefore remains opt-in and is not the recommended speed setting for the
current Standard experiment.

`printFilaments` and Simularium conversion remain compatible. The rounded FIL
coordinate format does not have enough precision to verify a 1e-8 relative length
tolerance. Use the solver's double-precision length diagnostic or profiling
`coordinates.csv` for that check.

Upstream source reviewed: [ssandrews/Smoldyn, smolfilament.c](https://github.com/ssandrews/Smoldyn/blob/master/source/Smoldyn/smolfilament.c),
reported modification 2026-10-29. The source excerpt and exact compatibility scope
are recorded in the report. This is an extension of the current exploratory fork,
not a merge or certification of the complete upstream branch.
