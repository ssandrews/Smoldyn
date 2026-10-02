# Proposal: constant-force boundary ("force clamp") for filament confinement

Roam project: `Project/Model branched actin network assembly under load in Smoldyn`
(uid `KbxdId0mP`, Task 3.1 of the Smoldyn R01).
Baseline: tip of `akamatsu/filament-capabilities` @ `6927b20`
(`filament-confinement-v1`), which provides `confine_surface` — the harmonic
filament–surface force this proposal closes a feedback loop around.
Files that would be touched: `source/Smoldyn/smoldyn.h`,
`source/Smoldyn/smolsurface.c`, `source/Smoldyn/smolfilament.c`,
`source/Smoldyn/smolsim.cpp` (one guarded call), `source/Smoldyn/smolcmd.c`
(observability command).

Status: **draft for discussion — not built.** (2026-08-28)

---

## 1. TL;DR

The Bieling et al. 2016 experiment that Task 3.1 must reproduce holds a growing
branched actin network under **constant force**, not constant position and not a
spring: an AFM feedback loop moves the cantilever base so its deflection — and
therefore the force on the network — stays at a setpoint while the network grows
(suppl. methods §4.1, "force-clamp mode"; external stiffness ≈ 0). Every primary
data point (force–velocity curve, ~8× density rise, ~3.3× free-barbed-end rise,
invariant filament length) was taken in this mode.

Smoldyn today can hold a boundary at constant *position* (static panel), or move
it along a *prescribed trajectory* (runtime `set surface … panel` re-issue, used
by our compression proof of concept). It cannot hold a boundary at constant
*force*, because nothing feeds the filament confinement force back into panel
position.

