use futures_io::{AsyncRead, AsyncWrite};
use nserror::nsresult;
use std::convert::TryInto;
use std::future::poll_fn;
use std::io;
use std::mem::ManuallyDrop;
use std::pin::Pin;
use std::ptr;
use std::sync::Mutex;
use std::task::{Context, Poll, Waker};
use xpcom::{
    interfaces::{nsIAsyncInputStream, nsIAsyncOutputStream, nsIInputStream, nsIOutputStream},
    xpcom, RefPtr, XpCom,
};

/// Helper type implementing `nsIInputStreamCallback` and
/// `nsIOutputStreamCallback` which can be used as an argument to the
/// `AsyncWait` methods. Will invoke the given waker when notified.
#[xpcom(implement(nsIInputStreamCallback, nsIOutputStreamCallback), atomic)]
struct StreamWaker {
    waker: Mutex<Option<Waker>>,
}

impl StreamWaker {
    fn new() -> RefPtr<Self> {
        Self::allocate(InitStreamWaker {
            waker: Mutex::new(None),
        })
    }

    fn listen(&self, waker: Waker) {
        *self.waker.lock().unwrap() = Some(waker);
    }

    fn take_waker(&self) -> Option<Waker> {
        self.waker.lock().unwrap().take()
    }

    #[allow(non_snake_case)]
    fn OnInputStreamReady(&self, _: *const nsIAsyncInputStream) -> nsresult {
        if let Some(waker) = self.take_waker() {
            waker.wake();
        }
        nserror::NS_OK
    }

    #[allow(non_snake_case)]
    fn OnOutputStreamReady(&self, _: *const nsIAsyncOutputStream) -> nsresult {
        if let Some(waker) = self.take_waker() {
            waker.wake();
        }
        nserror::NS_OK
    }
}

/// An XPCOM nsIInputStream wrapped to be usable from Rust code.
///
/// WARNING: While this method exposes async reader APIs, it is _NOT_ guaranteed
/// to be non-blocking. To determine if the stream is non-blocking, you must
/// check `is_non_blocking()`. An async read on this type may block the calling
/// thread.
pub struct InputStream {
    stream: RefPtr<nsIInputStream>,
    waker: Option<RefPtr<StreamWaker>>,
}

impl InputStream {
    /// Wrap the given nsIInputStream to make it easier to use from Rust.
    pub fn new(stream: RefPtr<nsIInputStream>) -> Self {
        InputStream {
            stream,
            waker: None,
        }
    }

    /// Extract the original `nsIInputStream` which was used to construct this
    /// `InputStream`, allowing it to be used elsewhere.
    pub fn into_inner(self) -> RefPtr<nsIInputStream> {
        let this = ManuallyDrop::new(self);
        // SAFETY: The refcount will not be decremented as we wrapped `self` in
        // a `ManuallyDrop`.
        let stream = unsafe { RefPtr::from_raw_dont_addref(&*this.stream).unwrap() };
        if let Some(waker) = &this.waker {
            // SAFETY: The refcount will not be decremented as we wrapped `self`
            // in a `ManuallyDrop`.
            let waker = unsafe { RefPtr::from_raw_dont_addref(&**waker).unwrap() };

            // If we were actively listening for a callback, disarm our waker and
            // clear the pending `AsyncWait` so that it can be set by a different
            // listener using our `nsIAsyncInputStream`.
            if waker.take_waker().is_some() {
                if let Some(async_stream) = stream.query_interface::<nsIAsyncInputStream>() {
                    let _ = unsafe { async_stream.AsyncWait(ptr::null(), 0, 0, ptr::null()) };
                }
            }
        }
        stream
    }

    /// Check if the underlying stream is considered "non-blocking". This may
    /// return an error if the underlying xpcom operation errors.
    pub fn is_non_blocking(&self) -> Result<bool, nsresult> {
        let mut non_blocking = false;
        unsafe { self.stream.IsNonBlocking(&mut non_blocking).to_result()? }
        Ok(non_blocking)
    }

