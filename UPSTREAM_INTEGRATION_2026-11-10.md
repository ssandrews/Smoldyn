# Upstream integration — 10 November 2026

The current local branch, `junjie/filament-sterics`, integrates upstream
[`ssandrews/Smoldyn` master at `3e754d5`](https://github.com/ssandrews/Smoldyn/commit/3e754d5).
Upstream calls its main development branch **master**, not main. There are 122
upstream commits since the previously merged upstream ancestor `1412fe6`.
This is a merge that preserves both histories, rather than a reset or rebase.

## Recovery and review

Before merging, all pending engine, test, documentation and profiling-tool work
was committed as `87250c5` and `4b24eac`. The complete pre-merge state is retained
as `backup/pre-upstream-complete-20261110` at `4b24eac`. Earlier backup branches
also remain. An existing unrelated untracked note/log was left untouched.

The repository used sparse checkout. `tools` was added to its materialized
directories so the profiler and benchmark sources are present after checkout;
these previously existed locally without being tracked. No force-push or
upstream-history rewrite is part of this integration.

For review from the Smoldyn checkout:

```powershell
git log --oneline 1412fe6..3e754d5
git diff upstream/master HEAD -- source tests tools CMakeLists.txt
git diff backup/pre-upstream-complete-20261110 HEAD -- source tests tools CMakeLists.txt
```

The first diff isolates our remaining extensions relative to current upstream.
The second shows how the exploratory implementation changed during integration.
Source snapshots, build/test logs and review patches are in the project-level
`../profiling/upstream_integration_2026_11_10/` directory. Experiment inputs and
old trajectories in the project directory were preserved.

## Main upstream progress

| Area | What upstream now provides | Representative commits |
| --- | --- | --- |
| End chemistry | Separate minus/plus states and length banks; growth, shrinkage and transitions among open/capped/shrink/barbed states | [a28252f](https://github.com/ssandrews/Smoldyn/commit/a28252f), [a988ef1](https://github.com/ssandrews/Smoldyn/commit/a988ef1) |
| Topology | Explicit junction objects, attachment by either daughter end, loop-closing springs, removal/reuse and minimum branch spacing | [5023b5a](https://github.com/ssandrews/Smoldyn/commit/5023b5a), [a78e3b2](https://github.com/ssandrews/Smoldyn/commit/a78e3b2), [55a6dbe](https://github.com/ssandrews/Smoldyn/commit/55a6dbe), [b1b42e9](https://github.com/ssandrews/Smoldyn/commit/b1b42e9) |
| Contacts and boxes | Native centerline/end lists in the ordinary molecule/panel boxes; filament softwall contacts and their contribution to the native Implicit Jacobian | [3446427](https://github.com/ssandrews/Smoldyn/commit/3446427), [2dd415f](https://github.com/ssandrews/Smoldyn/commit/2dd415f), [a5a342c](https://github.com/ssandrews/Smoldyn/commit/a5a342c) |
| Surface interactions | Softwall/repel/reflect, adsorption/desorption, panel/surface reaction forces, surfaces moving under force, and adsorbed filaments moving with surfaces | [7dd5146](https://github.com/ssandrews/Smoldyn/commit/7dd5146), [5d46211](https://github.com/ssandrews/Smoldyn/commit/5d46211), [1275f10](https://github.com/ssandrews/Smoldyn/commit/1275f10), [61b1b93](https://github.com/ssandrews/Smoldyn/commit/61b1b93), [46cd510](https://github.com/ssandrews/Smoldyn/commit/46cd510) |
| Load-sensitive growth | A native Brownian-ratchet barbed-end rate, surface interactions conditional on end state, and surface-associated branching | [acebe2f](https://github.com/ssandrews/Smoldyn/commit/acebe2f), [d020af2](https://github.com/ssandrews/Smoldyn/commit/d020af2), [8b76286](https://github.com/ssandrews/Smoldyn/commit/8b76286) |
| Mechanical formulation | More stable native implicit integration, energy-gradient bending torques, negative force constants as constraints, thickness-dependent stiffness/drag, persistence lengths, rotational mobility and viscosity-derived mobility | [9894890](https://github.com/ssandrews/Smoldyn/commit/9894890), [15133aa](https://github.com/ssandrews/Smoldyn/commit/15133aa), [5691281](https://github.com/ssandrews/Smoldyn/commit/5691281), [7b9c374](https://github.com/ssandrews/Smoldyn/commit/7b9c374), [4399b7f](https://github.com/ssandrews/Smoldyn/commit/4399b7f) |

The native `Implicit` method is substantially different from the older version
against which the exploratory branch was first developed. It remains available
alongside our `implicit_network`; it was not replaced with our solver. Upstream
also expanded libSmoldyn/Python APIs and refreshed examples and documentation.
Those upstream changes are included in the merge.

## Compatibility choices

### 1. Upstream owns topology and end state

Production code now uses `xyzminus`/`xyzplus`, `endjunct`, `juncts`, `junctlist`,
`endstate[2]`, `lenbank[2]`, `endrate` and `endtrans`. It does not maintain a
second mutable set of front/back branch arrays, capped flags or growth banks.

The coupled movement cache borrows native junction identities and checks node,
end, rigid/spring and bend information, including topology edits that leave
counts unchanged. Rigid spanning-tree junctions share network coordinates and
combined drag. Loop-closing junctions retain upstream's positional springs.
Contact trimming now works at either attached end and at target node zero.
Only a small neighborhood of a junction is exempted; distant material still
collides. The native plus-end branch/loop test checks both geometry and force
balance.

Legacy `plus_end back/front` and segment API `b/f` spellings remain adapters to
canonical `p/m` ends. New native end/state input is preferred. Test-only fixture
adapters translate old segment-based branch locations to native node locations;
they are not an alternative production topology implementation.

### 2. One spatial partition and one segment payload

Both contact implementations use upstream `boxstruct.filseg/nfilseg/maxfilseg`
and `filend*` storage. The old capsule-specific `segment/nsegment/maxsegment`
payload was removed. Centerlines are inserted with upstream's box traversal;
capsule queries expand their search by the relevant radius and skin instead
of creating an inflated second list. Candidate stamps prevent duplicate pair
tests across cells. Sorted cached pair indices preserve deterministic force
accumulation.

The small `BoxGrid` object is only a borrowed coordinate/query view of the
existing `boxss`; it owns neither cells nor segment lists. Occupied-box tracking
is retained to clear O(occupied boxes) rather than O(all chamber boxes).
Neighbor pairs still rebuild when topology, radius/stiffness, resolution or
the displacement skin requires it. Ordinary end-of-step list refreshes do not
invalidate an otherwise valid neighbor cache.

Anticipated-density initialization remains available for our coupled/steric
models or an explicit `filament_box_density` request. A plain upstream model
keeps upstream automatic molecule-based sizing. Explicit `boxsize`/`molperbox`
still takes precedence. No density-driven resizing during a run was introduced.

### 3. Native mechanics is the force reference

The merged `smolfilament.c` was rebuilt from upstream's implementation, then
extended at the dispatch points. Native stretching/bending thickness factors,
the energy-gradient bend torque, effective node mobility and rotational
mobility helpers are retained. The banded preconditioner and cached derivative
were adapted to those formulas; neither replaces the nonlinear force equation.

`implicit_network` retains its experimental network-wide Newton–GMRES method,
shared-node constraints, forest/SHAKE length projection, checked projected
predictor and full-step mechanical rollback. Its material unknowns remain
initial-frame spin and relative rolls; native integrators keep upstream's
absolute-frame transport. The two integrators are not asserted to be identical
in 3-D twist dynamics, or interchangeable without convergence checks.

For ordinary minus-attached branch trees the cached operator includes the
updated branch stiffness and azimuth terms. General joins/loops, native
filament contact interactions and native softwall/repel surface interactions
use the complete finite-difference residual fallback when their exact cached
blocks are unavailable. This preserves their forces rather than silently
omitting cross-coupling. Native contact forces are refreshed at each relevant
network trial, not held at the previous geometry throughout Newton.

The smoother, vanishing-at-the-pole azimuth regularization from the exploratory
solver is retained for `implicit_network`. Native integrators keep upstream's
bounded-magnitude rule. This difference is deliberate and covered by the
regularization and force-product tests.

### 4. Growth laws remain explicit choices

| Choice | Behavior |
| --- | --- |
| Native open-state/legacy growth | Uses upstream's end-state velocities and whole-segment banks, with configured insertion checks |
| Native barbed state | Uses upstream's capped exponential force factor and optional backward rate |
| `elongation_model overlap` | Prescribed geometric elongation into wall/capsule overlaps; existing forces relax the geometry; no force-dependent growth factor |
| `elongation_model mechanochemical` | Keeps our continuous terminal rest-length growth, chemical-work bound, refinement and optional fixed completed segments/compliant tip |

Our capsule endpoint forces feed native `endcontact`; the legacy continuous
wall penalty also contributes to native barbed-end force measurement. Overlap
mode explicitly bypasses barbed force feedback if that state is selected.
Native open/barbed/capped/shrink states otherwise remain available.

Mechanochemical growth stays plus-end-only and does not implement shrinking or
positive capped-state growth. Those combinations, and combining its force law
with native force-sensitive barbed kinetics, are rejected rather than silently
applying two kinetic laws. Oversized chemical steps retain the growth-spacing
guard. A fatal chemical error propagates through the new dispatch. Refinement
remaps native junction nodes, tip mobility and any tip surface binding.

The latest project free-growth input still selects **overlap**, with its existing
30 s endpoint and Brownian switch set to 0. No experimental kinetic rates or
mechanical timesteps were retuned for this merge.

### 5. Surfaces and ideal clamps have one owner of motion

Generic `surface_interaction`, adsorption, reflection, native surface motion
and surface reaction reporting remain upstream's implementations. The project
`confine_surface` alias keeps its existing continuous rectangular penalty for
coupled/steric models; ordinary native models use upstream's native alias.
The same legacy force helper supplies dynamics, barbed-end force measurement
and native panel/surface reaction totals.

The optional ideal force clamp remains attached to an existing rectangular
panel. Its load solve, approach/engagement, bounds and rollback remain available.
A surface with native positive mobility cannot simultaneously carry an ideal
clamp: two independent mechanisms must not move the same panel. Clamps still
require the supported front-side project confinement and coupled dynamics;
they are not silently substituted for upstream surface mobility.

Native `filament_interaction` contacts and our `steric_*` capsule penalty are
separate opt-ins. Enabling both for the same pair adds both interactions; the
merge does not automatically equate their radii or stiffness conventions.

### 6. Parallel work retains serial stochastic ordering

Mechanical force/geometry work, pair evaluation, node ownership and large
vector operations keep the optional worker scheduling. End-chemistry scheduling
now reads/writes native two-ended state tables. Transition draws and topology
births retain upstream serial ordering; private bank/state writes can run in
workers. Shrinking, competing growth at both ends, force-dependent barbed
kinetics and mechanochemical growth use the generic serial chemistry path.

Reseeding now clears Box–Muller Gaussian caches. Upstream's additional segment
thermal-torque draws exposed stale cached samples after odd draw counts when
multiple simulations were reseeded within one process. A regression checks
reproducibility directly. Zero translational mobility also avoids an undefined
0/0 rotational mobility factor. These are narrow compatibility fixes, not
changes to the chosen kT or stochastic rates.

### 7. Native trajectories remain native

`printFilaments` now retains upstream's format:

```text
FIL time type:name nseg minus-attachment plus-attachment minus-state plus-state coordinates...
```

Mechanochemical `LENGTH` metadata is still written separately. The project
`free_growth_test.py` reader, the compression analysis that reuses it, and the
sibling Simularium converter accept both this format and old six-field-header
FIL records. Shrinking plus ends are no longer counted as growing by the
analysis reader. The profiling snapshot importer accepts the ordinary
minus-attached open/capped tree format and explicitly rejects unsupported
loop/two-ended reconstruction instead of guessing.

The biological analysis is still intended for the actin branching experiments:
an attachment in generic join/loop output does not by itself establish an
Arp2/3 birth, and end-state metadata alone does not report every configured
end-specific growth rate. General network chemistry needs explicit event data
for those interpretations.

## Validation and limits

- Release/OpenMP build: **15/15 native CTest cases passed**.
- Release/serial build with workers disabled: **15/15 passed**.
- Project analysis: **8 Python tests passed**, including old/native-format
  equivalence and shrinking-end handling.
- Converter and analysis readers agreed on actual new overlap and
  mechanochemical output, legacy 3-D output and native 2-D end-state output.
  The optional `simulariumio` writer is absent from the test Python environment;
  an end-to-end Simularium export was not rerun as part of this integration.
- Short isolated runs completed for eight representative upstream examples:
  contacts, adsorption/desorption, moving surfaces, the growing layer/lid,
  end states, loops, persistence length and twist. Native contact and growing
  layer examples also completed with `implicit_network` selected.
- Both current project overlap and mechanochemical inputs completed isolated
  0.2 s checks. The upstream examples were shortened to 0.02 s.

Existing oracles cover contact search versus exhaustive geometry, old/new
insertion behavior, wall and capsule force relaxation, Brownian displacement,
exact worker reproducibility, fixed-length feasibility, cached force products,
clamp load balance and failure rollback. The added native-junction tests cover
plus-end attachment and a loop-closing spring with shared-node force balance.

These are compatibility and short integration checks, not full 30/60/100 s
production validation, Linux/Narval execution, a performance benchmark or
experimental calibration. Upstream's corrected mechanics can change old
trajectories even with unchanged input values. Historical timing tables in
the other exploratory documents describe earlier revisions. Existing Narval
archives were not regenerated by this branch update.

## Code map

| File | Integration responsibility |
| --- | --- |
| [smolfilament.c](source/Smoldyn/smolfilament.c) | Native topology/chemistry/mechanics, extension hooks, legacy aliases and force bridges |
| [smolboxes.c](source/Smoldyn/smolboxes.c) | Shared native segment payload, capsule halo queries, occupied boxes and initialization policy |
| [smolfilamentparallel.c](source/Smoldyn/smolfilamentparallel.c) | Optional scheduling of native end-state and length-bank work |
| [smolfilamentsteric.c](source/Smoldyn/smolfilamentsteric.c) | Capsule forces, native-junction trimming, topology cache and shared-node movement |
| [smolfilamentimplicit.c](source/Smoldyn/smolfilamentimplicit.c) | Coupled residual, native interaction refresh, rotational scaling and solver dispatch |
| [smolfilamentoperator.c](source/Smoldyn/smolfilamentoperator.c) | Updated cached derivatives and explicit reference fallback gates |
| [smolfilamentprecond.c](source/Smoldyn/smolfilamentprecond.c) | Native stiffness/thickness and junction-side banded blocks |
| [smolfilamentconstraint.c](source/Smoldyn/smolfilamentconstraint.c) | Shared-node fixed lengths using canonical end state and length banks |
| [smolfilamentgrowth.c](source/Smoldyn/smolfilamentgrowth.c) | Retained mechanochemical growth, native topology/binding remap and compatibility validation |
| [smolsurface.c](source/Smoldyn/smolsurface.c) | Ideal panel clamp alongside upstream surface architecture |
| [test_filament_upstream.c](tests/test_filament_upstream.c) | Added integration oracles at the native/extension boundary |

The analysis/converter changes live in the surrounding project and sibling
viewer working copy, outside the Smoldyn Git repository. They were updated
locally and tested; the Smoldyn merge commit itself versions the engine,
native fixtures, profiling tools and these compatibility notes.
