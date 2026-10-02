# Proposal: branch occupancy and spacing — limiting branches per node

Roam context: experiment page `@simulation/Test and add to filament capabilities in
Smoldyn` (uid `q-ZjFjlRE`); the biology below is drawn from the lab graph's Arp2/3
stoichiometry thread — the QUE `What is the stoichiometry of Arp2/3 complex binding
to actin filaments?` (uid `Ixf54SyN_`) and its two competing CLMs, "can" (uid
`XGu-5_0m_`) vs "cannot" (uid `rRkcZCw3c`) bind on opposite sides on adjacent
subunits, both grounded in the Schur/Fassler correspondence EVDs, with
`[[ISS]] - Look for opposite-strand binding of arp2/3 complex on an actin filament`
(uid `6iUQTj6GH`) open to resolve them.
Baseline: tip of `akamatsu/filament-capabilities` @ `6927b20`
(post `filament-confinement-v1`).
Files that would be touched: `source/Smoldyn/smoldyn.h`, `source/Smoldyn/smolfilament.c`.

Status: **spec only — nothing built.** Written as design input; the decision of which
option (if any) to build, and at which layer (this thinning rule vs the future
fiber–molecule nucleator mechanism), is open.

---

## 1. TL;DR

Branch *positions* are quantized and unregulated: `filBranchDynamics` picks a uniform
random segment and anchors the daughter at that segment's back node. So branch sites
land only on nodes (spacing in multiples of `standard_length`), and **nothing limits
how many branches occupy one node** — repeated draws of the same segment stack
junctions at a single point with no steric spacing. Real Arp2/3 has a footprint:
singles at most every ~5 subunits (≈13.75 nm) if opposite-side binding is excluded,
or near-coaxial opposite-facing pairs every ~7 subunits if it isn't — the lab graph
holds these as two competing, unresolved claims (§3).

Three options, in increasing fidelity:

- **Option 1 — `branch_max_per_node <n>`.** Reject a drawn event if the chosen node
  already hosts `n` branches. ~15 lines, but the rule's physical meaning changes with
  `standard_length` (it caps branches *per segment*, not per length of actin).
- **Option 2 — `branch_exclusion <length>` (recommended).** Reject a drawn event if
  any existing branch on the mother lies within `length` of the candidate node along
  the contour. The parameter *is* the Arp2/3 footprint (≈0.014 µm), it is independent
  of discretization, and any positive value also enforces one-per-node (same-node
  distance is 0).
- **Option 3 — opposite-facing pairs.** Allow occupancy 2 at a site when the second
  daughter's azimuth is pinned ~180° from the first. Instantiates the graph's
  unresolved "can" claim (§3) — a sweepable model choice, second in build order (§6).

Both 1 and 2 are deterministic accept tests in the same thinning style as
`branch_surface`: no new RNG draws, byte-identical when off, and a rejected event
consumes only its segment draw.

---

## 2. What the code does today

- Event count: Poisson with mean `branch_rate`·contour-length·dt — continuous in
  length, no quantization there.
- Spot: `seg = intrand(fil->nseg)`; the daughter is anchored at that segment's back
  node and `branchspots[br] = seg` records it. Consequences:
  1. **Branch positions sit on nodes** — inter-branch spacing along a mother is a
     multiple of `standard_length`. The spacing histogram of a grown network is a
     comb, not the continuous distribution real dendritic networks have.
  2. **The spot draw is uniform per segment, not per unit length** — exact only when
     segment lengths are equal (already noted for the region gate; same sampling).
  3. **No occupancy rule.** Nothing reads `branchspots` during nucleation, so two or
     more daughters can share one node. At high `branch_rate`·length·dt (e.g. the
     compression proof-of-concept, where the region gate concentrated all mother
     length into the gate slab) stacked junctions are common.
- Azimuth: uniform random per daughter (or a fixed `branch_azimuth`), so co-located
  daughters land at arbitrary relative azimuths — a geometry no Arp2/3 lattice
  arrangement produces.

Why it matters for the load-adaptation model specifically: mean filament length in
the Bieling validation target is ≈300 nm (≈110 subunits). At `standard_length`
0.005–0.01 µm a mother offers only ~30–60 candidate sites, each unlimited — so
branch-spacing statistics, junction clustering, and the mechanical connectivity of
the network are all set by an unphysical free parameter. The lab has prior Cytosim
evidence that branch-site periodicity is architecturally sensitive: sweeping Arp2/3
binding periodicity changed simulated endocytic internalization (>50 nm only for
periodicity ≤ 8.25 nm) and filaments-per-cluster plateaued ~15 above a 10 nm lattice
(`@cytosim/vary Arp2/3 complex periodicity on endocytic actin filaments`).

---

## 3. The biology (what the lab graph says)

