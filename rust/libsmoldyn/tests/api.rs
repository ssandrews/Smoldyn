//! Tests for the wider libsmoldyn API, one area per test.

use std::fs;
use std::path::PathBuf;
use std::sync::{Mutex, MutexGuard};

use libsmoldyn::{
    CmptLogic, MolecState, PanelFace, PanelShape, RateKind, RevParam, Side, Sim, SmolError,
    SrfAction, SurfaceStyle,
};

// libsmoldyn keeps global error state, so tests must not run concurrently.
static LOCK: Mutex<()> = Mutex::new(());

fn lock() -> MutexGuard<'static, ()> {
    LOCK.lock().unwrap_or_else(|e| e.into_inner())
}

/// 3D box from -1 to 1, 5 steps of 0.002.
fn box_sim() -> Sim {
    let mut sim = Sim::new(&[-1.0; 3], &[1.0; 3]).unwrap();
    sim.set_times(0.0, 0.01, 0.002).unwrap();
    sim.set_random_seed(1).unwrap();
    sim
}

#[test]
fn settings() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.set_time_start(0.5).unwrap();
    sim.set_time_stop(2.0).unwrap();
    sim.set_time_step(0.25).unwrap();
    assert_eq!(
        (sim.time_start(), sim.time_stop(), sim.time_step()),
        (0.5, 2.0, 0.25)
    );
    sim.set_flags("q").unwrap();
    assert!(matches!(
        sim.set_flags(&"q".repeat(300)),
        Err(SmolError::InvalidArgument(_))
    ));
    sim.set_partitions("molperbox", 4.0).unwrap();
    assert!(sim.set_partitions("nonsense", 4.0).is_err());
    for side in [Side::Low, Side::High, Side::Both] {
        sim.set_boundary_type(0, side, 'r').unwrap();
    }
    assert!(sim.set_boundary_type(3, Side::Both, 'r').is_err());
    assert!(sim.set_boundary_type(0, Side::Both, 'x').is_err());
}

#[test]
fn molecules() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    sim.add_mol_list("slow").unwrap();
    sim.add_species("B", Some("slow")).unwrap();

    assert_eq!(sim.species_index("A").unwrap(), 1);
    assert_eq!(sim.species_index("B").unwrap(), 2);
    assert!(sim.species_index("C").is_err());
    assert_eq!(sim.species_name(2).unwrap(), "B");
    assert!(sim.species_name(10).is_err());

    let slow = sim.mol_list_index("slow").unwrap();
    assert_eq!(sim.mol_list_name(slow).unwrap(), "slow");
    sim.set_mol_list("A", MolecState::MSsoln, "slow").unwrap();

    sim.set_species_mobility("A", MolecState::MSall, Some(1.0), Some(&[0.1, 0.0, 0.0]), None)
        .unwrap();
    assert!(matches!(
        sim.set_species_mobility("A", MolecState::MSall, None, Some(&[0.1]), None),
        Err(SmolError::InvalidArgument(_))
    ));
    sim.set_molecule_color("A", MolecState::MSall, [1.0, 0.0, 0.0]).unwrap();
    sim.set_molecule_size("A", MolecState::MSall, 2.0).unwrap();
    sim.set_molecule_style("B", MolecState::MSall, Some(3.0), None).unwrap();
    assert!(sim.set_molecule_color("A", MolecState::MSall, [2.0, 0.0, 0.0]).is_err());

    sim.set_max_molecules(1000).unwrap();
    sim.add_solution_molecules("A", 10, None, None).unwrap();
    sim.add_solution_molecules("B", 4, Some(&[0.0; 3]), Some(&[0.5; 3])).unwrap();
    assert!(sim.add_solution_molecules("B", 1, Some(&[0.0]), None).is_err());
    sim.update().unwrap();

    assert_eq!(sim.molecule_count("A", MolecState::MSsoln).unwrap(), 10);
    assert_eq!(sim.molecule_count("B", MolecState::MSall).unwrap(), 4);
    assert_eq!(sim.molecule_count("all", MolecState::MSall).unwrap(), 14);
    sim.run().unwrap();
}

