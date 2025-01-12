/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

use nsstring::{nsACString, nsCString};
use std::error::Error;
use std::fmt;
use std::io;

/// The type of errors in gecko.  Uses a newtype to provide additional type
/// safety in Rust and #[repr(transparent)] to ensure the same representation
/// as the C++ equivalent.
#[repr(transparent)]
#[allow(non_camel_case_types)]
#[derive(Clone, Copy, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct nsresult(pub u32);

impl nsresult {
    pub fn failed(self) -> bool {
        (self.0 >> 31) != 0
    }

    pub fn succeeded(self) -> bool {
        !self.failed()
    }

    pub fn to_result(self) -> Result<(), nsresult> {
        if self.failed() {
            Err(self)
        } else {
            Ok(())
        }
    }

    /// Get a printable name for the nsresult error code. This function returns
    /// a nsCString<'static>, which implements `Display`.
    pub fn error_name(self) -> nsCString {
        let mut cstr = nsCString::new();
        unsafe {
            Gecko_GetErrorName(self, &mut *cstr);
        }
        cstr
    }
}

impl fmt::Display for nsresult {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.error_name())
    }
}

impl fmt::Debug for nsresult {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.error_name())
    }
}

impl<T, E> From<Result<T, E>> for nsresult
where
    E: Into<nsresult>,
{
    fn from(result: Result<T, E>) -> nsresult {
        match result {
            Ok(_) => NS_OK,
            Err(e) => e.into(),
        }
    }
}

impl Error for nsresult {}

/// Implement a conversion from `nsresult` to `std::io::Error` to make it easier
/// to use in some places. This generally just wraps the error in
/// `ErrorKind::Other`, but may specify an error kind if it is known.
///
/// Can only be used on errored nsresult values.
impl From<nsresult> for io::Error {
    fn from(error: nsresult) -> io::Error {
        assert!(error.failed());
        // Map various errors which might come up to more specific
        // std::io::ErrorKind values.
        //
        // This is far from a complete mapping, and new mappings can be added if
        // they are found to be useful in the future.
        let kind = match error {
            NS_ERROR_INVALID_ARG => io::ErrorKind::InvalidInput,
            NS_ERROR_OUT_OF_MEMORY => io::ErrorKind::OutOfMemory,
            NS_BASE_STREAM_WOULD_BLOCK => io::ErrorKind::WouldBlock,
            NS_ERROR_FILE_NOT_FOUND => io::ErrorKind::NotFound,
            NS_ERROR_FILE_READ_ONLY | NS_ERROR_FILE_ACCESS_DENIED => {
                io::ErrorKind::PermissionDenied
            }
            NS_ERROR_FILE_ALREADY_EXISTS => io::ErrorKind::AlreadyExists,
            _ => io::ErrorKind::Other,
        };
        io::Error::new(kind, error)
    }
}

extern "C" {
    fn Gecko_GetErrorName(rv: nsresult, cstr: *mut nsACString);
}

mod error_list {
    include!(mozbuild::objdir_path!("xpcom/base/error_list.rs"));
}

pub use error_list::*;
