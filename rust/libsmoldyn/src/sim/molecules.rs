//! Species, molecule lists and molecules.

use std::ffi::{CStr, c_char};

use super::{
    Sim, count, cstring, index, name, ok, opt_cstring, opt_mut_ptr, opt_ptr, opt_vector, to_i32,
    vector,
};
use crate::error::{STRCHARLONG, SmolError, SmolResult};
use crate::ffi::{self, ErrorCode, MolecState, PanelShape};

impl Sim {
    /// Add a species; `mollist` is the molecule list to put it in, or `None`
    /// for the default (`smolAddSpecies`).
    pub fn add_species(&mut self, species: &str, mollist: Option<&str>) -> SmolResult<()> {
        let species = cstring(species)?;
        let mollist = opt_cstring(mollist)?;
        ok(unsafe { ffi::smolAddSpecies(self.p(), species.as_ptr(), opt_ptr(&mollist)) })
    }

    /// Names of the defined species, excluding smoldyn's internal "empty" species.
    pub fn species(&self) -> Vec<String> {
        let sim = self.raw();
        (1..ffi::smolrs_nspecies(sim))
            .filter_map(|i| {
                let name = ffi::smolrs_species_name(sim, i);
                // SAFETY: non-null names are NUL-terminated strings owned by the simulation.
                (!name.is_null())
                    .then(|| unsafe { CStr::from_ptr(name) }.to_string_lossy().into_owned())
            })
            .collect()
    }

    /// Index of `species` (1-based; 0 is the internal "empty" species)
    /// (`smolGetSpeciesIndex`).
    pub fn species_index(&self, species: &str) -> SmolResult<usize> {
        let species = cstring(species)?;
        index(unsafe { ffi::smolGetSpeciesIndex(self.p(), species.as_ptr()) })
    }

    /// Name of the species at `index` (`smolGetSpeciesName`).
    pub fn species_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "species index")?;
        let mut buf = vec![0 as c_char; STRCHARLONG];
        unsafe {
            ffi::smolClearError();
            ffi::smolGetSpeciesName(self.p(), index, buf.as_mut_ptr());
        }
        // smolGetSpeciesName returns nothing; an empty buffer means it failed.
        if buf[0] == 0 {
            return Err(SmolError::last(ErrorCode::ECnonexist));
        }
        Ok(unsafe { CStr::from_ptr(buf.as_ptr()) }.to_string_lossy().into_owned())
    }

    /// Set diffusion (`smolSetSpeciesMobility`). `None` leaves a value unchanged;
    /// `drift` has `dim` values and `difmatrix` is `dim`x`dim`, row-major.
    /// Use `"all"` for all species.
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

    /// Set the display color, RGB in 0..=1 (`smolSetMoleculeColor`).
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

    /// Set the display size (`smolSetMoleculeSize`).
    pub fn set_molecule_size(
        &mut self,
        species: &str,
        state: MolecState,
        size: f64,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        ok(unsafe { ffi::smolSetMoleculeSize(self.p(), species.as_ptr(), state, size) })
    }

    /// Set display size and/or color, `None` leaves it unchanged (`smolSetMoleculeStyle`).
    pub fn set_molecule_style(
        &mut self,
        species: &str,
        state: MolecState,
        size: Option<f64>,
        color: Option<[f64; 3]>,
    ) -> SmolResult<()> {
        let species = cstring(species)?;
        let mut color = color;
        let color_ptr = color.as_mut().map_or(std::ptr::null_mut(), |c| c.as_mut_ptr());
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

    /// Add a molecule list (`smolAddMolList`).
    pub fn add_mol_list(&mut self, mollist: &str) -> SmolResult<()> {
        let mollist = cstring(mollist)?;
        ok(unsafe { ffi::smolAddMolList(self.p(), mollist.as_ptr()) })
    }

    /// Index of a molecule list (`smolGetMolListIndex`).
    pub fn mol_list_index(&self, mollist: &str) -> SmolResult<usize> {
        let mollist = cstring(mollist)?;
        index(unsafe { ffi::smolGetMolListIndex(self.p(), mollist.as_ptr()) })
    }

    /// Name of the molecule list at `index` (`smolGetMolListName`).
    pub fn mol_list_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "molecule list index")?;
        name(|buf| unsafe { ffi::smolGetMolListName(self.p(), index, buf) })
    }

    /// Put `species` in `state` into molecule list `mollist` (`smolSetMolList`).
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

    /// Maximum number of molecules the simulation may hold (`smolSetMaxMolecules`).
    pub fn set_max_molecules(&mut self, max: usize) -> SmolResult<()> {
        let max = to_i32(max, "max molecules")?;
        ok(unsafe { ffi::smolSetMaxMolecules(self.p(), max) })
    }

    /// Add `number` molecules in solution, uniformly between `low` and `high`
    /// (`dim` values each; `None` means the simulation bounds)
    /// (`smolAddSolutionMolecules`).
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

    /// Add `number` molecules at random positions inside `compartment`
    /// (`smolAddCompartmentMolecules`).
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

    /// Add `number` surface-bound molecules (`smolAddSurfaceMolecules`).
    ///
    /// `surface` and `panel` may be `"all"` (with [`PanelShape::PSall`]) for
    /// random placement over all panels; `position` (`dim` values) needs a
    /// specific surface, shape and panel.
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

    /// Number of molecules of `species` in `state` (`smolGetMoleculeCount`).
    /// Use `"all"` for all species and [`MolecState::MSall`] for all states.
    pub fn molecule_count(&self, species: &str, state: MolecState) -> SmolResult<usize> {
        let species = cstring(species)?;
        count(unsafe { ffi::smolGetMoleculeCount(self.p(), species.as_ptr(), state) })
    }
}
