# Proposal: surface-localized branch nucleation — design options

Roam issue: `[[ISS]] - Model nucleation of soluble Arp2/3 complex via membrane-bound NPFs`
(uid `h-pRFuwJY`); next-step notes at uid `9RPbYVpn0`.
Baseline: tip of `akamatsu/thermal-junction` (currently in review), which sits on
`filament-elongation-capping-v1` on `filament-branching-v1` on `master` @ `dd9fd22`.
Files that would be touched: `source/Smoldyn/smoldyn.h`, `source/Smoldyn/smolfilament.c`.

Status: **decided and Option A built** (2026-08-27, branch `akamatsu/branch-region`).

> **Decision (Matt, 2026-08-27).** Build Option A now as an explicit stopgap. The
> nucleator mechanism (§5–§6) is *not* being built by the lab: filament–molecule
> interaction is engine-core coupling that belongs with Steve, and these sections are
> handed off as design input for whenever he implements fiber/molecule interactions.
> All Option A documentation — keywords, example, manifest — states that the location
> gate is meant to be **replaced** by explicit nucleator binding once that capability
> exists. Implementation record: `CHANGES.md` Unit 4 addendum. During validation the
> unit also surfaced and fixed a real bug: the 3D branch birth cone did not hold the
> 70° polar angle (details in the Unit 4 addendum and the fix commit).

---

## 1. TL;DR

Branch nucleation today is spontaneous and location-free: `filBranchDynamics` draws
Poisson(`branch_rate`·length·dt) events per mother and picks a uniform random segment.
The target physics (the manuscript wishlist paragraph, §2) wants branching to happen
**on a membrane surface**, because that is where NPF-activated Arp2/3 lives.

Three ways to get there, in increasing fidelity and cost:

- **Option A — implicit, location-gated.** Keep spontaneous nucleation but accept an
  event only if the branch point is near a named surface (or inside a compartment).
  Pure geometry, no molecules, no new chemistry. ~1–2 days.
- **Option B — explicit diffusing Arp2/3.** A designated Smoldyn species diffuses,
  and a filament segment that comes within a capture radius of one nucleates a branch
  there, consuming the molecule. Real reaction–diffusion: finite pool, depletion,
  saturation. ~1.5–2 weeks.
- **Option C — surface-bound NPF pathway (the Smoldyn special).** Same engine feature
  as B; the *model file* uses Smoldyn's native surface chemistry (surface-bound
  species states, adsorption, surface reactions) to put activated Arp2/3 on the
  membrane — optionally fed by an NPF activation reaction. Zero engine cost beyond B.

**The load-bearing observation: B and C are one engine mechanism.** The feature to
build is "a species/state designated as a branch nucleator, with a capture radius and
a nucleation rate." Whether nucleators float in solution (B) or sit on a membrane in a
ring (C) is decided entirely in the model file, using chemistry Smoldyn already has.

**Recommendation (§10):** build the nucleator mechanism with Option C as the target
configuration. It is the first molecule↔filament coupling in Smoldyn — a structural
step that upstream policy says to discuss with Steve first — so send that question
early, and optionally build Option A (which needs no molecule coupling and is
unblocked during the wait) as a cheap null model in the meantime.

---

## 2. The target physics

From the manuscript wishlist (Roam, Feb 2025 — the paragraph behind the ISS):

> The plasma membrane base that the pit internalizes away from is a flat surface,
> where pre-activated branched actin nucleators (Arp2/3 complexes) are arranged in a
> ring around the pit (Mund 2018, Almeida-Souza 2018). A small number of actin mother
> filaments initialize with random positions and orientations in the vicinity of the
> pit, where they are free to diffuse according to 3D Brownian dynamics. The filaments
> polymerize and stochastically undergo capping at experimentally estimated rates.
> When they diffuse in proximity of an active Arp2/3 complex at the membrane, they can
> spontaneously bind and nucleate a new daughter filament at a 70° angle
> (Mullins 1998, Blanchoin 2000).

Everything in that paragraph except the bolded clause already ships on our branch:
flat surfaces (native), diffusing semiflexible mothers (native + thermal-force fix),
polymerization (`elongation_rate`), capping (`capping_rate`), 70° branch geometry with
real junction mechanics (`branch_angle` + `branch_force_angle`). The missing piece is
**"when they diffuse in proximity of an active Arp2/3 complex at the membrane"** —
branching conditioned on *where the filament is* and *what it meets there*, instead of
a uniform rate along every mother.

