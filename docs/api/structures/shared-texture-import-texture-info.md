# SharedTextureImportTextureInfo Object

* `pixelFormat` string - The pixel format of the texture.
* `colorSpace` [ColorSpace](color-space.md) (optional) - The color space of the texture.
* `codedSize` [Size](size.md) - The full dimensions of the shared texture.
* `visibleRect` [Rectangle](rectangle.md) (optional) - A subsection of [0, 0, codedSize.width, codedSize.height]. In common cases, it is the full section area.
* `timestamp` number (optional) - A timestamp in microseconds that will be reflected to `VideoFrame`.
* `handle` [SharedTextureHandle](shared-texture-handle.md) - The shared texture handle.
