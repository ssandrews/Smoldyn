//! smoldyn library.
//!
//! `ffi` holds the raw bindings to `libsmoldyn.h`. [`Sim`] is the safe, owning
//! wrapper around `simptr`; all `unsafe` code should stay inside this crate.

mod error;
mod sim;

pub use error::{SmolError, SmolResult};
pub use sim::{Progress, Sim};

#[cxx::bridge]
pub mod ffi {
    /// Mirrors `enum ErrorCode` in `libsmoldyn.h`. cxx statically checks that the
    /// discriminants match the C++ definition.
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

    unsafe extern "C++" {
        include!("Smoldyn/libsmoldyn.h");

        type simstruct;
        type ErrorCode;

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
        unsafe fn smolReadConfigString(
            sim: *mut simstruct,
            statement: *const c_char,
            parameters: *mut c_char,
        ) -> ErrorCode;

        // simulation settings
        unsafe fn smolSetSimTimes(
            sim: *mut simstruct,
            timestart: f64,
            timestop: f64,
            timestep: f64,
        ) -> ErrorCode;
        unsafe fn smolSetTimeNow(sim: *mut simstruct, timenow: f64) -> ErrorCode;
        unsafe fn smolSetTimeStep(sim: *mut simstruct, timestep: f64) -> ErrorCode;

        // runtime commands
        unsafe fn smolAddCommandFromString(sim: *mut simstruct, string: *mut c_char) -> ErrorCode;
        unsafe fn smolRunCommand(sim: *mut simstruct, commandstring: *const c_char) -> ErrorCode;

        // species
        unsafe fn smolAddSpecies(
            sim: *mut simstruct,
            species: *const c_char,
            mollist: *const c_char,
        ) -> ErrorCode;
    }
}

/// Return smoldyn version
pub fn version() -> String {
    let version = unsafe { crate::ffi::smolGetVersion() };

    format!("{version}")
}
