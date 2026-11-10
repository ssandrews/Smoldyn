"""Profile short replays of real free-growth geometries using the native driver.

Run this file directly in VS Code after building profile_filaments with
OPTION_FILAMENT_PROFILE=ON. Uses only the Python standard library.
Replays are timing samples, not exact restarts of the biological experiment.
"""
from pathlib import Path
import csv
import hashlib
import itertools
import json
import subprocess
import time

# ---------------- All user parameters ----------------
PROJECT = Path(__file__).resolve().parents[2]
MODEL = PROJECT / "Free_growth_experiment_1_steric.txt"
FRAMES = PROJECT / "free_growth_steric_frames_2.txt"
EXECUTABLE = PROJECT / "build-filament-workers" / "profile_filaments.exe"
RESULTS = PROJECT / "profiling" / "free_growth_current"
SNAPSHOT_TIMES = [0.0, 10.0, 20.0, 29.9]
STEPS = 100                  # 0.01 simulated seconds with the current dt
WARMUP_STEPS = 10
OUTPUT_REPEATS = 5
OUTPUT_INTERVAL = 0.1        # actual printFilaments cadence, simulated seconds
CONTROL_TIMES = [20.0, 29.9] # repeat with timers disabled to check overhead/state
LIVE_STEPS = 10000           # longer run from original seeds; 0 skips this check
# -----------------------------------------------------


def read_snapshots():
    """Stream once; infer a tip's bank from the last observed length increase."""
    nearest = {}
    history = {}
    with FRAMES.open(encoding="utf-8") as source:
        def compatible_rows():
            for line in source:
                if not line.startswith("FIL "):
                    continue
                row = line.split()
                if len(row) >= 8 and row[6] in {"open", "capped", "shrink", "barbed"}:
                    # The timing driver's snapshot format intentionally stays
                    # simple. General loops/two-ended dynamics need native
                    # input, not a reconstructed one-parent geometry snapshot.
                    if row[5] != "-" or row[6] != "open" or row[7] not in {"open", "capped"}:
                        raise ValueError("Profiling snapshots require a minus-attached branching tree with open/capped plus ends")
                    parent = row[4].rsplit(":", 1)[0].partition(":")[2] if row[4] != "-" else "-"
                    row = row[:4] + [parent, str(int(row[7] == "capped"))] + row[8:]
                yield row
        rows = compatible_rows()
        for t, frame in itertools.groupby(rows, key=lambda row: float(row[1])):
            frame = list(frame)
            if any(len(row) != 6 + 3 * (int(row[3]) + 1) for row in frame):
                continue
            for row in frame:
                name, nseg = row[2], int(row[3])
                old = history.get(name)
                if old is None or nseg > old[0]:
                    history[name] = (nseg, t)
            for target in SNAPSHOT_TIMES:
                if target not in nearest or abs(t - target) < abs(nearest[target][0] - target):
                    nearest[target] = (t, frame, dict(history))
    if len(nearest) != len(SNAPSHOT_TIMES):
        raise ValueError("No complete frames were found")
    return nearest


def model_parameters(text):
    values = {}
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if len(words) == 2:
            values[words[0]] = words[1]
    return values


def prepare_model(text, keep_seeds=False):
    lines = []
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if words and (words[0] == 'cmd' or
                      (not keep_seeds and words[0] in {"random_filament", "filament"})):
            continue
        if words and words[0] == "output_files":
            line = "output_files profile_frames.txt"
        lines.append(line)
    return "\n".join(lines) + "\n"


def run(target, snapshot, timers, text, params, steps=None):
    steps = STEPS if steps is None else steps
    live = snapshot is None
    t, frame, history = (0.0, [], {}) if live else snapshot
    label = 'coarse' if timers == 2 else ('timed' if timers else 'control')
    prefix = str(target) if isinstance(target, str) else f't{target:g}'
    folder = RESULTS / f"{prefix}_{label}"
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "model.txt").write_text(prepare_model(text, keep_seeds=live), encoding="utf-8")
    velocity, length = float(params["elongation_rate"]), float(params["standard_length"])
    with (folder / "snapshot.txt").open("w", encoding="utf-8") as out:
        for row in frame:
            bank = 0 if int(row[5]) else min(length, velocity * (t - history[row[2]][1]))
            out.write(" ".join(row) + f" {bank:.12g}\n")
    description = ('original seed definitions' if live else
                   f"{len(frame)} filaments, {sum(int(r[3]) for r in frame)} segments")
    print(f"Profiling t={t:g}: {description}; timers={timers}", flush=True)
    begin = time.perf_counter()
    with (folder / "run.log").open("w", encoding="utf-8") as log:
        subprocess.run([str(EXECUTABLE), "model.txt", '-' if live else "snapshot.txt", str(steps),
                        str(WARMUP_STEPS), str(int(timers)), str(OUTPUT_REPEATS)],
                       cwd=folder, stdout=log, stderr=subprocess.STDOUT, check=True)
    metrics = json.loads((folder / "metrics.json").read_text())
    with (folder / "timing.csv").open() as source:
        timing = {r["category"]: {"calls": int(r["calls"]),
                                 "inclusive": float(r["inclusive_seconds"]),
                                 "exclusive": float(r["exclusive_seconds"])}
                  for r in csv.DictReader(source)}
    metrics.update(snapshot_time=t, source_filaments=len(frame),
                   source_segments=sum(int(r[3]) for r in frame),
                   process_wall_seconds=time.perf_counter() - begin,
                   output_sha256=hashlib.sha256((folder / "profile_frames.txt").read_bytes()).hexdigest(),
                   executable_sha256=hashlib.sha256(EXECUTABLE.read_bytes()).hexdigest())
    result = {"metrics": metrics, "timing": timing}
    (folder / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"  measured loop: {metrics['wall_seconds']:.3f} s; "
          f"{metrics['rebuilds']}/{metrics['force_evaluations']} index rebuilds", flush=True)
    return result