    /// Fetch the amount of data which is immediately available in this stream.
    ///
    /// This value is inherently out-of-date by the time it is used, as the
    /// stream could have gained more data since the call.
    ///
    /// A return value of `Ok(0)` does not necessarily indicate EOF.
    ///
    /// Prefer using the AsyncRead APIs over this method.
    pub fn available_xpcom(&mut self) -> Result<u64, nsresult> {
        let mut available = 0u64;
        unsafe { self.stream.Available(&mut available).to_result()? }
        Ok(available)
    }

    /// Consume and close the stream, telling the other side that no more data
    /// will be read.
    pub fn close(&mut self) -> Result<(), nsresult> {
        unsafe { self.stream.Close().to_result() }
    }

    /// Try to read data into the given buffer.
    ///
    /// This method may block if `is_non_blocking` does not return `true`.
    ///
    /// This method may return `NS_BASE_STREAM_WOULD_BLOCK` which indicates that
    /// it should be read asynchronously using one of the relevant async APIs.
    ///
    /// Prefer using the AsyncRead APIs over this method.
    pub fn read_xpcom(&mut self, buf: &mut [u8]) -> Result<usize, nsresult> {
        let count: u32 = buf.len().try_into().unwrap_or(u32::MAX);
        let mut read: u32 = 0;
        unsafe {
            self.stream
                .Read(buf.as_mut_ptr() as *mut libc::c_char, count, &mut read)
                .to_result()?
        }
        Ok(read as usize)
    }

    /// Raw version of `poll_read`, which returns a `nsresult` instead of
    /// `io::Error` in the error case.
    pub fn poll_read_xpcom(
        &mut self,
        cx: &mut Context<'_>,
        buf: &mut [u8],
    ) -> Poll<Result<usize, nsresult>> {
        match self.read_xpcom(buf) {
            Err(nserror::NS_BASE_STREAM_WOULD_BLOCK) => {
                if let Some(async_stream) = self.stream.query_interface::<nsIAsyncInputStream>() {
                    let waker = self.waker.get_or_insert_with(StreamWaker::new);
                    waker.listen(cx.waker().clone());
                    unsafe {
                        async_stream
                            .AsyncWait(waker.coerce(), 0, 0, ptr::null())
                            .to_result()?;
                    }
                    Poll::Pending
                } else {
                    Err(nserror::NS_BASE_STREAM_WOULD_BLOCK)?
                }
            }
            rv => Poll::Ready(rv),
        }
    }

    /// Perform a single read from this input stream, returning an xpcom
    /// `nsresult`.
    pub async fn async_read_xpcom(&mut self, buf: &mut [u8]) -> Result<usize, nsresult> {
        poll_fn(|cx| self.poll_read_xpcom(cx, buf)).await
    }
}

impl AsyncRead for InputStream {
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut [u8],
    ) -> Poll<io::Result<usize>> {
        self.poll_read_xpcom(cx, buf).map_err(|e| e.into())
    }
}

impl Drop for InputStream {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

// SAFETY: The interface is `builtinclass`, and all "reasonable"
// `nsIInputStream` implementations are safe to be used from any single
// sequence, so we assume that is the case here.
//
// We don't implement `Sync`, as they may not be safe to use concurrently from
// multiple threads.
unsafe impl Send for InputStream {}

/// An XPCOM nsIOutputStream wrapped to be usable from Rust code.
///
/// WARNING: While this method exposes async writer APIs, it is _NOT_ guaranteed
/// to be non-blocking. To determine if the stream is non-blocking, you must
/// check `is_non_blocking()`. An async write on this type may block the calling
/// thread.
pub struct OutputStream {
    stream: RefPtr<nsIOutputStream>,
    waker: Option<RefPtr<StreamWaker>>,
}

impl OutputStream {
    /// Wrap the given nsIOutputStream to make it easier to use from Rust.
    pub fn new(stream: RefPtr<nsIOutputStream>) -> Self {
        OutputStream {
            stream,
            waker: None,
        }
    }

