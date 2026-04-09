# SharedTextureTransfer Object

* `transfer` string _Readonly_ - The opaque transfer data of the shared texture. This can be transferred across Electron processes.
* `syncToken` string _Readonly_ - The opaque sync token data for frame creation.
* `pixelFormat` string _Readonly_ - The pixel format of the transferring texture.
* `codedSize` [Size](size.md) _Readonly_ - The full dimensions of the shared texture.
* `visibleRect` [Rectangle](rectangle.md) _Readonly_ - A subsection of [0, 0, codedSize.width(), codedSize.height()]. In common cases, it is the full section area.
* `timestamp` number _Readonly_ - A timestamp in microseconds that will be reflected to `VideoFrame`.

Use `sharedTexture.subtle.finishTransferSharedTexture` to get [`SharedTextureImportedSubtle`](shared-texture-imported-subtle.md) back.
