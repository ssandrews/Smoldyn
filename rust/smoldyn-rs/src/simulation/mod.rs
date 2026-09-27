//! Simulation related functions

pub mod simulation;
pub use simulation::*;

use std::path::Path;

use anyhow::Context;
use libsmoldyn::Sim;

/// Load a smoldyn model file and run it to its stop time.
pub fn run(model: &Path, flags: &str) -> anyhow::Result<()> {
    tracing::info!("Running model {:?}", model);

    let mut sim =
        Sim::from_file(model, flags).with_context(|| format!("failed to load model {model:?}"))?;
    sim.run()
        .with_context(|| format!("failed to run model {model:?}"))?;

    Ok(())
}