/// A sphere surface "ball" (radius 0.5) with compartment "inside" in a 3D box.
fn ball_sim() -> Sim {
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    sim.add_species("B", None).unwrap();
    sim.add_surface("ball").unwrap();
    sim.add_panel("ball", PanelShape::PSsph, Some("s1"), None, &[0.0, 0.0, 0.0, 0.5, 10.0, 10.0])
        .unwrap();
    sim.set_surface_action("ball", PanelFace::PFboth, "all", MolecState::MSsoln, SrfAction::SAreflect, None)
        .unwrap();
    sim.add_compartment("inside").unwrap();
    sim.add_compartment_surface("inside", "ball").unwrap();
    sim.add_compartment_point("inside", &[0.0, 0.0, 0.0]).unwrap();
    sim
}

#[test]
fn surfaces_and_compartments() {
    let _guard = lock();
    let mut sim = ball_sim();

    assert_eq!(sim.surface_index("ball").unwrap(), 0);
    assert_eq!(sim.surface_name(0).unwrap(), "ball");
    assert!(sim.surface_index("nope").is_err());
    assert_eq!(sim.panel_index("ball", "s1").unwrap(), (PanelShape::PSsph, 0));
    assert_eq!(sim.panel_name("ball", PanelShape::PSsph, 0).unwrap(), "s1");
    assert!(sim.panel_name("ball", PanelShape::PSsph, 3).is_err());
    assert!(sim.add_compartment_point("inside", &[0.0]).is_err());

    assert_eq!(sim.compartment_index("inside").unwrap(), 0);
    assert_eq!(sim.compartment_name(0).unwrap(), "inside");
    sim.add_compartment("outside").unwrap();
    sim.add_compartment_logic("outside", CmptLogic::CLequalnot, "inside").unwrap();

    sim.set_surface_rate("ball", "B", MolecState::MSsoln, MolecState::MSsoln, MolecState::MSfront, 0.1, None, false)
        .unwrap();
    sim.set_surface_sim_params("epsilon", 1e-6).unwrap();
    sim.set_surface_style(
        "ball",
        PanelFace::PFboth,
        &SurfaceStyle { thickness: Some(1.0), color: Some([0.0, 0.0, 1.0, 1.0]), ..Default::default() },
    )
    .unwrap();
    assert!(matches!(
        sim.set_surface_style("all", PanelFace::PFboth, &SurfaceStyle::default()),
        Err(SmolError::InvalidArgument(_))
    ));

    sim.add_compartment_molecules("A", 20, "inside").unwrap();
    sim.add_surface_molecules("B", MolecState::MSfront, 5, "ball", PanelShape::PSall, "all", None)
        .unwrap();
    sim.update().unwrap();
    assert_eq!(sim.molecule_count("A", MolecState::MSsoln).unwrap(), 20);
    assert_eq!(sim.molecule_count("B", MolecState::MSfront).unwrap(), 5);

    // A is reflected by the ball, so it stays inside.
    sim.set_species_mobility("A", MolecState::MSall, Some(1.0), None, None).unwrap();
    sim.run().unwrap();
    assert_eq!(sim.molecule_count("A", MolecState::MSsoln).unwrap(), 20);
}

#[test]
fn panel_jump_and_neighbor() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.add_surface("walls").unwrap();
    sim.add_panel("walls", PanelShape::PSrect, Some("r1"), Some("+0"), &[-0.5, -1.0, -1.0, 2.0, 2.0])
        .unwrap();
    sim.add_panel("walls", PanelShape::PSrect, Some("r2"), Some("-0"), &[0.5, -1.0, -1.0, 2.0, 2.0])
        .unwrap();
    assert!(sim.add_panel("walls", PanelShape::PSrect, Some("r3"), None, &[0.0; 5]).is_err());
    assert_eq!(sim.panel_index("walls", "r2").unwrap(), (PanelShape::PSrect, 1));
    sim.set_panel_jump("walls", "r1", PanelFace::PFfront, "r2", PanelFace::PFback, true)
        .unwrap();
    sim.add_panel_neighbor("walls", "r1", "walls", "r2", true).unwrap();
    sim.add_species("A", None).unwrap();
    sim.add_surface_unbounded_emitter("walls", PanelFace::PFfront, "A", 1.0, &[0.0; 3])
        .unwrap();
    sim.update().unwrap();
}

