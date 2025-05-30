# sharedTexture

> Import shared textures into Electron, converts platform specific handles into [`VideoFrame`](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame). Support all Web rendering systems, and can be transferred across Electron processes. Read [here](https://github.com/electron/electron/blob/main/shell/common/api/shared_texture/README.md) for more information.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Methods

The `sharedTexture` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in the future.

### `sharedTexture.importSharedTexture(options)` _Experimental_

* `options` Object - The information of shared texture to import.
  * `copy` boolean | Function (optional) - Defaults to `true`. If `true`, when importing, the shared texture will be copied to a new texture, allowing you to release the original native texture early. Otherwise, you have to manually manage the lifetime and wait for all transferred object's `release(callback)` callbacks are invoked, before releasing the original native texture, as the underlying buffer does not copy and may become flaky if lifetime handled poorly. If a function is provided, it will be called when the copy is completed, you can use this event to release the original native texture early.
  * `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
  * `colorSpace` [ColorSpace](structures/color-space.md) (optional) - The color space of the texture.
  * `codedSize` [Size](structures/size.md) - The full dimensions of the shared texture.
  * `visibleRect` [Rectangle](structures/rectangle.md) (optional) - A subsection of [0, 0, codedSize.width, codedSize.height]. In common cases, it is the full section area.
  * `timestamp` number (optional) - A timestamp in microseconds that will be reflected to `VideoFrame`.
  * `handle` [SharedTextureHandle](structures/shared-texture-handle.md) - The shared texture handle.

Imports the shared texture from the given options.

Returns [`SharedTextureImported`](structures/shared-texture-imported.md) - The imported shared texture.

### `sharedTexture.finishTransferSharedTexture(transfer)` _Experimental_

* `transfer` [SharedTextureTransfer](structures/shared-texture-imported.md) - The transfer object of the shared texture.

Finishes the transfer of the shared texture and get the transferred shared texture.

Returns [`SharedTextureImported`](structures/shared-texture-imported.md) - The imported shared texture from the transfer object.

### `sharedTexture.sendToRenderer(webContents, imported, ...args)` _Experimental_

* `webContents` [WebContents](web-contents.md) - The web contents to transfer the shared texture to.
* `importedSharedTexture` [SharedTextureImported](structures/shared-texture-imported.md) - The imported shared texture.
* `...args` any[] - Additional arguments to pass to the renderer process.

Send the imported shared texture to a renderer process. You must register receiver at renderer process before calling this method. This method has a timeout.

> [!NOTE]
> This method is only available in the main process.

Returns `Promise<void>` - Resolves when the transfer is complete.

### `sharedTexture.receiveFromMain(receiver)` _Experimental_

* `receiver` Function\<Promise\<void\>\> - The function to receiver the imported shared texture.
  * `importedSharedTexture` [SharedTextureImported](structures/shared-texture-imported.md) - The imported shared texture.
  * `...args` any[] - Additional arguments passed from the main process.

Receives imported shared textures from the main process.

> [!NOTE]
> This method is only available in the renderer process.
