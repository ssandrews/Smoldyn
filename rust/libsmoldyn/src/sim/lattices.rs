use super::{Sim, cstring, index, invalid, name, ok, opt_mut_ptr, opt_vector, to_i32, vector};
use crate::error::SmolResult;
use crate::ffi;

impl Sim {
    pub fn add_lattice(
        &mut self,
        lattice: &str,
        min: &[f64],
        max: &[f64],
        dx: &[f64],
        btype: &str,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let min = vector(min, dim, "min")?;
        let max = vector(max, dim, "max")?;
        let dx = vector(dx, dim, "dx")?;
        if btype.len() < dim {
            return Err(invalid(format!("btype needs {dim} characters")));
        }
        let lattice = cstring(lattice)?;
        let btype = cstring(btype)?;
        ok(unsafe {
            ffi::smolAddLattice(
                self.p(),
                lattice.as_ptr(),
                min.as_ptr(),
                max.as_ptr(),
                dx.as_ptr(),
                btype.as_ptr(),
            )
        })
    }

    pub fn add_lattice_port(&mut self, lattice: &str, port: &str) -> SmolResult<()> {
        let lattice = cstring(lattice)?;
        let port = cstring(port)?;
        ok(unsafe { ffi::smolAddLatticePort(self.p(), lattice.as_ptr(), port.as_ptr()) })
    }

    pub fn add_lattice_species(&mut self, lattice: &str, species: &str) -> SmolResult<()> {
        let lattice = cstring(lattice)?;
        let species = cstring(species)?;
        ok(unsafe { ffi::smolAddLatticeSpecies(self.p(), lattice.as_ptr(), species.as_ptr()) })
    }

    pub fn lattice_index(&self, lattice: &str) -> SmolResult<usize> {
        let lattice = cstring(lattice)?;
        index(unsafe { ffi::smolGetLatticeIndex(self.p(), lattice.as_ptr()) })
    }

    pub fn lattice_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "lattice index")?;
        name(|buf| unsafe { ffi::smolGetLatticeName(self.p(), index, buf) })
    }

    pub fn add_lattice_molecules(
        &mut self,
        lattice: &str,
        species: &str,
        number: usize,
        low: Option<&[f64]>,
        high: Option<&[f64]>,
    ) -> SmolResult<()> {
        let dim = self.dim();
        let mut low = opt_vector(low, dim, "low position")?;
        let mut high = opt_vector(high, dim, "high position")?;
        let lattice = cstring(lattice)?;
        let species = cstring(species)?;
        let number = to_i32(number, "number")?;
        ok(unsafe {
            ffi::smolAddLatticeMolecules(
                self.p(),
                lattice.as_ptr(),
                species.as_ptr(),
                number,
                opt_mut_ptr(&mut low),
                opt_mut_ptr(&mut high),
            )
        })
    }

    pub fn add_lattice_reaction(
        &mut self,
        lattice: &str,
        reaction: &str,
        move_products: bool,
    ) -> SmolResult<()> {
        let lattice = cstring(lattice)?;
        let reaction = cstring(reaction)?;
        ok(unsafe {
            ffi::smolAddLatticeReaction(
                self.p(),
                lattice.as_ptr(),
                reaction.as_ptr(),
                move_products as i32,
            )
        })
    }
}
