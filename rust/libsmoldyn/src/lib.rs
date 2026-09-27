//! smoldyn library.
//!
//! `ffi` holds the raw bindings to `libsmoldyn.h`. [`Sim`] is the safe, owning
//! wrapper around `simptr`; all `unsafe` code should stay inside this crate.
//!
//! Not wrapped: `smolSetLogging` (C variadic callback), `smolSetError`/`smolSetErrorNT`
//! (internal), `smolSetThrowing` (a C++ exception crossing into Rust aborts), and the
//! `*NT` lookup variants (the wrappers use the variants that record an error message).

mod error;
mod sim;

pub use error::{SmolError, SmolResult};
pub use ffi::{CmptLogic, DrawMode, MolecState, PanelFace, PanelShape, RevParam, SrfAction};
pub use sim::{OutputData, Progress, RateKind, Side, Sim, SurfaceStyle};

// Raw bindings: safety requirements are those of the C API, and the argument
// lists mirror libsmoldyn.h.
#[allow(clippy::missing_safety_doc, clippy::too_many_arguments)]
#[cxx::bridge]
pub mod ffi {
    /// Mirrors `enum ErrorCode` in `libsmoldyn.h`. cxx statically checks that the
    /// discriminants of these enums match the C++ definitions.
    #[repr(i32)]
    #[derive(Debug)]
    enum ErrorCode {
        ECok = 0,
        ECnotify = -1,
        ECwarning = -2,
        ECnonexist = -3,
        ECall = -4,
        ECmissing = -5,
        ECbounds = -6,
        ECsyntax = -7,
        ECerror = -8,
        ECmemory = -9,
        ECbug = -10,
        ECsame = -11,
        ECwildcard = -12,
    }

    /// Mirrors `enum MolecState` in `smoldyn.h`: where a molecule is, relative
    /// to a surface.
    #[repr(i32)]
    #[derive(Debug)]
    enum MolecState {
        /// in solution
        MSsoln = 0,
        /// bound to the front of a surface
        MSfront = 1,
        /// bound to the back of a surface
        MSback = 2,
        /// transmembrane, sticking up
        MSup = 3,
        /// transmembrane, sticking down
        MSdown = 4,
        /// in solution but bound to a surface
        MSbsoln = 5,
        /// any state
        MSall = 6,
        MSnone = 7,
        MSsome = 8,
    }

    /// Mirrors `enum PanelFace` in `smoldyn.h`.
    #[repr(i32)]
    #[derive(Debug)]
    enum PanelFace {
        PFfront = 0,
        PFback = 1,
        PFnone = 2,
        PFboth = 3,
    }

    /// Mirrors `enum PanelShape` in `smoldyn.h`.
    #[repr(i32)]
    #[derive(Debug)]
    enum PanelShape {
        /// rectangle
        PSrect = 0,
        /// triangle
        PStri = 1,
        /// sphere
        PSsph = 2,
        /// cylinder
        PScyl = 3,
        /// hemisphere
        PShemi = 4,
        /// disk
        PSdisk = 5,
        PSall = 6,
        PSnone = 7,
    }

    /// Mirrors `enum SrfAction` in `smoldyn.h`: what a surface does to a molecule.
    #[repr(i32)]
    #[derive(Debug)]
    enum SrfAction {
        SAreflect = 0,
        SAtrans = 1,
        SAabsorb = 2,
        SAjump = 3,
        SAport = 4,
        SAmult = 5,
        SAno = 6,
        SAnone = 7,
        SAadsorb = 8,
        SArevdes = 9,
        SAirrevdes = 10,
        SAflip = 11,
    }

    /// Mirrors `enum DrawMode` in `smoldyn.h`: how surfaces are drawn.
    #[repr(i32)]
    #[derive(Debug)]
    enum DrawMode {
        DMno = 0,
        DMvert = 1,
        DMedge = 2,
        DMve = 3,
        DMface = 4,
        DMvf = 5,
        DMef = 6,
        DMvef = 7,
        /// leave the drawing mode unchanged
        DMnone = 8,
    }

