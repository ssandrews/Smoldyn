//! Safe owning wrapper around `simptr`.

use std::ffi::{CString, c_char};
use std::marker::PhantomData;
use std::path::Path;
use std::ptr::NonNull;

use crate::error::{SmolError, SmolResult, Status, check};
use crate::ffi::{self, ErrorCode};

/// Result of advancing the simulation by one time step.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Progress {
    /// More time steps remain.
    Running,
    /// The stop time was reached or a runtime command stopped the simulation.
    Finished,
}

/// An owned Smoldyn simulation (`simptr`). The simulation is freed on drop.
///
/// `Sim` is `Send` but not `Sync`: libsmoldyn functions mutate the simulation
/// without locking, so it must be used from one thread at a time. Note that
/// libsmoldyn's error state is process-global, so running several `Sim`s on
/// different threads at the same time can interleave their error messages.
///
/// `Sim` is deliberately not `Clone`: two handles would free the same `simptr`.
///
/// ```compile_fail
/// fn is_clone<T: Clone>() {}
/// is_clone::<libsmoldyn::Sim>();
/// ```
///
/// ```compile_fail
/// fn is_sync<T: Sync>() {}
/// is_sync::<libsmoldyn::Sim>();
/// ```
pub struct Sim {
    ptr: NonNull<ffi::simstruct>,
    // raw pointer marker makes `Sim` !Sync (and !Send, re-enabled below).
    _not_sync: PhantomData<*mut ()>,
}

// SAFETY: `Sim` exclusively owns its `simstruct`, and nothing else keeps a
// pointer to it, so moving it to another thread is fine.
unsafe impl Send for Sim {}

impl Sim {
    /// Load, update and display a simulation from a configuration file
    /// (`smolPrepareSimFromFile`).
    pub fn from_file(path: impl AsRef<Path>, flags: &str) -> SmolResult<Self> {
        let path = path.as_ref();
        let filename = path
            .file_name()
            .ok_or_else(|| SmolError::InvalidArgument(format!("no file name in {path:?}")))?;
        // libsmoldyn concatenates filepath and filename, so filepath must end
        // with a separator (or be empty for the current directory).
        let mut fileroot = match path.parent() {
            Some(parent) if !parent.as_os_str().is_empty() => path_to_bytes(parent),
            _ => Vec::new(),
        };
        if !fileroot.is_empty() {
            fileroot.push(std::path::MAIN_SEPARATOR as u8);
        }
        let fileroot = cstring(fileroot)?;
        let filename = cstring(path_to_bytes(filename))?;
        let flags = cstring(flags)?;

        let ptr = unsafe {
            ffi::smolPrepareSimFromFile(fileroot.as_ptr(), filename.as_ptr(), flags.as_ptr())
        };
        Self::from_raw(ptr)
    }

    /// Create an empty simulation with transparent walls at `low` and `high`
    /// (`smolNewSim`). The dimensionality is `low.len()` and must be 1, 2 or 3.
    pub fn new(low: &[f64], high: &[f64]) -> SmolResult<Self> {
        if low.len() != high.len() || !(1..=3).contains(&low.len()) {
            return Err(SmolError::InvalidArgument(format!(
                "low and high must have the same length (1 to 3), got {} and {}",
                low.len(),
                high.len()
            )));
        }
        // smolNewSim only reads the bounds, but takes them as `double *`.
        let mut low = low.to_vec();
        let mut high = high.to_vec();
        let ptr = unsafe { ffi::smolNewSim(low.len() as i32, low.as_mut_ptr(), high.as_mut_ptr()) };
        Self::from_raw(ptr)
    }

    fn from_raw(ptr: *mut ffi::simstruct) -> SmolResult<Self> {
        NonNull::new(ptr)
            .map(|ptr| Sim {
                ptr,
                _not_sync: PhantomData,
            })
            .ok_or_else(|| SmolError::last(ErrorCode::ECerror))
    }

    /// Raw pointer for calling libsmoldyn functions that are not wrapped yet.
    ///
    /// # Safety
    /// The pointer must not be freed, and must not be used after `self` is
    /// dropped or while another `&mut self` method is running.
    pub unsafe fn as_ptr(&self) -> *mut ffi::simstruct {
        self.ptr.as_ptr()
    }

    /// Recompute internal data structures after changing the model (`smolUpdateSim`).
    pub fn update(&mut self) -> SmolResult<()> {
        check(unsafe { ffi::smolUpdateSim(self.ptr.as_ptr()) }).map(drop)
    }

