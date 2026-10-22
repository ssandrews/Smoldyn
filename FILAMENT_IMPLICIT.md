# Coupled implicit filament dynamics

Implemented and tested locally on 2026-10-13. This is an additional method,
`implicit_network`; the older `implicit` and `implicitold` methods retain their
existing algorithms. No external numerical library is required.

## Configuration

Keep `time_step 0.0001` at global scope. Inside every **mobile** filament type:

```text
dynamics implicit_network
steric_substeps 1
implicit_tolerance 1e-7
implicit_iterations 12
implicit_krylov 60
brownian_motion 0
```

`implicit_tolerance`, `implicit_iterations`, and `implicit_krylov` have the above
defaults, so they can be omitted. `brownian_motion` is independent of integrator
selection; leave it at 0 for the current deterministic mechanical experiment.
With multiple mobile types, the solver uses the smallest tolerance and largest
iteration limits. Fixed types use `dynamics none`. Mixing mobile Euler and
implicit types within one coupled network is rejected.

`steric_substeps` still divides the outer step for **all** mechanics, including
this method, even with sterics disabled. The mechanical step is
`h = time_step / max(steric_substeps)`. Capping, branching and elongation remain
at the outer chemical timestep. Their rates are unchanged. A larger chemical
timestep still needs chemical/time-discretization convergence checks.

## How the method works

For each shared branch node, sum forces from its attached mobile filaments and
sum their drags. The node's effective mobility is the inverse total drag. A fixed
member fixes the entire shared node. All attached filaments use exactly the same
coordinate throughout the solve, rather than being pinned after independent moves.

The mechanical equation is solved at the new position:

```text
R(displacement) = displacement - h * effective_mobility * F(new_state) = 0
```

First-frame spin and subsequent relative material rolls are also unknowns. Their
residual uses the existing local torque and rotational mobility conventions;
angles are scaled by `standard_length` to express every residual in length units.
First-frame rotation uses the starting segment axis, as in the existing Euler
material update. This remains a first-order method.

Newton iterations solve the complete nonlinear residual. Each linear correction
uses restarted GMRES (restart 20), with directional finite differences of that
same residual. A diagonal estimate of stretching, bending, junction, wall and
contact stiffness preconditions the linear system; it does **not** replace any
force in the physical equation. There is no dense network Jacobian. Vector
storage is approximately 30 doubles per degree of freedom and is reused.

Every residual evaluation installs the entire trial geometry before evaluating
forces. It calls the original stretch, bend, junction, confinement and thermal
force helpers, plus the existing capsule contact pass. Trial movement invalidates
the existing neighbor cache when needed; pairs are rebuilt through the original
Smoldyn boxes. No second spatial grid is introduced. Ordinary force and geometry
passes reuse `filament_workers`; contact accumulation and Krylov reductions stay
serial and deterministic.

A line search rejects corrections that do not reduce the residual. Invalid
geometry, exhausted iterations, or failure to satisfy the residual tolerance
stops the simulation with an error. All mechanical substeps of that outer interval
are restored using existing spare filament work arrays. Chemistry already applied
at the start of the interval is retained; this is a fatal-stop diagnostic, not a
complete simulation checkpoint or an automatic retry mechanism.

