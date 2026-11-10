# Optional projected semi-implicit filament stepping

This is an optional integration method for the existing fixed-length network
model. It preserves the filament type, shared-node, box/contact, surface force
clamp and mechanical rollback structures. Full Newton remains the default.

## Select the method

Inside each mobile filament type:

```text
dynamics implicit_network
force_length -1
implicit_method projected
projected_tolerance 1e-5
constraint_tolerance 1e-8
```

`implicit_method newton` selects the original fully nonlinear reference solver.
Every mobile type in the coupled network must use the same method and
preconditioner. Projected mode requires negative `force_length` on every mobile
type. Fixed obstacles can retain their existing parameters.

The project-level `Standard_experiment.txt` groups these options with its
stretching selector. It still defaults to stretching and `newton`.
`Standard_experiment_inextensible.txt` defaults to fixed lengths and `newton`.
`Standard_experiment_projected.txt` selects fixed lengths and `projected`, with
the separate output `standard_experiment_projected_frames.txt`. It explicitly
uses `projected_tolerance 1e-4` (0.1 nm in this model's micrometer units), the
faster setting measured in the short steric replay. The library default and
grouped reference configurations retain the more conservative 1e-5 setting.

From the project directory:

```powershell
.\build-ninja-rtools\smoldyn.exe .\Standard_experiment_projected.txt -t
```

## What one step does

1. Reuse the existing physical-root coordinates and summed drag. Capture the
   start-of-step length gradients and draw thermal forces once, as before.
2. Evaluate forces at the initial feasible geometry. Freeze the existing
   constraint tangent and selected diagonal/banded preconditioner.
3. Solve one linearized implicit system with existing matrix-free GMRES.
4. Install the proposed displacement, restore segment lengths with the existing
   start-gradient SHAKE solver, solve the panel-owned clamp, and reevaluate all
   forces and contacts at the resulting geometry.
5. Accept the predictor if its full displacement defect is within
   `projected_tolerance` and all geometric/clamp solves succeeded. Otherwise
   continue the existing Newton line search and corrections, with the same
   thermal forces, to the stricter `implicit_tolerance`.

The predictor uses the free-displacement variables already used by fixed-length
Newton. Writing F for the existing forces, M for physical-node mobility, J for
the initial force Jacobian, and P for the initial length-compatible tangent,
its translational system is schematically

```text
(I - h M J P) u = h M F(q_n)
q_new = SHAKE(q_n + u)
```

The first-order physical displacement is P u. Multiplying the equation by P
recovers a tangent-projected linearized implicit step. Material-roll coordinates
retain their existing torque, mobility and coordinate conventions. This is a
linearly implicit predictor followed by geometric restoration, rather than full
nonlinear backward Euler.

At each GMRES product, J is still applied through directional differences of
the original force function. We do not substitute a small-angle bending force
for the original finite-angle forces. The analytic stiffness blocks remain a
preconditioner only. Consequently this is not a full implementation of Cytosim's
assembled linear operator, and it can still require many force evaluations.

No full nonlinear SHAKE is performed inside predictor GMRES products: the cached
constraint derivative already handles those probes. Nonlinear projection is
used for the initial and final geometry. Branch-point copies use the original
shared roots and remain coincident.

## Tolerances and fallback

- `constraint_tolerance`: unchanged, dimensionless relative segment-length
  error. Both methods enforce the same length tolerance.
- `projected_tolerance`: positive length, default **1e-5**. Bounds the maximum
  component of the unpreconditioned final free-displacement defect
  `u - h M F(q_new)` (including scaled rotational residuals). This is a step
  acceptance check, not a bound on trajectory or observable error.
- `implicit_tolerance`: unchanged, positive length; the stricter target used
  when full Newton is selected or the predictor falls back.
- `implicit_iterations`, `implicit_krylov`: existing iteration budgets. The
  first linearization counts toward the Newton budget on fallback.

One-solve integration cannot rely on later Newton corrections to remove a
loose inner solve's error. Its GMRES relative target is therefore
`min(0.01, ||b||/(sqrt(n)*minimum_standard_length))`, with the existing 1e-14
absolute floor. Here b is the preconditioned displacement RHS. The target
tightens with displacement so linear error tends to second order in h in the
small-step regime. A further absolute floor of
`0.1*implicit_tolerance/max(1,max(diagonal))` avoids oversolving tiny startup
motions below the requested physical residual precision. The full final defect
is always checked; timestep refinement below a fixed tolerance's error floor
also requires tolerance refinement. Normal Newton retains its original 5%
relative target.

If the predictor exhausts this tighter Krylov target, the solver retries the
ordinary inexact-Newton linear solve using the same frozen geometry and noise.
If a final projection/clamp evaluation fails or the defect is excessive, the
normal line search and nonlinear iterations take over. If those also fail, the
existing outer-interval rollback restores mechanics and panel geometry;
already accepted chemistry is retained. No automatic timestep changes or noise
redraws are introduced.

The log reports accepted predictors, Newton fallbacks and the largest accepted
defect separately. The legacy `Newton iterations` counter now includes the
predictor's linearization, not only fallback corrections. A last residual above
`implicit_tolerance` is expected when a predictor is accepted under
`projected_tolerance`.

## Limits and validation

Contacts and confinement remain soft penalties. Their force laws and overlap
handling are unchanged. Every final force-clamp solve must meet its own load
tolerance. These checks do not imply that large timesteps accurately resolve
contact rearrangements or bending. High fallback rates can erase the speed
advantage. Brownian increments are reused through every trial and fallback;
finite-step equilibrium accuracy still needs independent validation.

This method reuses the upstream-compatible negative-length and start-gradient
constraints documented in [FILAMENT_CONSTRAINTS.md](FILAMENT_CONSTRAINTS.md).
The new `implicit_method` and `projected_tolerance` keywords are local extensions;
an unmodified upstream executable does not necessarily recognize them.

See [the validation report](../profiling/projected_2026-10-30/REPORT.md) for the
native tests, timestep convergence, repeated replay timings and limitations.
