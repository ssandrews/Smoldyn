# Optional cached force Jacobian

> Updated for upstream integration on 10 November 2026: see [compatibility choices and validation](UPSTREAM_INTEGRATION_2026-11-10.md). Native junction/end-state names and shared box storage supersede older implementation descriptions below. Historical timing tables describe their original revisions, not a new benchmark of the merged engine.

Inside a mobile filament-type block:

```text
dynamics implicit_network
implicit_method newton
implicit_jacobian cached
```

`implicit_jacobian finite_difference` is the unchanged default. All mobile
types in the coupled network must select the same Jacobian mode. Cached mode
works with elastic or fixed-length segments, both preconditioners, and the
optional projected integration method. This selects the operator used by
GMRES; it does not select a different force model or preconditioner.

`../Standard_experiment_cached.txt` selects fixed lengths, full Newton and the
cached operator. Its output is `standard_experiment_cached_frames.txt`. Run
from the project directory:

```powershell
.\build-ninja-rtools\smoldyn.exe .\Standard_experiment_cached.txt -t
```

The existing Standard configurations group this choice with the other solver
parameters and retain finite differences by default. The example retains the
100 s requested duration; short validation does not establish full-run accuracy.

## Organization and numerical behavior

The existing Newton residual, line search, Brownian draws, shared-node drag,
length retraction, surface clamp and outer-interval rollback are retained.
The new private `smolfilamentoperator.c` workspace is owned and freed by the
existing implicit workspace. It caches geometric coefficients at each Newton
linearization and keeps them fixed throughout GMRES.

With ordinary coordinates, a product is `v-h*M*JF*v`. With fixed-length free
proposal coordinates, it is `v-h*M*JF*(P*v)`, where P is the existing cached
derivative of start-gradient SHAKE. Scaled material-roll variables and torque
mobilities retain their original conventions. Preconditioning follows this
product, exactly as in the reference path.

The cached operator applies local analytical derivatives and a material-frame
derivative sweep. It does not store a global dense Jacobian, nor assume that
the full 3D Jacobian consists of independent banded filament blocks.

- **Stretching:** exact axial and transverse derivatives of the existing spring,
  including transverse compression terms. Negative `force_length` contributes
  no spring, as before.
- **Bending/twisting:** differentiate the original `filBendTorque` expressions,
  changing tangents and torque-to-force conversion. Angular variations propagate
  down the material frames. The first frame differentiates the actual up-vector
  construction and its rotation about the saved start axis. The paper's simpler
  small-angle force is not substituted.
- **Branches:** both polar couples are differentiated with their respective type
  parameters. The daughter-only azimuth force includes the mother's frame
  response, wrapped deviation and existing near-pole fade. Its asymmetric
  coupling is preserved.
- **Rectangular confinement:** exact currently active normal spring derivatives.
- **Ideal force clamp:** for an active loaded panel, its height variation is
  `sum(k_i*dz_i)/sum(k_i)`. This weighted rank-one response includes fixed
  supports in the denominator and shared-node copies with their original
  per-filament wall contributions.
- **Capsule contacts:** cache closest-point features, interpolate endpoints,
  and differentiate closest parameters, distance, normal and force weights.
  This includes changes of normal direction, not just a normal spring block.
  Existing local branch-volume trimming is respected.
- **Thermal forces:** already sampled Cartesian forces are constant during the
  mechanical solve, so their derivative is zero. No extra random draws occur.

Every cached product reads the direction and cached coefficients; it does not
install coordinates, call the physical force routines, solve the physical clamp,
or update boxes/neighbor lists. The original nonlinear residual reevaluates all
of those effects for actual Newton trials and acceptance. Full Newton retains
its original `implicit_tolerance`; no tolerance is relaxed for cached mode.
The projected method retains its separately documented predictor-defect check.

## Parallelism and memory

The existing `filWorkSchedule` balances filament work. Each filament owns its
derivative force/torque writes. A barrier makes frame variations available before
branch derivatives read another filament. Contact evaluation can run in parallel;
its accumulation and shared-root gathers preserve deterministic summation order.
No full-network force copies are allocated per worker.

Buffers grow and are reused. Cache memory is proportional to nodes, branches,
active contacts and active wall entries, shared by all mechanical workers. The log reports allocated
MiB, setup/product counts and reference-operator fallback phases. The new optional
profiler buckets are `force_operator_setup` and `force_operator_apply`.

## Nonsmooth cases and fallback

Contact/wall active sets and closest features are frozen only during a linear
solve. Actual line-search trials use the original nonlinear forces, and the cache
is rebuilt at each accepted Newton geometry. Exact derivatives need not exist
where an active set or closest feature changes.

For nearly parallel interior contacts with a singular closest-parameter system,
the operator freezes that parameter gauge. This is a quasi-Newton contact block;
the original residual still decides acceptance. Trimmed contacts with elastic
contour lengths, nonrectangular confinement, coincident capsule centerlines and
Euler-coordinate poles currently use the reference operator instead of an
incomplete analytical derivative.

If cache setup fails, that substep uses finite differences. If cached GMRES or
line search fails, restore its accepted base geometry and constraint tangent and
retry using the reference operator, with the same noise. Cached use stays blocked
for the rest of that mechanical substep. If cached corrections exhaust the
Newton budget, one reference-operator phase gets the configured Newton budget.
If it fails too, the existing full outer-interval mechanical rollback applies;
chemistry already accepted in the interval is retained.

These checks preserve the nonlinear acceptance rule; they do not prove global
convergence or equilibrium accuracy for every geometry. Brownian and scientific
observables still need timestep/tolerance convergence studies.

## Compatibility and evidence

Existing dynamics enum values and public filament/segment/box organization are
unchanged. The type gains one selector field. The only change to an original
force implementation exposes the existing branch-trim helper for reuse; its
formula is unchanged. This is a local extension of the exploratory branch;
unmodified upstream does not necessarily recognize the new keyword.

The native operator test compares products to centered differences of the
original forces, including frame/roll coupling, branching, unequal mobility,
fixed anchors, contact features, trimming and clamp response in 2D/3D. It also
checks that products leave physical coordinates, forces, frames and spatial
index counters unchanged. The existing constraint, stochastic-increment,
worker-reproducibility and force-clamp suites are repeated in cached mode.

See `../profiling/cached_operator_2026-10-30/REPORT.md` for measured costs,
regression results, raw data and the incremental source patch.