The graph treats this as an **open question with two competing claims** — the QUE
"What is the stoichiometry of Arp2/3 complex binding to actin filaments?" (uid
`Ixf54SyN_`) lists them side by side, literally joined by "versus":

- `[[CLM]] - Arp2/3 complex CAN bind on opposite sides of a single filament on
  adjacent subunits` (uid `XGu-5_0m_`): first contacts at subunits n and n+1 (the two
  strands; ~2.75 nm axial offset, roughly opposite azimuth by the ~167°/subunit
  helical rotation), then n+6 and n+7 — up to **4 complexes per 7 subunits**,
  arranged as near-coaxial opposite-facing pairs ~16.5 nm apart.
- `[[CLM]] - Arp2/3 complex CANNOT bind on opposite sides on adjacent subunits`
  (uid `rRkcZCw3c`): singles at n, n+5, n+10 — at most **1 per 5 subunits
  ≈ 13.75 nm**.

**Both claims derive from the same source, and neither has direct evidence.** The
Schur/Fassler correspondence (personal communication, alongside Fassler/Dimchev/Schur
2020 subtomogram averaging) is a steric-clash analysis of what the ArpC3 footprint
*permits*; it grounds both CLMs and adjudicates neither. What observation adds:

- The measured footprint covers parts of **five mother subunits on one side**
  (Fassler 2020).
- Observed in-cell branch spacing runs as sparse as **once per helical turn**
  (~13 subunits ≈ 36 nm; Fassler 2020, Vinzenz 2012) — well below either steric
  ceiling, so the ceilings bound the model, they don't set its operating point.
- On the "can" side: Florian Fassler has seen an opposite-strand double branch in
  tomograms once, and a June 2025 lab note flags a SPIN90 preprint possibly showing
  two complexes on opposite sides (unconfirmed lead).
- An open ISS ("Look for opposite-strand binding of arp2/3 complex on an actin
  filament", uid `6iUQTj6GH`) exists precisely to resolve between the two claims.

Consequences for this spec: an **exclusion rule is common to both worlds** — the
disagreement is about pairing at a site, not about whether a footprint exists — so
`branch_exclusion` is safe to build regardless of how the question resolves, with a
sweepable value from ≈0.014 µm (steric ceiling, singles) to ≈0.036 µm (observed
in-cell spacing). The **pair allowance (§6) is the knob that instantiates the "can"
claim** — a model choice to sweep, not a default and not a discard.

---

## 4. Option 1 — `branch_max_per_node <n>`

Parser: integer ≥ 0 on the filament type; default **0 = unlimited** (inert, no
behavior change). In `filBranchDynamics`, after the region-gate accept and before any
angle draw: scan `fil->branchspots[]` for entries equal to the drawn `seg`; reject the
event if the count ≥ n. O(nbranch) per event, no allocation, no RNG.

Why we don't recommend it as the durable knob: the rule is "per `standard_length` of
mother", so halving `standard_length` doubles the permitted branch density — a
discretization choice changing the physics, which violates the design principle every
other parameter on this branch follows. It only matches the biology at
`standard_length` ≈ 13.75 nm. Fine as a stopgap; wrong as the interface.

---

## 5. Option 2 — `branch_exclusion <length>` (recommended)

Parser: length ≥ 0 on the filament type; default **0 = off** (inert). Recommended
model value **0.014 µm** (5 subunits × 2.75 nm), worth sweeping — the periodicity
literature disagrees, and the lab has swept exactly this parameter in Cytosim.

Accept test, placed with the region gate (before any angle draw, so a rejected event
still consumes only its segment draw): compute the candidate node's contour position
(prefix sum of segment lengths up to `seg`); for each existing branch, compute the
same from `branchspots[i]`; reject if any |Δ| < `branch_exclusion`. O(nseg + nbranch)
per event with a per-filament prefix-sum pass; negligible at current scales.

Properties worth arguing with:

- **Discretization-independent.** The saturated branch density is 1 per exclusion
  length regardless of `standard_length` — refining the mesh does not change the
  physics. This is the argument for Option 2 over Option 1, and it is testable (§9).
- **Subsumes one-per-node.** Same-node candidates are at distance 0, so any positive
  exclusion forbids stacking. There is no need for both keywords.
- **Placement stays node-quantized.** The exclusion governs *spacing*; branches still
  land on nodes. That residual comb is bounded by `standard_length` < exclusion and
  disappears as the mesh refines — the opposite of Option 1's behavior.
- **Composes with the region gate** (both are deterministic thinning; order between
  them is irrelevant to the RNG sequence) and with `filArrayShift` (positions are
  recomputed fresh from `branchspots` each event; front-end removal already drops the
  branches whose segment goes).
- **Rate semantics change at saturation.** As sites fill, the realized nucleation rate
  falls below `branch_rate` — this is the desired steric saturation, but rate-recovery
  validation must run in the unsaturated regime, and the manifest should say so.

Cross-parameter checks in `filCheckParams`: error on negative; warn when
`branch_exclusion` > 0 with `branch_rate` 0 (dead parameter); warn when
`branch_exclusion` < `standard_length` (the rule then degenerates to one-per-node —
legitimate, but the user should know the length is not resolved).

---

## 6. Option 3 — opposite-facing pairs (the "can"-claim knob)

The n/n+1 pair geometry maps onto the engine as: a site may hold **two** daughters
iff the second's azimuth is pinned ≈180° from the first (the ~2.75 nm intra-pair
offset is far below any realistic `standard_length`, so "same node" is the right
approximation). The per-junction birth azimuth is already recorded (`branchazim0[]`),
so the state exists; the accept path would treat an occupied-by-one site as
accept-with-constrained-azimuth (the uniform phi is still drawn, then overridden,
preserving the draw sequence) and an occupied-by-two site as reject. Under Option 2
bookkeeping a pair counts as one site for exclusion against neighbors.