Two properties of the target are worth naming because they discriminate between
options:

1. **Localization** — branches form at the membrane (in a ring), nowhere else.
2. **Accounting** — the nucleator pool is finite. A ring holds some number of Arp2/3
   complexes; each nucleation consumes one. Branch number saturates; local depletion
   shapes the network. This is likely load-bearing for the endocytosis model (branch
   count per pit is a measured, finite quantity — Serwas 2022 medians imply ~tens of
   filaments per site, not unbounded dendritic growth).

Option A delivers (1) only. Options B/C deliver both.

---

## 3. What the engine gives us today (relevant inventory)

Read from the current branch tip; all of this is stock Smoldyn or already-shipped work.

**Filament side** (`smolfilament.c`):
- `filBranchDynamics` — spontaneous Poisson nucleation, rate `branch_rate`·L·dt,
  uniform random segment, then `filAddBranch` at that segment's back node with the
  cone geometry (`branch_angle`/`branch_spread`/`branch_azimuth`).
- `filAddBranch(sim, mother, seg, angle, thickness, name)` — the geometric primitive.
  Takes an explicit segment index: **already exactly the right API for "branch here
  because a molecule is here."** Records the junction for `filPinBranches` and the
  junction springs.
- `filSegmentXSurface` — precedent that filament code reads surface geometry
  (used by `filAddOneRandomSegment` constraint checking).
- `filSegmentXFilament` — precedent for a proximity query against a segment, using
  `Geo_NearestSeg2SegDist`.

**Geometry / spatial-query side** (all public, all stock):
- `Geo_NearestLineSegPt(pt1, pt2, point, ans, dim, margin)` — nearest point on a
  segment to a point → point-to-segment distance. The capture-shell test.
- `closestpanelpt(pnl, dim, testpt, pnlpt, margin)` — nearest point on a panel →
  distance from a filament node to a named surface. Option A's test.
- `posincompart(sim, pos, cmpt, ...)` — compartment membership. Option A's alternate test.
- `boxscansphere(sim, pos, radius, bptr, wrap)` — iterate the virtual boxes
  overlapping a sphere, with periodic wrap. Molecules live in boxes, so this is the
  locality structure for finding nucleators near a segment without an O(N_mol) sweep.
  Idiom precedent: Steve's own command code (`smolcmd.c:1834ff`).

**Molecule / surface side** (native Smoldyn, zero new code):
- Species states: solution plus surface-bound (`front`/`back`/`up`/`down`), with
  per-state diffusion coefficients — a surface-bound Arp2/3 can be immobile or diffuse
  in-plane.
- `surface_mol` places molecules on surfaces/panels; adsorption (`rate` in a surface
  block) moves solution species onto surfaces at calibrated rates; surface reactions
  couple surface-bound and solution species. **The entire NPF → membrane-bound
  activated Arp2/3 chain is expressible in a config file today.**
- `molstring2index1` parses `species(state)` strings — parser precedent for
  designating a nucleator species+state.
- Timestep ordering (`simulatetimestep`): diffuse → surface collisions → assign to
  boxes → reactions → molsort → **filament dynamics**. At filament-dynamics time,
  molecule positions and box assignments are current — a nucleation scan there sees a
  consistent world.

**Known constraints that shape any design:**
- Every nucleation calls `filAddFilament`, which knocks the sim condition off `SCok`
  → full `simupdate` that step. Already true for spontaneous branching; not new cost,
  but nucleation-heavy models pay it every step.
- `filWrite` is a stub — no new per-filament state is serialized. `printFilaments` is
  the observability channel.
- No filament-destruction path exists → debranching/recycling of junction-bound
  Arp2/3 is out of reach until that lands (known limitation, carried forward).
- RNG discipline: any new dynamics block must be appended after existing ones and
  must return before drawing any random number when off, to keep fixed-seed
  regression byte-identical.

---

## 4. Option A — implicit: location-gated spontaneous branching

**Idea.** Branching stays spontaneous, but events only happen where the filament is
near a designated surface (or inside a compartment). No molecules; the membrane's
role is purely geometric.