def report(results, controls, params, coarse=None, live=None):
    lines = ["# Free-growth CPU profile", "",
             f"Model: `{MODEL.name}`; source: `{FRAMES.name}`.", "",
             f"Chemistry dt={params['time_step']} s; mechanical substeps={params['steric_substeps']}; "
             f"mechanical dt={float(params['time_step'])/int(params['steric_substeps']):g} s.", "",
             f"Release build; {STEPS} measured steps after {WARMUP_STEPS} warmup steps per snapshot. "
             "Elapsed wall times, single process. No graphics or scheduled dumps in the step loop. "
             "Frame formatting/file flush measured separately.", "",
             "## Main costs", "",
             "Percentages use nonoverlapping phase totals from coarse timers (one scope per phase). "
             "Steric includes chemistry cache preparation. Detailed per-filament timers are a separate run.", "",
             "| Snapshot (s) | Segments / filaments | Loop wall (s) | Ordinary forces | Movement | Steric + cache | Branch / cap / elongation | Other |",
             "|---|---|---:|---:|---:|---:|---:|---:|"]
    for target, result in (coarse or results).items():
        m, buckets = result["metrics"], result["timing"]
        inc = lambda key: buckets[key]["inclusive"]
        total = inc("timestep")
        groups = [inc("ordinary_forces"), inc("movement"),
                  inc("steric_forces") + inc("chemistry_prepare"),
                  inc("branching") + inc("capping") + inc("elongation") + inc("treadmilling")]
        groups.append(total - sum(groups))
        lines.append(f"| {m['snapshot_time']:g} | {m['segments_start']:,} segments / {m['filaments_start']:,} filaments "
                     f"| {m['wall_seconds']:.3f} | " + " | ".join(f"{100*g/total:.2f}%" for g in groups) + " |")
    lines += ["", "## Phase detail", "",
              "Exclusive timings exclude child scopes, so they can be added. Inclusive timings include them. "
              "Detailed timers can perturb per-filament costs; compare the instrumented and untimed "
              "controls below. Wall time includes scheduling noise. Treat results as bottleneck "
              "rankings, not sub-percent precision. With multiple workers, fine per-filament scopes "
              "are suppressed to avoid overlapping shared profiler writes; outer scopes still measure "
              "the complete worker phase.", ""]
    for target, result in results.items():
        m, buckets = result["metrics"], result["timing"]
        total = buckets["timestep"]["inclusive"]
        lines += [f"### Geometry near {m['snapshot_time']:g} s", "",
                  "| Phase | Calls | Inclusive ms | Exclusive ms | Exclusive % of step loop |",
                  "|---|---:|---:|---:|---:|"]
        for key, b in buckets.items():
            if key != "frame_output" and b["calls"]:
                lines.append(f"| {key} | {b['calls']:,} | {b['inclusive']*1000:.3f} | "
                             f"{b['exclusive']*1000:.3f} | {100*b['exclusive']/total:.2f}% |")
        lines += ["", f"Topology: {m['filaments_start']} → {m['filaments_end']} filaments; "
                  f"{m['segments_start']} → {m['segments_end']} segments; "
                  f"{m['growing_start']} → {m['growing_end']} uncapped ends.", "",
                  f"Cache: {m['rebuilds']} rebuilds / {m['force_evaluations']} force evaluations; "
                  f"last retained pairs={m['pairs_last']}, actual contacts={m['contacts_last']}; "
                  f"{m['occupied_boxes']} occupied / {m['boxes']} boxes. "
                  f"Blocked growth attempts={m['blocked_growth']}, blocked branch attempts={m['blocked_branches']}.", "",
                  f"Output: {m['output_seconds_per_frame']*1000:.3f} ms/frame. "
                  f"At a {OUTPUT_INTERVAL:g} s simulated cadence, estimated output overhead relative "
                  f"to the sampled loop={100*m['output_seconds_per_frame']/OUTPUT_INTERVAL/(m['wall_seconds']/(m['steps']*m['dt'])):.4f}%. "
                  "This measures repeated flushes to local disk, not long-run storage throughput.", "",
                  f"Sampled wall / simulated second={m['wall_seconds']/(m['steps']*m['dt']):.1f} s/s. "
                  "Do not extrapolate to a full run with changing network size.", ""]
    lines += ["## Instrumentation controls", "",
              "Controls run the same loaded state, warmup, RNG seed and window with timers disabled.", ""]
    for target, result in controls.items():
        m, timed = result["metrics"], results[target]["metrics"]
        lines.append(f"- t≈{target:g}: control {m['wall_seconds']:.3f} s; "
                     f"timed {timed['wall_seconds']:.3f} s; "
                     f"difference={100*(timed['wall_seconds']/m['wall_seconds']-1):+.2f}%; "
                     f"final frame hash identical={m['output_sha256']==timed['output_sha256']}.")
        if coarse:
            cm = coarse[target]['metrics']
            lines.append(f"  Coarse timers: {cm['wall_seconds']:.3f} s; "
                         f"difference from control={100*(cm['wall_seconds']/m['wall_seconds']-1):+.2f}%; "
                         f"frame hash identical={cm['output_sha256']==m['output_sha256']}.")
    if live:
        m, b = live['metrics'], live['timing']
        total = b['timestep']['inclusive']
        lines += ["", "## Longer check from the original seeds", "",
                  f"This run loads the original seed definitions directly, without frame reconstruction. "
                  f"After {m['warmup']} warmup steps, it measures {m['steps']} steps "
                  f"({m['steps']*m['dt']:g} simulated seconds) with coarse timers. "
                  f"Wall time: {m['wall_seconds']:.3f} s. "
                  f"Filaments: {m['filaments_start']} → {m['filaments_end']}; "
                  f"segments: {m['segments_start']} → {m['segments_end']}; "
                  f"uncapped ends: {m['growing_start']} → {m['growing_end']}.", "",
                  "| Chemistry phase (including any child collision queries) | Wall ms | % of loop |",
                  "|---|---:|---:|"]
        for key in ['branching', 'capping', 'elongation', 'growth_collision_query']:
            lines.append(f"| {key} | {b[key]['inclusive']*1000:.3f} | "
                         f"{100*b[key]['inclusive']/total:.4f}% |")
        lines += ["", "The collision-query row is nested inside branching/elongation and is not additive.", ""]
    lines += ["", "## Limits", "",
              "These are representative computational replays, **not exact trajectory checkpoints**. "
              "The frame file has rounded coordinates and omits material roll/frame, birth azimuth, "
              "RNG state and growth bank. The loader chooses zero rolls, reconstructs material frames, "
              "uses current junction azimuth as its rest value, resets the RNG from the model seed, "
              "and infers growth bank from the last observed length increase (clamped to one segment). "
              "Shared junctions are matched to mother nodes. Warmup reduces initial reconstruction effects. "
              "Short samples measure routine chemistry scans; rare event costs need longer windows if they "
              "become significant. Results apply to this model and machine, not every Smoldyn experiment.", "",
              "The original model has time_stop=90 but its printFilaments command stops at 30 s. "
              "Recorded geometry beyond 30 s is unavailable here. No model rates, solver, output file "
              "or original experimental coordinates were changed. Normal builds default to profiling OFF.", ""]
    (RESULTS / "REPORT.md").write_text("\n".join(lines), encoding="utf-8")


