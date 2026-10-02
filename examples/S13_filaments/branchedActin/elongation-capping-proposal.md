# Proposal: plus-end elongation and capping for filaments

Second atomic task on branch `akamatsu/filament-capabilities`, following
`filament-branching-v1`. Companion audit of the current engine:
`elongation-capping-report.md` — read §2, §3 and §4 of that first; this document assumes
them.

Same ground rules as the branching work: **additive, opt-in, backward-compatible**, with
every new parameter defaulting to off and no RNG draw executed when a feature is off, so
fixed-seed runs stay byte-identical to the current tip and to stock `master`.

---

## 1. TL;DR

Two capabilities, in priority order:

1. **Elongation** — a filament grows at a *defined velocity* from its plus (barbed) end.
   New per-type parameter `elongation_rate` in **length per time**, implemented with a
   per-filament length accumulator so the growth velocity is exact and independent of the
   `standard_length` discretization. This is the thing the module has never had: a rate
   that increases length (report §2.4).
2. **Capping** — a stochastic, per-filament halt of that growth. New per-type
   `capping_rate` (and optional `uncapping_rate`) in **1/time**, setting a per-filament
   `capped` flag that blocks plus-end monomer addition.

Plus one supporting concept: an explicit **`plus_end`** designation on the filament type,
so polarity stops being three separate implicit conventions (report §3) and every future
polarized process references one field.

Together these are worth more than the sum: elongation alone makes filaments grow without
limit; elongation + capping gives a filament population an **exponential length
distribution with mean `elongation_rate / capping_rate`**, which is both the biologically
right answer and an exact analytic validation target (§7, V5).

Nothing in `filTreadmill` changes.

Two framing sections were added after review: **§2.7** checks every decision below against
how Cytosim (phenomenological) and MEDYAN (mechanistic) implement the same physics; **§2.8**
spells out why Smoldyn's existing *coupled-treadmilling* model is the wrong growth mechanism
for branched actin — which is the reason this task adds a net-elongation rate at the plus end
rather than tuning `treadmill_rate`.

---

## 2. Design decisions, and why

### 2.1 Elongation rate is a velocity (length/time), not an event rate (segments/time)

`treadmill_rate` is events per unit time (report §2.1). The obvious move is to copy it:
`poisrandD(rate*dt)` segment additions per step. **I propose not to**, for three reasons:

- **Segment length is a numerical choice, not physics.** `standard_length` is the
  discretization of the polymer. If elongation were segments/time, halving `standard_length`
  for a better-resolved filament would silently halve its growth speed. A velocity is
  invariant to the discretization; that invariance is directly testable (§7, V1).
- **A velocity is what the literature gives you.** Barbed-end growth is `k_on·[G]·δ`, which
  lands in µm/s. An event rate would have to be back-computed from `standard_length` every
  time a concentration changed.
- **Different units signal different semantics.** `treadmill_rate` is a turnover rate and
  `elongation_rate` is a growth velocity; giving them the same units would invite exactly
  the confusion the report's §2.2 warns about.

Cost: one new piece of per-filament state (the accumulator). Worth it.

### 2.2 The accumulator, and why it is exact

```c
fil->growbank += elongation_rate * dt;
while(fil->growbank >= filtype->stdlen) {
    add one segment at the plus end;          // it draws its own length L from filRandomLength
    fil->growbank -= L; }
```

The bank carries the fractional segment forward. After the loop `growbank` is bounded in
`(−max L, stdlen)`, so total contour length satisfies

&nbsp;&nbsp;&nbsp;&nbsp;`L(t) = elongation_rate · t + O(stdlen)`

with an error that is **bounded, not accumulating**, regardless of the segment-length
distribution — the bank is self-correcting. This is the property that makes the velocity
*exact in the long run* rather than exact only in expectation, and it holds even though
`filRandomLength`'s truncation at `>0` puts a slight positive bias on the mean segment
length.

Naive "emit a segment every `stdlen/v` seconds" schemes do not have this property.

One honesty caveat this buys: the exactness is in *contour length*, not in *tip position*.
The bank makes length track `v·t` in the long run, but the plus-end node is stationary
between additions and then jumps by ~`stdlen` — a staircase, not a ramp. That is invisible to
the population statistics this task delivers (V5) and is the price of keeping interior nodes
and `branchspots` untouched; it is the one property Cytosim's continuous-length scheme has
and this one does not. §2.7 point 1 explains the trade and the smooth-tip upgrade path.

### 2.3 Stochasticity: v1 is deterministic; Poisson monomer noise is deferred

v1 grows deterministically: the bank grows by exactly `v·dt` each step, and the only length
noise is the (bounded) segment-length draw. This is the whole growth model for the first
release.

There is a natural, physically honest extension — a per-type `elongation_monomer δ`, with the
bank growing by `δ · poisrandD(v·dt/δ)` (actual Poisson arrival of monomers of size δ), which
gives the correct length-diffusion coefficient `D_L = vδ/2` and reduces to the deterministic
case as δ → 0. It is what makes filament-length *variance* meaningful rather than an artifact
of the segment size. But it is not needed for the v1 deliverable (length *distributions*,
which come from capping, not from growth noise), so it is **deferred to a future feature**
(§8) and left out of the v1 configuration surface, struct, and validation. It is trivially
separable — one parameter, one line — so adding it later touches nothing in v1.

