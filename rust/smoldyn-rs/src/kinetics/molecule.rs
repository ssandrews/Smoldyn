//! molecule related.

#[derive(Debug, Eq, PartialEq, strum::Display, strum::EnumString)]
#[strum(serialize_all = "lowercase")]
pub enum MoleculeState {
    Soln,
    Front,
    Back,
    Down,
    BSoln,
    FSoln, // really Soln
    All,
    None,
    Some,
}
