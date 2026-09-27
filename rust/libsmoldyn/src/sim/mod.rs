//! Safe owning wrapper around `simptr`.
//!
//! The core (construction, running, time) is here; the rest of the libsmoldyn API
//! is split by topic into submodules, each adding methods to [`Sim`].

mod compartments;
mod graphics;
mod lattices;
mod molecules;
mod output;
mod ports;
mod reactions;
mod surfaces;

pub use output::OutputData;
pub use reactions::RateKind;
pub use surfaces::{Side, SurfaceStyle};

use std::ffi::{CStr, CString, c_char};
use std::marker::PhantomData;
use std::path::Path;
use std::ptr::NonNull;
use std::sync::atomic::{AtomicBool, Ordering};

use crate::error::{STRCHARLONG, SmolError, SmolResult, Status, check};
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
    // set once `run_with` has opened output files, so resuming doesn't truncate them.
    outputs_open: bool,
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
        let (fileroot, filename) = split_path(path.as_ref())?;
        let flags = cstring(flags)?;
        let ptr = unsafe {
            ffi::smolPrepareSimFromFile(fileroot.as_ptr(), filename.as_ptr(), flags.as_ptr())
        };
        Self::from_raw(ptr)
    }

    /// Load a simulation from a configuration file without updating it
    /// (`smolLoadSimFromFile`), so it can be changed further before
    /// [`Sim::update`].
    pub fn load_from_file(path: impl AsRef<Path>, flags: &str) -> SmolResult<Self> {
        let (fileroot, filename) = split_path(path.as_ref())?;
        let flags = cstring(flags)?;
        let mut ptr = std::ptr::null_mut();
        let code = unsafe {
            ffi::smolLoadSimFromFile(fileroot.as_ptr(), filename.as_ptr(), &mut ptr, flags.as_ptr())
        };
        // take ownership first, so the sim is freed even if loading failed half-way.
        let sim = Self::from_raw(ptr);
        check(code)?;
        sim
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
                outputs_open: false,
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

    /// Raw pointer for the wrappers in this module tree.
    fn p(&self) -> *mut ffi::simstruct {
        self.ptr.as_ptr()
    }

    fn raw(&self) -> &ffi::simstruct {
        // SAFETY: `ptr` is valid for the lifetime of `self`, and `&self`
        // guarantees no `&mut self` method is changing it.
        unsafe { self.ptr.as_ref() }
    }

    /// Dimensionality of space (1, 2 or 3).
    pub fn dim(&self) -> usize {
        ffi::smolrs_dim(self.raw()) as usize
    }

    /// Current simulation time.
    pub fn time(&self) -> f64 {
        ffi::smolrs_time(self.raw())
    }

    /// Simulation start time.
    pub fn time_start(&self) -> f64 {
        ffi::smolrs_time_start(self.raw())
    }

    /// Simulation stop time.
    pub fn time_stop(&self) -> f64 {
        ffi::smolrs_time_stop(self.raw())
    }

    /// Simulation time step.
    pub fn time_step(&self) -> f64 {
        ffi::smolrs_time_step(self.raw())
    }

    /// Low and high corners of the simulation volume, one value per dimension.
    pub fn bounds(&self) -> (Vec<f64>, Vec<f64>) {
        let sim = self.raw();
        (0..self.dim() as i32)
            .map(|d| (ffi::smolrs_wall_pos(sim, d, 0), ffi::smolrs_wall_pos(sim, d, 1)))
            .unzip()
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

    /// Run the simulation to its stop time.
    ///
    /// This calls `smolRunSimUntil(time_stop)` rather than `smolRunSim`:
    /// `smolRunTimeStep` leaves the break time at `time + dt/2`, so a plain
    /// `smolRunSim` after [`Sim::step`] would stop after one more step.
    /// `smolRunSimUntil` resets the break time afterwards.
    pub fn run(&mut self) -> SmolResult<()> {
        self.run_until(self.time_stop())
    }

    /// Step the simulation to its stop time, calling `on_step` after every
    /// step, until it finishes or `stop` is set.
    ///
    /// Returns [`Progress::Finished`] if the simulation completed, or
    /// [`Progress::Running`] if `stop` interrupted it; calling `run_with` again
    /// continues from where it stopped. Output files are opened (and truncated),
    /// as `smolRunSim` does, on the first call only.
    ///
    /// `on_step` gets `&Sim`, so it can read state (time, molecule counts) but
    /// not change the simulation. To watch from another thread, send what you
    /// need from `on_step` over a channel instead of sharing the `Sim`.
    pub fn run_with(
        &mut self,
        stop: &AtomicBool,
        mut on_step: impl FnMut(&Sim),
    ) -> SmolResult<Progress> {
        if !self.outputs_open {
            check(unsafe { ffi::smolOpenOutputFiles(self.ptr.as_ptr(), 1) })?;
            self.outputs_open = true;
        }
        while !stop.load(Ordering::Relaxed) {
            let progress = self.step()?;
            on_step(self);
            if progress == Progress::Finished {
                return Ok(Progress::Finished);
            }
        }
        Ok(Progress::Running)
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

    /// Set the start time (`smolSetTimeStart`).
    pub fn set_time_start(&mut self, start: f64) -> SmolResult<()> {
        ok(unsafe { ffi::smolSetTimeStart(self.p(), start) })
    }

    /// Set the stop time (`smolSetTimeStop`).
    pub fn set_time_stop(&mut self, stop: f64) -> SmolResult<()> {
        ok(unsafe { ffi::smolSetTimeStop(self.p(), stop) })
    }

    /// Set the current simulation time (`smolSetTimeNow`).
    pub fn set_time_now(&mut self, now: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolSetTimeNow(self.ptr.as_ptr(), now) }).map(drop)
    }

    /// Set the time step (`smolSetTimeStep`).
    pub fn set_time_step(&mut self, step: f64) -> SmolResult<()> {
        check(unsafe { ffi::smolSetTimeStep(self.ptr.as_ptr(), step) }).map(drop)
    }

    /// Replace the simulation flags, e.g. `"q"` for quiet (`smolSetSimFlags`).
    pub fn set_flags(&mut self, flags: &str) -> SmolResult<()> {
        // smolSetSimFlags strncpy's into a STRCHAR (256) buffer without terminating it.
        if flags.len() >= 256 {
            return Err(SmolError::InvalidArgument("flags must be < 256 bytes".into()));
        }
        let flags = cstring(flags)?;
        ok(unsafe { ffi::smolSetSimFlags(self.p(), flags.as_ptr()) })
    }

    /// Seed the random number generator (`smolSetRandomSeed`).
    pub fn set_random_seed(&mut self, seed: i64) -> SmolResult<()> {
        ok(unsafe { ffi::smolSetRandomSeed(self.p(), seed) })
    }

    /// Set the virtual box partitioning, `method` is `"molperbox"` or
    /// `"boxsize"` (`smolSetPartitions`).
    pub fn set_partitions(&mut self, method: &str, value: f64) -> SmolResult<()> {
        let method = cstring(method)?;
        ok(unsafe { ffi::smolSetPartitions(self.p(), method.as_ptr(), value) })
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

/// Split a model path into the `filepath` (with trailing separator) and
/// `filename` arguments libsmoldyn expects.
fn split_path(path: &Path) -> SmolResult<(CString, CString)> {
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
    Ok((cstring(fileroot)?, cstring(path_to_bytes(filename))?))
}

fn invalid(msg: impl Into<String>) -> SmolError {
    SmolError::InvalidArgument(msg.into())
}

fn cstring(s: impl Into<Vec<u8>>) -> SmolResult<CString> {
    CString::new(s).map_err(|e| SmolError::InvalidArgument(e.to_string()))
}

fn opt_cstring(s: Option<&str>) -> SmolResult<Option<CString>> {
    s.map(cstring).transpose()
}

/// Pointer to an optional C string, NULL for `None`.
fn opt_ptr(s: &Option<CString>) -> *const c_char {
    s.as_ref().map_or(std::ptr::null(), |s| s.as_ptr())
}

/// NUL-terminated private copy, for parameters libsmoldyn takes as `char *`.
fn mut_cstring(s: &str) -> SmolResult<Vec<u8>> {
    Ok(cstring(s)?.into_bytes_with_nul())
}

/// `check` for calls whose status (ok/notify/warning) doesn't matter.
fn ok(code: ErrorCode) -> SmolResult<()> {
    check(code).map(drop)
}

/// Lookup functions return an index, or a negative `ErrorCode` on failure.
fn index(ret: i32) -> SmolResult<usize> {
    if ret < 0 {
        Err(SmolError::last(ErrorCode { repr: ret }))
    } else {
        Ok(ret as usize)
    }
}

/// Count functions return a count, or a negative `ErrorCode` on failure.
fn count(ret: i32) -> SmolResult<usize> {
    index(ret)
}

fn to_i32(n: usize, what: &str) -> SmolResult<i32> {
    i32::try_from(n).map_err(|_| invalid(format!("{what} is too large: {n}")))
}

/// Call a libsmoldyn `char *get...Name(..., char *buf)` function, which
/// `strcpy`s into `buf` and returns NULL on failure.
fn name(get: impl FnOnce(*mut c_char) -> *mut c_char) -> SmolResult<String> {
    let mut buf = vec![0 as c_char; STRCHARLONG];
    let ret = get(buf.as_mut_ptr());
    if ret.is_null() {
        return Err(SmolError::last(ErrorCode::ECnonexist));
    }
    Ok(unsafe { CStr::from_ptr(buf.as_ptr()) }.to_string_lossy().into_owned())
}

/// Copy of `v` after checking it has `n` values, for `double *` parameters.
fn vector(v: &[f64], n: usize, what: &str) -> SmolResult<Vec<f64>> {
    if v.len() != n {
        return Err(invalid(format!("{what} needs {n} values, got {}", v.len())));
    }
    Ok(v.to_vec())
}

fn opt_vector(v: Option<&[f64]>, n: usize, what: &str) -> SmolResult<Option<Vec<f64>>> {
    v.map(|v| vector(v, n, what)).transpose()
}

/// Pointer to an optional array, NULL for `None`.
fn opt_mut_ptr(v: &mut Option<Vec<f64>>) -> *mut f64 {
    v.as_mut().map_or(std::ptr::null_mut(), |v| v.as_mut_ptr())
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