### 2.4 Capping blocks plus-end addition; it does not (yet) make filaments shrink

Physically, capping a treadmilling filament should produce net depolymerization and
eventual disappearance. **The engine cannot express "disappearance"** (report §4.2): there
is no filament-destruction path, and a vanishing mother would orphan its daughters'
`branches[]`/`branchspots[]` entries.

So v1: a capped plus end **stops accepting segments**, from elongation *and* from
treadmilling. A capped treadmilling filament therefore freezes rather than shrinking away.
That is a real modelling compromise and it should be stated in the example files, not
buried. The shrink-to-nothing version needs a filament-removal design first, and that is a
separate task (§8).

Because `capped` is always 0 when `capping_rate` is 0, adding the guard to the treadmill
loop changes nothing for existing models.

### 2.5 `filTreadmill` is not refactored

It is tempting to re-express treadmilling as "elongation at the plus end +
depolymerization at the minus end", which is the clean end state. Rejected for now:
it would change the RNG draw sequence and forfeit the byte-identical regression result
that the branching handoff leads with, in exchange for no capability the user can see.
Revisit if and when minus-end depolymerization lands (§8), at which point `treadmill_rate`
can become documented sugar over the two.

Consequence to document: if a model sets *both* `treadmill_rate` and `elongation_rate`,
they compose — turnover plus net growth. That is physically sensible and `filCheckParams`
should note it at warning level rather than forbid it.

### 2.6 Growth needs a guardrail

Elongation is the first mechanism that grows a filament without bound, and per-step cost
is linear in `nseg` (report §5). `elongation_max_length` (default 0 = unlimited) stops a
filament growing past a given contour length. Cheap insurance, and a crude stand-in for
"the barbed end reached something" until load coupling exists.

### 2.7 Prior art: how Cytosim and MEDYAN do this, and where this design lands

Two mature cytoskeleton simulators solve this problem from opposite ends of a spectrum.
Reading their source (Cytosim from GitLab HEAD; MEDYAN via the `simularium/medyan` fork,
since the canonical repo ships docs only) both validates the core decisions above and
sharpens three of them.

**The spectrum.** MEDYAN is fully mechanistic: there is no velocity variable anywhere.
Growth is a bimolecular reaction `G-actin + PlusEnd → Filament + PlusEnd` fired by a
Gillespie / Next-Reaction stochastic simulation, propensity `k_on · N(G-actin in the local
compartment) · N(plus-end marker)`; length is an *emergent* count of monomer-addition
events, each advancing the tip by one 2.7 nm monomer and depleting a spatially-resolved
diffusing G-actin pool. Cytosim is phenomenological: its `Fiber` classes carry a
`growing_speed` (µm/s) and integrate length deterministically each step — exactly the
velocity primitive this proposal adopts. This design sits with Cytosim, one notch more
detailed than nucleation-only: velocity is the primitive, monomer chemistry is abstracted,
and a future Poisson mode (§2.3, deferred to §8) would reach one step toward MEDYAN's shot
noise.

**What the comparison corroborates:**

- **Velocity units (Q1) — confirmed by both.** Cytosim's primary config unit is a velocity
  (`growing_speed`); MEDYAN's rate constant is `k_on·[G]`, a velocity divided by monomer
  size. Tellingly, Cytosim's `DynamicFiber` takes a velocity in config and converts it
  *internally* to a monomer-addition rate by dividing by `unit_length` — precisely this
  proposal's relationship between `elongation_rate` and `elongation_monomer`. Velocity as
  the interface, monomer as optional granularity, is the field consensus.
- **Capping as a per-end state that gates addition (§2.4) — confirmed by both.** Cytosim
  caps by putting an end in `STATE_WHITE` (dormant), which blocks assembly; MEDYAN caps by
  switching the plus end into a species for which no polymerization reaction exists. Both
  are exactly the semantics of the proposed `capped` bitmask — a per-filament, per-end flag
  that makes addition impossible without touching mechanics — and both treat the two ends as
  independently cappable, which is what the reserved minus-end bit is for.
- **A phenomenological cap rate is defensible even in the mechanistic engine.** MEDYAN's
  built-in capping is a *unimolecular* `AGINGREACTION` — a first-order plus-end state switch
  at rate `k_cap`, **not** a `[CP]`-dependent bimolecular binding. So even MEDYAN does not
  spend a diffusing capping-protein species by default. That is independent support for
  §6.1's decision to set `capping_rate` geometrically (`v/L̄`) rather than from CP kinetics.
- **Two length scales (monomer ⊂ segment) is the consensus architecture.** MEDYAN nests
  2.7 nm monomer sites inside ~108 nm mechanical cylinders; Cytosim separates target
  `segmentation` from monomer `unit_length`. The proposal's *future* split of
  `elongation_monomer` (≈2.75 nm, sub-segment) from `standard_length` (≈10 nm mechanical
  segment) is the same design (§6.4) — exactly what the deferred Poisson mode (§8) would add,
  which is why the monomer scale is a planned future feature rather than an oversight. v1 uses
  only the mechanical `standard_length` scale.

**Where the comparison changes the design:**

