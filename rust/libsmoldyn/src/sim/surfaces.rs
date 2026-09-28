use super::{Sim, cstring, index, invalid, name, ok, opt_cstring, opt_ptr, to_i32, vector};
use crate::error::SmolResult;
use crate::ffi::{self, DrawMode, MolecState, PanelFace, PanelShape, SrfAction};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Side {
    Low,
    High,
    Both,
}

#[derive(Debug, Clone, Copy)]
pub struct SurfaceStyle {
    pub mode: DrawMode,
    pub thickness: Option<f64>,
    pub color: Option<[f64; 4]>,
    pub stipple_factor: Option<u32>,
    pub stipple_pattern: Option<u16>,
    pub shininess: Option<f64>,
}

impl Default for SurfaceStyle {
    fn default() -> Self {
        SurfaceStyle {
            mode: DrawMode::None,
            thickness: None,
            color: None,
            stipple_factor: None,
            stipple_pattern: None,
            shininess: None,
        }
    }
}

// Enough for the parameters of any panel shape (at most 9, for 3D triangles,
// cylinders and hemispheres). smolAddPanel reads a shape-dependent number of
// values, so we always pass a buffer this large and missing values read as 0
// instead of out of bounds.
const MAX_PANEL_PARAMS: usize = 16;

// MAXPRODUCT in smoldyn.h: RxnSetIntersurfaceRules reads one rule per product.
const MAX_PRODUCT: usize = 256;

impl Sim {
    pub fn set_boundary_type(&mut self, dim: usize, side: Side, kind: char) -> SmolResult<()> {
        if dim >= self.dim() {
            return Err(invalid(format!("dimension {dim} out of range")));
        }
        if !matches!(kind, 'r' | 'p' | 'a' | 't') {
            return Err(invalid(format!("invalid boundary type {kind:?}")));
        }
        let highside = match side {
            Side::Low => 0,
            Side::High => 1,
            Side::Both => -1,
        };
        ok(unsafe { ffi::smolSetBoundaryType(self.p(), dim as i32, highside, kind as u8 as _) })
    }

    pub fn add_surface(&mut self, surface: &str) -> SmolResult<()> {
        let surface = cstring(surface)?;
        ok(unsafe { ffi::smolAddSurface(self.p(), surface.as_ptr()) })
    }

    pub fn surface_index(&self, surface: &str) -> SmolResult<usize> {
        let surface = cstring(surface)?;
        index(unsafe { ffi::smolGetSurfaceIndex(self.p(), surface.as_ptr()) })
    }

