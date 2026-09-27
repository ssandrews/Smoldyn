//! End-to-end tests that drive the linked Smoldyn C library.

use std::fs;
use std::path::PathBuf;
use std::sync::Mutex;

// The Smoldyn C library keeps global error/warning state, so tests that touch it
// must not run concurrently within the same process.
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

fn tmp_dir() -> PathBuf {
    let dir = PathBuf::from(env!("CARGO_TARGET_TMPDIR"));
    fs::create_dir_all(&dir).unwrap();
    dir
}

#[test]
fn runs_tiny_model() {
    let _guard = LOCK.lock().unwrap();

    let model = tmp_dir().join("tiny_e2e.txt");
    fs::write(&model, TINY_MODEL).unwrap();

    println!("{}", smoldyn::version());
    smoldyn::run(&model, "").expect("tiny model should run");
}

#[test]
fn missing_model_fails() {
    let _guard = LOCK.lock().unwrap();

    let model = tmp_dir().join("does_not_exist.txt");
    assert!(smoldyn::run(&model, "").is_err());
}