**New filament-type parameters** (mutually exclusive; both require `branch_rate > 0`):

```
branch_surface <surface-name> <distance>   # branch point within distance of surface
branch_compartment <compartment-name>      # branch point inside compartment
```

**Mechanism: thinning.** Keep the existing draw exactly as is — Poisson event count,
uniform segment choice — then *accept* the event only if the candidate branch point
(the chosen segment's back node) passes the location test: within `distance` of any
panel of the named surface (via `closestpanelpt`), or inside the compartment (via
`posincompart`). Rejected events do nothing.

Thinning an inhomogeneous Poisson process is exact: the realized process is Poisson
with density `branch_rate` per unit mother length *inside the zone*, zero outside.
`branch_rate` keeps its meaning and units; the zone only masks it. Two properties fall
out for free:

- **No new RNG draws.** The accept test is deterministic geometry, so a model with the
  gate on draws the same random sequence as one with it off — only the accepted subset
  (and hence downstream `filAddBranch` draws) differs. With the feature off, nothing
  changes at all.
- **Cost ∝ candidate events, not segments.** `closestpanelpt` runs only on the rare
  drawn events (a few per step at most), never as a per-segment-per-step sweep.

(Existing approximation, inherited not introduced: segment choice is uniform by
*count*, not by length. Segments are ≈`standard_length` each, so this is the same
approximation the spontaneous path already makes.)

**`filCheckParams`:** error if both gates set; warn if a gate is set while
`branch_rate` is 0 (gate is dead); error if the named surface/compartment doesn't
exist (caught at parse).

**What A buys:** localization (property 1 of §2) — including ring patterning, if the
zone is drawn as a ring-shaped compartment or a dedicated gating surface. Also the
cheapest possible demo of membrane-localized dendritic growth for figures.

**What A cannot buy:** accounting (property 2). Branch density scales with mother
length residing in the zone, unboundedly. No depletion, no saturation, no per-pit
Arp2/3 budget. A alone does not satisfy the ISS.

**Why build it anyway (maybe):** it is a 1–2 day unit with no molecule coupling — no
structural question for Steve, buildable immediately — and it is the exact null model
for the scientific question "is finite-nucleator accounting load-bearing for network
architecture?" (compare A vs C at matched mean branch number). See sequencing, §10.

---

## 5. Option B — explicit diffusing Arp2/3 (the nucleator mechanism)

**Idea.** A designated Smoldyn species is a branch nucleator. When a filament segment
sits within a capture radius ρ of a nucleator molecule, a branch nucleates there at
rate λ, consuming (or transforming) the molecule. Branching becomes a genuine
bimolecular reaction–diffusion event between the chemistry engine and the filament
engine — the payoff of doing branched actin *inside* Smoldyn rather than in a
standalone actin code.

**New filament-type parameters:**

```
branch_nucleator <species>(<state>)     # designate the nucleator; state may be soln
                                        #   or a surface-bound state (fsoln/front/back/up/down)
branch_capture_radius <rho>             # length; capture shell around the filament axis
branch_nucleation_rate <lambda>         # 1/time, per nucleator within rho (Doi convention)
branch_product <species>(<state>)|none  # molecule's fate on nucleation; default none = consumed
```

All four live on the filament type: consistent with every other `branch_*` knob, and
the filament module owns the behavior. They compose with the existing parameters —
`branch_angle`, `branch_spread`, `branch_azimuth`, `branch_force_angle`,
`branch_segments` all apply to nucleator-born daughters unchanged. `branch_rate`
(spontaneous) and `branch_nucleator` (molecule-driven) are independent channels that
sum; either can be zero.

**Rate convention: Doi, not Smoluchowski.** Smoldyn's own bimolecular reactions use
Smoluchowski contact capture (react with probability 1 within a binding radius
calibrated from the rate constant). That calibration machinery is built for
sphere–sphere pairs and cannot be reused for a point-to-cylinder encounter. Since this
is new code, we are free to choose the Doi model instead: react at rate λ *while*
within radius ρ, applied per timestep as P = 1 − exp(−λ·dt) — the same
rate-to-probability convention as `smolreact.c` unimolecular reactions and our
capping/elongation work. Doi is timestep-convergent, recovers contact capture as
λ→∞, and gives a clean reaction-limited regime where λ is directly recoverable from
simulation output (§9). Accuracy constraint to document (and warn on in
`filCheckParams`): the nucleator's rms diffusive step √(2D·dt) should be ≲ ρ/3, or
molecules can jump across the capture shell between samples — the same class of
constraint Smoldyn's bimolecular algorithm carries.

**The scan** (new `filNucleatorDynamics(sim, filtype)`, called from `filDynamics`
*after* `filElongationDynamics` so all previously-tagged features keep their RNG draw
sequences; returns immediately when `branch_nucleator` is unset, before any draw):

```
for each filament f in a snapshot of fillist (no branching off daughters born this step):
    for each segment s:
        for each box B in boxscansphere(midpoint(s), rho + len(s)/2 + margin):
            for each live molecule m in B with (ident, mstate) == nucleator, not yet claimed this step:
                d = Geo_NearestLineSegPt(s.front, s.back, m.pos)      # distance to segment axis
                if d < rho:
                    claim m                                            # once per molecule per step
                    with probability 1 - exp(-lambda*dt):
                        filAddBranch(sim, f, s.index, cone_angle_draw, thk, NULL)
                        consume or transmute m per branch_product
```

**Dedupe is a correctness requirement, not an optimization.** A molecule within ρ of
two adjacent segments of the same mother must experience rate λ once, not once per
segment — otherwise halving `standard_length` (more, shorter segments in range)
changes the branching rate, violating the discretization-must-not-change-physics
principle that governs this whole project. Implementation: a per-step scratch list of
claimed molecule pointers in the filament work structure (only in-range molecules ever
enter it, so it stays tiny); a molecule already claimed is skipped by later segments
and later filaments. Side effect: consumption on success automatically prevents one
molecule nucleating two branches. Residual bias — earlier filaments in the list get
first claim on a contested molecule — is stochastically negligible at sane densities
and worth a code comment, not machinery.

**Where the branch lands.** The junction goes at the claimed segment: `filAddBranch`
with that segment's index, branch point at its back node (the existing contract; the
node is within ~`standard_length` of the true nearest point, which is below the
geometric noise of the cone draw). Daughter geometry, azimuth, and junction mechanics
are exactly the shipped machinery. A v2 nicety — setting the birth azimuth so the
daughter tilts away from the membrane the nucleator sits on — is deliberately out of
scope (§11).

