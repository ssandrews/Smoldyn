use super::{Sim, cstring, index, name, ok, to_i32, vector};
use crate::error::SmolResult;
use crate::ffi::{self, CmptLogic};

impl Sim {
    pub fn add_compartment(&mut self, compartment: &str) -> SmolResult<()> {
        let compartment = cstring(compartment)?;
        ok(unsafe { ffi::smolAddCompartment(self.p(), compartment.as_ptr()) })
    }

    pub fn compartment_index(&self, compartment: &str) -> SmolResult<usize> {
        let compartment = cstring(compartment)?;
        index(unsafe { ffi::smolGetCompartmentIndex(self.p(), compartment.as_ptr()) })
    }

    pub fn compartment_name(&self, index: usize) -> SmolResult<String> {
        let index = to_i32(index, "compartment index")?;
        name(|buf| unsafe { ffi::smolGetCompartmentName(self.p(), index, buf) })
    }

    pub fn add_compartment_surface(&mut self, compartment: &str, surface: &str) -> SmolResult<()> {
        let compartment = cstring(compartment)?;
        let surface = cstring(surface)?;
        ok(unsafe {
            ffi::smolAddCompartmentSurface(self.p(), compartment.as_ptr(), surface.as_ptr())
        })
    }

    pub fn add_compartment_point(&mut self, compartment: &str, point: &[f64]) -> SmolResult<()> {
        let mut point = vector(point, self.dim(), "point")?;
        let compartment = cstring(compartment)?;
        ok(unsafe {
            ffi::smolAddCompartmentPoint(self.p(), compartment.as_ptr(), point.as_mut_ptr())
        })
    }

    pub fn add_compartment_logic(
        &mut self,
        compartment: &str,
        logic: CmptLogic,
        compartment2: &str,
    ) -> SmolResult<()> {
        let compartment = cstring(compartment)?;
        let compartment2 = cstring(compartment2)?;
        ok(unsafe {
            ffi::smolAddCompartmentLogic(
                self.p(),
                compartment.as_ptr(),
                logic,
                compartment2.as_ptr(),
            )
        })
    }
}
