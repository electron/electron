# SharedTextureTransfer Object

* `transfer` string - The opaque transfer data of the shared texture, can be transferred across Electron processes.
* `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
* `codedSize` [Size](size.md) - The full dimensions of the shared texture.
* `visibleRect` [Rectangle](rectangle.md) - A subsection of [0, 0, codedSize.width(), codedSize.height()]. In common cases, it is the full section area.
* `timestamp` number - A timestamp in microseconds that will be reflected to `VideoFrame`.

Do not modify any property, and use `sharedTexture.finishTransferSharedTexture` to get [`SharedTextureImported`](shared-texture-imported.md) back.
