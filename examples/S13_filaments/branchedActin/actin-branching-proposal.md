# Proposal: filament branching for branched-actin simulation

Branch: `feature/actin-branching`
Files touched: `source/Smoldyn/smoldyn.h`, `source/Smoldyn/smolfilament.c`
New example: `examples/S13_filaments/branching2D.txt`

Status: **design-complete draft.** The translation unit passes `clang -fsyntax-only`
(as C) against the real headers with zero errors. It has **not** been run through the
full CMake build or simulated yet — see "Build & test" at the end.

---

## 1. TL;DR

Smoldyn's filament engine already models semiflexible polymers (worm-like-chain
stretch + bend energy, thermal forces, Euler/RK2/RK4/implicit dynamics) and
treadmilling. **Branching is not implemented** — the data model reserves slots for it
(`nbranch`, `branchspots`, `branches`, `frontend`/`backend` in `filamentstruct`), but
nothing ever writes them, no parser command creates a branch, and the dynamics never
read them. There is also a latent NULL-deref bug in the reserved code path.

This branch adds the minimum needed to nucleate and hold branches:

1. A **latent-bug fix** in `filAlloc` (branch-array growth never reassigned
   `fil->branchspots`, guaranteeing a segfault the first time a branch was recorded).
2. Three new filament-type parameters: `branch_rate`, `branch_angle`, `branch_spread`.
3. A geometric primitive, **`filAddBranch()`**, that nucleates a daughter filament off
   the side of a mother at a chosen segment and the Arp2/3 ~70° angle.
4. **`filBranchDynamics()`** — stochastic (Poisson) nucleation each timestep,
   proportional to `branch_rate × mother_length × dt`, mirroring the treadmilling block.
5. **`filPinBranches()`** — a rigid positional constraint that keeps each daughter's
   anchored end attached to the mother's (moving) branch point after the mechanical step.
6. A manual `branch` command (for deterministic/test setups) and log output.

Daughters default to the mother's own filament type, so daughters become mothers and a
**dendritic network** emerges — the essential topology of lamellipodial/endocytic actin.

---

## 2. What the filament engine can do today

Read from `smolfilament.c` (v2.75). A **filament** is a chain of **segments**; each
segment stores a length, thickness, and a *relative* rotation quaternion `qrel` plus an
*absolute* one `qabs`, with `qabs_i = qabs_{i-1} · qrel_i` (`filAddSegment`). Nodes are
the joints between segments.

Supported:

- **Geometry / construction**: `add_segment`, `random_segments`, `random_filament`,
  `remove_segment`, `translate`, `modify_segment`, `copy_to`, `sequence`.
- **Mechanics** (`filComputeForces`): stretch energy about `standard_length` (`klen`),
  bending energy about `standard_angle` (`kypr`), and thermal forces (`kT`). Integrators:
  `none`, `euler`, `RK2`, `RK4`, `eulermat`, `implicit`, `implicitold`
  (`filDynamics` → `filEulerDynamics`/…).
- **Turnover**: `treadmill_rate` — each step, a Poisson number of monomer add/remove
  events shift the filament (add random segment at one end, remove from the other).
- **Interactions**: `filSegmentXSurface`, `filSegmentXFilament` detect crossings.

Key structural facts that shape this proposal:

- **`filComputeForces` is per-filament and isolated** — it sums stretch/bend/thermal
  forces within one filament only. There is **no cross-filament force coupling**, and
  `frontend`/`backend` are never read by the dynamics. So a branch junction cannot yet
  be enforced by a shared force term without new machinery.
- **`branchspots`/`branches` are allocated and copied but never populated.** The only
  writes anywhere in the engine are `= NULL`/`= 0` and bulk copy in `filCopyFilament`.
- The `filAlloc` growth path for branch arrays is **buggy** (see §5.1).

---

## 3. What branched actin needs (biology → model)

The Arp2/3 complex binds the side of an existing ("mother") filament and nucleates a new
("daughter") filament at a stereotyped **~70°** angle; the daughter elongates at its
barbed end while its pointed end stays anchored at the junction. Daughters nucleate
their own branches → a dendritic network. Branches are stable but can **debranch**, and
elongation is terminated by **capping**.

Mapping onto Smoldyn's model:

| Biology | Smoldyn representation | Status |
|---|---|---|
| Mother filament | `filamentstruct` (segment chain) | exists |
| Branch point on the side | segment index / node on the mother | **added** (`branchspots`) |
| Daughter at ~70° | new filament, seg-0 orientation = mother `qabs` ∘ 70° rotation | **added** (`filAddBranch`) |
| Barbed-end growth | `treadmill_rate` (add at back, remove at front) | exists |
| Junction stays attached | positional constraint each step | **added, v1** (`filPinBranches`) |
| Arp2/3 as a diffusing species | molecule ↔ filament reaction | **not yet** (§6) |
| Debranching / capping | rate-driven branch removal / growth stop | **not yet** (§6) |

---

## 4. The proposed additions (this branch)

### 4.1 New filament-type parameters (`smoldyn.h`, `filtypeSetParam`, parser)

```
branch_rate     <rate>            # nucleation per unit mother length per time (0 = off)
branch_angle    <yaw> [pitch roll] # mean daughter orientation rel. to mother, radians (default 70°)
branch_spread   <sigma>           # Gaussian jitter added to the polar angle, radians
branch_segments <n>               # segments a daughter is born with (default 1)
```