**Molecule fate.** Default: consumed (the physical Arp2/3 is incorporated into the
junction; with no debranching path yet, removing it from play is the honest model).
`branch_product` instead transmutes it: same position, new species/state — a
surface-bound product keeps its panel; a solution product is released at the junction
point. A product species gives mass-conservation bookkeeping for free (`molcount` of
product = cumulative nucleation count) and is the future hook for
debranching-releases-Arp2/3.

**What B buys:** both target properties — localization *if the model localizes the
nucleators*, and full finite-pool accounting: depletion, saturation, dose–response of
branch number on Arp2/3 count.

**Biology caveat, stated plainly:** soluble Arp2/3 does not nucleate on filament
contact in reality — it needs NPF activation. Option B *as a model configuration*
(nucleators uniformly in solution) is a well-mixed control and a validation rig, not
the endocytosis model. The endocytosis configuration is Option C — same code.

---

## 6. Option C — surface-bound NPF pathway (the Smoldyn special)

**Idea.** Use the engine feature from §5 unchanged, and let Smoldyn's native surface
chemistry decide where nucleators are. The membrane-localized branching of the
wishlist paragraph is then a *model file*, not an engine feature:

```
# species and states
species arp npf                              # + whatever else the model carries
difc arp(soln) 3                             # soluble Arp2/3 diffuses
difc arp(front) 0                            # membrane-bound active Arp2/3: immobile (or slow)

# the membrane, with NPFs on it
start_surface membrane
  ...
  rate arp fsoln front <k_on_surface>        # simplest: direct adsorption = "activation"
end_surface
surface_mol 200 arp(front) membrane ...      # or: pre-activated ring placement, per the wishlist

# the filament type designates the SURFACE-BOUND state as nucleator
start_filament_type actin
  ...
  branch_nucleator arp(front)
  branch_capture_radius 0.01
  branch_nucleation_rate 10
  branch_product junction(front)             # bookkeeping species left at the branch site
end_filament_type
```