    /// Extract the original `nsIOutputStream` which was used to construct this
    /// `OutputStream`, allowing it to be used elsewhere.
    pub fn into_inner(self) -> RefPtr<nsIOutputStream> {
        let this = ManuallyDrop::new(self);
        // SAFETY: The refcount will not be decremented as we wrapped `self` in
        // a `ManuallyDrop`.
        let stream = unsafe { RefPtr::from_raw_dont_addref(&*this.stream).unwrap() };
        if let Some(waker) = &this.waker {
            // SAFETY: The refcount will not be decremented as we wrapped `self`
            // in a `ManuallyDrop`.
            let waker = unsafe { RefPtr::from_raw_dont_addref(&**waker).unwrap() };

            // If we were actively listening for a callback, disarm our waker and
            // clear the pending `AsyncWait` so that it can be set by a different
            // listener using our `nsIAsyncOutputStream`.
            if waker.take_waker().is_some() {
                if let Some(async_stream) = stream.query_interface::<nsIAsyncOutputStream>() {
                    let _ = unsafe { async_stream.AsyncWait(ptr::null(), 0, 0, ptr::null()) };
                }
            }
        }
        stream
    }

    /// Check if the underlying stream is considered "non-blocking". This may
    /// return an error if the underlying xpcom operation errors.
    pub fn is_non_blocking(&self) -> Result<bool, nsresult> {
        let mut non_blocking = false;
        unsafe { self.stream.IsNonBlocking(&mut non_blocking).to_result()? }
        Ok(non_blocking)
    }

    /// Close the stream, telling the other side that no more data will be written.
    pub fn close(&mut self) -> Result<(), nsresult> {
        unsafe { self.stream.Close().to_result() }
    }

    /// Flush any data pending in the stream.
    pub fn flush(&mut self) -> Result<(), nsresult> {
        unsafe { self.stream.Flush().to_result() }
    }

    /// Try to write data from the given buffer.
    ///
    /// This method may block if `is_non_blocking` does not return `true`.
    ///
    /// This method may return `NS_BASE_STREAM_WOULD_BLOCK` which indicates that
    /// it should be written asynchronously using one of the relevant async APIs.
    ///
    /// Prefer using the AsyncWrite APIs over this method.
    pub fn write_xpcom(&mut self, buf: &[u8]) -> Result<usize, nsresult> {
        let count: u32 = buf.len().try_into().unwrap_or(u32::MAX);
        let mut written: u32 = 0;
        unsafe {
            self.stream
                .Write(buf.as_ptr() as *const libc::c_char, count, &mut written)
                .to_result()?
        }
        Ok(written as usize)
    }

    /// Raw version of `poll_write`, which returns a `nsresult` instead of
    /// `io::Error` in the error case.
    pub fn poll_write_xpcom(
        &mut self,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<Result<usize, nsresult>> {
        match self.write_xpcom(buf) {
            Err(nserror::NS_BASE_STREAM_WOULD_BLOCK) => {
                if let Some(async_stream) = self.stream.query_interface::<nsIAsyncOutputStream>() {
                    let waker = self.waker.get_or_insert_with(StreamWaker::new);
                    waker.listen(cx.waker().clone());
                    unsafe {
                        async_stream
                            .AsyncWait(waker.coerce(), 0, 0, ptr::null())
                            .to_result()?;
                    }
                    Poll::Pending
                } else {
                    Err(nserror::NS_BASE_STREAM_WOULD_BLOCK)?
                }
            }
            rv => Poll::Ready(rv),
        }
    }

    /// Perform a single write from this input stream, returning an xpcom
    /// `nsresult`.
    pub async fn async_write_xpcom(&mut self, buf: &[u8]) -> Result<usize, nsresult> {
        poll_fn(|cx| self.poll_write_xpcom(cx, buf)).await
    }
}

impl AsyncWrite for OutputStream {
    fn poll_write(
        mut self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &[u8],
    ) -> Poll<io::Result<usize>> {
        self.poll_write_xpcom(cx, buf).map_err(|e| e.into())
    }

    fn poll_close(mut self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        self.close()?;
        Poll::Ready(Ok(()))
    }

    fn poll_flush(mut self: Pin<&mut Self>, _cx: &mut Context<'_>) -> Poll<io::Result<()>> {
        self.flush()?;
        Poll::Ready(Ok(()))
    }
}

impl Drop for OutputStream {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

// SAFETY: The interface is `builtinclass`, and all "reasonable"
// `nsIOutputStream` implementations are safe to be used from any single
// sequence, so we assume that is the case here.
//
// We don't implement `Sync`, as they may not be safe to use concurrently from
// multiple threads.
unsafe impl Send for OutputStream {}
