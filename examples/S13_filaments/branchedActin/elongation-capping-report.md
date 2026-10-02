# Report: what Smoldyn's filament module can and cannot do about filament growth

Audit of `source/Smoldyn/smolfilament.c` on branch `akamatsu/filament-capabilities`
(tip `42dbdcc`, = tag `filament-branching-v1`), written as the input to the
elongation + capping design. Companion document: `elongation-capping-proposal.md`.

Everything below was read off the code, not inferred from the manual. Line numbers
are `smolfilament.c` unless stated.

---

## 1. TL;DR

The filament engine has **no concept of net growth**. It has exactly one length-changing
mechanism, `treadmill_rate`, and that mechanism is deliberately **segment-count
conserving**: every addition is paired with a removal in the same call. There is no rate
that increases filament length, no per-filament growth state, and no notion of filament
polarity beyond a geometric `'f'`/`'b'` end character threaded through the segment-editing
functions.

The good news: the primitives elongation needs already exist and are well factored.
`filAddSegment` / `filAddOneRandomSegment` do the geometry and the surface-collision
retry; the memory grows itself; `filDynamics` already has the per-type, per-filament,
Poisson-draw loop pattern that a growth rate would slot into. Building elongation is
mostly wiring a new rate through the same six touchpoints branching used, plus one new
piece of per-filament state.

The bad news is two structural gotchas that elongation and capping will expose, both
documented in §4: **segment renumbering invalidates `branchspots`**, and the engine has
**no filament-destruction path**, which constrains how far capping can go in v1.

---

## 2. The one growth mechanism that exists: `treadmill_rate`

### 2.1 What it actually does

`filDynamics` (3475–3486), once per filament type per timestep:

```c
if(filtype->treadrate!=0) {
    for(f=0;f<filtype->nfil;f++) {
        fil=filtype->fillist[f];
        treadnum=poisrandD(fabs(filtype->treadrate)*sim->dt);
        for(i=0;i<treadnum;i++)
            filTreadmill(sim,fil,filtype->treadrate>0?'b':'f'); }}
```

and `filTreadmill` (2403–2411):

```c
er=filAddOneRandomSegment(sim,fil,NULL,thk,endchar,1);   // add at one end
if(!er)
    filRemoveSegment(fil,(endchar=='b')?'f':'b');        // remove from the other
```

So one "treadmill event" = add one segment at one end, remove one segment from the other.

| property | value |
|---|---|
| units of `treadmill_rate` | **events per unit time** (parser tag `\|/T`, 1310) — a *turnover* rate, not a velocity |
| sign | selects direction only: `> 0` grows at the **back**, `< 0` at the **front** |
| default | `0` (off) |
| stochasticity | Poisson in event count per step (`poisrandD(|rate|*dt)`) |
| net length change | **zero in segment count, a random walk in contour length** — see 2.2 |

### 2.2 "Length-conserving" is not quite right — it conserves *segment count*

The added segment's length comes from `filRandomLength` (1814–1823), a Gaussian about
`stdlen` with σ = √(kT / (thickness · klen)), truncated at > 0. The removed segment has
whatever length it happened to be given when *it* was added. So contour length does an
unbiased random walk with step ~σ per event, not a constant.

For `examples/S13_filaments/treadmill3D.txt` (kT 0.3, thickness 2, klen 10) that is
σ ≈ 0.12 against `stdlen` 2 — about 6% per event. Harmless for existing uses, but worth
knowing now, because once we add a *deliberate* elongation velocity we will want to
measure contour length vs. time and this is the noise floor sitting underneath it.

(If `klen < 0`, `filRandomLength` returns `stdlen` exactly and treadmilling *is* strictly
length-conserving.)

### 2.3 Treadmilling already stalls against surfaces

`filAddOneRandomSegment` (2368–2399) with `constraints=1` — which is what `filTreadmill`
passes — retries up to `FILMAXTRIES` times to place a segment that does not cross a
surface panel, and if it fails, un-does the addition and returns 2. `filTreadmill` then
skips the removal, so the filament is left untouched.

This is a genuine, already-working "a blocked end cannot polymerize" mechanism. It is the
natural attachment point for a Brownian-ratchet load model later, and it means elongation
gets obstacle-blocking for free if it reuses the same call.

### 2.4 What is *not* there

- No rate that increases length. Nothing upstream of `filTreadmill` supplies net polymerization.
- No per-filament growth state (no accumulator, no clock, no "how much do I owe").
- No monomer pool. Growth would not deplete a G-actin species; nucleation is already
  phenomenological in exactly the same way (proposal §5.5).
