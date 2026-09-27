use smoldyn::Simulation;
use std::path::PathBuf;
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

fn main() -> anyhow::Result<()> {
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
        return show_version();
    }

    if let Some(model) = cli.model {
        let mut sim = Simulation::new().with_model_path(model)?;
        sim.run()?;
    }

    Ok(())
}

fn show_version() -> anyhow::Result<()> {
    println!("{}", smoldyn::version());
    Ok(())
}