#[test]
fn reactions() {
    let _guard = lock();
    let mut sim = box_sim();
    for s in ["A", "B", "C"] {
        sim.add_species(s, None).unwrap();
    }
    // a bimolecular reaction needs diffusing reactants to get a binding radius.
    sim.set_species_mobility("all", MolecState::MSall, Some(1.0), None, None).unwrap();
    sim.add_reaction(
        "bind",
        &[("A", MolecState::MSsoln), ("B", MolecState::MSsoln)],
        &[("C", MolecState::MSsoln)],
        Some(10.0),
    )
    .unwrap();
    sim.add_reaction("decay", &[("C", MolecState::MSsoln)], &[], None).unwrap();
    sim.add_reaction("make", &[], &[("A", MolecState::MSsoln)], Some(0.0)).unwrap();
    assert!(sim.add_reaction("bad", &[("X", MolecState::MSsoln)], &[], None).is_err());
    assert!(matches!(
        sim.add_reaction("three", &[("A", MolecState::MSsoln); 3], &[], None),
        Err(SmolError::InvalidArgument(_))
    ));

    assert_eq!(sim.reaction_index("bind").unwrap(), (2, 0));
    assert_eq!(sim.reaction_index("decay").unwrap(), (1, 0));
    assert_eq!(sim.reaction_index("make").unwrap(), (0, 0));
    assert!(sim.reaction_index("nope").is_err());
    assert_eq!(sim.reaction_name(1, 0).unwrap(), "decay");
    assert_eq!(sim.reaction_rate("bind").unwrap(), 10.0);

    sim.set_reaction_rate("decay", 2.5, RateKind::Rate).unwrap();
    assert_eq!(sim.reaction_rate("decay").unwrap(), 2.5);
    sim.set_reaction_products("bind", RevParam::RPirrev, 0.0, None, None).unwrap();
    sim.set_reaction_intersurface("bind", &[1]).unwrap();
    sim.set_reaction_intersurface("bind", &[]).unwrap();

    sim.add_compartment("everywhere").unwrap();
    sim.add_compartment_point("everywhere", &[0.0; 3]).unwrap();
    sim.set_reaction_region("decay", Some("everywhere"), None).unwrap();

    sim.add_solution_molecules("A", 50, None, None).unwrap();
    sim.add_solution_molecules("B", 50, None, None).unwrap();
    sim.update().unwrap();
    sim.run().unwrap();
}

#[test]
fn output_data_and_commands() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    sim.add_solution_molecules("A", 7, None, None).unwrap();
    sim.add_output_data("counts").unwrap();
    sim.add_command("E molcount counts").unwrap();
    sim.add_timed_command('@', 0.004, 0.0, 0.0, 0.0, "molcount counts").unwrap();
    assert!(sim.add_timed_command('é', 0.0, 0.0, 0.0, 0.0, "stop").is_err());
    sim.update().unwrap();
    sim.run_command("molcount counts").unwrap();
    sim.run().unwrap();

    let data = sim.output_data("counts", false).unwrap();
    assert_eq!(data.ncol, 2, "time + one species");
    assert!(data.nrow >= 7, "{data:?}");
    assert!(data.rows().all(|row| row[1] == 7.0), "{data:?}");
    assert_eq!(data.rows().count(), data.nrow);
    assert!(data.row(data.nrow).is_none());

    let erased = sim.output_data("counts", true).unwrap();
    assert_eq!(erased, data);
    assert_eq!(sim.output_data("counts", false).unwrap().nrow, 0);
    assert!(sim.output_data("nope", false).is_err());
}

#[test]
fn output_files() {
    let _guard = lock();
    let dir = PathBuf::from(env!("CARGO_TARGET_TMPDIR"));
    let _ = fs::remove_file(dir.join("api_out.txt"));
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    sim.set_output_path(&format!("{}/", dir.display())).unwrap();
    sim.add_output_file("api_out.txt", None, false).unwrap();
    sim.add_command("E molcount api_out.txt").unwrap();
    sim.update().unwrap();
    sim.run().unwrap();
    drop(sim); // flushes and closes the file
    let lines = fs::read_to_string(dir.join("api_out.txt")).unwrap().lines().count();
    assert!(lines >= 5, "{lines} lines");
}

