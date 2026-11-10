"""Compare cached serial mechanics and optional workers on recorded geometry.
Requires the profiling driver; parameters below can be edited in VS Code.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import profile_filaments as profile

PROJECT = Path(__file__).resolve().parents[2]
MODEL = PROJECT / "Free_growth_experiment_1_steric.txt"
FRAMES = PROJECT / "free_growth_steric_frames_2.txt"
EXECUTABLE = PROJECT / "build-filament-workers" / "profile_filaments.exe"
BASELINE = PROJECT / "build-filament-profile" / "profile_filaments_before_workers.exe"
RESULTS = PROJECT / "profiling" / "workers_2026-10-12"
SNAPSHOT_TIME = 29.9
STEPS = 100
WARMUP = 10
WORKERS = [1, 2, 4, 8, 0]  # 0 selects automatically
RUN_BASELINE = True


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(name, workers, executable, snapshot, model, params, steps=STEPS):
    folder = RESULTS / name
    folder.mkdir(parents=True, exist_ok=True)
    t, frame, history = snapshot
    cleaned = profile.prepare_model(model)
    cleaned = "\n".join(line for line in cleaned.splitlines()
                        if not line.strip().startswith("filament_workers "))
    if workers is not None:
        cleaned = f"filament_workers {workers}\n" + cleaned
    else:
        # The old baseline has no Brownian switch. Only its default-on behavior
        # can be compared, and it must not receive the new input keyword.
        if float(params.get('brownian_motion', '1')) != 1:
            raise ValueError('Old baseline cannot represent Brownian motion disabled')
        cleaned = "\n".join(line for line in cleaned.splitlines()
                            if not line.strip().startswith('brownian_motion '))
    (folder / "model.txt").write_text(cleaned + "\n", encoding="utf-8")
    with (folder / "snapshot.txt").open("w") as out:
        for row in frame:
            bank = 0 if int(row[5]) else min(float(params['standard_length']),
                     float(params['elongation_rate']) * (t-history[row[2]][1]))
            out.write(" ".join(row) + f" {bank:.12g}\n")
    print(f"{name}: {steps} steps at recorded time {t:g}, workers={workers}", flush=True)
    with (folder / "run.log").open("w") as log:
        subprocess.run([str(executable), "model.txt", "snapshot.txt", str(steps),
                        str(WARMUP), "0", "1"], cwd=folder, stdout=log,
                       stderr=subprocess.STDOUT, check=True)
    result = json.loads((folder / "metrics.json").read_text())
    result.update(frame_sha256=digest(folder / 'profile_frames.txt'),
                  state_sha256=digest(folder / 'final_state.bin')
                  if (folder / 'final_state.bin').exists() else None,
                  executable_sha256=digest(executable))
    (folder / "result.json").write_text(json.dumps(result, indent=2)+"\n")
    print(f"  loop {result['wall_seconds']:.3f} s; selected workers="
          f"{result.get('workers_selected', 'baseline')}", flush=True)
    return result


def main():
    model = MODEL.read_text(encoding='utf-8-sig')
    params = profile.model_parameters(model)
    profile.FRAMES = FRAMES
    profile.SNAPSHOT_TIMES = [20.0, SNAPSHOT_TIME]
    snapshots = profile.read_snapshots()
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / 'provenance.json').write_text(json.dumps({
        'model': str(MODEL), 'model_sha256': digest(MODEL),
        'frames': str(FRAMES), 'frames_sha256': digest(FRAMES),
        'steps': STEPS, 'warmup': WARMUP, 'snapshot': SNAPSHOT_TIME,
    }, indent=2)+'\n')
    # Shorter geometry tests catch RNG and material-frame differences before
    # spending time on the complete dense benchmark.
    pilot1 = run('pilot20_w1', 1, EXECUTABLE, snapshots[20.0], model, params, 20)
    pilot4 = run('pilot20_w4', 4, EXECUTABLE, snapshots[20.0], model, params, 20)
    if pilot1['state_sha256'] != pilot4['state_sha256']:
        raise RuntimeError('Full-precision worker pilot states differ')
    results = {}
    if RUN_BASELINE and BASELINE.exists() and float(params.get('brownian_motion', '1')) == 1:
        results['before'] = run('before', None, BASELINE, snapshots[SNAPSHOT_TIME], model, params)
    elif RUN_BASELINE and float(params.get('brownian_motion', '1')) == 0:
        print('Skipping earlier executable: it cannot disable only Brownian motion.', flush=True)
    for workers in WORKERS:
        results[f'w{workers}'] = run(f'w{workers}', workers, EXECUTABLE,
                                   snapshots[SNAPSHOT_TIME], model, params)
        if workers != 1 and results[f'w{workers}']['state_sha256'] != results['w1']['state_sha256']:
            raise RuntimeError(f'Full-precision state differs with {workers} workers')
    if 'before' in results and results['before']['frame_sha256'] != results['w1']['frame_sha256']:
        raise RuntimeError('Cached movement changed the baseline frame')
    reference = results.get('before', results['w1'])['wall_seconds']
    lines = ['# Filament worker benchmark', '',
             f'Model `{MODEL.name}`; geometry near {snapshots[SNAPSHOT_TIME][0]:g} s. '
             f'{STEPS} measured steps after {WARMUP} warmup steps; timers disabled.', '',
             '| Mode | Workers selected | Loop wall seconds | Speedup vs before | Speedup vs cached serial |',
             '|---|---:|---:|---:|---:|']
    for name, r in results.items():
        lines.append(f"| {name} | {r.get('workers_selected', 1)} | {r['wall_seconds']:.3f} | "
                     f"{reference/r['wall_seconds']:.2f}× | "
                     f"{results['w1']['wall_seconds']/r['wall_seconds']:.2f}× |")
    lines += ['', 'All new worker choices produced identical full-precision node coordinates, '
              'material frames, growth/capping state and subsequent Gaussian RNG probes. '
              + ('The earlier executable produced the same rounded FIL output; it did not '
                 'support the full-precision state dump.' if 'before' in results else
                 'No comparison with the earlier executable was performed.'), '',
              'These are short reconstructed geometry replays, not exact biological checkpoints. '
              'One timing sample per choice; scheduling, CPU frequency and memory traffic can '
              'affect small differences. The model physics and source frames were unchanged.', '']
    (RESULTS / 'REPORT.md').write_text('\n'.join(lines), encoding='utf-8')
    print(f"Saved {RESULTS / 'REPORT.md'}", flush=True)


if __name__ == '__main__':
    main()