This follows the Newton/Krylov approach described in the primary
[SUNDIALS mathematical documentation](https://sundials.readthedocs.io/en/develop/cvode/Mathematics_link.html).
It is a small local implementation, not CVODE: there is no variable-order BDF,
automatic timestep controller, or automatic local integration-error estimate.

## Brownian forces

Thermal forces are drawn once per mechanical substep, in the existing serial RNG
order, and held fixed through Newton iterations, Jacobian products, and rejected
line-search trials. An unsuccessful solve does not redraw or automatically retry.
Disabling Brownian movement retains `kT` for seed geometry and branch spread.

Implicit damping changes thermal fluctuations at finite timestep. For the scalar
linear mode `dx = -lambda*x*dt + sqrt(2D)*dW`, backward Euler has stationary variance
`(D/lambda)/(1 + h*lambda/2)`. Thus unconditional stability of this linear mode
does **not** preserve its equilibrium variance at arbitrary `h`. Brownian network
statistics need timestep convergence, especially before increasing mobility.

## Material-roll correction

The existing `filCopyRollToYpr2` and `filCopyYpr2ToRoll` bodies copied in the reverse
direction to their names and call sites. Euler's computed relative-roll updates
were consequently overwritten. Their two assignments are corrected, with native
tests for each copy direction, an analytic Euler roll step, and an analytic
implicit roll step. This correction also affects other existing integrators that
call these helpers. Historical trajectories with torsional forces may change.

## Stability and force thresholds

For an isolated stretching spring with two mobile endpoints, the extension obeys
`d(extension)/dt = -2*mobility*force_length*extension`. Explicit Euler needs
`h*mobility*force_length < 1` for this mode; backward Euler multiplies extension by
`1/(1 + 2*h*mobility*force_length)` and is stable for any positive stiffness and h.
The native 2D/3D sweep checks this discrete solution through
`h*mobility*force_length = 10000`.

The previous contact safety limit, `contact relaxation factor <= 0.5`, is retained
for Euler. It is **not** imposed on a converged implicit solve. Instead, the new
acceptance requirement is that every displacement/scaled-roll residual is at
most `implicit_tolerance`. This tolerance controls the nonlinear solve, not the
physical trajectory's integration error. Large steps may fail GMRES/Newton or
converge to an inaccurate, overly damped trajectory. There is no universal new
force or stiffness ceiling for the nonlinear network.

For the current capsule law, a pair has `F = k * overlap`. With radius 0.0035 um
and stiffness 4000, the geometric maximum overlap of two equal capsules is
0.007 um, giving a pair-force bound of 28 model units (about 115.2 pN at the
current unit conversion). This is a property of the force law, **not** a numerical
integration threshold. Multiple contacts can produce larger accumulated nodal
forces. Soft contacts still permit overlap, and this method adds no continuous
swept collision detection to prevent tunneling at large timesteps.

## Tests and measured performance

Native checks cover 2D/3D analytic spring relaxation, stiff capsule contacts,
radius-aware stiff walls, relative roll, free Brownian increments, exact shared
branch constraints, fixed nodes, failure restoration, and exact 1/4-worker
reproducibility. The existing steric/geometry/growth tests also run.

The reproducible timing script is `tools/benchmark_filament_implicit.py`; settings
are grouped at the top. Its replay starts from the saved near-29.9 s geometry:
505 filaments / 6214 segments, four workers, equal 10 ms windows, no warmup,
Brownian noise and chemistry disabled. See the generated report and raw logs in
`../profiling/implicit_2026-10-13/dense/`. Euler with 80 substeps is the reference;
Euler with 40 substeps assesses its convergence. All timings exclude initialization
and frame output, and are single local measurements.

The timestep sweep tests h = 25, 100, 200, 500, and 1000 microseconds. With the
default Newton/GMRES limits, 100 and 200 microseconds converged; 500 and 1000 did
not. Start with 100 microseconds (`time_step 0.0001`, `steric_substeps 1`), rather
than treating the largest observed convergent step as an accuracy guarantee.
Independent 1 ms replays also test increased stretching/contact/bending stiffness;
their convergence results are in `stiffness_limits.json`. Convergence alone does
not validate their mechanics or equilibrium distributions.

The saved FIL snapshot has rounded coordinates and lacks material frames, original
RNG state, and junction rest azimuths. The driver reconstructs these; therefore the
replay compares identical reconstructed states, not exact biological checkpoints.
The separate live 3 s growth test exercises branching, capping, elongation and
topology changes. Different substep counts consume different numbers of Brownian
RNG draws even when noise amplitude is zero, so live trajectory timings are not
comparisons of identical networks. A full 30/90 s run is not certified by these
short tests.

Existing restrictions remain: periodic filament sterics, rigid
`branch_azimuth_fix`, and radius-aware nonrectangular confinement are unsupported.

## Local results (2026-10-13)

| Method | Mechanical h (s) | Wall time (s) | Speedup vs Euler40 | Maximum node error vs Euler80 (nm) |
|---|---:|---:|---:|---:|
| euler80 | 1.25e-06 | 31.191 | 0.50 | 0.0000 |
| euler40 | 2.5e-06 | 15.568 | 1.00 | 0.0015 |
| implicit4 | 2.5e-05 | 10.741 | 1.45 | 0.0281 |
| implicit1 | 0.0001 | 7.185 | 2.17 | 0.1119 |
| implicit_0p2ms | 0.0002 | 6.355 | 2.45 | 0.2616 |
| implicit_0p5ms | 0.0005 | did not converge | | |
| implicit_1ms | 0.001 | did not converge | | |

6214 segments / 505 filaments, four workers, equal 10 ms mechanics windows.
Brownian noise and chemistry disabled. Single timings; source coordinates reconstructed
from a rounded FIL snapshot. Maximum penetration was about 1.019 nm for all converged cases.

At h=100 us the largest accepted contact factor was about 6.2 (Euler guard: 0.5);
at 200 us it was about 12.6. The old guard at these configurations corresponds to
roughly h <= 8 us. Convergence is now checked by the full implicit residual.

| Independently changed parameter | Test value | 1 ms replay at h=100 us |
|---|---:|---|
| force_length | 100000 | converged |
| force_length | 1000000 | converged |
| steric_stiffness | 40000 | converged |
| force_angle | 10 10 10 | converged |

These short stiffness tests establish solver convergence only; they do not establish
trajectory accuracy, a universal stiffness ceiling, or an acceptable overlap tolerance.

The final compiled implicit method also completed the live 3 s experiment in
6.31 s: 3 to 14 filaments, 15 to 88 segments, with no solver failures. It used
30,000 outer steps and 151,016 contact-force evaluations, including solver trials.
This verifies growing topology and chemistry; it is not a full-duration benchmark.
Logs and metrics are in `../profiling/implicit_2026-10-13/live_final/`.

## Source map

| File | Change |
|---|---|
| `source/Smoldyn/smoldyn.h` | New enum and three per-type solver settings |
| `source/Smoldyn/smolfilament.c` | Defaults, keyword parsing, output, dispatch, corrected material-roll copies |
| `source/Smoldyn/smolfilamentsteric.h` | Owned solver workspace and shared mechanics interfaces |
| `source/Smoldyn/smolfilamentsteric.c` | Method validation, shared force/topology/worker access, implicit dispatch and rollback |
| `source/Smoldyn/smolfilamentimplicit.c` | Residual, matrix-free Newton/GMRES, material coordinates, preconditioning, diagnostics and backup |
| `CMakeLists.txt` | Register the new C source in existing targets |
| `tests/test_filament_steric.c` | Meaningful native integrator checks |
| `tools/profile_filaments.c` | Full-precision coordinate export and contact/solver diagnostics outside timing |
| `tools/benchmark_filament_implicit.py` | Equal-duration timing, position-error and convergence/stiffness sweeps |

Build normally with CMake. The benchmark driver additionally requires
`OPTION_FILAMENT_PROFILE=ON`. The efficient console progress display remains in
`smolsim.cpp`; integrator iterations do not print or refresh it.