    /// Mirrors `enum CmptLogic` in `smoldyn.h`: how compartments are combined.
    #[repr(i32)]
    #[derive(Debug)]
    enum CmptLogic {
        CLequal = 0,
        CLequalnot = 1,
        CLand = 2,
        CLor = 3,
        CLxor = 4,
        CLandnot = 5,
        CLornot = 6,
        CLnone = 7,
    }

    /// Mirrors `enum RevParam` in `smoldyn.h`: how reaction products are placed.
    #[repr(i32)]
    #[derive(Debug)]
    enum RevParam {
        RPnone = 0,
        RPirrev = 1,
        RPconfspread = 2,
        RPbounce = 3,
        RPpgem = 4,
        RPpgemmax = 5,
        RPpgemmaxw = 6,
        RPratio = 7,
        RPunbindrad = 8,
        RPpgem2 = 9,
        RPpgemmax2 = 10,
        RPratio2 = 11,
        RPoffset = 12,
        RPfixed = 13,
    }

    unsafe extern "C++" {
        include!("Smoldyn/libsmoldyn.h");

        type simstruct;
        type ErrorCode;
        type MolecState;
        type PanelFace;
        type PanelShape;
        type SrfAction;
        type DrawMode;
        type CmptLogic;
        type RevParam;

        unsafe fn smolGetVersion() -> f64;

        // errors
        unsafe fn smolGetError(
            errorfunction: *mut c_char,
            errorstring: *mut c_char,
            clearerror: i32,
        ) -> ErrorCode;
        unsafe fn smolClearError();
        unsafe fn smolSetDebugMode(debugmode: i32);

        // sim structure
        unsafe fn smolNewSim(dim: i32, lowbounds: *mut f64, highbounds: *mut f64)
        -> *mut simstruct;
        unsafe fn smolUpdateSim(sim: *mut simstruct) -> ErrorCode;
        unsafe fn smolRunTimeStep(sim: *mut simstruct) -> ErrorCode;
        unsafe fn smolRunSim(sim: *mut simstruct) -> ErrorCode;
        unsafe fn smolRunSimUntil(sim: *mut simstruct, breaktime: f64) -> ErrorCode;
        unsafe fn smolFreeSim(sim: *mut simstruct) -> ErrorCode;
        unsafe fn smolDisplaySim(sim: *mut simstruct) -> ErrorCode;

        // configuration
        unsafe fn smolPrepareSimFromFile(
            filepath: *const c_char,
            filename: *const c_char,
            flags: *const c_char,
        ) -> *mut simstruct;
        unsafe fn smolLoadSimFromFile(
            filepath: *const c_char,
            filename: *const c_char,
            simpointer: *mut *mut simstruct,
            flags: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolReadConfigString(
            sim: *mut simstruct,
            statement: *const c_char,
            parameters: *mut c_char,
        ) -> ErrorCode;

        // simulation settings
        unsafe fn smolSetSimFlags(sim: *mut simstruct, flags: *const c_char) -> ErrorCode;
        unsafe fn smolSetSimTimes(
            sim: *mut simstruct,
            timestart: f64,
            timestop: f64,
            timestep: f64,
        ) -> ErrorCode;
        unsafe fn smolSetTimeStart(sim: *mut simstruct, timestart: f64) -> ErrorCode;
        unsafe fn smolSetTimeStop(sim: *mut simstruct, timestop: f64) -> ErrorCode;
        unsafe fn smolSetTimeNow(sim: *mut simstruct, timenow: f64) -> ErrorCode;
        unsafe fn smolSetTimeStep(sim: *mut simstruct, timestep: f64) -> ErrorCode;
        // `long int`, which is 64 bits on LP64 platforms.
        unsafe fn smolSetRandomSeed(sim: *mut simstruct, seed: i64) -> ErrorCode;
        unsafe fn smolSetPartitions(
            sim: *mut simstruct,
            method: *const c_char,
            value: f64,
        ) -> ErrorCode;

        // graphics
        unsafe fn smolSetGraphicsParams(
            sim: *mut simstruct,
            method: *const c_char,
            timesteps: i32,
            delay: i32,
        ) -> ErrorCode;
        unsafe fn smolSetTiffParams(
            sim: *mut simstruct,
            timesteps: i32,
            tiffname: *const c_char,
            lowcount: i32,
            highcount: i32,
        ) -> ErrorCode;
        unsafe fn smolSetLightParams(
            sim: *mut simstruct,
            lightindex: i32,
            ambient: *mut f64,
            diffuse: *mut f64,
            specular: *mut f64,
            position: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolSetBackgroundStyle(sim: *mut simstruct, color: *mut f64) -> ErrorCode;
        unsafe fn smolSetFrameStyle(sim: *mut simstruct, thickness: f64, color: *mut f64)
        -> ErrorCode;
        unsafe fn smolSetGridStyle(sim: *mut simstruct, thickness: f64, color: *mut f64)
        -> ErrorCode;
        unsafe fn smolSetTextStyle(sim: *mut simstruct, color: *mut f64) -> ErrorCode;
        unsafe fn smolAddTextDisplay(sim: *mut simstruct, item: *mut c_char) -> ErrorCode;

        // runtime commands and output
        unsafe fn smolSetOutputPath(sim: *mut simstruct, path: *const c_char) -> ErrorCode;
        unsafe fn smolAddOutputFile(
            sim: *mut simstruct,
            filename: *mut c_char,
            suffix: i32,
            append: i32,
        ) -> ErrorCode;
        unsafe fn smolAddOutputData(sim: *mut simstruct, dataname: *mut c_char) -> ErrorCode;
        unsafe fn smolOpenOutputFiles(sim: *mut simstruct, overwrite: i32) -> ErrorCode;
        unsafe fn smolAddCommand(
            sim: *mut simstruct,
            kind: c_char,
            on: f64,
            off: f64,
            step: f64,
            multiplier: f64,
            commandstring: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolAddCommandFromString(sim: *mut simstruct, string: *mut c_char) -> ErrorCode;
        unsafe fn smolGetOutputData(
            sim: *mut simstruct,
            dataname: *mut c_char,
            nrow: *mut i32,
            ncol: *mut i32,
            array: *mut *mut f64,
            erase: i32,
        ) -> ErrorCode;
        unsafe fn smolRunCommand(sim: *mut simstruct, commandstring: *const c_char) -> ErrorCode;

        // molecules
        unsafe fn smolAddSpecies(
            sim: *mut simstruct,
            species: *const c_char,
            mollist: *const c_char,
        ) -> ErrorCode;
        /// Returns the index (>0), or a negative `ErrorCode` on failure.
        unsafe fn smolGetSpeciesIndex(sim: *mut simstruct, species: *const c_char) -> i32;
        unsafe fn smolGetSpeciesName(sim: *mut simstruct, speciesindex: i32, species: *mut c_char);
        unsafe fn smolSetSpeciesMobility(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            difc: f64,
            drift: *mut f64,
            difmatrix: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolSetMoleculeColor(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            color: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolSetMoleculeSize(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            size: f64,
        ) -> ErrorCode;
        unsafe fn smolAddMolList(sim: *mut simstruct, mollist: *const c_char) -> ErrorCode;
        unsafe fn smolGetMolListIndex(sim: *mut simstruct, mollist: *const c_char) -> i32;
        unsafe fn smolGetMolListName(
            sim: *mut simstruct,
            mollistindex: i32,
            mollist: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolSetMolList(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            mollist: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolSetMaxMolecules(sim: *mut simstruct, maxmolecules: i32) -> ErrorCode;
        unsafe fn smolAddSolutionMolecules(
            sim: *mut simstruct,
            species: *const c_char,
            number: i32,
            lowposition: *mut f64,
            highposition: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolAddCompartmentMolecules(
            sim: *mut simstruct,
            species: *const c_char,
            number: i32,
            compartment: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolAddSurfaceMolecules(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            number: i32,
            surface: *const c_char,
            panelshape: PanelShape,
            panel: *const c_char,
            position: *mut f64,
        ) -> ErrorCode;
        /// Returns the count, or a negative `ErrorCode` on failure.
        unsafe fn smolGetMoleculeCount(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
        ) -> i32;
        unsafe fn smolSetMoleculeStyle(
            sim: *mut simstruct,
            species: *const c_char,
            state: MolecState,
            size: f64,
            color: *mut f64,
        ) -> ErrorCode;

        // surfaces
        unsafe fn smolSetBoundaryType(
            sim: *mut simstruct,
            dimension: i32,
            highside: i32,
            kind: c_char,
        ) -> ErrorCode;
        unsafe fn smolAddSurface(sim: *mut simstruct, surface: *const c_char) -> ErrorCode;
        unsafe fn smolGetSurfaceIndex(sim: *mut simstruct, surface: *const c_char) -> i32;
        unsafe fn smolGetSurfaceName(
            sim: *mut simstruct,
            surfaceindex: i32,
            surface: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolSetReactionIntersurface(
            sim: *mut simstruct,
            reaction: *const c_char,
            rulelist: *mut i32,
        ) -> ErrorCode;
        unsafe fn smolSetSurfaceAction(
            sim: *mut simstruct,
            surface: *const c_char,
            face: PanelFace,
            species: *const c_char,
            state: MolecState,
            action: SrfAction,
            newspecies: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolSetSurfaceRate(
            sim: *mut simstruct,
            surface: *const c_char,
            species: *const c_char,
            state: MolecState,
            state1: MolecState,
            state2: MolecState,
            rate: f64,
            newspecies: *const c_char,
            isinternal: i32,
        ) -> ErrorCode;
        unsafe fn smolAddPanel(
            sim: *mut simstruct,
            surface: *const c_char,
            panelshape: PanelShape,
            panel: *const c_char,
            axisstring: *const c_char,
            params: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolGetPanelIndex(
            sim: *mut simstruct,
            surface: *const c_char,
            panelshapeptr: *mut PanelShape,
            panel: *const c_char,
        ) -> i32;
        unsafe fn smolGetPanelName(
            sim: *mut simstruct,
            surface: *const c_char,
            panelshape: PanelShape,
            panelindex: i32,
            panel: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolSetPanelJump(
            sim: *mut simstruct,
            surface: *const c_char,
            panel1: *const c_char,
            face1: PanelFace,
            panel2: *const c_char,
            face2: PanelFace,
            isbidirectional: i32,
        ) -> ErrorCode;
        unsafe fn smolAddSurfaceUnboundedEmitter(
            sim: *mut simstruct,
            surface: *const c_char,
            face: PanelFace,
            species: *const c_char,
            emitamount: f64,
            emitposition: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolSetSurfaceSimParams(
            sim: *mut simstruct,
            parameter: *const c_char,
            value: f64,
        ) -> ErrorCode;
        unsafe fn smolAddPanelNeighbor(
            sim: *mut simstruct,
            surface1: *const c_char,
            panel1: *const c_char,
            surface2: *const c_char,
            panel2: *const c_char,
            reciprocal: i32,
        ) -> ErrorCode;
        unsafe fn smolSetSurfaceStyle(
            sim: *mut simstruct,
            surface: *const c_char,
            face: PanelFace,
            mode: DrawMode,
            thickness: f64,
            color: *mut f64,
            stipplefactor: i32,
            stipplepattern: i32,
            shininess: f64,
        ) -> ErrorCode;

        // compartments
        unsafe fn smolAddCompartment(sim: *mut simstruct, compartment: *const c_char)
        -> ErrorCode;
        unsafe fn smolGetCompartmentIndex(sim: *mut simstruct, compartment: *const c_char) -> i32;
        unsafe fn smolGetCompartmentName(
            sim: *mut simstruct,
            compartmentindex: i32,
            compartment: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolAddCompartmentSurface(
            sim: *mut simstruct,
            compartment: *const c_char,
            surface: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolAddCompartmentPoint(
            sim: *mut simstruct,
            compartment: *const c_char,
            point: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolAddCompartmentLogic(
            sim: *mut simstruct,
            compartment: *const c_char,
            logic: CmptLogic,
            compartment2: *const c_char,
        ) -> ErrorCode;

        // reactions
        unsafe fn smolAddReaction(
            sim: *mut simstruct,
            reaction: *const c_char,
            reactant1: *const c_char,
            rstate1: MolecState,
            reactant2: *const c_char,
            rstate2: MolecState,
            nproduct: i32,
            productspecies: *mut *const c_char,
            productstates: *mut MolecState,
            rate: f64,
        ) -> ErrorCode;
        unsafe fn smolGetReactionIndex(
            sim: *mut simstruct,
            orderptr: *mut i32,
            reaction: *const c_char,
        ) -> i32;
        unsafe fn smolGetReactionName(
            sim: *mut simstruct,
            order: i32,
            reactionindex: i32,
            reaction: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolSetReactionRate(
            sim: *mut simstruct,
            reaction: *const c_char,
            rate: f64,
            kind: i32,
        ) -> ErrorCode;
        unsafe fn smolGetReactionRate(
            sim: *mut simstruct,
            reaction: *const c_char,
            rate: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolSetReactionRegion(
            sim: *mut simstruct,
            reaction: *const c_char,
            compartment: *const c_char,
            surface: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolSetReactionProducts(
            sim: *mut simstruct,
            reaction: *const c_char,
            method: RevParam,
            parameter: f64,
            product: *const c_char,
            position: *mut f64,
        ) -> ErrorCode;

        // ports
        unsafe fn smolAddPort(
            sim: *mut simstruct,
            port: *const c_char,
            surface: *const c_char,
            face: PanelFace,
        ) -> ErrorCode;
        unsafe fn smolGetPortIndex(sim: *mut simstruct, port: *const c_char) -> i32;
        unsafe fn smolGetPortName(
            sim: *mut simstruct,
            portindex: i32,
            port: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolAddPortMolecules(
            sim: *mut simstruct,
            port: *const c_char,
            nmolec: i32,
            species: *const c_char,
            positions: *mut *mut f64,
        ) -> ErrorCode;
        /// Returns the count, or a negative `ErrorCode` on failure.
        unsafe fn smolGetPortMolecules(
            sim: *mut simstruct,
            port: *const c_char,
            species: *const c_char,
            state: MolecState,
            remove: i32,
        ) -> i32;

        // lattices
        unsafe fn smolAddLattice(
            sim: *mut simstruct,
            lattice: *const c_char,
            min: *const f64,
            max: *const f64,
            dx: *const f64,
            btype: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolAddLatticePort(
            sim: *mut simstruct,
            lattice: *const c_char,
            port: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolAddLatticeSpecies(
            sim: *mut simstruct,
            lattice: *const c_char,
            species: *const c_char,
        ) -> ErrorCode;
        unsafe fn smolGetLatticeIndex(sim: *mut simstruct, lattice: *const c_char) -> i32;
        unsafe fn smolGetLatticeName(
            sim: *mut simstruct,
            latticeindex: i32,
            lattice: *mut c_char,
        ) -> *mut c_char;
        unsafe fn smolAddLatticeMolecules(
            sim: *mut simstruct,
            lattice: *const c_char,
            species: *const c_char,
            number: i32,
            lowposition: *mut f64,
            highposition: *mut f64,
        ) -> ErrorCode;
        unsafe fn smolAddLatticeReaction(
            sim: *mut simstruct,
            lattice: *const c_char,
            reaction: *const c_char,
            mov: i32,
        ) -> ErrorCode;
    }

    // Field accessors and helpers, see include/smolrs.h.
    unsafe extern "C++" {
        include!("smolrs.h");

        fn smolrs_dim(sim: &simstruct) -> i32;
        fn smolrs_time(sim: &simstruct) -> f64;
        fn smolrs_time_start(sim: &simstruct) -> f64;
        fn smolrs_time_stop(sim: &simstruct) -> f64;
        fn smolrs_time_step(sim: &simstruct) -> f64;
        fn smolrs_wall_pos(sim: &simstruct, d: i32, highside: i32) -> f64;
        fn smolrs_nspecies(sim: &simstruct) -> i32;
        fn smolrs_species_name(sim: &simstruct, i: i32) -> *const c_char;
        fn smolrs_filepath_len(sim: &simstruct) -> usize;
        unsafe fn smolrs_free_doubles(array: *mut f64);
    }
}

/// Return smoldyn version
pub fn version() -> String {
    let version = unsafe { crate::ffi::smolGetVersion() };

    format!("{version}")
}

/// Turn libsmoldyn's own error/warning printing to stderr on or off (on by default).
/// Errors are still returned as [`SmolError`] either way.
pub fn set_debug_mode(on: bool) {
    unsafe { ffi::smolSetDebugMode(on as i32) };
}