Gradations, all config-level, all zero engine cost:

- **v1, matching the wishlist verbatim:** skip activation chemistry entirely —
  `surface_mol` places pre-activated `arp(front)` in a ring around the pit position,
  finite count, immobile. This is exactly "pre-activated Arp2/3 arranged in a ring."
  (Placement of a literal annulus is a model-construction detail — explicit positioned
  `surface_mol` statements or a dedicated ring of panels — not an engine concern.)
- **Later, mechanistic:** an explicit NPF species on the surface plus a surface
  reaction `npf(front) + arp(fsoln) → npf_arp(front)` with `npf_arp(front)` as the
  designated nucleator; NPF recycling on nucleation via `branch_product`. Every piece
  of that chain is native, already-validated Smoldyn chemistry — the only
  lab-validated step is the one new coupling: nucleator + filament → branch.

**Why this is the right shape:** the engine stays minimal (one mechanism), the
biology lives in config files where it can be varied per experiment without
recompiling, and the division of labor matches Smoldyn's philosophy — surfaces and
chemistry are the simulator's mature core; we couple to it rather than duplicating it
inside the filament module.

**One capture-geometry note:** a surface-bound nucleator sits *on* the membrane
plane, so mothers must approach within ρ of the membrane for their segments to enter
the capture shell — proximity to the membrane is enforced by the capture geometry
itself, no extra gate needed.

---

## 7. Variants considered and set aside

- **Surface-owned nucleation rate** (a per-area `branch_nucleation` property on a
  surface, capturing any filament within a distance): Option A with the knob moved
  into Steve's surface module. Adds a parser/struct footprint in a module we
  otherwise don't touch, for no capability A doesn't have. If Steve prefers the knob
  there, it's a mechanical move — worth asking, not worth pre-building.
- **Nucleator budget without molecules** (location gate + a counter of remaining
  nucleation events per zone): re-implements a molecule count, badly — no spatial
  depletion, no rebinding, no chemistry hooks. Rejected.
- **De novo nucleation** (surface NPF/Arp2/3 spawns a *new mother* with no parent):
  different biology (formin-like/spontaneous nucleation), needs seed-filament
  machinery and interacts with the no-destruction limitation. Out of scope; the
  wishlist initializes mothers separately, and `random_filament` covers that.
- **Reusing Smoldyn's bimolecular reaction engine** by making segments pseudo-
  molecules: segments aren't in boxes, have extent, and renumber on `filArrayShift`;
  forcing them into the pair machinery is far more invasive than a read-only scan.
  Rejected.

---

## 8. Cross-module contract — the open question for Steve

This is the first molecule↔filament coupling in Smoldyn, and it has two halves of
very different weight:

- **Reading** molecule positions from filament code (boxes → molecules, compare
  distances): read-only traversal of another module's structures. Precedent exists —
  command code and observation code do this freely.
- **Writing** — consuming or transmuting the nucleator: a state change in the
  molecule module triggered from filament code. The project's module-boundary rule
  says filament code must not write molecule structures directly; the clean path is
  the same public API reaction code uses (`molkill` / ident-and-state change +
  `molchangeident`-style bookkeeping), called from `filNucleatorDynamics` with the
  list/box indices the scan already has in hand.