#[test]
fn ports() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    sim.add_surface("edge").unwrap();
    sim.add_panel("edge", PanelShape::PSrect, Some("r1"), Some("+0"), &[0.9, -1.0, -1.0, 2.0, 2.0])
        .unwrap();
    sim.set_surface_action("edge", PanelFace::PFfront, "A", MolecState::MSsoln, SrfAction::SAport, None)
        .unwrap();
    sim.add_port("out", "edge", PanelFace::PFfront).unwrap();
    assert_eq!(sim.port_index("out").unwrap(), 0);
    assert_eq!(sim.port_name(0).unwrap(), "out");
    sim.update().unwrap();

    sim.add_port_molecules("out", "A", 3, None).unwrap();
    let at = [0.95, 0.0, 0.0];
    sim.add_port_molecules("out", "A", 1, Some(&[&at])).unwrap();
    assert!(sim.add_port_molecules("out", "A", 2, Some(&[&at])).is_err());
    assert_eq!(sim.molecule_count("A", MolecState::MSall).unwrap(), 4);
    assert_eq!(sim.port_molecules("out", "A", MolecState::MSall, false).unwrap(), 0);
}

#[test]
fn lattices() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.add_species("A", None).unwrap();
    // this used to crash: smolAddLattice read sim->latticess before it existed.
    sim.add_lattice("lat", &[-1.0; 3], &[1.0; 3], &[0.5; 3], "rrr").unwrap();
    assert!(sim.add_lattice("lat2", &[-1.0; 2], &[1.0; 3], &[0.5; 3], "rrr").is_err());
    assert_eq!(sim.lattice_index("lat").unwrap(), 0);
    assert_eq!(sim.lattice_name(0).unwrap(), "lat");
    sim.add_lattice_species("lat", "A").unwrap();
    sim.add_lattice_molecules("lat", "A", 5, None, None).unwrap();
    sim.add_reaction("decay", &[("A", MolecState::MSsoln)], &[], Some(1.0)).unwrap();
    sim.add_lattice_reaction("lat", "decay", false).unwrap();
}

#[test]
fn graphics_settings() {
    let _guard = lock();
    let mut sim = box_sim();
    sim.set_graphics_params("none", Some(1), Some(0)).unwrap();
    assert!(sim.set_graphics_params("nonsense", None, None).is_err());
    sim.set_background_style([0.0, 0.0, 0.0, 1.0]).unwrap();
    sim.set_frame_style(Some(2.0), Some([1.0, 1.0, 1.0, 1.0])).unwrap();
    sim.set_grid_style(Some(1.0), None).unwrap();
    sim.set_text_style([1.0, 1.0, 1.0, 1.0]).unwrap();
    sim.set_light_params(0, None, Some([1.0, 1.0, 1.0, 1.0]), None, None).unwrap();
    assert!(sim.set_light_params(-1, None, Some([1.0; 4]), None, None).is_err());
    sim.add_text_display("time").unwrap();
    sim.set_tiff_params(Some(10), Some("snap"), Some(1), None).unwrap();
}

#[test]
fn load_from_file_then_modify() {
    let _guard = lock();
    let model = PathBuf::from(env!("CARGO_TARGET_TMPDIR")).join("api_load.txt");
    fs::write(
        &model,
        "dim 2\nboundaries 0 0 1\nboundaries 1 0 1\ntime_start 0\ntime_stop 0.01\ntime_step 0.002\nspecies A\n",
    )
    .unwrap();
    let mut sim = Sim::load_from_file(&model, "q").unwrap();
    assert_eq!(sim.dim(), 2);
    sim.add_species("B", None).unwrap();
    sim.add_solution_molecules("B", 3, None, None).unwrap();
    sim.update().unwrap();
    assert_eq!(sim.species(), ["A", "B"]);
    assert_eq!(sim.molecule_count("B", MolecState::MSall).unwrap(), 3);
    sim.run().unwrap();

    assert!(Sim::load_from_file("/nonexistent/model.txt", "q").is_err());
}
