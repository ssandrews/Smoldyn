use std::ffi::c_char;

use super::{Sim, cstring, invalid, mut_cstring, ok, opt_cstring, opt_ptr};
use crate::error::SmolResult;
use crate::ffi;

fn opt4(v: &mut Option<[f64; 4]>) -> *mut f64 {
    v.as_mut().map_or(std::ptr::null_mut(), |v| v.as_mut_ptr())
}

fn opt_count(n: Option<u32>) -> i32 {
    n.map_or(-1, |n| n.min(i32::MAX as u32) as i32)
}

impl Sim {
    pub fn set_graphics_params(
        &mut self,
        method: &str,
        timesteps: Option<u32>,
        delay: Option<u32>,
    ) -> SmolResult<()> {
        let method = cstring(method)?;
        ok(unsafe {
            ffi::smolSetGraphicsParams(
                self.p(),
                method.as_ptr(),
                opt_count(timesteps).max(0),
                opt_count(delay),
            )
        })
    }

    pub fn set_tiff_params(
        &mut self,
        timesteps: Option<u32>,
        name: Option<&str>,
        low_count: Option<u32>,
        high_count: Option<u32>,
    ) -> SmolResult<()> {
        // libsmoldyn strcpy's the model's file path into a STRCHAR (256) buffer
        // before appending the name.
        if name.is_some() && ffi::smolrs_filepath_len(self.raw()) >= 256 {
            return Err(invalid("model file path too long for a TIFF name"));
        }
        let name = opt_cstring(name)?;
        ok(unsafe {
            ffi::smolSetTiffParams(
                self.p(),
                opt_count(timesteps).max(0),
                opt_ptr(&name),
                opt_count(low_count),
                opt_count(high_count),
            )
        })
    }

    pub fn set_light_params(
        &mut self,
        index: i32,
        mut ambient: Option<[f64; 4]>,
        mut diffuse: Option<[f64; 4]>,
        mut specular: Option<[f64; 4]>,
        mut position: Option<[f64; 4]>,
    ) -> SmolResult<()> {
        ok(unsafe {
            ffi::smolSetLightParams(
                self.p(),
                index,
                opt4(&mut ambient),
                opt4(&mut diffuse),
                opt4(&mut specular),
                opt4(&mut position),
            )
        })
    }

    pub fn set_background_style(&mut self, color: [f64; 4]) -> SmolResult<()> {
        let mut color = color;
        ok(unsafe { ffi::smolSetBackgroundStyle(self.p(), color.as_mut_ptr()) })
    }

    pub fn set_frame_style(
        &mut self,
        thickness: Option<f64>,
        mut color: Option<[f64; 4]>,
    ) -> SmolResult<()> {
        ok(
            unsafe {
                ffi::smolSetFrameStyle(self.p(), thickness.unwrap_or(-1.0), opt4(&mut color))
            },
        )
    }

    pub fn set_grid_style(
        &mut self,
        thickness: Option<f64>,
        mut color: Option<[f64; 4]>,
    ) -> SmolResult<()> {
        ok(unsafe { ffi::smolSetGridStyle(self.p(), thickness.unwrap_or(-1.0), opt4(&mut color)) })
    }

    pub fn set_text_style(&mut self, color: [f64; 4]) -> SmolResult<()> {
        let mut color = color;
        ok(unsafe { ffi::smolSetTextStyle(self.p(), color.as_mut_ptr()) })
    }

    pub fn add_text_display(&mut self, item: &str) -> SmolResult<()> {
        let mut item = mut_cstring(item)?;
        ok(unsafe { ffi::smolAddTextDisplay(self.p(), item.as_mut_ptr() as *mut c_char) })
    }
}