Per upstream policy (and this project's own practice), this is a **discuss-first**
addition: the structural question — "filament dynamics calling the molecule module's
public mutation API: acceptable, or would you rather a different seam (e.g., a
deferred kill list, or the coupling living outside `smolfilament.c`)?" — goes to
Steve *before* implementation, in exactly this framing. Note that the answer does not
block Option A, which never touches molecules.

Also worth flagging to Steve in the same conversation: nucleation already knocks the
sim condition off `SCok` per event (existing `filAddFilament` behavior, so
nucleation-heavy models re-run `simupdate` most steps) — we live with it, but he may
want to know it's about to get exercised much harder.

---

## 9. Validation plan (three layers, per project standard)

**Layer 1 — fixed-seed regression.** Feature off ⇒ byte-identical output vs stock
`master`, vs `filament-branching-v1`, vs `filament-elongation-capping-v1`, and vs the
thermal-junction tip, for the standard molecule model and filament model. Guaranteed
by construction (new blocks return before any RNG draw when off) — verified anyway.

**Layer 2 — analytic recovery of every new parameter.**

| # | Test | Prediction | Method |
|---|------|-----------|--------|
| 1 | Gate zone (A): static mother half in-zone, `dynamics none` | event rate = `branch_rate`·L_in; zero out-of-zone | branch-point positions + MLE rate; dt/2 invariance |
| 2 | Capture cutoff (B): immobile nucleators (D=0) placed at set distances from a static mother | events iff d < ρ, sharp | placement sweep across ρ |
| 3 | λ recovery, reaction-limited: immobile nucleators inside the shell | per-molecule waiting time ~ Exp(λ) | MLE λ ± CI; invariant under dt/2 |
| 4 | `standard_length` invariance: same geometry, segment length halved (molecule now in range of 2 segments) | identical realized λ | the dedupe test — this is the one that fails if claiming is per-segment |
| 5 | Well-mixed on-rate, reaction-limited: diffusing nucleators at concentration c, fast D | k = λ·c·V_cap, V_cap = πρ²L + (4/3)πρ³ (2D: 2ρL + πρ²) | branch count rate vs analytic |
| 6 | Doi accuracy bound: sweep dt so √(2D·dt) crosses ρ | rate rolls off as jumps out-sample the shell | characterization curve, documents the warning threshold |
| 7 | Mass conservation | branches born = nucleators consumed = product molecules created | `molcount` + `printFilaments` parent census, exact |
| 8 | C-chain integration: surface-bound nucleators in a ring, mothers with elongation+capping | branch points confined to the ring; junction angles 70° ± spread; branch count saturates at ring occupancy | positional histogram + angle census + saturation curve |

**Layer 3 — official suites.** Full pytest + CTest on the branch and on stock
`master` first (environmental failures are known and must reproduce on stock before
any debugging of ours).

**Examples** (all seeded, per S13 conventions): `branchZone3D.txt` (A — gated ring,
if A is built), `nucleatorBranching3D.txt` (B — well-mixed depletion/saturation),
`membraneNucleation3D.txt` (C — the wishlist configuration: flat membrane, ring of
pre-activated surface-bound Arp2/3, diffusing mothers, elongation + capping). The C
example doubles as the manuscript-facing demo.

---

## 10. Scope, sequencing, recommendation

| Unit | Contents | Tag | Estimate |
|------|----------|-----|----------|
| Steve conversation | §8 framing, email | — | send first; latency is the long pole |
| Unit 1 (optional) | Option A: 2 keywords, thinning gate, example, validation rows 1 | `filament-branch-region-v1` | 1–2 days |
| Unit 2 | Nucleator mechanism (§5): 4 params × the 8-touchpoint checklist, scan + dedupe + fate, 2 examples, validation rows 2–8 | `filament-branch-nucleator-v1` | ~1.5–2 weeks incl. validation |

**Recommended path:** send Steve the §8 question now; build Unit 1 while waiting
(it needs no molecule coupling, so it is unblocked, and it is the null model that
lets us later isolate whether finite-nucleator accounting changes network
architecture — a comparison worth a manuscript panel on its own); then build Unit 2
with Option C as the acceptance configuration. If time pressure forces a cut, Unit 1
is the cut — the ISS deliverable is Unit 2.

Both units branch off the thermal-junction tip **after** its review lands; the
junction springs are what make molecule-born junctions mechanically meaningful, but
nothing here depends on the content of that review.

---

## 11. Deliberately out of scope (carried as known limitations)

- **Debranching / Arp2/3 recycling from junctions** — blocked by the absent
  filament-destruction path (known engine limitation, already on Steve's list).
  `branch_product` is the forward-compatible hook.
- **Membrane-directed birth azimuth** (daughter tilted away from the nucleator's
  panel) — geometry nicety, revisit after Unit 2 validation shows whether random
  azimuth already reproduces target network architecture.
- **NPF activation kinetics fitting** — pure config; belongs to the endocytosis
  model, not the engine.
- **Brownian-ratchet load feedback at the membrane** — separate roadmap item;
  `filAddOneRandomSegment`'s retry-on-crossing path remains the natural hook.
- **Serialization** — `filWrite` remains a stub; nucleator params are not saved by
  `savesim` (project-wide known limitation).
