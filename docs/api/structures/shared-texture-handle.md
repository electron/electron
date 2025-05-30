# SharedTextureHandle Object

* `ntHandle` Buffer (optional) _Windows_ - NT HANDLE holds the shared texture. Note that this NT HANDLE is local to current process.
* `ioSurface` Buffer (optional) _macOS_ - IOSurfaceRef holds the shared texture. Note that this IOSurface is local to current process (not global).
* `nativePixmap` Object (optional) _Linux_ - Structure contains planes of shared texture.
  * `planes` Object[] _Linux_ - Each plane's info of the shared texture.
    * `stride` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `offset` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `size` number - Size in bytes of the plane. This is necessary to map the buffers.
    * `fd` number - File descriptor for the underlying memory object (usually dmabuf).
  * `modifier` string _Linux_ - The modifier is retrieved from GBM library and passed to EGL driver.
  * `supportsZeroCopyWebGpuImport` boolean _Linux_ - Indicates whether supports zero copy import to WebGPU.
