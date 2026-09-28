use std::ffi::c_char;

use super::{Sim, cstring, mut_cstring, ok};
use crate::error::{SmolError, SmolResult};
use crate::ffi;

#[derive(Debug, Clone, Default, PartialEq)]
pub struct OutputData {
    pub nrow: usize,
    pub ncol: usize,
    pub data: Vec<f64>,
}

impl OutputData {
    pub fn row(&self, i: usize) -> Option<&[f64]> {
        (i < self.nrow).then(|| &self.data[i * self.ncol..(i + 1) * self.ncol])
    }

    pub fn rows(&self) -> impl Iterator<Item = &[f64]> {
        self.data.chunks(self.ncol.max(1)).take(self.nrow)
    }
}

impl Sim {
    pub fn set_output_path(&mut self, path: &str) -> SmolResult<()> {
        let path = cstring(path)?;
        ok(unsafe { ffi::smolSetOutputPath(self.p(), path.as_ptr()) })
    }

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

    pub fn add_output_data(&mut self, dataname: &str) -> SmolResult<()> {
        let mut dataname = mut_cstring(dataname)?;
        ok(unsafe { ffi::smolAddOutputData(self.p(), dataname.as_mut_ptr() as *mut c_char) })
    }

    pub fn open_output_files(&mut self, overwrite: bool) -> SmolResult<()> {
        ok(unsafe { ffi::smolOpenOutputFiles(self.p(), overwrite as i32) })?;
        self.outputs_open = true;
        Ok(())
    }

    pub fn add_command(&mut self, command: &str) -> SmolResult<()> {
        let mut command = mut_cstring(command)?;
        ok(unsafe { ffi::smolAddCommandFromString(self.p(), command.as_mut_ptr() as *mut c_char) })
    }

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
            return Err(SmolError::InvalidArgument(format!(
                "invalid command type {kind:?}"
            )));
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

    pub fn run_command(&mut self, command: &str) -> SmolResult<()> {
        let command = cstring(command)?;
        ok(unsafe { ffi::smolRunCommand(self.p(), command.as_ptr()) })
    }

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