    pub fn surface_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "surface index")?;
        name(|buf| unsafe { ffi::smolGetSurfaceName(self.p(), index, buf) })
    }

    pub fn set_reaction_intersurface(&mut self, reaction: &str, rules: &[i32]) -> SmolResult<()> {
        if rules.len() > MAX_PRODUCT {
            return Err(invalid(format!("at most {MAX_PRODUCT} rules")));
        }
        let reaction = cstring(reaction)?;
        // libsmoldyn reads as many rules as the reaction has products.
        let mut buf = [0i32; MAX_PRODUCT];
        if rules.is_empty() {
            buf[0] = -1;
        } else {
            buf[..rules.len()].copy_from_slice(rules);
        }
        ok(unsafe {
            ffi::smolSetReactionIntersurface(self.p(), reaction.as_ptr(), buf.as_mut_ptr())
        })
    }

    pub fn set_surface_action(
        &mut self,
        surface: &str,
        face: PanelFace,
        species: &str,
        state: MolecState,
        action: SrfAction,
        new_species: Option<&str>,
    ) -> SmolResult<()> {
        let surface = cstring(surface)?;
        let species = cstring(species)?;
        let new_species = opt_cstring(new_species)?;
        ok(unsafe {
            ffi::smolSetSurfaceAction(
                self.p(),
                surface.as_ptr(),
                face,
                species.as_ptr(),
                state,
                action,
                opt_ptr(&new_species),
            )
        })
    }

    #[allow(clippy::too_many_arguments)]
    pub fn set_surface_rate(
        &mut self,
        surface: &str,
        species: &str,
        state: MolecState,
        state1: MolecState,
        state2: MolecState,
        rate: f64,
        new_species: Option<&str>,
        internal: bool,
    ) -> SmolResult<()> {
        let surface = cstring(surface)?;
        let species = cstring(species)?;
        let new_species = opt_cstring(new_species)?;
        ok(unsafe {
            ffi::smolSetSurfaceRate(
                self.p(),
                surface.as_ptr(),
                species.as_ptr(),
                state,
                state1,
                state2,
                rate,
                opt_ptr(&new_species),
                internal as i32,
            )
        })
    }

    pub fn add_panel(
        &mut self,
        surface: &str,
        shape: PanelShape,
        panel: Option<&str>,
        axis: Option<&str>,
        params: &[f64],
    ) -> SmolResult<()> {
        if params.len() > MAX_PANEL_PARAMS {
            return Err(invalid(format!(
                "at most {MAX_PANEL_PARAMS} panel parameters"
            )));
        }
        let surface = cstring(surface)?;
        let panel = opt_cstring(panel)?;
        let axis = opt_cstring(axis)?;
        let mut buf = [0.0; MAX_PANEL_PARAMS];
        buf[..params.len()].copy_from_slice(params);
        ok(unsafe {
            ffi::smolAddPanel(
                self.p(),
                surface.as_ptr(),
                shape,
                opt_ptr(&panel),
                opt_ptr(&axis),
                buf.as_mut_ptr(),
            )
        })
    }

    pub fn panel_index(&self, surface: &str, panel: &str) -> SmolResult<(PanelShape, usize)> {
        let surface = cstring(surface)?;
        let panel = cstring(panel)?;
        let mut shape = PanelShape::None;
        let i = index(unsafe {
            ffi::smolGetPanelIndex(self.p(), surface.as_ptr(), &mut shape, panel.as_ptr())
        })?;
        Ok((shape, i))
    }

    pub fn panel_name(&self, surface: &str, shape: PanelShape, index: usize) -> SmolResult<String> {
        let surface = cstring(surface)?;
        let index = to_i32(index, "panel index")?;
        name(|buf| unsafe { ffi::smolGetPanelName(self.p(), surface.as_ptr(), shape, index, buf) })
    }

    pub fn set_panel_jump(
        &mut self,
        surface: &str,
        panel1: &str,
        face1: PanelFace,
        panel2: &str,
        face2: PanelFace,
        bidirectional: bool,
    ) -> SmolResult<()> {
        let surface = cstring(surface)?;
        let panel1 = cstring(panel1)?;
        let panel2 = cstring(panel2)?;
        ok(unsafe {
            ffi::smolSetPanelJump(
                self.p(),
                surface.as_ptr(),
                panel1.as_ptr(),
                face1,
                panel2.as_ptr(),
                face2,
                bidirectional as i32,
            )
        })
    }

    pub fn add_surface_unbounded_emitter(
        &mut self,
        surface: &str,
        face: PanelFace,
        species: &str,
        amount: f64,
        position: &[f64],
    ) -> SmolResult<()> {
        let mut position = vector(position, self.dim(), "position")?;
        let surface = cstring(surface)?;
        let species = cstring(species)?;
        ok(unsafe {
            ffi::smolAddSurfaceUnboundedEmitter(
                self.p(),
                surface.as_ptr(),
                face,
                species.as_ptr(),
                amount,
                position.as_mut_ptr(),
            )
        })
    }

    pub fn set_surface_sim_params(&mut self, parameter: &str, value: f64) -> SmolResult<()> {
        let parameter = cstring(parameter)?;
        ok(unsafe { ffi::smolSetSurfaceSimParams(self.p(), parameter.as_ptr(), value) })
    }

    pub fn add_panel_neighbor(
        &mut self,
        surface1: &str,
        panel1: &str,
        surface2: &str,
        panel2: &str,
        reciprocal: bool,
    ) -> SmolResult<()> {
        let (surface1, panel1) = (cstring(surface1)?, cstring(panel1)?);
        let (surface2, panel2) = (cstring(surface2)?, cstring(panel2)?);
        ok(unsafe {
            ffi::smolAddPanelNeighbor(
                self.p(),
                surface1.as_ptr(),
                panel1.as_ptr(),
                surface2.as_ptr(),
                panel2.as_ptr(),
                reciprocal as i32,
            )
        })
    }

    pub fn set_surface_style(
        &mut self,
        surface: &str,
        face: PanelFace,
        style: &SurfaceStyle,
    ) -> SmolResult<()> {
        // libsmoldyn accepts "all" here but then indexes srflist[-5].
        if surface == "all" {
            return Err(invalid(
                "set_surface_style needs a specific surface, not \"all\"",
            ));
        }
        let surface = cstring(surface)?;
        let mut color = style.color;
        let color_ptr = color
            .as_mut()
            .map_or(std::ptr::null_mut(), |c| c.as_mut_ptr());
        ok(unsafe {
            ffi::smolSetSurfaceStyle(
                self.p(),
                surface.as_ptr(),
                face,
                style.mode,
                style.thickness.unwrap_or(-1.0),
                color_ptr,
                style
                    .stipple_factor
                    .map_or(-1, |f| f.min(i32::MAX as u32) as i32),
                style.stipple_pattern.map_or(-1, i32::from),
                style.shininess.unwrap_or(-1.0),
            )
        })
    }
}