    /// Run a single time step (`smolRunTimeStep`).
    pub fn step(&mut self) -> SmolResult<Progress> {
        match check(unsafe { ffi::smolRunTimeStep(self.ptr.as_ptr()) })? {
            Status::Notify => Ok(Progress::Finished),
            Status::Ok | Status::Warning => Ok(Progress::Running),
        }
    }

    /// Run the simulation to its stop time (`smolRunSim`).
    pub fn run(&mut self) -> SmolResult<()> {
        check(unsafe { ffi::smolRunSim(self.ptr.as_ptr()) }).map(drop)
    }

    /// Run the simulation until `breaktime`, leaving the stop time unchanged
    /// (`smolRunSimUntil`).
    pub fn run_until(&mut self, breaktime: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolRunSimUntil(self.ptr.as_ptr(), breaktime) }).map(drop)
    }

    /// Print the simulation parameters to stdout (`smolDisplaySim`).
    pub fn display(&self) -> SmolResult<()> {
        check(unsafe { ffi::smolDisplaySim(self.ptr.as_ptr()) }).map(drop)
    }

    /// Set start, stop and step time (`smolSetSimTimes`).
    pub fn set_times(&mut self, start: f64, stop: f64, step: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolSetSimTimes(self.ptr.as_ptr(), start, stop, step) }).map(drop)
    }

    /// Set the current simulation time (`smolSetTimeNow`).
    pub fn set_time_now(&mut self, now: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolSetTimeNow(self.ptr.as_ptr(), now) }).map(drop)
    }

    /// Set the time step (`smolSetTimeStep`).
    pub fn set_time_step(&mut self, step: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolSetTimeStep(self.ptr.as_ptr(), step) }).map(drop)
    }

    /// Apply one configuration-file statement, e.g. `("difc", "A 1")`
    /// (`smolReadConfigString`).
    pub fn read_config(&mut self, statement: &str, parameters: &str) -> SmolResult<()> {
        let statement = cstring(statement)?;
        // libsmoldyn may modify `parameters` in place, so hand it a private copy.
        let mut parameters = cstring(parameters)?.into_bytes_with_nul();
        check(unsafe {
            ffi::smolReadConfigString(
                self.ptr.as_ptr(),
                statement.as_ptr(),
                parameters.as_mut_ptr() as *mut c_char,
            )
        })
        .map(drop)
    }

    /// Add a runtime command using config-file syntax, e.g. `"i 0 10 0.1 molcount out.txt"`
    /// (`smolAddCommandFromString`).
    pub fn add_command(&mut self, command: &str) -> SmolResult<()> {
        let mut command = cstring(command)?.into_bytes_with_nul();
        check(unsafe {
            ffi::smolAddCommandFromString(self.ptr.as_ptr(), command.as_mut_ptr() as *mut c_char)
        })
        .map(drop)
    }

    /// Run a command immediately (`smolRunCommand`).
    pub fn run_command(&mut self, command: &str) -> SmolResult<()> {
        let command = cstring(command)?;
        check(unsafe { ffi::smolRunCommand(self.ptr.as_ptr(), command.as_ptr()) }).map(drop)
    }

    /// Add a species; `mollist` is the molecule list to put it in, or `None`
    /// for the default (`smolAddSpecies`).
    pub fn add_species(&mut self, species: &str, mollist: Option<&str>) -> SmolResult<()> {
        let species = cstring(species)?;
        let mollist = mollist.map(cstring).transpose()?;
        let mollist_ptr = mollist.as_ref().map_or(std::ptr::null(), |m| m.as_ptr());
        check(unsafe { ffi::smolAddSpecies(self.ptr.as_ptr(), species.as_ptr(), mollist_ptr) })
            .map(drop)
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe { ffi::smolFreeSim(self.ptr.as_ptr()) };
    }
}

impl std::fmt::Debug for Sim {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Sim").field("ptr", &self.ptr).finish()
    }
}

fn cstring(s: impl Into<Vec<u8>>) -> SmolResult<CString> {
    CString::new(s).map_err(|e| SmolError::InvalidArgument(e.to_string()))
}

// Thanks <https://stackoverflow.com/a/57667836/1805129>
#[cfg(unix)]
fn path_to_bytes<P: AsRef<Path>>(path: P) -> Vec<u8> {
    use std::os::unix::ffi::OsStrExt;
    path.as_ref().as_os_str().as_bytes().to_vec()
}

#[cfg(not(unix))]
fn path_to_bytes<P: AsRef<Path>>(path: P) -> Vec<u8> {
    path.as_ref().to_string_lossy().to_string().into_bytes()
}