- No capping, no per-filament flag of any kind. `filamentstruct` has no state field at all
  besides geometry, branch bookkeeping, and the `sequence` string.
- No filament destruction. Nothing ever removes a filament from `filtype->fillist`;
  `nfil` only increases. A filament that shrank to `nseg == 0` would sit in the list as a
  zero-length ghost (most loops `continue` on `nseg < 1`, so it is inert rather than fatal).

---

## 3. Polarity: implicit, geometric, and inconsistently labelled

There is no `plus_end` / `minus_end` concept. Polarity is carried by a `char endchar`
that is `'f'` (front, node 0) or `'b'` (back, node `nseg`), threaded through
`filAddSegment`, `filRemoveSegment`, `filAddOneRandomSegment`, `filTreadmill`,
`filLengthenSegment`, `filRotateVertex`.

Three independent pieces of the code nevertheless agree on which end is biologically the
barbed end:

1. `treadmill_rate > 0` grows at the back (3482).
2. `filPinBranches` (2750–2767) anchors a daughter by translating it so **node 0 (its
   front)** sits at the mother's branch point — i.e. the daughter's front is its pointed
   end, and its back is the free barbed end that grows away from the junction. This is the
   correct Arp2/3 geometry.
3. All the existing examples use positive `treadmill_rate`.

**So the established convention is: back = plus/barbed, front = minus/pointed.** Nothing
states this anywhere; it is three separate conventions that happen to line up.

**One inconsistency, ours, cosmetic but worth fixing while we are in here.**
`filamentstruct` has `frontend` / `backend`, documented in `smoldyn.h:825-826` as "what
front attaches to" / "what back attaches to". `filAddBranch` (2697) sets
`daughter->backend = mother` with the comment "daughter's pointed end is anchored to the
mother". The comment is biologically right and the field is the wrong one — it is the
daughter's **front** that `filPinBranches` actually pins. The fields are only read by
`filOutput` (789–792) and by our `printFilaments` parent column
(`smolcmd.c:3202`), so nothing breaks today; but anyone reading the struct will get the
polarity backwards.

---

## 4. The two structural gotchas elongation and capping will hit

### 4.1 Segment renumbering silently invalidates `branchspots`

`filArrayShift` (2211–2248) rotates the `segments[]` and `nodes[]` arrays and then
re-stamps `segment->index = i` for every segment. It is called whenever a segment is added
or removed at the **front**:

| operation | front array shift? | segment indices change? |
|---|---|---|
| add at back (`filAddSegment(...,'b')`) | no | **no** |
| remove at back (`filRemoveSegment(...,'b')`) | no — just `nseg--` | **no** |
| add at front (`filAddSegment(...,'f')`) | `filArrayShift(fil,+1)` | yes, all +1 |
| remove at front (`filRemoveSegment(...,'f')`) | `filArrayShift(fil,-1)` | yes, all −1 |

`fil->branchspots[]` stores branch positions as **segment indices** and is not updated by
`filArrayShift`. So *every* treadmill event slides every branch point on that filament by
one segment — including the common `treadmill_rate > 0` case, because although it adds at
the back it *removes at the front*. `filPinBranches` then re-pins daughters to the wrong
place, and only guards the out-of-range case (`if(seg<0 || seg>=fil->nseg) continue`, 2760).

