//! Runtime commands and output files/data.

use std::ffi::c_char;

use super::{Sim, cstring, mut_cstring, ok};
use crate::error::{SmolError, SmolResult};
use crate::ffi;

/// A data table collected by runtime commands (see [`Sim::add_output_data`]),
/// stored row-major.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct OutputData {
    pub nrow: usize,
    pub ncol: usize,
    pub data: Vec<f64>,
}

impl OutputData {
    /// Row `i`, or `None` if out of range.
    pub fn row(&self, i: usize) -> Option<&[f64]> {
        (i < self.nrow).then(|| &self.data[i * self.ncol..(i + 1) * self.ncol])
    }

    /// Iterate over rows.
    pub fn rows(&self) -> impl Iterator<Item = &[f64]> {
        self.data.chunks(self.ncol.max(1)).take(self.nrow)
    }
}

impl Sim {
    /// Directory that output file names are relative to (`smolSetOutputPath`).
    pub fn set_output_path(&mut self, path: &str) -> SmolResult<()> {
        let path = cstring(path)?;
        ok(unsafe { ffi::smolSetOutputPath(self.p(), path.as_ptr()) })
    }

    /// Declare an output file that commands can write to (`smolAddOutputFile`).
    /// `suffix` appends a number to the file name; `append` appends to an
    /// existing file instead of overwriting it.
    pub fn add_output_file(
        &mut self,
        filename: &str,
        suffix: Option<u32>,
        append: bool,
    ) -> SmolResult<()> {
        let mut filename = mut_cstring(filename)?;
        let suffix = suffix.map_or(-1, |s| s.min(i32::MAX as u32) as i32);
        ok(unsafe {
            ffi::smolAddOutputFile(
                self.p(),
                filename.as_mut_ptr() as *mut c_char,
                suffix,
                append as i32,
            )
        })
    }

    /// Declare an in-memory data table that commands can write to, read it
    /// back with [`Sim::output_data`] (`smolAddOutputData`).
    pub fn add_output_data(&mut self, dataname: &str) -> SmolResult<()> {
        let mut dataname = mut_cstring(dataname)?;
        ok(unsafe { ffi::smolAddOutputData(self.p(), dataname.as_mut_ptr() as *mut c_char) })
    }

    /// Open the declared output files (`smolOpenOutputFiles`). [`Sim::run`] and
    /// [`Sim::run_with`] do this themselves.
    pub fn open_output_files(&mut self, overwrite: bool) -> SmolResult<()> {
        ok(unsafe { ffi::smolOpenOutputFiles(self.p(), overwrite as i32) })?;
        self.outputs_open = true;
        Ok(())
    }

    /// Add a runtime command using config-file syntax, e.g. `"i 0 10 0.1 molcount out.txt"`
    /// (`smolAddCommandFromString`).
    pub fn add_command(&mut self, command: &str) -> SmolResult<()> {
        let mut command = mut_cstring(command)?;
        ok(unsafe { ffi::smolAddCommandFromString(self.p(), command.as_mut_ptr() as *mut c_char) })
    }

    /// Add a runtime command with explicit timing (`smolAddCommand`).
    ///
    /// `kind` is the config-file timing code: `'b'` before, `'a'` after, `'@'` at
    /// `on`, `'i'` every `step` from `on` to `off`, `'x'` like `'i'` with the step
    /// multiplied by `multiplier`, `'e'` every time step, `'n'` every `step`
    /// time steps, and so on.
    pub fn add_timed_command(
        &mut self,
        kind: char,
        on: f64,
        off: f64,
        step: f64,
        multiplier: f64,
        command: &str,
    ) -> SmolResult<()> {
        if !kind.is_ascii() {
            return Err(SmolError::InvalidArgument(format!("invalid command type {kind:?}")));
        }
        let command = cstring(command)?;
        ok(unsafe {
            ffi::smolAddCommand(
                self.p(),
                kind as u8 as c_char,
                on,
                off,
                step,
                multiplier,
                command.as_ptr(),
            )
        })
    }

    /// Run a command immediately (`smolRunCommand`).
    pub fn run_command(&mut self, command: &str) -> SmolResult<()> {
        let command = cstring(command)?;
        ok(unsafe { ffi::smolRunCommand(self.p(), command.as_ptr()) })
    }

    /// Copy of the data table `dataname`; `erase` clears it afterwards
    /// (`smolGetOutputData`).
    pub fn output_data(&mut self, dataname: &str, erase: bool) -> SmolResult<OutputData> {
        let mut dataname = mut_cstring(dataname)?;
        let (mut nrow, mut ncol) = (0i32, 0i32);
        let mut array: *mut f64 = std::ptr::null_mut();
        ok(unsafe {
            ffi::smolGetOutputData(
                self.p(),
                dataname.as_mut_ptr() as *mut c_char,
                &mut nrow,
                &mut ncol,
                &mut array,
                erase as i32,
            )
        })?;
        if array.is_null() {
            return Ok(OutputData::default());
        }
        let (nrow, ncol) = (nrow.max(0) as usize, ncol.max(0) as usize);
        // SAFETY: libsmoldyn calloc'd nrow*ncol doubles for us to free.
        let data = unsafe { std::slice::from_raw_parts(array, nrow * ncol) }.to_vec();
        unsafe { ffi::smolrs_free_doubles(array) };
        Ok(OutputData { nrow, ncol, data })
    }
}
