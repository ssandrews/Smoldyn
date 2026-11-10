"""Compare identical frozen-topology replays; run from VS Code or a terminal.

No third-party Python packages. Build profile_filaments with
OPTION_FILAMENT_PROFILE=ON first. All adjustable settings are below.
The FIL snapshot is rounded geometry, not an exact biological checkpoint.
"""
from pathlib import Path
import csv
import hashlib
import json
import math
import re
import shutil
import subprocess

PROJECT = Path(__file__).resolve().parents[2]
MODEL = PROJECT / "Free_growth_experiment_1_steric.txt"
SNAPSHOT = PROJECT / "profiling/workers_2026-10-12/w1/snapshot.txt"
EXECUTABLE = PROJECT / "build-filament-workers/profile_filaments.exe"
RESULTS = PROJECT / "profiling/implicit_2026-10-13/dense"
WORKERS = 4
DURATION = 0.01              # equal simulated duration for every case
REPEATS = 1                   # increase for median timings
# name, method, outer timestep, mechanical substeps
CASES = [
    ("euler80", "euler", 1e-4, 80),
    ("euler40", "euler", 1e-4, 40),
    ("implicit4", "implicit_network", 1e-4, 4),
    ("implicit1", "implicit_network", 1e-4, 1),
    ("implicit_0p2ms", "implicit_network", 2e-4, 1),
    ("implicit_0p5ms", "implicit_network", 5e-4, 1),
    ("implicit_1ms", "implicit_network", 1e-3, 1),
]
TOLERANCE = 1e-7
NEWTON_LIMIT = 12
KRYLOV_LIMIT = 60
RUN_STIFFNESS_SWEEP = True
STIFFNESS_DURATION = 0.001
STIFFNESS_CASES = {
    "stretch100k": {"force_length": "100000"},
    "stretch1M": {"force_length": "1000000"},
    "contacts40k": {"steric_stiffness": "40000"},
    "bending10": {"force_angle": "10 10 10"},
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def model_text(method, dt, substeps):
    replace = {"dynamics": method, "time_step": str(dt),
               "steric_substeps": str(substeps), "filament_workers": str(WORKERS),
               "brownian_motion": "0", "elongation_rate": "0",
               "branch_rate": "0", "capping_rate": "0", "uncapping_rate": "0",
               "output_files": "profile_frames.txt"}
    lines = []
    for line in MODEL.read_text(encoding="utf-8-sig").splitlines():
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        key = words[0]
        if key in {"random_filament", "filament", "cmd", "implicit_tolerance",
                   "implicit_iterations", "implicit_krylov"}:
            continue
        if key == "end_filament_type":
            lines.extend([f"implicit_tolerance {TOLERANCE}",
                          f"implicit_iterations {NEWTON_LIMIT}",
                          f"implicit_krylov {KRYLOV_LIMIT}"])
        lines.append(key + " " + replace[key] if key in replace else line)
    return "\n".join(lines) + "\n"


def coordinates(folder):
    with (folder / "coordinates.csv").open(encoding="utf-8", newline="") as stream:
        return {(row["filament"], row["node"]): tuple(float(row[k]) for k in "xyz")
                for row in csv.DictReader(stream)}


def main():
    RESULTS.mkdir(parents=True, exist_ok=True)
    results = []
    for name, method, dt, substeps in CASES:
        steps = round(DURATION / dt)
        assert steps > 0 and abs(steps * dt - DURATION) < 1e-12
        for repeat in range(REPEATS):
            folder = RESULTS / f"{name}_{repeat}"
            folder.mkdir(parents=True, exist_ok=True)
            (folder / "model.txt").write_text(model_text(method, dt, substeps), encoding="utf-8")
            shutil.copyfile(SNAPSHOT, folder / "snapshot.txt")
            print(f"{name}: {steps} steps, h={dt/substeps:g}, workers={WORKERS}", flush=True)
            with (folder / "run.log").open("wb") as log:
                process = subprocess.run([str(EXECUTABLE), "model.txt", "snapshot.txt",
                                          str(steps), "0", "0", "1"], cwd=folder,
                                         stdout=log, stderr=subprocess.STDOUT)
            row = dict(name=name, repeat=repeat, method=method, dt=dt, substeps=substeps,
                       h=dt/substeps, completed=process.returncode == 0)
            if row["completed"]:
                row.update(json.loads((folder / "metrics.json").read_text()))
                if results and results[0]["completed"]:
                    ref = coordinates(RESULTS / f"{CASES[0][0]}_0")
                    actual = coordinates(folder)
                    assert ref.keys() == actual.keys(), "Topology changed in mechanics-only replay"
                    errors = [math.dist(ref[key], actual[key]) for key in ref]
                    row.update(rms_error_um=math.sqrt(sum(e*e for e in errors)/len(errors)),
                               max_error_um=max(errors),
                               speedup=results[0]["wall_seconds"]/row["wall_seconds"])
                print(f"  wall {row['wall_seconds']:.4f}s, contacts {row['contacts_last']}, "
                      f"penetration {row['max_penetration']:.3g}um", flush=True)
            else:
                print("  failed; see " + str(folder / "run.log"), flush=True)
            results.append(row)
            (RESULTS / "results.json").write_text(json.dumps(dict(
                duration=DURATION, workers=WORKERS, model_sha256=digest(MODEL),
                snapshot_sha256=digest(SNAPSHOT), executable_sha256=digest(EXECUTABLE),
                cases=results), indent=2), encoding="utf-8")
    lines = ["# Coupled implicit mechanics replay", "",
             f"Equal {DURATION:g}s windows; chemistry and Brownian noise disabled; {WORKERS} workers.",
             "No warmup: every case starts at exactly the same reconstructed geometry.", "",
             "| Case | h (s) | Wall (s) | Speedup vs Euler80 | RMS error (nm) | Max error (nm) | Penetration (nm) |",
             "|---|---:|---:|---:|---:|---:|---:|"]
    for row in results:
        if not row["completed"]:
            lines.append(f"| {row['name']} | {row['h']:g} | failed | | | | |")
        else:
            lines.append(f"| {row['name']} | {row['h']:g} | {row['wall_seconds']:.4f} | "
                         f"{row.get('speedup',1):.2f} | {1000*row.get('rms_error_um',0):.4f} | "
                         f"{1000*row.get('max_error_um',0):.4f} | {1000*row['max_penetration']:.4f} |")
    lines.extend(["", "Errors compare coordinates with Euler80, not an exact solution. "
                  "Inspect Euler40-vs-Euler80 error to assess reference convergence. "
                  "Snapshot reconstruction lacks original material rolls, RNG state and rest azimuths; "
                  "this is a mechanics benchmark at a representative density, not a trajectory restart."])
    (RESULTS / "REPORT.md").write_text("\n".join(lines)+"\n", encoding="utf-8")
    if RUN_STIFFNESS_SWEEP:
        limits = []
        for name, overrides in STIFFNESS_CASES.items():
            folder = RESULTS / name
            folder.mkdir(parents=True, exist_ok=True)
            text = model_text("implicit_network", 1e-4, 1)
            for key, value in overrides.items():
                text, count = re.subn(r"(?m)^" + key + r"\s+.*$", key + " " + value, text)
                assert count == 1
            (folder / "model.txt").write_text(text, encoding="utf-8")
            shutil.copyfile(SNAPSHOT, folder / "snapshot.txt")
            print("stiffness sweep: " + name, flush=True)
            with (folder / "run.log").open("wb") as log:
                process = subprocess.run([str(EXECUTABLE), "model.txt", "snapshot.txt",
                                          str(round(STIFFNESS_DURATION/1e-4)), "0", "0", "1"],
                                         cwd=folder, stdout=log, stderr=subprocess.STDOUT)
            row = dict(name=name, overrides=overrides, completed=process.returncode == 0)
            if row["completed"]:
                row.update(json.loads((folder / "metrics.json").read_text()))
            limits.append(row)
            print("  completed=" + str(row["completed"]), flush=True)
        (RESULTS / "stiffness_limits.json").write_text(json.dumps(dict(
            h=1e-4, duration=STIFFNESS_DURATION, tolerance=TOLERANCE,
            newton_limit=NEWTON_LIMIT, krylov_limit=KRYLOV_LIMIT,
            executable_sha256=digest(EXECUTABLE), cases=limits), indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
