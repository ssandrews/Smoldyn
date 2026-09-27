//! Simulation related functions

pub mod simulation;
pub use simulation::*;

use std::path::Path;
use std::sync::atomic::AtomicBool;

use anyhow::Context;
use libsmoldyn::Sim;
pub use libsmoldyn::Progress;

/// Load a smoldyn model file and run it to its stop time, or until `stop` is
/// set. Returns [`Progress::Running`] if it was stopped early.
pub fn run(model: &Path, flags: &str, stop: &AtomicBool) -> anyhow::Result<Progress> {
    tracing::info!("Running model {:?}", model);

    let mut sim =
        Sim::from_file(model, flags).with_context(|| format!("failed to load model {model:?}"))?;
    let progress = sim
        .run_with(stop, |sim| tracing::trace!("t = {}", sim.time()))
        .with_context(|| format!("failed to run model {model:?}"))?;
    if progress == Progress::Running {
        tracing::warn!("stopped at t = {} of {}", sim.time(), sim.time_stop());
    }

    Ok(progress)
}
