# Optional banded preconditioning for implicit_network

Implemented locally on 2026-10-29. This changes the Newton linear-solve
preconditioner, not the physical forces or backward Euler acceptance criterion.
The existing diagonal path remains the default and is retained as a fallback.

## Select the method

Inside **every mobile** filament type:

```text
dynamics implicit_network
implicit_preconditioner banded
```

Omit the keyword or select `implicit_preconditioner diagonal` to use the old
path. Fixed types (`dynamics none`) need not match. Mixed mobile selections are
rejected, including after a runtime `set`. Solver tolerances, iteration limits,
mechanical substeps, chemistry and Brownian settings have their existing meanings.
Existing model files are not changed. A separate project model,
`../Standard_experiment_force_clamp_banded.txt`, provides an opt-in example with
separate output names. It retains the example's illustrative, uncalibrated load.

## Local stiffness and upstream compatibility

The residual is `R(u)=u-h*M*F(x+u)`. Its Jacobian is approximately `I+h*M*K`
when `K` denotes positive elastic stiffness. Each filament contributes a small
banded translational stiffness matrix:

- Stretching uses the analytic spring Hessian. The axial stiffness is `klen`;
  transverse stiffness is `klen*(1-standard_length/length)` in tension and zero
  in compression. Omitting negative transverse curvature keeps the approximation
  positive. It does not change the actual compressed spring force.
- Bending uses positive Gauss-Newton outer products `kypr[a]*g*g^T` on triplets
  of nodes. The 2D gradient agrees with the angular spring at a straight,
  unstressed configuration. In 3D the upstream yaw/pitch gradient construction
  supplies a local positive surrogate, with its cosine guard of 0.05.
- Local mother and daughter polar-junction gradients, and the daughter's
  regularized azimuth gradient, add positive outer products within each band.
  The partner filament's orientation is frozen only in this approximation.
- Active rectangular wall normals add their confinement stiffness. Other panel
  shapes use a conservative diagonal estimate. Steric estimates use half the
  existing contact row bounds, without another distance search.

The stretching and bending construction is adapted from Steven Andrews'
[`filComputeDerivForceMat`](https://github.com/ssandrews/Smoldyn/blob/master/source/Smoldyn/smolfilament.c)
(source inspected on 2026-10-28, reported modification 2026-10-24). Our local
function of that name is older and is **not** called or replaced. Two important
adaptations are required:

1. The current local stretch and bend helpers do not multiply force constants
   by segment thickness, so this preconditioner does not introduce that factor.
2. Upstream's absolute material-frame variables differ from our first-frame spin
   plus relative-roll variables. Rotation stays on the existing diagonal
   preconditioner. Translation-roll and twisting blocks are deliberately omitted.

The older local 3D torque also differs from upstream at finite bends. Therefore
the 3D band is a surrogate, not an exact Jacobian of the local force law. All
forces, material-frame updates, junction conventions and pole regularization
continue to be evaluated by the original full residual.

The neighboring-node construction is consistent with the banded bending
assembly in [Nedelec and Foethke, Figure 5](https://arxiv.org/html/0903.5178).
Their inextensibility projection and integration scheme are not adopted here.

## Shared points and mobility

The implementation reuses the existing node roots, member lists and summed
shared-point drag. Each mobile filament is an overlapping subdomain. For
translation, let `D` be the physical shared-node drag and `M=D^-1`.

We factor symmetric local approximations to `S=I+h*sqrt(M)*K*sqrt(M)`, avoiding
the nonsymmetric row scaling of `I+h*M*K`. Every shared node receives the elastic
diagonal contributions of **all** its attached mobile filaments. Off-diagonal
entries retained in a filament block come from that filament. Other-filament
diagonal contributions are nonnegative, so blocks remain positive definite after
adding identity and eliminating fixed mobility.

If a shared root occurs in `c` mobile filament records, restriction and gathering
each use weight `1/sqrt(c)`. For local restrictions `R_f`, the applied inverse is

```text
P^-1 = sqrt(M) * [sum_f R_f^T W_f S_f^-1 W_f R_f] * sqrt(D)
```

where `W_f=diag(1/sqrt(c))`. This is an additive Schwarz approximation. With
zero stiffness it is exactly identity, including shared points with unequal
member drags. Fixed roots and unused duplicate variables retain identity
equations. A fixed attached member fixes the physical root as before.

This does not create independent physical branch positions. All geometry trials
still install the single root coordinate. Off-diagonal interactions across
different filaments, shared-root collective modes, steric pairs and the force
clamp's global height response remain in the matrix-free operator. The clamp
still solves its load equation on every residual evaluation.

## Cost, threading and lifecycle

The lower scalar half-bandwidth is `3*dim-1` (8 in 3D). A direct Cholesky
factorization and repeated triangular solves have linear cost for fixed bandwidth.
Factors are rebuilt at the accepted geometry before each Newton correction and
remain fixed throughout that GMRES solve. No force evaluations or random draws
are added by setup or application.

The existing `RnSparse` interface provides band storage and BiCGSTAB but no
reusable Cholesky factorization. Compact lower-band factors therefore live in
the optional implicit-owned workspace; legacy `filwork->forcemat` is untouched.

Work arrays grow when needed and are reused. Rebuilding offsets and weights
from the existing movement topology handles elongation, branching, removal,
fixed-node changes and runtime mobility changes. The additional workspace is
35 doubles (280 bytes) per recorded node in 3D and 18 doubles (144 bytes) in 2D,
including duplicated filament records at shared points. It is allocated only
after banded preconditioning is used and freed with the implicit workspace.

Block assembly, factorization and solves use the existing length-balanced worker
schedule. Each block has one writer. Shared-node stiffness assembly and member
gathers retain fixed summation order; a barrier completes all block solutions
before gathering. Native checks compare one and four workers bit for bit.

If allocation or Cholesky setup fails, that Newton correction uses the original
diagonal scaling. A warning is issued on the first fallback, and the total is
reported. Physical geometry is not modified by setup. The original fatal-stop
and whole-outer-mechanical-interval rollback behavior is unchanged.

Reports now include total implicit residual evaluations, banded setups,
applications and diagonal fallbacks. Profiling builds separately record
`implicit_preconditioner_setup` and `implicit_preconditioner_apply`.

## Validation and performance

`test_filament_preconditioner` checks the inverse against central differences
of the actual straight-filament forces in 2D/3D, heterogeneous drag, fixed nodes,
shared-node partition of unity, growing topology, nonlinear branched trajectories,
runtime parsing and worker reproducibility. The existing implicit and complete
force-clamp suites also run with banded preconditioning selected.

The full report, repeated timing inputs, logs and coordinate differences are in
`../profiling/banded_preconditioner_2026-10-29/REPORT.md`. This is optional because
setup and triangular solves can exceed the saved GMRES cost. A faster short
steric replay does not establish a speedup for every network or a full growth run.

No larger stable timestep, improved trajectory accuracy or changed physical
stiffness threshold is claimed. Compare at identical timesteps and tolerances.
