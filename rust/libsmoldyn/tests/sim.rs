//! Tests for the safe `Sim` wrapper.

use std::fs;
use std::path::PathBuf;
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};

use libsmoldyn::{MolecState, Progress, Sim, SmolError};

// libsmoldyn keeps global error state, so tests must not run concurrently.
static LOCK: Mutex<()> = Mutex::new(());

const TINY_MODEL: &str = r#"
dim 3
boundaries 0 -1 1
boundaries 1 -1 1
boundaries 2 -1 1
time_start 0
time_stop 0.01
time_step 0.002

species A
difc A 1
"#;

fn tiny_model(name: &str) -> PathBuf {
    let dir = PathBuf::from(env!("CARGO_TARGET_TMPDIR"));
    fs::create_dir_all(&dir).unwrap();
    let model = dir.join(name);
    fs::write(&model, TINY_MODEL).unwrap();
    model
}

fn lock() -> std::sync::MutexGuard<'static, ()> {
    LOCK.lock().unwrap_or_else(|e| e.into_inner())
}

#[test]
fn from_file_and_run() {
    let _guard = lock();
    let mut sim = Sim::from_file(tiny_model("sim_run.txt"), "q").unwrap();
    sim.run().unwrap();
}

#[test]
fn missing_file_is_smoldyn_error() {
    let _guard = lock();
    let err = Sim::from_file("/nonexistent/model.txt", "q").unwrap_err();
    assert!(matches!(err, SmolError::Smoldyn { .. }), "{err}");
}

#[test]
fn nul_byte_is_invalid_argument() {
    let _guard = lock();
    let err = Sim::from_file("bad\0name.txt", "").unwrap_err();
    assert!(matches!(err, SmolError::InvalidArgument(_)), "{err}");
}

#[test]
fn new_rejects_bad_dims() {
    let _guard = lock();
    assert!(matches!(
        Sim::new(&[0.0; 4], &[1.0; 4]),
        Err(SmolError::InvalidArgument(_))
    ));
    assert!(matches!(
        Sim::new(&[0.0], &[1.0, 1.0]),
        Err(SmolError::InvalidArgument(_))
    ));
    // checked by libsmoldyn itself
    assert!(matches!(
        Sim::new(&[1.0], &[0.0]),
        Err(SmolError::Smoldyn { .. })
    ));
}

#[test]
fn build_and_step_to_completion() {
    let _guard = lock();
    let mut sim = Sim::new(&[-1.0; 3], &[1.0; 3]).unwrap();
    sim.set_times(0.0, 0.01, 0.002).unwrap();
    sim.add_species("A", None).unwrap();
    sim.read_config("difc", "A 1").unwrap();
    sim.update().unwrap();

    let mut steps = 0;
    while sim.step().unwrap() == Progress::Running {
        steps += 1;
        assert!(steps < 100, "simulation never finished");
    }
    assert!(steps >= 4, "only {steps} steps");
}

#[test]
fn bad_config_statement_is_error() {
    let _guard = lock();
    let mut sim = Sim::new(&[-1.0; 3], &[1.0; 3]).unwrap();
    assert!(sim.read_config("no_such_statement", "1 2 3").is_err());
}

#[test]
fn getters_reflect_model() {
    let _guard = lock();
    let mut sim = Sim::new(&[-1.0, -2.0], &[1.0, 3.0]).unwrap();
    sim.set_times(0.0, 0.01, 0.002).unwrap();
    sim.add_species("A", None).unwrap();
    sim.add_species("B", None).unwrap();
    sim.read_config("difc", "A 1").unwrap();
    sim.read_config("mol", "10 A u u").unwrap();
    sim.read_config("mol", "3 B u u").unwrap();
    sim.update().unwrap();

    assert_eq!(sim.dim(), 2);
    assert_eq!(sim.bounds(), (vec![-1.0, -2.0], vec![1.0, 3.0]));
    assert_eq!(sim.time_start(), 0.0);
    assert_eq!(sim.time_stop(), 0.01);
    assert_eq!(sim.time_step(), 0.002);
    assert_eq!(sim.species(), ["A", "B"]);

    assert_eq!(sim.molecule_count("A", MolecState::MSsoln).unwrap(), 10);
    assert_eq!(sim.molecule_count("all", MolecState::MSall).unwrap(), 13);
    assert!(sim.molecule_count("C", MolecState::MSall).is_err());

    sim.run_until(0.006).unwrap();
    assert!(sim.time() >= 0.006 - 1e-12, "time is {}", sim.time());
    assert!(sim.time() < sim.time_stop());
}

