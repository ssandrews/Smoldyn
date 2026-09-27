//! Ports, for exchanging molecules with other simulators.

use super::{Sim, count, cstring, index, invalid, name, ok, to_i32, vector};
use crate::error::SmolResult;
use crate::ffi::{self, MolecState, PanelFace};

impl Sim {
    /// Add a port on `face` of `surface` (`smolAddPort`).
    pub fn add_port(&mut self, port: &str, surface: &str, face: PanelFace) -> SmolResult<()> {
        let port = cstring(port)?;
        let surface = cstring(surface)?;
        ok(unsafe { ffi::smolAddPort(self.p(), port.as_ptr(), surface.as_ptr(), face) })
    }

    /// Index of a port (`smolGetPortIndex`).
    pub fn port_index(&self, port: &str) -> SmolResult<usize> {
        let port = cstring(port)?;
        index(unsafe { ffi::smolGetPortIndex(self.p(), port.as_ptr()) })
    }

    /// Name of the port at `index` (`smolGetPortName`).
    pub fn port_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "port index")?;
        name(|buf| unsafe { ffi::smolGetPortName(self.p(), index, buf) })
    }

    /// Put `number` molecules of `species` into the simulation through `port`
    /// (`smolAddPortMolecules`). `positions` has one `dim`-value position per
    /// molecule; `None` places them randomly on the port surface.
    pub fn add_port_molecules(
        &mut self,
        port: &str,
        species: &str,
        number: usize,
        positions: Option<&[&[f64]]>,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let port = cstring(port)?;
        let species = cstring(species)?;
        let nmolec = to_i32(number, "number")?;
        let mut positions = match positions {
            Some(p) if p.len() != number => {
                return Err(invalid(format!("{number} molecules but {} positions", p.len())));
            }
            Some(p) => Some(
                p.iter()
                    .map(|p| vector(p, dim, "position"))
                    .collect::<SmolResult<Vec<_>>>()?,
            ),
            None => None,
        };
        let mut rows: Option<Vec<*mut f64>> = positions
            .as_mut()
            .map(|p| p.iter_mut().map(|row| row.as_mut_ptr()).collect());
        let rows_ptr = rows.as_mut().map_or(std::ptr::null_mut(), |r| r.as_mut_ptr());
        ok(unsafe {
            ffi::smolAddPortMolecules(self.p(), port.as_ptr(), nmolec, species.as_ptr(), rows_ptr)
        })
    }

    /// Number of molecules of `species` in `state` waiting at `port`; `remove`
    /// takes them out of the simulation (`smolGetPortMolecules`).
    pub fn port_molecules(
        &mut self,
        port: &str,
        species: &str,
        state: MolecState,
        remove: bool,
    ) -> SmolResult<usize> {
        let port = cstring(port)?;
        let species = cstring(species)?;
        count(unsafe {
            ffi::smolGetPortMolecules(self.p(), port.as_ptr(), species.as_ptr(), state, remove as i32)
        })
    }
}
