//! Errors reported by libsmoldyn.

use std::ffi::{CStr, c_char};
use std::fmt;

use crate::ffi::{self, ErrorCode};

/// Size of libsmoldyn's global error buffers (`STRCHARLONG` in `smoldyn.h`).
/// `smolGetError` `strcpy`s into the caller's buffers, so ours must be at least this large.
pub(crate) const STRCHARLONG: usize = 4096;

pub type SmolResult<T> = Result<T, SmolError>;

/// An error returned by a libsmoldyn call.
#[derive(Debug, Clone)]
pub enum SmolError {
    /// libsmoldyn returned an error code (worse than `ECwarning`).
    Smoldyn {
        code: ErrorCode,
        function: String,
        message: String,
    },
    /// A Rust string or path could not be passed to C (e.g. interior NUL byte).
    InvalidArgument(String),
}

impl SmolError {
    /// Build an error from libsmoldyn's global error state and clear it.
    /// `fallback` is used as the code if libsmoldyn did not record one.
    pub(crate) fn last(fallback: ErrorCode) -> Self {
        let mut function = [0 as c_char; STRCHARLONG];
        let mut message = [0 as c_char; STRCHARLONG];
        let code = unsafe { ffi::smolGetError(function.as_mut_ptr(), message.as_mut_ptr(), 1) };
        let code = if code == ErrorCode::ECok {
            fallback
        } else {
            code
        };
        let to_string = |buf: &[c_char]| {
            unsafe { CStr::from_ptr(buf.as_ptr()) }
                .to_string_lossy()
                .into_owned()
        };
        SmolError::Smoldyn {
            code,
            function: to_string(&function),
            message: to_string(&message),
        }
    }
}

impl fmt::Display for SmolError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            SmolError::Smoldyn {
                code,
                function,
                message,
            } => write!(f, "smoldyn error {code:?} in {function}: {message}"),
            SmolError::InvalidArgument(msg) => write!(f, "invalid argument: {msg}"),
        }
    }
}

impl std::error::Error for SmolError {}

/// Outcome of a libsmoldyn call that did not fail.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Status {
    Ok,
    /// `ECnotify`, e.g. "Simulation complete".
    Notify,
    /// `ECwarning`.
    Warning,
}

/// Convert an `ErrorCode` into `Ok(Status)` or the last libsmoldyn error.
/// `ECnotify` and `ECwarning` are not failures.
pub(crate) fn check(code: ErrorCode) -> SmolResult<Status> {
    match code {
        ErrorCode::ECok => Ok(Status::Ok),
        ErrorCode::ECnotify => {
            // clear it, otherwise libsmoldyn keeps returning ECnotify from later calls.
            let note = SmolError::last(code);
            tracing::debug!("{note}");
            Ok(Status::Notify)
        }
        ErrorCode::ECwarning => {
            let err = SmolError::last(code);
            tracing::warn!("{err}");
            Ok(Status::Warning)
        }
        _ => Err(SmolError::last(code)),
    }
}