This is the mechanism behind the v1 limitation already recorded in `CLAUDE.md` ("under
motion the junction angle decorrelates … treadmilling renumbers segments") and behind the
measured 91° ± 51° dynamic branch angle vs. 70.1° ± 3.1° static.

Two consequences for this project:

- **Pure plus-end elongation does not renumber anything.** Adding at the back leaves every
  index and every `branchspot` valid. Elongation is strictly *safer* for branched networks
  than treadmilling is, and a branching + elongation demo should hold its junction geometry
  much better than the branching + treadmilling movie did.
- The moment we combine elongation with treadmilling, or elongate at the front, the bug is
  back. Worth fixing separately and first (see the proposal's commit 1).

`fil->nodemobility[]` has the same problem — it is indexed by node and is not shifted by
`filArrayShift`, so a node pinned with mobility 0 does not follow its material node under
treadmilling. No current example uses both, so it is latent.

### 4.2 There is no way to remove a filament

Relevant because the physically correct behaviour of "capped filament that is still
treadmilling" is *net depolymerization to nothing*. The engine cannot express the "to
nothing" part: nothing compacts `fillist`, and a mother that vanished would leave its
daughters holding stale `branches[]`/`branchspots[]` entries. Any capping design that lets
filaments shrink has to answer "what happens to the branches on a disappearing mother"
first. This is the main reason the proposal keeps v1 capping to *growth-blocking only*.

---

## 5. Memory and cost under sustained growth

Elongation is the first feature that makes an existing filament grow without bound, so:

- **Growth path works.** `filAddSegment` (2257) calls `filAlloc(fil, fil->maxseg*2+1, 0,0)`
  when full; `filAlloc` (459–505) reallocates `segments`, `nodes`, `nodesx`, `roll`,
  `nodemobility`, preserving existing node pointers, and then re-runs `filWorkAlloc` if
  `dynamics != FDnone`. `filWorkAlloc` (289–377) frees and rebuilds the whole working set
  including the sparse force matrix. Working arrays are scratch, recomputed each step, so
  rebuilding them mid-simulation is safe. Doubling keeps this amortized O(1).
- **One real waste.** `filAlloc:460` allocates the segment *pointer* array as
  `calloc(maxseg, sizeof(struct segmentstruct))` — the struct, not the pointer. Measured on
  this platform that is 136 bytes per slot instead of 8: a **17× over-allocation** on an
  array that doubles as the filament grows. Invisible today (filaments are tens of
  segments); it becomes tens of MB per filament under sustained elongation. One-word fix,
  worth taking as its own commit.
- **Per-step cost is linear in `nseg`** for the explicit integrators, and the implicit ones
  run a BiCGSTAB solve on a banded matrix of size `2(nseg+1)` (2D) or `4(nseg+1)−1` (3D)
  per filament per step. Unbounded growth degrades the timestep cost. Argues for an
  opt-in maximum-length guardrail.

---

## 6. State that cannot be saved

`filWrite` (897–908) is a stub — "code not written yet" — so `savesim` does not serialize
filaments at all. Any new per-filament state (a growth accumulator, a capped flag) is
equally unsaveable, and equally invisible to a restart. Not a blocker, since the module is
already in this position, but it means our only observability channel is the
`printFilaments` command we added, and new state should be dumped there if we want to
measure it.

---

## 7. How a new filament-type parameter gets wired (the pattern branching established)

Six touchpoints, in the order a change should be made:

| # | file:function | what goes there |
|---|---|---|
| 1 | `smoldyn.h` — `filamenttypestruct` | the field |
| 2 | `smolfilament.c` — `filamentTypeAlloc` (~603) | the default, chosen so the feature is **off** |
| 3 | `filtypeSetParam` (971) | string dispatch + range validation; internal name is short and unprefixed (`"treadrate"`, `"branchsegments"`) |
| 4 | `filtypeReadString` (1199) | the config keyword, `CHECKS`/`CHECKM` validation, unit tag (`\|/T`, `\|L`, `\|E`) |
| 5 | `filtypeOutput` (815) | a `simLog` line, using `simLog(sim, active?2:1, ...)` so inert parameters stay quiet at verbosity 1 |
| 6 | `filDynamics` (3466) | the hook, gated on `rate != 0` so no code — **and no RNG draw** — executes by default |

Per-filament (not per-type) state additionally needs: init in `filAlloc` (436–457) and in
`filAddFilament` (2638–2643, which resets `nseg`/`nbranch`/`nsequence` on reuse), and a
copy line in `filCopyFilament` (2570–2616).

The RNG discipline in touchpoint 6 is what made the branching work provably
byte-identical to stock `master` on fixed seeds. It is the constraint to design around.

---

## 8. Summary table: growth capabilities today

| capability | status |
|---|---|
| add a segment at either end, with thermal length/angle | **exists** (`filAddSegment`, `filAddOneRandomSegment`) |
| refuse to add a segment that would cross a surface | **exists** (`constraints=1`, retry then fail) |
| remove a segment from either end | **exists** (`filRemoveSegment`) |
| turnover at a rate, segment-count conserving | **exists** (`treadmill_rate`) |
| net elongation at a defined velocity | **absent** |
| elongation at a *named* (plus) end | **absent** — polarity is an implicit `'f'`/`'b'` convention |
| per-filament growth state | **absent** |
| capping / any per-filament flag | **absent** |
| uncapping | **absent** |
| bounded growth / max length | **absent** |
| monomer-pool coupling | **absent** (nucleation is phenomenological too) |
| filament destruction | **absent** |
| branch points that survive renumbering | **broken** (§4.1) |
| serialization of filament state | **absent** (`filWrite` stub) |
| observability of new per-filament state | via our `printFilaments` only |
