use smoldyn::{Progress, Simulation};
use std::path::PathBuf;
use std::process::ExitCode;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use tracing_subscriber::{EnvFilter, fmt, prelude::*};

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(about, long_about = None)]
struct Cli {
    /// smoldyn model
    model: Option<PathBuf>,

    /// Turn debugging information on
    #[arg(short, long, action = clap::ArgAction::Count)]
    debug: u8,

    /// version
    #[arg(long)]
    version: bool,

    #[command(subcommand)]
    command: Option<Commands>,
}

#[derive(Subcommand)]
enum Commands {
    /// does testing things
    Simulate,
}

fn main() -> anyhow::Result<ExitCode> {
    let cli = Cli::parse();

    // You can see how many times a particular flag or argument occurred
    // Note, only flags can have multiple occurrences
    let log_level = match cli.debug {
        0 => "warn",
        1 => "info",
        2 => "debug",
        _ => "trace",
    };

    unsafe {
        std::env::set_var("SMOLDYN_LOG", log_level);
    }

    tracing_subscriber::registry()
        .with(fmt::layer())
        .with(EnvFilter::from_env("SMOLDYN_LOG"))
        .init();

    if cli.version {
        show_version();
        return Ok(ExitCode::SUCCESS);
    }

    if let Some(model) = cli.model {
        let stop = stop_on_ctrlc()?;
        let mut sim = Simulation::new().with_model_path(model)?;
        if sim.run(&stop)? == Progress::Running {
            // interrupted; output files are closed by now.
            return Ok(ExitCode::from(130));
        }
    }

    Ok(ExitCode::SUCCESS)
}

/// The first Ctrl-C asks the simulation to stop after the current time step
/// (so output files are flushed); a second one exits immediately.
fn stop_on_ctrlc() -> anyhow::Result<Arc<AtomicBool>> {
    let stop = Arc::new(AtomicBool::new(false));
    let handler_stop = stop.clone();
    ctrlc::set_handler(move || {
        if handler_stop.swap(true, Ordering::Relaxed) {
            std::process::exit(130);
        }
        eprintln!("stopping after the current time step (Ctrl-C again to quit now)");
    })?;
    Ok(stop)
}

fn show_version() {
    println!("{}", libsmoldyn::version());
}
