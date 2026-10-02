# Branched actin in Smoldyn — Akamatsu Lab contribution

Documents and model files for the filament capabilities contributed by the
Akamatsu Lab (branching, plus-end elongation and capping, thermal branch
junctions, surface-gated branching, surface confinement), and for the first
model built on them: a branched actin network assembling against a moving
boundary, set up to recapitulate the load-adaptation measurements of Bieling et
al. 2016 (Cell 164:115). This is Task 3.1 of the Smoldyn actin R01 (Akamatsu /
Andrews / Sauro).

The capabilities were developed on branch `akamatsu/filament-capabilities` and
merged into `master` on 2026-09-29 (PR #176). The feature examples are in the
sibling folders `../branching/` and `../elongation/`, and the keywords are
described in the Smoldyn manual. This folder holds what those do not: the
design record and the compression model.

Start with `CHANGES.md`. It is the manifest for the contribution: where the
code is (tags, commit table), what each new keyword does with units and
defaults, the design decisions worth arguing with, what did not change (with
the regression proof), the verification numbers, known limitations, and open
questions for the maintainer.

## Documents

| File | What it is | Status |
|------|------------|--------|
| `CHANGES.md` | Maintainer manifest for the contribution; the front door | current |
| `actin-branching-proposal.md` | Design for branching off existing filaments (`branch_rate`, `branch_angle`, …) | built, tag `filament-branching-v1` |
| `elongation-capping-report.md` | Audit of what the stock filament module could and could not do about growth | reference |
| `elongation-capping-proposal.md` | Design for plus-end elongation as a velocity plus stochastic capping | built, tag `filament-elongation-capping-v1` |
| `thermal-junction-proposal.md` | Torsional and azimuthal springs holding branch junctions under thermal dynamics; the FDT thermal-force fix | built, tag `filament-thermal-junction-v1` |
| `surface-branching-proposal.md` | Options for confining nucleation to a surface. Option A (a location gate, `branch_surface` / `branch_compartment`) was built as a stopgap; §5–6 sketch the explicit nucleator mechanism, offered as design input | Option A built, tag `filament-branch-region-v1`; §5–6 not built |
| `force-clamp-proposal.md` | Constant-force boundary for the confining surface (filament forces moving a panel) | not built; design input |
| `branch-occupancy-proposal.md` | Limiting branches per node / minimum branch spacing (`branch_exclusion`) | not built; design input |

The proposals were written for the lab and for the maintainer as the work
progressed and are kept as written. They refer to each other, to git hashes on
the branch (all now ancestors of `master`), and in a few places to pages of the
lab's private notebook by uid; those uids are lab handles and nothing in the
code depends on them. Example paths in them predate the reorganization of
`S13_filaments` into subfolders: `examples/S13_filaments/branchZone3D.txt` is
now `examples/S13_filaments/branching/branchZone3D.txt`, and so on.

## Model files

Three configurations of the compression model. All run headless and write one
`printFilaments` dump per 10 ms of model time; the dump is the only record of
per-filament state (`savesim` does not serialize filaments).

| File | What changed | Filaments at t = 3.5 s | Wall clock | Reference output SHA-256 |
|------|--------------|------------------------|------------|--------------------------|
| `compressionPoC3D_baseline.txt` | Literature-constrained rates; `branch_rate` 60 (inherited placeholder); branching gated on the whole 1 × 1 µm floor | 3,606 | 255 s | `b648f80cab3c8dd0…2573a169692` (`baseline_frames.txt`, 44 MB) |
| `compressionPoC3D_v3.txt` | `branch_rate` 60 → 10.8, calibrated to the assay's areal nucleation rate at the density the baseline reached at t = 1.96 s | 11 | 5 s | `17dd7a723a9d85c6…2cdca30ab256` (`v3_frames.txt`, 1 MB) |
| `compressionPoC3D_v4.txt` | Branching gated on a 0.5 × 0.5 µm square patch in the middle of the floor (the assay uses 14 × 14 µm NPF squares; the patch reproduces the column geometry, not the size); `branch_rate` 10.8 → 25 so the patch holds the assay's unloaded free-barbed-end density when the press starts | 236 | 33 s | `cd1c63b4246b966f…8184e3b0e3359b398` (`v4_frames.txt`, 6 MB) |

All three use `random_seed 1`. The reference outputs were produced by a binary
built at `6927b20` (`filament-confinement-v1`) and reproduce byte for byte at
`92430b92` (`master` at `e21d6dd2` plus its merge back into the branch, built
2026-10-02 with the flags below). The comment block in each file tags every
parameter as `[measured]`, `[derived]`, `[numerical]` or `[no value]` and says
what breaks if you change it.

The reference outputs, the analysis script (`analyze_compression.py`: wall
force as Σ k·penetration, network height, filament count) and the Simularium
renderings are in the lab's shared drive, and the converter that turns a
`printFilaments` dump into a `.simularium` file (with the membrane, the
branching gate and the piston drawn as planes) is in
`MatsulabUW/simularium-viewer`, branch `matsulab-ui`, `tools/`. The analysis
script is not in this folder because the examples CMake target registers every
`.py` under `examples/` as a test.

Two known properties of these runs, both documented in `CHANGES.md` and the
config comments: branching is supercritical (the filament count doubles every
0.4 s in the baseline and every ~0.6 s in v4, and never saturates, because the
location gate has no nucleator to deplete), and force read while the boundary
moves is drag, not stress (a node riding the plate lags by v/(mobility·k));
read force during the hold, after the ramp.

## Building and running

From the repository root:

```
cmake -S . -B build -DOPTION_USE_OPENGL=OFF -DOPTION_PYTHON=OFF \
      -DOPTION_TARGET_LIBSMOLDYN=OFF -DOPTION_NSV=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
cd examples/S13_filaments/branchedActin
rm -f v3_frames.txt
SMOLDYN_NO_PROMPT=1 ../../../build/smoldyn compressionPoC3D_v3.txt -t < /dev/null
```

`-t` runs without graphics. Output paths resolve relative to the config file,
not the working directory, and an existing output file triggers an interactive
overwrite prompt that `SMOLDYN_NO_PROMPT` does not suppress, so delete the
declared `output_files` before a rerun. With `OPTION_PYTHON=ON` at a tagged
commit, pass `-DSMOLDYN_VERSION=2.76.dev0`; the capability tag names are not
PEP 440 versions and the wheel target rejects them.

## Not in this folder

- The tabbed HTML overview of the capabilities (3 MB) and the branch-zone
  timelapse pages (1.3 MB each): shared separately, to keep the repository
  small.
- The per-unit validation configs and scripts (analytic rate recovery, dt- and
  discretization-invariance, regression diffs): in the lab notebook; their
  results are the numbers in `CHANGES.md` §Verification.