Status per the graph (§3): the "can" and "cannot" claims are both live and both
evidence-free, so this is neither a default nor a discard — it is the flag that lets
the model run in either world. Build order: second, behind `branch_exclusion`, which
both worlds need. There is also a payoff beyond fidelity: running the load-adaptation
model in pair mode vs single mode would show whether the two claims are even
*discriminable* by network architecture or mechanics under load — if they are, the
simulation hands the open ISS a testable signature instead of waiting on it.

---

## 7. What this does and does not fix

- **Fixes:** stacked junctions; the unbounded per-node occupancy; the
  `standard_length`-dependence of maximum branch density; the unphysical
  branch-spacing comb (bounded, and vanishing with mesh refinement, under Option 2).
- **Bounds but does not fix:** the compression runaway seen in the proof-of-concept
  (135 → 12,271 filaments). Exclusion caps branches per unit mother length at
  1/`branch_exclusion`, but total mother length still grows autocatalytically as
  daughters elongate into the gate slab. The runaway's actual cause is the absence of
  nucleator depletion, which belongs to the explicit-nucleator mechanism (§8) — this
  rule just keeps the density finite per length while that arrives.

---

## 8. Relation to the explicit-nucleator mechanism

Like the `branch_surface` gate, this rule is thinning-as-stopgap for something the
fiber–molecule mechanism will make emergent: once a branch nucleates by a **bound
Arp2/3 molecule** consuming its site, occupancy is explicit and the exclusion length
becomes the binding-site footprint in the binding rule itself. If you'd rather own
the spacing rule at that layer, Option 2's parameter and semantics are written to
transfer (an exclusion length on nucleator binding, not on spontaneous draws) — in
that case we would still want the stopgap in the interim, for the same reason the
region gate exists.

---

## 9. Validation plan (three layers, per the branch's convention)

1. **Fixed-seed regression.** `branch_exclusion` 0 → byte-identical to `6927b20` on
   the standard molecule/reaction model and a branching filament model (the accept
   test draws no random numbers, and the guard returns before any work when off).
2. **Analytic checks.**
   - *Hard invariants:* in any run with exclusion on, minimum inter-branch contour
     spacing ≥ exclusion and zero same-node duplicates — assertable in
     post-processing over every `printFilaments` frame.
   - *Unsaturated rate recovery:* at low `branch_rate`, realized rate within a few %
     of nominal (thinning negligible), invariant under dt halving.
   - *Discretization invariance (the Option 2 payoff):* saturate a static mother at
     high `branch_rate`; the jammed branch density per µm must be unchanged when
     `standard_length` is halved at fixed exclusion. (Option 1 fails this by exactly
     2× — worth measuring once as the argument against it.) For `standard_length` ≪
     exclusion the jammed density has a closed form to compare against: continuum
     random sequential adsorption, Rényi's parking constant ≈ 0.7476/exclusion.
3. **Suites + example.** Full pytest/CTest against the stock baseline failure set; a
   seeded example (either extend `branchZone3D.txt` or a minimal
   `branchSpacing2D.txt` whose header states the footprint provenance).

---

## 10. Recommendation

Build **Option 2** (`branch_exclusion`, default 0), skip Option 1, and hold Option 3
as the follow-on knob — build it when the model is ready to sweep pair-vs-single
(§6), since both worlds need the exclusion first. One small unit in the established shape: struct field + inert
default + parser + `filOutput` echo + `filCheckParams` cross-checks + the accept test
in `filBranchDynamics` + seeded example + the three validation layers. Flag in
CHANGES.md as a thinning stopgap with the same replace-when-nucleators-exist status
as the region gate, and put the layer-ownership question (§8) to Steve alongside it.
