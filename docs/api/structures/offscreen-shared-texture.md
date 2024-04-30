# OffscreenSharedTexture Object

* `textureInfo` Object - The shared texture info.
  * `widgetType` string - The widget type of the texture, could be `popup` or `frame`.
  * `pixelFormat` string - The pixel format of the texture, could be `rgba` or `bgra`.
  * `sharedTextureHandle` string - _Windows_ _macOS_ The handle to the shared texture.
  * `planes` Object[] - _Linux_ Each plane's info of the shared texture.
    * `stride` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `offset` number - The strides and offsets in bytes to be used when accessing the buffers via a memory mapping. One per plane per entry.
    * `size` number - Size in bytes of the plane. This is necessary to map the buffers.
    * `fd` number - File descriptor for the underlying memory object (usually dmabuf).
  * `modifier` string - _Linux_ The modifier is retrieved from GBM library and passed to EGL driver.
* `release` Function - Release the resources. The `texture` cannot be directly passed to another process, users need to maintain texture lifecycles in
main process, but it is safe to pass the `textureInfo` to another process.
