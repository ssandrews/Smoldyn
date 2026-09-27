//! Reactions.

use std::ffi::{CString, c_char};

use super::{Sim, cstring, index, invalid, name, ok, opt_cstring, opt_mut_ptr, opt_ptr, opt_vector, to_i32};
use crate::error::SmolResult;
use crate::ffi::{self, MolecState, RevParam};

// MAXPRODUCT in smoldyn.h.
const MAX_PRODUCT: usize = 256;

/// What [`Sim::set_reaction_rate`] sets.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RateKind {
    /// the macroscopic rate constant
    Rate,
    /// the internal value: reaction probability for order 0 and 1, binding
    /// radius for order 2
    Internal,
    /// the reaction probability
    Probability,
}

impl Sim {
    /// Add a reaction (`smolAddReaction`).
    ///
    /// `reactants` has 0, 1 or 2 entries (the reaction order). `rate` of `None`
    /// leaves the rate unset, to be set later with [`Sim::set_reaction_rate`].
    pub fn add_reaction(
        &mut self,
        reaction: &str,
        reactants: &[(&str, MolecState)],
        products: &[(&str, MolecState)],
        rate: Option<f64>,
    ) -> SmolResult<()> {
        if reactants.len() > 2 {
            return Err(invalid("a reaction has at most 2 reactants"));
        }
        if products.len() > MAX_PRODUCT {
            return Err(invalid(format!("a reaction has at most {MAX_PRODUCT} products")));
        }
        let reaction = cstring(reaction)?;
        let reactant = |i: usize| -> SmolResult<(Option<CString>, MolecState)> {
            match reactants.get(i) {
                Some(&(species, state)) => Ok((Some(cstring(species)?), state)),
                None => Ok((None, MolecState::MSnone)),
            }
        };
        let (r1, s1) = reactant(0)?;
        let (r2, s2) = reactant(1)?;
        let names = products
            .iter()
            .map(|&(species, _)| cstring(species))
            .collect::<SmolResult<Vec<_>>>()?;
        let mut name_ptrs: Vec<*const c_char> = names.iter().map(|n| n.as_ptr()).collect();
        let mut states: Vec<MolecState> = products.iter().map(|&(_, state)| state).collect();
        ok(unsafe {
            ffi::smolAddReaction(
                self.p(),
                reaction.as_ptr(),
                opt_ptr(&r1),
                s1,
                opt_ptr(&r2),
                s2,
                products.len() as i32,
                name_ptrs.as_mut_ptr(),
                states.as_mut_ptr(),
                rate.unwrap_or(-1.0),
            )
        })
    }

    /// Order and index of a reaction (`smolGetReactionIndex`).
    pub fn reaction_index(&self, reaction: &str) -> SmolResult<(usize, usize)> {
        let reaction = cstring(reaction)?;
        let mut order = -1;
        let i = index(unsafe { ffi::smolGetReactionIndex(self.p(), &mut order, reaction.as_ptr()) })?;
        Ok((order as usize, i))
    }

    /// Name of reaction `index` of `order` (`smolGetReactionName`).
    pub fn reaction_name(&self, order: usize, index: usize) -> SmolResult<String> {
        let order = to_i32(order, "reaction order")?;
        let index = to_i32(index, "reaction index")?;
        name(|buf| unsafe { ffi::smolGetReactionName(self.p(), order, index, buf) })
    }

    /// Set a reaction's rate, or another value per `kind` (`smolSetReactionRate`).
    pub fn set_reaction_rate(&mut self, reaction: &str, rate: f64, kind: RateKind) -> SmolResult<()> {
        let reaction = cstring(reaction)?;
        let kind = match kind {
            RateKind::Rate => 0,
            RateKind::Internal => 1,
            RateKind::Probability => 2,
        };
        ok(unsafe { ffi::smolSetReactionRate(self.p(), reaction.as_ptr(), rate, kind) })
    }

    /// A reaction's rate constant (`smolGetReactionRate`).
    pub fn reaction_rate(&self, reaction: &str) -> SmolResult<f64> {
        let reaction = cstring(reaction)?;
        let mut rate = 0.0;
        ok(unsafe { ffi::smolGetReactionRate(self.p(), reaction.as_ptr(), &mut rate) })?;
        Ok(rate)
    }

    /// Restrict a reaction to a compartment and/or surface (`smolSetReactionRegion`).
    pub fn set_reaction_region(
        &mut self,
        reaction: &str,
        compartment: Option<&str>,
        surface: Option<&str>,
    ) -> SmolResult<()> {
        let reaction = cstring(reaction)?;
        let compartment = opt_cstring(compartment)?;
        let surface = opt_cstring(surface)?;
        ok(unsafe {
            ffi::smolSetReactionRegion(
                self.p(),
                reaction.as_ptr(),
                opt_ptr(&compartment),
                opt_ptr(&surface),
            )
        })
    }

    /// How reaction products are placed (`smolSetReactionProducts`). `product`
    /// selects one product for methods that need it; `position` (`dim` values)
    /// is the offset for [`RevParam::RPoffset`] and [`RevParam::RPfixed`].
    pub fn set_reaction_products(
        &mut self,
        reaction: &str,
        method: RevParam,
        parameter: f64,
        product: Option<&str>,
        position: Option<&[f64]>,
    ) -> SmolResult<()> {
        let mut position = opt_vector(position, self.dim(), "position")?;
        let reaction = cstring(reaction)?;
        let product = opt_cstring(product)?;
        ok(unsafe {
            ffi::smolSetReactionProducts(
                self.p(),
                reaction.as_ptr(),
                method,
                parameter,
                opt_ptr(&product),
                opt_mut_ptr(&mut position),
            )
        })
    }
}
