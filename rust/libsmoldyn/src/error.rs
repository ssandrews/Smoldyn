use std::ffi::{CStr, c_char};
use std::fmt;

use crate::ffi::{self, ErrorCode};

pub(crate) const STRCHARLONG: usize = 4096;

pub type SmolResult<T> = Result<T, SmolError>;

#[derive(Debug, Clone)]
pub enum SmolError {
    Smoldyn {
        code: ErrorCode,
        function: String,
        message: String,
    },
    InvalidArgument(String),
}

impl SmolError {
    pub(crate) fn last(fallback: ErrorCode) -> Self {
        let mut function = [0 as c_char; STRCHARLONG];
        let mut message = [0 as c_char; STRCHARLONG];
        let code = unsafe { ffi::smolGetError(function.as_mut_ptr(), message.as_mut_ptr(), 1) };
        let code = if code == ErrorCode::Ok {
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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Status {
    Ok,
    Notify,
    Warning,
}

pub(crate) fn check(code: ErrorCode) -> SmolResult<Status> {
    match code {
        ErrorCode::Ok => Ok(Status::Ok),
        ErrorCode::Notify => {
            // clear it, otherwise libsmoldyn keeps returning ECnotify from later calls.
            let note = SmolError::last(code);
            tracing::debug!("{note}");
            Ok(Status::Notify)
        }
        ErrorCode::Warning => {
            let err = SmolError::last(code);
            tracing::warn!("{err}");
            Ok(Status::Warning)
        }
        _ => Err(SmolError::last(code)),
    }
}