Proposed capability: a **piston panel** — one named panel that carries an applied
normal force `F_app` and a mobility `μ_w`, and moves along its own normal by
overdamped force balance against the confinement reaction the filaments already
exert on it:

    x(t+dt) = x(t) + μ_w · (F_app − F_reaction) · dt        (along the panel's front normal)

where `F_reaction` is the normal component of −Σ k·(closest-point − node) summed
over every node currently penalizing against that panel — the equal-and-opposite
partner of the force `filAddConfineForces` (smolfilament.c:3840) already applies
to the nodes. At steady state `F_reaction = F_app` exactly: the panel is a
constant-force wall that retreats at whatever velocity the network grows.
The mechanism is deterministic (no RNG draws) and opt-in (no keyword → no
behavior change → byte-identical regression).

This is the smallest closure of a loop whose two halves both exist: your
moving-surface machinery (`surftranslatesurf` / `surfupdateoldpos`,
smolsurface.c:2901/2919, driven today by `comparttranslate`) moves geometry, and
our confinement unit computes filament–surface forces. Neither knows about the
other yet.

Appendix A weighs the computational alternatives (velocity clamp, body-force
fields, per-node loads) and why the piston is still the right primitive — the
short version is that a constant-force wall with a positional degree of freedom
is the standard constant-pressure boundary from particle simulation, not mere
experiment mimicry, and the wall-free alternatives founder on the fact that
load currently reaches *growth* only through steric blocking at surfaces.

## 2. The target physics — why constant force specifically

Bieling et al. distinguish three mechanical boundary conditions, and show they
give *different* physics (their Fig 7):

| mode | what is held | what the network feels | Bieling usage | Smoldyn today |
|---|---|---|---|---|
| force clamp | cantilever deflection (via feedback) | constant force, k_ext ≈ 0 | Figs 2–6 — all steady-state data | **missing (this proposal)** |
| stiffness clamp | cantilever base position | Hookean spring k_ext = 0.01–0.1 N/m | Fig 7 only | missing (cheap v2 extension, §8) |
| displacement clamp | boundary trajectory | prescribed position, k_ext = ∞ | — | works today (`set surface … panel`) |

The steady-state force–velocity curve, the density adaptation, and the
barbed-end counts — the numbers Task 3.1 validates against — are all
force-clamp measurements. Under a spring the force depends on where the boundary
is; under the clamp the force is independent of position and the network selects
its own trajectory. Simulating the published curves with a spring or a
prescribed ramp would be simulating a different experiment.

Force is converted to the published stress by the fixed 200 µm² micropattern
cross-section; in a simulation patch of area A with periodic x-y boundaries,
`F_app = stress × A`. With lengths in µm and kT 0.001 (≈ room temperature),
1 model force unit = 4.114 pN, so the experimental range 25–1,250 pN/µm² on a
1 µm² patch is `F_app` ≈ 6–304 model units. Unlike the experiment — which
stepped one network through a staircase of setpoints to amortize setup time
(valid because steady-state velocities showed no hysteresis, their suppl.
Fig 3) — simulations would run one independent seeded run per setpoint.

## 3. What exists today (both halves of the loop)

**Filament side.** `confine_surface <srf> <k> [front|back]` applies a per-node
harmonic penalty toward the nearest point of each violated panel
(`filAddConfineForces`, called from `filComputeForces`). The reaction on the
panel is exactly the negative sum of these node forces; our compression PoC
already recovers it in post-processing as Σ k·penetration and measured a
sustained ≈ 43-unit (≈ 176 pN) piston force. Nothing stores this reaction in the
engine.

**Surface side.** Panels store `oldpoint`/`oldfront` alongside `point`/`front`;
`surfupdateoldpos` snapshots them and `surftranslatesurf` translates a surface,
with collision tests accepting a use-old-position flag — the machinery
`comparttranslate` uses when the `translatecmpt` command moves a compartment's
bounding surfaces (and, per its code argument, the molecules they contain). Our
PoC moves a single panel per timestep through the `set surface … panel` /
`surfaddpanel` update-in-place path. So panel motion per se is established
behavior; what is new is *what drives it*.

## 4. Proposed design

### Syntax (surface block)

    start_surface chamber
      panel rect -2 -0.5 -0.5 0.15 1 1 ceiling
      piston ceiling force 30.4 mobility 0.1
    end_surface

- `piston <panelname> force <F_app> mobility <μ_w>` — one piston per surface
  (v1; see open question O2). `F_app` in force units (energy/length), applied
  along the panel's **front normal** (so a ceiling that confines from above
  declares its front facing −z, as the PoC's does). `μ_w` in (length/time)/force,
  the same convention as filament node mobility. Defaults absent → feature off.
- v1 restricts to `rect` panels (constant axis-aligned normal — the use case;
  translating a sphere "along its normal" is not meaningful). `filCheckParams`-
  style validation in `surfcheckparams`: named panel exists, is rect, and at
  least one filament type declares `confine_surface` against this surface
  (else warning: piston will free-fall).

### Runtime behavior

One new function pair, factored to respect module ownership:

- `filComputePanelReaction(sim, pnl, &F_reaction)` (**smolfilament.c**) — sweeps
  all filaments of all types whose `confinesrf` owns `pnl`, and for each node on
  the penalized side accumulates the normal component of k·penetration. Reads
  surface geometry, writes nothing outside filament scope. One evaluation per
  step at end-of-step node positions — deliberately *not* instrumented inside
  `filAddConfineForces`, so RK2/RK4 sub-stage force evaluations are never
  double-counted and the hot loop is untouched.
- `surfPistonDynamics(sim)` (**smolsurface.c**) — for each armed piston:
  engagement check (§ below), then `dx = μ_w·(F_app − F_reaction)·dt` clamped to
  a per-step cap (default `0.5·standard_length`-equivalent; a hit is counted and
  reported), then `surfupdateoldpos`-for-that-panel + translate the panel's
  points along its front normal. Surface code owns panel motion; filament code
  owns force computation; each reads the other, writes only its own.

Called once from `simulatetimestep` (smolsim.cpp) immediately after
`filDynamics(sim)` returns — one guarded line (`if(sim->srfss …)`), returning
immediately when no piston is declared.

**Engagement.** The AFM engages its clamp only after the growing network reaches
the cantilever and the contact force first reaches the setpoint. The piston does
the same: it holds position while `F_reaction < F_app` has never yet been met,
and switches permanently to force balance on the first step where
`F_reaction ≥ F_app`. Without this, a constant downward force with nothing
resisting it would drive the panel through the seed filaments at
`μ_w·F_app` per unit time. (Optional `engage_time <t>` override for scripted
protocols.) After engagement, downward excursions are allowed and physical —
stepping `F_app` up transiently crushes older network, which is Bieling Fig 5.

**No thermal noise on the piston.** The cantilever is macroscopic and its
thermal motion is filtered out by the experimental feedback (30 Hz low-pass);
the piston is deterministic. Consequently the RNG draw sequence is untouched
even in models that use the feature — regression against no-piston models needs
no RNG argument at all.

### Observability

New command `printPiston <surface> <file>`: one line per invocation —
`time  displacement  F_reaction  engaged  n_penetrating_nodes`. This is the
simulation's version of the AFM height trace: with deflection (penetration lag)
constant at steady state, **piston displacement rate = network growth velocity**,
which is the experiment's primary readout. It also lets validation cross-check
the engine's `F_reaction` against the independent post-processing Σ k·penetration
we already trust.

## 5. Numerical considerations

- **Stability.** The piston update is explicit Euler against an effective
  contact stiffness `K = k·N_pen` (confinement constant × currently-penetrating
  nodes): stable for `μ_w·K·dt < 2`, well-behaved below ~0.2. PoC-scale numbers
  (k = 400, N_pen ~ 100, dt = 1e-5): `μ_w ≤ 0.5` for comfort; `μ_w = 0.1` gives
  a wall relaxation time τ = 1/(μ_w·K) ≈ 2.5e-4 s — quasi-static relative to
  network growth (seconds), exactly the role the 30 Hz feedback bandwidth plays
  in the experiment. `N_pen` is not known at parse time, so this is a runtime
  diagnostic (warn when the per-step cap is hit repeatedly), plus a
  `surfcheckparams` note printing `μ_w·k·dt` per confining type.
- **Steady state is insensitive to the knobs.** `μ_w` and `dt` set only the
  transient; the balance point (`F_reaction = F_app`) and hence the measured
  growth velocity must be invariant under `dt/2` and `μ_w×10` — both are
  explicit validation gates (§6). Refining `standard_length` changes `K` per
  unit area (the documented per-node convention of `confine_surface`) and thus
  the penetration *lag*, but not the force balance or the velocity readout.
- **Geometry caveat inherited from confinement:** the side test classifies
  against the rect panel's infinite plane, so the piston panel should span the
  periodic patch cross-section — which is also the physical situation (tipless
  cantilever much larger than the pattern).

## 6. Validation plan (three layers, per house discipline)

1. **Fixed-seed regression.** Models without `piston`: byte-identical output vs
   `filament-confinement-v1` (feature fully guarded, no RNG anywhere in it).
2. **Analytic recovery.**
   a. *Static balance:* piston vs N pinned nodes (`node_mobility 0`) —
      equilibrium displacement satisfies Σ k·penetration = F_app to tolerance;
      relaxation toward it exponential at rate μ_w·k·N.
   b. *Fluctuating balance:* piston over a thermal filament — time-averaged
      `F_reaction` = F_app (mean recovered with error bar), invariant under
      `dt/2` and `μ_w×10`.
   c. *Cross-check:* `printPiston` force equals post-processed Σ k·penetration
      from `printFilaments` coordinates at matching times.
3. **Official suites + example.** pytest and CTest sets identical to the stock
   baseline; new seeded example `examples/S13_filaments/forceClamp3D.txt` —
   the compression-PoC geometry with the prescribed ramp replaced by
   `piston ceiling force <F> mobility 0.1`, growth on, demonstrating
   grow → engage → constant-force steady state.

Then the science run (not part of the engine unit): sweep `F_app` over
6–304 units, one seeded run per setpoint, recover v(F), network density, and
free-end counts against Bieling Figs 2–3.

## 7. What deliberately does not change

- No molecule interaction in v1: molecules already collide with panels wherever
  they are, and the panel's old-position snapshot is updated on every move so
  the swept-collision path stays coherent; but no claim is made about fast
  pistons vs diffusing molecules (Task 3.1 models are filament-only). See O3.
- No cross-filament coupling, no new RNG, no change to `filComputeForces`
  content or call pattern, no change to integrators.
- The existing `set surface … panel` displacement-clamp path is untouched and
  remains the right tool for prescribed trajectories.

## 8. Roadmap after v1

- **Stiffness clamp (Bieling Fig 7):** replace the constant `F_app` with
  `k_ext·(x_base − x)` — one keyword (`piston <pnl> spring <k_ext> [base]`),
  same machinery, gives the constant-stiffness experiment family and the
  two-springs-in-series analysis for free.
- **Brownian-ratchet load feedback** (polymerization rate vs local load) is a
  separate, filament-side feature; the piston gives it the measured load.

## 9. Open questions for Steve

- **O1 — factoring.** Is `filComputePanelReaction` (filament module) +
  `surfPistonDynamics` (surface module) the right ownership split, or would you
  rather the reaction accumulate on a neutral struct? The one edit to
  `simulatetimestep` is the part we most want your eyes on.
- **O2 — scope.** One piston per surface (proposed) vs per-panel piston state on
  `panelstruct`? One-per-surface keeps struct growth off every panel.
- **O3 — molecules.** Should a piston move sweep molecules the way
  `comparttranslate` can (its code argument), or is "molecules see the panel at
  its new position next step" acceptable indefinitely for slow pistons?
- **O4 — naming.** `piston` vs something in your vocabulary (`panel_force`,
  `drive_panel`, …).

---

## Appendix A — alternatives to a moving-boundary force clamp

Considered 2026-08-28 (Matt's prompt: "could we impose a region with a constant
force field instead?"). Kept here so the design can be argued with.

**A0 — the engine fact that constrains every option.** In the current engine,
applied load reaches *growth* through exactly one channel: steric failure of
new-segment placement at a surface (`filAddOneRandomSegment` retry-then-fail,
reached via `filElongate`; `elongation_rate` itself is force-independent).
Forces bend and compress filaments but never slow assembly. Any loading scheme
without a physical wall therefore measures compaction, not a force–velocity
curve, until force-dependent elongation (a Brownian-ratchet rate law with
per-tip load estimation) exists — a strictly larger feature that stays on the
roadmap regardless of how load is applied.

**A1 — velocity clamp (works today, zero engine change).** Prescribe the
ceiling to *retreat* at constant speed v — the compression PoC's runtime
`set surface … panel` mechanism with `z = z0 + v·(time − t0)` — and measure the
steady-state reaction Σ k·penetration; sweep v and read the same curve from the
other axis, F(v) instead of v(F). At v = 0 it measures the stall stress
directly; at v at or above the free growth speed the force falls to zero.
Steady-state points are equivalent to force-clamp points wherever the
steady-state curve is single-valued — which Bieling's own no-hysteresis
control (their suppl. Fig 3) supports. Caveats: stress is an output with
fluctuations rather than a controlled setpoint; the *transient* adaptation
dynamics (their Fig 5 memory experiments) are not equivalent between clamps;
near-stall points converge slowly. This is the recommended interim path for
first force–velocity data while the piston design is discussed.

**A2 — body-force field in a region (rejected for Task 3.1).** A constant
per-node force in a slab has three independent problems. (1) The total load
grows with the amount of material in the region, so holding total force
constant requires the global renormalization f = F/N(t) each step — the same
feedback machinery as the piston with worse bookkeeping. (2) It loads the bulk
in proportion to local mass — centrifugation, not plate loading — and it
abolishes the plate's load-sharing rule: a rigid plate distributes F among the
trees touching it according to their stiffness at a common height, and since
the engine has no filament–filament sterics, that plate constraint is the
*only* collective mechanical coupling between separate trees in the model. A
field loads each tree by its own weight and removes even that. (3) A0 applies:
no wall, no growth coupling. The primitive is still worth having eventually,
where a point/end load is the right physics: membrane tension pulling on
filament tips, or validating a future ratchet rate law on a single filament
against a constant end load.

**A3 — constant force distributed over the current contact set** ("push down
on whatever touches the top"). Defining the contact set requires a surface to
define contact, and making that surface's position self-consistent with the
force is exactly the piston. Likewise a rigid "ghost plate" potential with a
force-balance degree of freedom is the piston under another name. This family
is the standard constant-pressure boundary of particle simulation (the
overdamped limit of an Andersen-style barostat wall) — the proposal is the
canonical computational device, not experiment mimicry.

**A4 — outer-loop servo (no engine change, piston physics).** A
libsmoldyn/Python driver alternating short run intervals with panel updates
`dz = μ_w (F_app − F_reaction) dt`, reading the reaction from `printFilaments`
output — a discrete-time force clamp that mimics the AFM feedback literally.
Useful for prototyping the protocol before the engine feature lands; same
physics as v1 with more moving parts.
