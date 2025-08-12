# SharedTextureSubtle Object

* `importSharedTexture` Function\<[SharedTextureImportedSubtle](shared-texture-imported-subtle.md)\> - Imports the shared texture from the given options. Returns the imported shared texture.
  * `subtleOptions` Object - The information of shared texture to import.
    * `copy` boolean | Function (optional) - Defaults to `true`. If `true`, when importing, the shared texture will be copied to a new texture, allowing you to release the original native texture early. Otherwise, you have to manually manage the lifetime and wait for all transferred object's `release(callback)` callbacks are invoked, before releasing the original native texture, as the underlying buffer does not copy and may become flaky if lifetime handled poorly. If a function is provided, it will be called when the copy is completed, you can use this event to release the original native texture early.
    * `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
    * `colorSpace` [ColorSpace](color-space.md) (optional) - The color space of the texture.
    * `codedSize` [Size](size.md) - The full dimensions of the shared texture.
    * `visibleRect` [Rectangle](rectangle.md) (optional) - A subsection of [0, 0, codedSize.width, codedSize.height]. In common cases, it is the full section area.
    * `timestamp` number (optional) - A timestamp in microseconds that will be reflected to `VideoFrame`.
    * `handle` [SharedTextureHandle](shared-texture-handle.md) - The shared texture handle.
* `finishTransferSharedTexture` Function\<[SharedTextureImportedSubtle](shared-texture-imported-subtle.md)\> - Finishes the transfer of the shared texture and get the transferred shared texture. Returns the imported shared texture from the transfer object.
  * `transfer` [SharedTextureTransfer](shared-texture-imported.md) - The transfer object of the shared texture.
