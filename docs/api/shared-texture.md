# sharedTexture

> Import shared textures into Electron, converts platform specific handles into [`VideoFrame`](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame). Support all Web rendering systems, and can be transferred across Electron processes. Read [here](https://github.com/electron/electron/blob/main/shell/common/api/shared_texture/README.md) for more information.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Methods

The `sharedTexture` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in future.

### `sharedTexture.importSharedTexture(options)` _Experimental_

* `options` Object - The information of shared texture to import.
  * `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
  * `colorSpace` [ColorSpace](structures/color-space.md) (optional) - The color space of the texture.
  * `codedSize` [Size](structures/size.md) - The full dimensions of the shared texture.
  * `visibleRect` [Rectangle](structures/rectangle.md) (optional) - A subsection of [0, 0, codedSize.width, codedSize.height]. In common cases, it is the full section area.
  * `timestamp` number (optional) - A timestamp in microseconds that will be reflected to `VideoFrame`.
  * `sharedTextureData` [SharedTextureData](structures/shared-texture-data.md) - The shared texture data.

Returns [`SharedTextureImported`](structures/shared-texture-imported.md) - The imported shared texture.

### `sharedTexture.finishTransferSharedTexture(transfer)` _Experimental_

* `transfer` [SharedTextureTransfer](structures/shared-texture-imported.md) - The transfer object of the shared texture.

Returns [`SharedTextureImported`](structures/shared-texture-imported.md) - The imported shared texture from the transfer object.