`branch_segments` makes daughters short multi-segment filaments rather than single
stubs: `filAddBranch` places the first segment at the branch angle, then extends the
daughter straight for the remaining `n-1` segments. This gives longer, more legible
branches with multiple internal sites for sub-branching. It is *not* elongation over
time (§6, item 2) — a daughter is born at `n` segments and stays there.

Stored on `filamenttypestruct` as `branchrate`, `branchangle[3]`, `branchspread`, `branchsegments`, plus
`branchtype` (a `filamenttypestruct*`; `NULL` ⇒ daughters share the mother's type — the
correct default for actin, where all filaments are F-actin).

### 4.2 `filAddBranch()` — the geometric primitive

Creates the daughter, positions its anchored end at the mother's branch point, orients it
by composing the branch rotation onto the mother segment's absolute frame, then records
the mother↔daughter link:

```c
Sph_Ypr2Qtn(angle, qbranch);              // branch rotation (yaw≈70°, roll=azimuth)
Sph_QtnxQtn(mseg->qabs, qbranch, qdaughter);  // compose in mother's absolute frame
Sph_Qtn2Ypr(qdaughter, daughterypr);      // filAddSegment(seg 0) wants a lab-frame ypr
filAddSegment(daughter, branchpos, len, daughterypr, thickness, 'b');
mother->branchspots[br] = seg;  mother->branches[br] = daughter;  daughter->backend = mother;
```

In 3D the azimuth (`roll`) is randomized so branches form a **cone** at the fixed polar
angle around the mother axis (dendritic in 3D); in 2D the branch is ±70° in-plane.

### 4.3 `filBranchDynamics()` — stochastic nucleation

Called from `filDynamics` right after the treadmilling block, gated by `branchrate > 0`.
For each existing mother, draw `Poisson(branch_rate · length · dt)` new branches and
attach each at a uniformly random segment. Uses a snapshot of `nfil` so daughters born
this step don't immediately branch (and so the loop is realloc-safe — filament structs
are stable even when the type's `fillist` array grows).

### 4.4 `filPinBranches()` — keeping branches attached (v1 constraint)

Because forces are computed per-filament, this branch enforces the junction as a **rigid
positional constraint** rather than a shared force: after all filaments move, each daughter
is translated so its anchored end sits on the mother's *current* branch point. Runs as a
final pass in `filDynamics`. It is O(total branches), needs no global solver, and composes
with existing dynamics and treadmilling.

---

## 5. Known limitations of v1 (call these out in review)

### 5.1 (fixed here) latent branch-array bug
`filAlloc` allocated `newbranchspots`, copied into it, but never did
`fil->branchspots = newbranchspots` (and leaked it). Since nothing ever created a branch,
this never fired; the first `branchspots[br] = seg` would have dereferenced a NULL/stale
pointer. Fixed in this branch.

### 5.2 position-only junction
`filPinBranches` pins the branch *point*, not the branch *angle*. Under strong bending the
~70° junction can splay; the anchor holds but the angle is not restored. A force-based
junction (below) fixes this.

### 5.3 constraint ordering for deep trees
Pinning is a single pass in arbitrary filament order, so a multi-level branch (A→B→C) may
take a few timesteps to fully settle after a large move. Fine at small `dt`; a root-first
topological ordering would make it exact.

### 5.4 no removal yet
No debranching or capping — networks only grow. Add rate-driven `branches[]` removal and a
per-filament "capped" flag to stop treadmilling.

### 5.5 not physically coupled to monomers
Nucleation is a phenomenological rate, not a function of Arp2/3 or G-actin concentration.

---

## 6. Roadmap to a real Arp2/3 actin simulator

1. **Angle-holding junction (v2 mechanics).** Add a junction term to `filComputeForces`
   so a daughter's basal node feels a stiff harmonic tether to the mother branch point
   plus a torsional spring toward `branch_angle`. This is the first place the engine needs
   a *cross-filament* force; simplest implementation is an external-force hook applied to
   node 0 of each daughter (and reaction force on the mother node), leaving the existing
   per-filament solver intact.
2. **Molecule-coupled nucleation.** Represent Arp2/3 (and optionally NPFs like WASP) as a
   diffusing species and let it react with a mother segment via the existing
   `filSegmentXFilament`/surface-crossing machinery, calling `filAddBranch` on success.
   This turns branching into a genuine reaction-diffusion process and connects filaments to
   Smoldyn's core chemistry — the payoff of doing this *inside* Smoldyn rather than a
   standalone actin code.
3. **Debranching + capping.** Rate-driven branch removal; a capping-protein reaction that
   sets a per-filament flag halting barbed-end treadmilling.
4. **Force feedback (Brownian ratchet).** Couple barbed-end growth rate to local load from
   a surface/membrane so the network can push — the endocytic-relevant regime.

Item 1 is the highest-value next step (makes branches mechanically real); item 2 is what
makes it an *actin* simulator rather than a branched-polymer simulator.

---

## 7. Build & test

```sh
# configure + build (from repo root)
mkdir -p build && cd build
cmake .. -DOPTION_STATIC=OFF && make -j

# run the demo (headless works too: add -t and set SMOLDYN_NO_PROMPT=1)
./smoldyn ../examples/S13_filaments/branching2D.txt
```

Sanity checks to add as regression tests:
- With `branch_rate 0`, behavior is byte-identical to current master (branching is inert).
- With `branch_rate > 0`, `nbranch` grows and `filOutput` lists daughters "at" mother
  segments; daughters' anchored ends stay coincident with the mother branch point across
  steps (verify `filPinBranches`).
- Free-energy/relaxation tests for a single filament are unchanged.