#[test]
fn run_after_step_reaches_stop_time() {
    let _guard = lock();
    let mut sim = Sim::new(&[-1.0; 3], &[1.0; 3]).unwrap();
    sim.set_times(0.0, 0.01, 0.002).unwrap();
    sim.update().unwrap();
    sim.step().unwrap();
    sim.step().unwrap();
    sim.run().unwrap();
    assert!(
        (sim.time() - sim.time_stop()).abs() < 1e-9,
        "stopped at {} instead of {}",
        sim.time(),
        sim.time_stop()
    );
}

fn tiny_sim() -> Sim {
    let mut sim = Sim::new(&[-1.0; 3], &[1.0; 3]).unwrap();
    sim.set_times(0.0, 0.01, 0.002).unwrap();
    sim.add_species("A", None).unwrap();
    sim.read_config("mol", "5 A u u u").unwrap();
    sim.update().unwrap();
    sim
}

#[test]
fn run_with_calls_back_every_step() {
    let _guard = lock();
    let mut sim = tiny_sim();
    let mut times = Vec::new();
    let progress = sim
        .run_with(&AtomicBool::new(false), |s| {
            times.push(s.time());
            assert_eq!(s.molecule_count("A", MolecState::MSall).unwrap(), 5);
        })
        .unwrap();

    assert_eq!(progress, Progress::Finished);
    assert!(times.len() >= 5, "{times:?}");
    assert!(times.windows(2).all(|w| w[0] < w[1]), "{times:?}");
    assert!((sim.time() - sim.time_stop()).abs() < 1e-9);
}

#[test]
fn run_with_stops_and_resumes() {
    let _guard = lock();
    let mut sim = tiny_sim();
    let stop = AtomicBool::new(false);

    let mut steps = 0;
    let progress = sim
        .run_with(&stop, |_| {
            steps += 1;
            if steps == 2 {
                stop.store(true, Ordering::Relaxed);
            }
        })
        .unwrap();
    assert_eq!(progress, Progress::Running);
    assert_eq!(steps, 2);
    assert!(sim.time() < sim.time_stop());

    stop.store(false, Ordering::Relaxed);
    assert_eq!(sim.run_with(&stop, |_| {}).unwrap(), Progress::Finished);
    assert!((sim.time() - sim.time_stop()).abs() < 1e-9);
}

#[test]
fn run_with_on_other_thread() {
    let _guard = lock();
    let mut sim = tiny_sim();
    let (tx, rx) = std::sync::mpsc::channel();
    let worker = std::thread::spawn(move || {
        sim.run_with(&AtomicBool::new(false), |s| tx.send(s.time()).unwrap())
            .unwrap()
    });
    let times: Vec<f64> = rx.iter().collect();
    assert_eq!(worker.join().unwrap(), Progress::Finished);
    assert!(!times.is_empty());
}

#[test]
fn run_with_resume_keeps_output() {
    let _guard = lock();
    let out_dir = PathBuf::from(env!("CARGO_TARGET_TMPDIR"));
    let out = out_dir.join("run_with_resume.txt");
    let _ = fs::remove_file(&out);

    let mut sim = tiny_sim();
    sim.read_config("output_root", &format!("{}/", out_dir.display())).unwrap();
    sim.read_config("output_files", "run_with_resume.txt").unwrap();
    sim.add_command("E molcount run_with_resume.txt").unwrap();
    sim.update().unwrap();

    let stop = AtomicBool::new(false);
    let mut steps = 0;
    sim.run_with(&stop, |_| {
        steps += 1;
        if steps == 2 {
            stop.store(true, Ordering::Relaxed);
        }
    })
    .unwrap();
    stop.store(false, Ordering::Relaxed);
    sim.run_with(&stop, |_| {}).unwrap();

    let lines = fs::read_to_string(&out).unwrap().lines().count();
    // one molcount line per step (plus the initial one), none lost on resume.
    assert!(lines >= 5, "only {lines} lines in {out:?}");
}