def main():
    if not EXECUTABLE.exists():
        raise FileNotFoundError(f"Build OPTION_FILAMENT_PROFILE=ON first: {EXECUTABLE}")
    text = MODEL.read_text(encoding="utf-8-sig")
    params, snapshots = model_parameters(text), read_snapshots()
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "provenance.json").write_text(json.dumps({
        "model": str(MODEL), "model_sha256": hashlib.sha256(MODEL.read_bytes()).hexdigest(),
        "frames": str(FRAMES), "frames_sha256": hashlib.sha256(FRAMES.read_bytes()).hexdigest(),
        "executable": str(EXECUTABLE), "executable_sha256": hashlib.sha256(EXECUTABLE.read_bytes()).hexdigest(),
        "steps": STEPS, "warmup": WARMUP_STEPS, "snapshot_times": SNAPSHOT_TIMES,
    }, indent=2) + "\n")
    results = {target: run(target, snapshots[target], True, text, params) for target in SNAPSHOT_TIMES}
    coarse = {target: run(target, snapshots[target], 2, text, params) for target in SNAPSHOT_TIMES}
    controls = {target: run(target, snapshots[target], False, text, params) for target in CONTROL_TIMES}
    live = run('live', None, 2, text, params, LIVE_STEPS) if LIVE_STEPS else None
    report(results, controls, params, coarse, live)
    print(f"Report saved: {RESULTS / 'REPORT.md'}")


if __name__ == "__main__":
    main()