1. **The accumulator makes a staircase tip; Cytosim's continuous length does not — and that
   gap is where load-coupling will live.** This is the deepest engineering contrast. Cytosim
   carries *no* per-tip accumulator: `growP(δ)` lengthens the whole fiber continuously and it
   re-segments hysteretically, adding/removing a vertex only when the mean segment length
   leaves a 2/3–4/3 band around target. So Cytosim's tip advances smoothly every step; this
   proposal's tip is stationary between additions and jumps by ~`standard_length`. For the
   population-statistics deliverable (V5) the staircase is invisible and the accumulator's
   simplicity wins — and crucially, tip-only addition leaves every interior node and every
   `branchspot` untouched, whereas Cytosim's whole-fiber re-interpolation moves every vertex
   each step (actively harmful under Smoldyn's index-based branch bookkeeping). **The
   accumulator is the right v1 call *for this engine*.** But the staircase is a real
   limitation for the future Brownian-ratchet model, which reads an *instantaneous* tip
   force — a lurching tip gives a bursty signal. The clean upgrade, when load coupling lands,
   is to grow the terminal segment continuously with the existing `filLengthenSegment`
   primitive and split it once it exceeds ~2·`standard_length`: a smooth tip on the same
   segment model. Recorded so the accumulator is not mistaken for the end state.
2. **A blocked tip should not bank growth it will later surge (resolves Q3).** Both
   references answer the open Q3 the same way, against this draft's earlier lean. Cytosim
   multiplies speed by `exp(force/growing_force)`; MEDYAN multiplies the tip on-rate by the
   Brownian ratchet `exp(−f·x/kT)` — in both, growth an obstacle blocks is simply growth that
   does not happen, with nothing stored to discharge when the obstacle clears. A stalled
   Smoldyn tip that keeps banking `v·dt` and then adds a run of segments the instant it is
   unblocked is an artifact with no counterpart in either model. Resolution (folded into
   §4.2 and Q3): on a blocked add, skip the bank increment and clamp the bank to ≤
   `standard_length`. This is also the correct `f→∞` limit of the `exp(−f·δ/kT)` ratchet, so
   v1 stays forward-compatible instead of baking in behaviour the ratchet must undo.
3. **Monomer-pool depletion need not be all-or-nothing (noted, not planned).** §8 defers
   pool coupling because full spatial coupling "needs molecule↔filament reactions" — which
   MEDYAN does pay for (local reaction-diffusion G-actin). Cytosim shows there is also a
   cheap middle option: a single global scalar `free_polymer` (fraction of monomer not yet
   polymerized) multiplying the velocity, `speed = growing_speed · free_polymer · …`,
   capturing the dominant self-limiting effect (a growing network slows as bulk G-actin runs
   out) for ~15 lines and no per-molecule bookkeeping. **We do not plan to use it** — for
   these models an imposed velocity is the intended behaviour, so it stays a documented
   future option, not a v1 item. The only correction to §8 is factual: the pool is not gated
   on having reactions.

**One place this design is deliberately stricter than Cytosim.** Cytosim applies its
per-step transition probabilities inconsistently: catastrophe (its frequent transition)
uses linearized `rate·dt`, while rescue and rebirth use exact `1−exp(−rate·dt)`. This
proposal uses `1−exp(−rate·dt)` uniformly for `capping_rate` (§4.2), which is
timestep-independent and matches Smoldyn's own unimolecular convention (`smolreact.c:1323`).
That is the more rigorous choice, and Smoldyn's code — not Cytosim — is the precedent to
cite, so a reviewer who knows Cytosim doesn't misread this as gold-plating.

### 2.8 Why coupled treadmilling is the wrong growth model for branched actin

This is the biological reason the task adds a *new* net-elongation rate rather than reaching
for the growth mechanism Smoldyn already ships. Smoldyn's `treadmill_rate` is **lockstep-
coupled**: `filTreadmill` pairs one plus-end addition with one minus-end removal in the same
call (report §2.1), which structurally imposes **equal subunit flux at the two ends**
(`v_minus = v_plus`). That equality is false for the systems these models target, in two
independent ways:

- **Minus-end kinetics are intrinsically ~an order of magnitude slower than the plus end.**
  Pollard 1986 measured, for ATP-actin, an association rate constant of ≈ **11.6 µM⁻¹s⁻¹ at
  the barbed (plus) end vs ≈ 1.3 µM⁻¹s⁻¹ at the pointed (minus) end** — a ~9× difference in
  the same study, with the pointed end also having the higher critical concentration. At any
  shared monomer pool the pointed end therefore turns over far more slowly than the barbed
  end. Even the classic in-vitro *treadmilling steady state* — the one regime where the net
  fluxes balance — runs at a slow velocity (order nm/s) set by the *difference* between the
  two ends' critical concentrations and paid for by ATP hydrolysis; it is nothing like
  barbed-end elongation. A model that ties minus-end loss to plus-end gain in lockstep both
  overstates pointed-end dynamics by ~10× **and cannot represent a net-growing filament at
  all** — which is the physiological case during endocytic actin assembly.
- **In dendritic (Arp2/3) networks the minus end does not depolymerize, period.** The Arp2/3
  complex nucleates each daughter and remains bound at the daughter's pointed (minus) end,
  capping it and anchoring it to the side of the mother (Mullins, Heuser & Pollard 1998;
  Pollard & Borisy 2003). That end is simultaneously **buried at the junction and
  biochemically capped**: it cannot shed subunits until the branch is dismantled
  (debranching / Arp2/3 release, itself out of scope, §8). So the minus-end depolymerization
  rate is ≈ 0, not equal to the plus-end rate. Coupled treadmilling is *categorically*
  inapplicable to a branched network.

So the correct growth primitive here is **plus-end (barbed) addition with the pointed end
held fixed and capped** — precisely `filAddSegment(...,'b')` with no paired front removal,
which is what `elongation_rate` provides and what `treadmill_rate` structurally cannot
express.

The numerics point the same way, which is a useful check that the biology and the engine
agree. Treadmilling's front removal renumbers every segment through `filArrayShift`,
invalidating `branchspots` — the mechanism behind the measured 91° ± 51° junction-angle
decorrelation (report §4.1). Pure plus-end addition never renumbers (it adds at the back and
removes nothing), so it holds branch geometry (V8). Coupled treadmilling is thus both
biologically inapplicable *and* numerically hostile to branched actin; plus-end elongation is
both biologically right *and* numerically safe.

To be clear about what is **not** being claimed: `treadmill_rate` is not wrong in general and
is not being removed or altered (§2.5). It is the right tool for a genuinely treadmilling,
unbranched filament at steady state. We are declining to use it as the growth mechanism for
*branched, net-elongating* actin, and adding the primitive that fits that biology instead.

---

## 3. Proposed configuration surface

All new, all optional, all inert at their defaults.

| keyword | units | default | meaning |
|---|---|---|---|
| `plus_end` | `back` \| `front` | `back` | which geometric end is the barbed/plus end |
| `elongation_rate` | length/time (`\|L/T`) | `0` (off) | mean plus-end elongation velocity |
| `elongation_max_length` | length (`\|L`) | `0` (unlimited) | stop elongating past this contour length |
| `capping_rate` | 1/time (`\|/T`) | `0` (off) | plus-end capping rate, per filament |
| `uncapping_rate` | 1/time (`\|/T`) | `0` | plus-end uncapping rate, per filament |

`plus_end back` is the default because it is what all three existing implicit conventions
already assume (report §3). Setting it to `front` makes elongation grow at node 0 — but note
that front growth goes through `filAddSegment(...,'f')`, which calls `filArrayShift` on every
addition: it is O(`nseg`) per segment (vs. O(1) amortized at the back) and it renumbers,
re-invalidating `branchspots` unless the deferred `segoffset` fix is in (§8). `back` is both
the faster and the safer growing end; `front` is a generality knob, not a performance-equal
option.

The `units` column above is the parser's dimensional tag (`\|L/T` = length/time, etc.), which
enables *optional* per-value unit suffixes like `elongation_rate 0.32|um/s`. It does **not**
require the model to declare units: Smoldyn is scale-free unless a `units` statement is given,
and the new examples stay scale-free like every existing filament example (see §6).

Example — numbers chosen so that, read as µm and s, they match the endocytic scale (§6); no
`units` statement is needed:

```
start_filament_type actin
  color blue
  dynamics none
  standard_length 0.01         # 10 nm segments -> a 75 nm filament is ~8 segments
  elongation_rate 0.32         # um/s: k_on 11.6 uM^-1s^-1 x 10 uM x 2.75 nm/subunit
  capping_rate 4.3             # 1/s: set so mean length = 0.32/4.3 = 75 nm
  branch_rate 0.01
  branch_angle 1.22
end_filament_type
```

---

## 4. Proposed implementation

### 4.1 New state

`filamenttypestruct` (`smoldyn.h`), following the branching block's comment style:

```c
// --- polarized elongation and capping ---
char plusend;          // 'b' (default) or 'f': which end is the barbed/plus end
double elongrate;      // plus-end elongation velocity, length/time; 0 = off
double elongmaxlen;    // stop growing past this contour length; 0 = unbounded
double caprate;        // plus-end capping rate, 1/time; 0 = off
double uncaprate;      // plus-end uncapping rate, 1/time
// --- end elongation and capping ---
// (elongmonomer, for Poisson growth noise, is deferred — §8)
```

`filamentstruct`:

```c
double growbank;       // plus-end growth owed but not yet emitted as a segment
int capped;            // bitmask; bit 0 = plus end capped (bit 1 reserved for minus end)
```

A bitmask rather than a bool so pointed-end capping (tropomodulin) can be added later
without touching the struct layout again. `#define FILCAPPLUS 1`.

### 4.2 New functions

```c
double filContourLength(const filamentptr fil);
int    filElongate(simptr sim, filamentptr fil, double dlength);
int    filElongationDynamics(simptr sim, filamenttypeptr filtype);
int    filCappingDynamics(simptr sim, filamenttypeptr filtype);
```

`filContourLength` is an extraction of the loop `filBranchDynamics` already inlines
(`smolfilament.c:2720-2721`); reusing it in both is a behaviour-free, RNG-free refactor.

`filElongate` is the banked-growth primitive of §2.2. Two safety requirements:

- It reuses `filAddOneRandomSegment(..., constraints=1)`, so a plus end pressed against a
  surface **retries then fails**, exactly as treadmilling already does (report §2.3). On
  failure it must **not** carry the blocked step's growth forward: skip the bank increment
  for that step and clamp the bank to ≤ `stdlen`, so a stalled end sits at true zero velocity
  and can surge by at most one segment when unblocked. Banking indefinitely and then
  discharging a run of segments the instant the obstacle clears is an artifact with no
  counterpart in either Cytosim or MEDYAN (§2.7 point 2). This clamp is also the correct
  `f→∞` limit of the Brownian-ratchet law both simulators use, so it is the hook that model
  will *refine* rather than replace (§9, Q3).
- The `while` loop needs a hard iteration cap (say 1000 segments per filament per step)
  with a `simLog` warning, and `filCheckParams` must **error** if `elongation_rate > 0`
  while `standard_length <= 0` — otherwise the loop threshold is 0 and it never terminates.

`filCappingDynamics` uses `1 - exp(-rate*dt)` per filament per step, matching the
unimolecular-reaction convention at `smolreact.c:1323`, *not* `rate*dt`. This matters:
it is what makes the measured capping rate independent of the timestep (§7, V5).

No new includes are needed — `smolfilament.c` already pulls in `math.h` (for `exp`) and
`random2.h` (for `poisrandD`, `coinrandD`).

### 4.3 The hook in `filDynamics`

Appended after the existing blocks, inside the same per-type loop:

```
for each filament type:
    treadmill block        (unchanged, + one `if(fil->capped & FILCAPPLUS) continue;` guard)
    branch block           (unchanged)
    capping block          (NEW)
    elongation block       (NEW)
    integrator             (unchanged)
pin branches               (unchanged)
```

Order rationale:

- **Appended, not interleaved**, so the RNG draw sequence of any model that doesn't use the
  new parameters is untouched — byte-identity holds against both `filament-branching-v1`
  and stock `master`.
- **Capping before elongation**, so a filament capped on step *t* does not also grow on
  step *t*. Arbitrary but should be stated rather than left to fall out of the code.
- **Elongation before the integrator**, so a new segment relaxes within the same step.

The treadmill guard is `if(fil->capped & FILCAPPLUS) continue;` placed *before* the
`poisrandD` draw. `capped` is identically 0 whenever `capping_rate` is 0, so this is a
no-op for every existing model.

### 4.4 Wiring checklist

The six touchpoints from report §7, plus the per-filament state:

| where | what |
|---|---|
| `smoldyn.h` | 5 type fields, 2 filament fields, `FILCAPPLUS` |
| `filamentTypeAlloc` | defaults: `plusend='b'`, all rates 0 |
| `filAlloc` | `growbank=0`, `capped=0` on fresh allocation |
| `filAddFilament` | reset `growbank`/`capped` on list reuse (alongside `nseg`/`nbranch`) |
| `filCopyFilament` | copy both fields |
| `filtypeSetParam` | `"plusend"`, `"elongrate"`, `"elongmaxlen"`, `"caprate"`, `"uncaprate"` + range checks |
| `filtypeReadString` | 5 keyword blocks with `CHECKS`/`CHECKM` and unit tags |
| `filtypeOutput` | `simLog(sim, active?2:1, ...)` lines, quiet when inert |
| `filOutput` | per-filament capped state and pending `growbank` |
| `filCheckParams` | error on `elongation_rate>0 && standard_length<=0`; warn on `treadmill_rate!=0 && elongation_rate>0` |
| `filDynamics` | the two new blocks + the treadmill guard |
| `smolcmd.c` `cmdprintFilaments` | one `capped` column (§5) |

### 4.5 Two companion fixes, taken first

Risk-ordered per the project's commit convention (fix/tool → feature → examples). Both are
independent of the features and independently revertable. (A third fix — `branchspots`
surviving segment renumbering, via a `segoffset` — is **deferred to a future feature**, §8:
v1 is pure plus-end elongation, which never renumbers, so v1 does not need it.)

1. **`filAlloc:460` allocates the segment pointer array at struct size** — 17× memory
   over-allocation on an array that doubles as a filament grows (report §5). One word.
   Invisible today; a real cost once elongation runs. *Take this one regardless.*
2. **`filAddBranch` sets `daughter->backend` where it pins the daughter's front**
   (report §3) — cosmetic, no behaviour change, but the struct currently says the polarity
   backwards. Flip to `frontend` and have `printFilaments` read whichever is set.

---

## 5. Observability

`filWrite` is a stub, so filament state is unsaveable (report §6) and `printFilaments` is
the only channel. Add one field:

```
FIL <time> <type>:<name> <nseg> <parent-or-"-"> <capped> x0 y0 [z0] x1 y1 [z1] ...
```

This is a **breaking change to a format our own render scripts consume** —
`branching-figure.html` and `branching-timelapse.html` both parse it. Updating them is
part of the task, not an afterthought.

Contour length is not added as a column; it is exactly computable from the node
coordinates already in the line.

Deliberately **not** proposed: a `capped_color` graphics parameter. Distinguishing capped
filaments visually matters for the figure, but it can be done in our own HTML renderer off
the new column, without touching Smoldyn's drawing code. Keeps the contribution tight.

---

## 6. Physical calibration

Smoldyn is **scale-free unless a `units` statement is given** — and units are decidedly not
the convention: only 3 of ~1960 shipped example files declare them, and no filament example
does (the manual, SmoldynManual.tex §on units, presents `units` as a suggested-but-optional
convenience). So the new examples **stay scale-free like every existing filament example**;
we do *not* introduce a `units` declaration. The literature anchoring comes entirely from
*choosing the numeric values* so that, read as µm and s, they match the endocytic scale, with
the derivation in a header comment. This is Q5 in §9, resolved: no break with convention, no
dimensions introduced. (A modeler who wants explicit dimensions can still add `units um s` —
the parser tags in §3 support per-value suffixes — but nothing in this task requires it.)

### 6.1 Numbers, with the caveats that matter

| quantity | value | source |
|---|---|---|
| barbed-end `k_on`, ATP-actin | 11.6 µM⁻¹s⁻¹ (EM) | Pollard 1986, *JCB* 103:2747 |
| barbed-end `k_off` | 1.4 s⁻¹ | Pollard 1986 |
| axial rise per subunit δ | **2.75 nm** (≈364 subunits/µm) | cryo-EM helical params; Carlsson 2010 |
| ⇒ `elongation_rate` at 10 µM G-actin | **0.32 µm/s** | `(k_on·C − k_off)·δ` |
| ⇒ at 20 µM | 0.64 µm/s | |
| filament length, mammalian CME | branched median **59 nm**, unbranched **108 nm** (in situ cryo-ET) | Serwas et al. 2022, *Dev Cell* 57:1132 |
| ⇒ `capping_rate` = v / L̄ | **3–10 s⁻¹** | geometric, see below |
| CP uncapping `k_off` | 1.3 × 10⁻⁴ s⁻¹ (t½ ≈ 25–30 min) | Funk et al. 2021; Schafer et al. 1996 |

**The elongation constant has a real 35% uncertainty band.** TIRF (Kuhn & Pollard 2005)
gives `k_on` = 7.4, EM gives 11.6, microfluidics (Jégou et al. 2011) gives 10. Kuhn &
Pollard attribute the gap to undetected pauses in TIRF and to EM biasing toward the
longest filaments. Use 11.6 and cite the 7.4–11.6 range; separately, profilin-actin — which
is what essentially all polymerization-competent G-actin is in a cell — elongates barbed
ends **~30% slower** than free actin at the same concentration.

**Set `capping_rate` from the target length, not from CP kinetics.** The kinetic route,
`k_on,CP × [CP]` ≈ 0.49 × 1 µM ≈ 0.5 s⁻¹, predicts ~600 nm filaments — ten times longer
than the cryo-ET measurement. Berro et al. 2010 hit exactly this wall and had to fit
7 µM⁻¹s⁻¹, noting capping in endocytic patches runs ~10× faster than the cytoplasmic
concentration predicts (free CP is further reduced by V-1/myotrophin sequestration, which
makes the discrepancy worse, and CARMIL-family proteins release it locally). CP's `k_on`
is the weakest number in the whole set — published values span 0.11 → 0.49 → 7 µM⁻¹s⁻¹.

So the honest calibration is **geometric**: `capping_rate = elongation_rate / L̄`. At
v = 0.32 µm/s and L̄ = 60–110 nm that is **3–5 s⁻¹**; Akamatsu et al. 2020 used 6.3 s⁻¹.

This is a happy coincidence with §7's V5: the model's mean filament length *is* `v/k`, so
the validation test and the calibration procedure are the same measurement. Worth saying
in the example file's header.

`uncapping_rate` should be **0** for any simulation shorter than minutes — 1.3 × 10⁻⁴ s⁻¹
is a 90-minute half-life against a ~20 s endocytic event. The parameter exists for
completeness and for long-timescale network turnover, not for CME.

### 6.2 Two things to check before inheriting numbers

- **Branch angle.** Our examples use `branch_angle 1.22` = 70°, which is the
  *Acanthamoeba* EM value (Mullins et al. 1998, 70 ± 7°). For a mammalian endocytic model
  the best-matched number is **68 ± 9° measured in situ by cryo-ET at actual mammalian CME
  sites** (Serwas et al. 2022); Akamatsu et al. 2020 used 77 ± 13° from Blanchoin et al.
  2000 (bovine Arp2/3), which was the only in vitro study to report the variance. All three
  cluster in 68–78°, so little turns on it dynamically, but the citation should match the
  value. `branch_spread` currently 0.09 rad (~5°) is tighter than any of the measured
  spreads (7–13°).
- **Akamatsu et al. 2020 has two arithmetic slips in its Methods that we should not copy
  forward.** It states 20 µM actin with `k_on` = 11.6 gives "500 nm/s", but
  11.6 × 20 × 2.75 nm = **638 nm/s** (500 nm/s implies an effective `k_on` ≈ 9.1); and it
  states `k_cap` = 6.3 s⁻¹ "sets the mean filament length at 150 nm", but 500/6.3 =
  **79 nm** (the reported simulation output, 90 ± 80 nm, is consistent with ~79 nm plus
  load-dependent stalling, not with 150 nm). Re-derive `capping_rate` from whichever
  `elongation_rate` and target length we pick rather than importing the pair.

### 6.3 This connects to two open questions already in the lab graph

`akamatsulab` has two `[[QUE]]` stubs, both currently empty, that §6.1 walks straight into:
*"What is the basal capping rate of actin filaments by capping protein?"* and *"What is the
association rate constant of capping protein to actin filaments in the presence of CPI?"*
The 10× gap between `k_on,CP · [CP]` and the length-calibrated rate is exactly that
question, and V5 turns the simulation into a way to state it quantitatively.

The graph already holds EVD nodes for the branch angle (68 ± 9°, Serwas 2022; 71°,
Fäßler 2020) and for the 108 nm unbranched length distribution. It holds **no** EVD for the
elongation rate constants, for CP kinetics, or for the 2.75 nm axial rise — those three are
good `#evd-candidate` material if we want this spec's provenance in the graph.

One caveat worth carrying, because it bears directly on V5: the grounding notes on the
Serwas length EVD observe that the measured distribution is **not especially exponential —
there are few filaments below 60 nm**, possibly a manual-segmentation artifact. Our model
will produce a clean exponential by construction. That makes the exponential the *null*,
and the data's deficit of short filaments a real discrepancy worth probing rather than
matching. (Cassani 2024 on HeLa cortical blebs found roughly exponential lengths with 84%
under 100 nm, so the deviation may well be specific to the CME dataset.)

### 6.4 What the physical scale implies for `standard_length`

Endocytic filaments are 60–110 nm. To resolve them at all, `standard_length` has to be
≲ 10 nm, which puts a CME filament at roughly 6–11 segments. Two consequences:

- The accumulator's `O(stdlen)` error (§2.2) is bounded in absolute terms but is ~10% of a
  filament at this scale. Fine for network statistics, worth knowing before quoting a
  single filament's length.
- This scale is also why the deferred Poisson mode (§8) is the *right* future feature rather
  than a v1 need: a monomer is `elongation_monomer ≈ 0.00275` (in µm units), a *sub*-segment
  quantity (a 10 nm segment is ~4 monomers), so if length variance ever matters it must be
  added at the monomer scale — segment-granular growth noise would badly overstate it. v1
  simply does not model growth-noise variance, which is correct for the length-*distribution*
  deliverable.


---

## 7. Validation plan

The branching work's credibility came from measuring things and from the byte-identical
regression. Same standard here. V1, V4 and V5 are the ones that actually prove the
features work; the rest are guards.

| # | test | expectation |
|---|---|---|
| **V1** | single filament, elongation only, measure contour length vs. *t* | slope = `elongation_rate` within a few %; **and unchanged when `standard_length` is varied 1×/2×/4×** — this is the test that justifies velocity units (§2.1) |
| **V2** | same run, residual `L(t) − v·t` | bounded by one segment length for all *t*, not growing — proves the accumulator self-corrects (§2.2) |
| **V4** | ~500 filaments, `capping_rate k`, fraction uncapped vs. *t* | `exp(−k t)`; fitted rate recovers `k` |
| **V5** | elongation `v` + capping `k`, no treadmilling, no branching; a cohort of N filaments all seeded identically at *t*=0, run until all are capped | capping time `T ~ Exp(k)` and length is `L(0) + v·T`, so **`L(∞) − L(0) ~ Exponential(mean v/k)`** — one run validating both features *and* their coupling. Repeat at `dt` and `dt/10`: the mean must not move (this is what `1−exp(−k dt)` buys over `k·dt`) |
| **V6** | all new parameters unset, fixed seed | `molcount` and `printFilaments` output byte-identical to `filament-branching-v1`; treadmill+euler case byte-identical to stock `master` |
| **V7** | the 17 files in `examples/S13_filaments` | all still load and simulate |
| **V8** | branched network, elongation only vs. treadmilling only | junction angle stays at 70° ± thermal under elongation, unlike the 91° ± 51° treadmilling result — the payoff for report §4.1, *and* the demonstration that v1 needs no `segoffset` fix: pure plus-end elongation never renumbers, so `branchspots` stay valid without it |

(There is no V3: the `Var(L) = v·δ·t` test for Poisson growth noise is deferred with the
`elongation_monomer` feature, §8. Numbering is kept so V5/V6/V8 references elsewhere still
resolve.)

New example files:

- `elongation2D.txt` — one filament, one velocity, analytic expectation in the header comment.
- `cappedElongation2D.txt` — a population; the header states the expected exponential
  length distribution and its mean.
- `dendriticNetwork2D.txt` (+ a movie variant) — branching + elongation + capping, no
  treadmilling: the first configuration in this module that is actually a growing
  branched-actin network rather than a nucleation cartoon.

---

## 8. Explicitly out of scope

Each of these is a real thing we want; none belongs in this task.

Each row is a deliberate future feature, not an omission — flagged here so the scope of v1 is
unambiguous.

| deferred | why not now |
|---|---|
| **Poisson growth noise (`elongation_monomer`)** | the honest length-*variance* model (§2.3): bank grows by `δ·poisrandD(v·dt/δ)`, giving `Var(L)=v·δ·t` / `D_L=vδ/2` (its validation is the deferred V3). v1's deliverable is length *distributions* (from capping), not growth-noise variance, so v1 ships deterministic-only. One parameter, one line, one struct field, one validation test — fully separable, adds nothing to v1 |
| **`branchspots` surviving renumbering (`segoffset`)** | store branch positions as `index + segoffset`, a renumbering-invariant coordinate: add `long int segoffset` to `filamentstruct`, adjust it in `filArrayShift` (~30 lines). Fixes the 91° ± 51° dynamic-angle artifact (report §4.1). **v1 does not need it** — pure plus-end elongation never renumbers (V8) — so it is deferred; it becomes required the moment elongation is combined with treadmilling *or* `plus_end front`, and before any branching+treadmilling figure |
| minus-end depolymerization; capped filaments shrinking to nothing | needs a filament-destruction path and a policy for orphaned branches (report §4.2). Prerequisite for the physically complete capping story, and it is its own design problem |
| refactoring `treadmill_rate` into elongation + depolymerization | clean end state, but forfeits byte-identical regression for zero user-visible gain today (§2.5) |
| monomer-pool coupling (G-actin depleting as filaments grow) | roadmap item 2. Full spatial coupling needs molecule↔filament reactions (MEDYAN's route). A cheap non-spatial stand-in also exists — a global `free_polymer` velocity scale, as Cytosim uses (§2.7 point 3) — **but we do not plan to use it**; an imposed velocity is intended here. Elongation stays phenomenological in v1, consistent with nucleation |
| load-dependent / Brownian-ratchet elongation | roadmap item 4. The hook exists already (report §2.3) — a blocked plus end is a stalled end — but the force coupling is a separate design |
| debranching | pairs with capping conceptually, needs the same removal machinery as row 1 |
| angle-holding junction force | roadmap item 1; orthogonal to growth, and still the highest-value item for branch *mechanics* |
| pointed-end capping (tropomodulin) | the `capped` bitmask reserves the bit; no demand yet |
| `capped_color` in Smoldyn's graphics | do it in our renderer instead (§5) |

---

## 9. Effort, packaging, and the questions I need answered

### Effort

| commit | scope | estimate |
|---|---|---|
| 1. fix `filAlloc` pointer-array size | one word | 5 min |
| 2. fix daughter `frontend`/`backend` label | ~5 lines | 15 min |
| 3. feat: `plus_end` + elongation (deterministic) | ~140 lines across 2 files | half a day |
| 4. feat: capping / uncapping | ~70 lines | ~2 h |
| 5. tool: `printFilaments` capped column + renderer updates | ~10 lines C, 2 HTML files | ~1 h |
| 6. examples | 3–4 files | ~2 h |
| — validation V1, V2, V4–V8 + analysis scripts | this is the real cost | ~1 day |

**~2 days for both capabilities with validation at the standard the branching task set.
Elongation alone: ~1–1.5 days.** The code is the small part; V1/V4/V5 and the regression run
are most of it. (The two deferred features — Poisson noise and `segoffset`, §8 — are what
trimmed the estimate below the earlier ~2–2.5 days.)

### Packaging

Two tags on the same accumulating branch, per the one-tag-one-atomic-task convention, so
elongation can land without waiting on capping:

- **`filament-elongation-v1`** — commits 1, 2, 3, and the elongation example.
- **`filament-capping-v1`** — commits 4, 5, and the capping + dendritic-network examples.

Both additive, opt-in, backward-compatible; state it explicitly in `CHANGES.md`, with the
V6 byte-identity result quoted as evidence, exactly as the branching handoff did.

### Questions for review

1. **Units** — confirm `elongation_rate` as a velocity (length/time) rather than
   segments/time. Everything else follows from this. **Both Cytosim (`growing_speed`) and
   MEDYAN (`k_on·[G]`) express growth as a velocity, so this is the field-standard choice
   (§2.7).** *(§2.1; my recommendation: velocity)*
2. **Capped semantics** — confirm v1 capping *freezes* a treadmilling filament rather than
   letting it depolymerize away. *(§2.4; the alternative needs a filament-removal design
   first)*
3. **Blocked plus end** — when a surface blocks segment addition, does the filament keep its
   growth bank (a stalled end that surges when unblocked) or drop it? **Reading Cytosim and
   MEDYAN settled this: drop it** — clamp the bank to ≤ `stdlen` and skip the increment on a
   blocked step. In both simulators a load-blocked tip simply does not grow (`exp(f/f₀)` /
   `exp(−f·x/kT)`), with nothing stored to discharge later, so a surge-on-unblock has no
   biophysical basis. Revised from "lean keep" in the pre-review draft. *(§2.7 point 2; §4.2)*
4. **Poisson noise** — ~~include `elongation_monomer` in v1?~~ **Resolved (Matt, in review):
   deferred to a future feature.** v1 ships deterministic growth; the monomer-noise model is
   documented in §8 and fully separable. *(§2.3)*
5. **Units convention** — ~~adopt physical µm/s or stay dimensionless?~~ **Resolved: stay
   scale-free.** Smoldyn does not require unit declarations (only 3 of ~1960 examples use one,
   no filament example does), so the new examples introduce no `units` statement and simply
   choose numeric values that read as µm/s, with the derivation in a header comment. *(§6)*
7. **Branch angle citation** *(open — not part of this task)* — our examples use 70°
   (*Acanthamoeba*, Mullins 1998), while Akamatsu 2020 used 77° (Blanchoin 2000) and the best
   in-situ mammalian CME measurement is 68 ± 9° (Serwas 2022). Which do we standardize on, and
   should `branch_spread` go from 0.09 rad (~5°) up to the measured 7–13°? *(§6.2 — a
   one-character change to the examples, worth getting right before the next figure)*
6. **Companion fix `segoffset`** — ~~in this task, or its own?~~ **Resolved: deferred (§8).**
   v1 is pure plus-end elongation, which never renumbers, so it does not need the fix (V8
   proves this); it lands with treadmilling+elongation or `plus_end front`. *(§8)*
