use std::ffi::{CStr, c_char};

use super::{
    Sim, count, cstring, index, name, ok, opt_cstring, opt_mut_ptr, opt_ptr, opt_vector, to_i32,
    vector,
};
use crate::error::{STRCHARLONG, SmolError, SmolResult};
use crate::ffi::{self, ErrorCode, MolecState, PanelShape};

impl Sim {
    pub fn add_species(&mut self, species: &str, mollist: Option<&str>) -> SmolResult<()> {
        let species = cstring(species)?;
        let mollist = opt_cstring(mollist)?;
        ok(unsafe { ffi::smolAddSpecies(self.p(), species.as_ptr(), opt_ptr(&mollist)) })
    }

    pub fn species(&self) -> Vec<String> {
        let sim = self.raw();
        (1..ffi::smolrs_nspecies(sim))
            .filter_map(|i| {
                let name = ffi::smolrs_species_name(sim, i);
                // SAFETY: non-null names are NUL-terminated strings owned by the simulation.
                (!name.is_null()).then(|| {
                    unsafe { CStr::from_ptr(name) }
                        .to_string_lossy()
                        .into_owned()
                })
            })
            .collect()
    }

    pub fn species_index(&self, species: &str) -> SmolResult<usize> {
        let species = cstring(species)?;
        index(unsafe { ffi::smolGetSpeciesIndex(self.p(), species.as_ptr()) })
    }

    pub fn species_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "species index")?;
        let mut buf = vec![0 as c_char; STRCHARLONG];
        unsafe {
            ffi::smolClearError();
            ffi::smolGetSpeciesName(self.p(), index, buf.as_mut_ptr());
        }
        // smolGetSpeciesName returns nothing; an empty buffer means it failed.
        if buf[0] == 0 {
            return Err(SmolError::last(ErrorCode::Nonexist));
        }
        Ok(unsafe { CStr::from_ptr(buf.as_ptr()) }
            .to_string_lossy()
            .into_owned())
    }

    pub fn set_species_mobility(
        &mut self,
        species: &str,
        state: MolecState,
        difc: Option<f64>,
        drift: Option<&[f64]>,
        difmatrix: Option<&[f64]>,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let species = cstring(species)?;
        let mut drift = opt_vector(drift, dim, "drift")?;
        let mut difmatrix = opt_vector(difmatrix, dim * dim, "difmatrix")?;
        ok(unsafe {
            ffi::smolSetSpeciesMobility(
                self.p(),
                species.as_ptr(),
                state,
                difc.unwrap_or(-1.0),
                opt_mut_ptr(&mut drift),
                opt_mut_ptr(&mut difmatrix),
            )
        })
    }

    pub fn set_molecule_color(
        &mut self,
        species: &str,
        state: MolecState,
        color: [f64; 3],
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        let mut color = color;
        ok(unsafe {
            ffi::smolSetMoleculeColor(self.p(), species.as_ptr(), state, color.as_mut_ptr())
        })
    }

    pub fn set_molecule_size(
        &mut self,
        species: &str,
        state: MolecState,
        size: f64,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        ok(unsafe { ffi::smolSetMoleculeSize(self.p(), species.as_ptr(), state, size) })
    }

    pub fn set_molecule_style(
        &mut self,
        species: &str,
        state: MolecState,
        size: Option<f64>,
        color: Option<[f64; 3]>,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        let mut color = color;
        let color_ptr = color
            .as_mut()
            .map_or(std::ptr::null_mut(), |c| c.as_mut_ptr());
        ok(unsafe {
            ffi::smolSetMoleculeStyle(
                self.p(),
                species.as_ptr(),
                state,
                size.unwrap_or(-1.0),
                color_ptr,
            )
        })
    }

    pub fn add_mol_list(&mut self, mollist: &str) -> SmolResult<()> {
        let mollist = cstring(mollist)?;
        ok(unsafe { ffi::smolAddMolList(self.p(), mollist.as_ptr()) })
    }

    pub fn mol_list_index(&self, mollist: &str) -> SmolResult<usize> {
        let mollist = cstring(mollist)?;
        index(unsafe { ffi::smolGetMolListIndex(self.p(), mollist.as_ptr()) })
    }

    pub fn mol_list_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "molecule list index")?;
        name(|buf| unsafe { ffi::smolGetMolListName(self.p(), index, buf) })
    }

    pub fn set_mol_list(
        &mut self,
        species: &str,
        state: MolecState,
        mollist: &str,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        let mollist = cstring(mollist)?;
        ok(unsafe { ffi::smolSetMolList(self.p(), species.as_ptr(), state, mollist.as_ptr()) })
    }

    pub fn set_max_molecules(&mut self, max: usize) -> SmolResult<()> {
        let max = to_i32(max, "max molecules")?;
        ok(unsafe { ffi::smolSetMaxMolecules(self.p(), max) })
    }

    pub fn add_solution_molecules(
        &mut self,
        species: &str,
        number: usize,
        low: Option<&[f64]>,
        high: Option<&[f64]>,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let species = cstring(species)?;
        let number = to_i32(number, "number")?;
        let mut low = opt_vector(low, dim, "low position")?;
        let mut high = opt_vector(high, dim, "high position")?;
        ok(unsafe {
            ffi::smolAddSolutionMolecules(
                self.p(),
                species.as_ptr(),
                number,
                opt_mut_ptr(&mut low),
                opt_mut_ptr(&mut high),
            )
        })
    }

    pub fn add_compartment_molecules(
        &mut self,
        species: &str,
        number: usize,
        compartment: &str,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        let number = to_i32(number, "number")?;
        let compartment = cstring(compartment)?;
        ok(unsafe {
            ffi::smolAddCompartmentMolecules(
                self.p(),
                species.as_ptr(),
                number,
                compartment.as_ptr(),
            )
        })
    }

    #[allow(clippy::too_many_arguments)]
    pub fn add_surface_molecules(
        &mut self,
        species: &str,
        state: MolecState,
        number: usize,
        surface: &str,
        shape: PanelShape,
        panel: &str,
        position: Option<&[f64]>,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let species = cstring(species)?;
        let number = to_i32(number, "number")?;
        let surface = cstring(surface)?;
        let panel = cstring(panel)?;
        let mut position = position.map(|p| vector(p, dim, "position")).transpose()?;
        ok(unsafe {
            ffi::smolAddSurfaceMolecules(
                self.p(),
                species.as_ptr(),
                state,
                number,
                surface.as_ptr(),
                shape,
                panel.as_ptr(),
                opt_mut_ptr(&mut position),
            )
        })
    }

    pub fn molecule_count(&self, species: &str, state: MolecState) -> SmolResult<usize> {
        let species = cstring(species)?;
        count(unsafe { ffi::smolGetMoleculeCount(self.p(), species.as_ptr(), state) })
    }
}
