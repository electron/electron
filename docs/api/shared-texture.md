# sharedTexture

> Import shared textures into Electron, converts platform specific handles into [`VideoFrame`](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame). Support all Web rendering systems, and can be transferred across Electron processes. Read [here](https://github.com/electron/electron/blob/main/shell/common/api/shared_texture/README.md) for more information.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Methods

The `sharedTexture` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in the future.

### `sharedTexture.importSharedTexture(options)` _Experimental_

* `options` Object - The information of shared texture to import.
  * `pixelFormat` string - The pixel format of the texture. Can be `rgba` or `bgra`.
  * `colorSpace` [ColorSpace](structures/color-space.md) (optional) - The color space of the texture.
  * `codedSize` [Size](structures/size.md) - The full dimensions of the shared texture.
  * `visibleRect` [Rectangle](structures/rectangle.md) (optional) - A subsection of [0, 0, codedSize.width, codedSize.height]. In common cases, it is the full section area.
  * `timestamp` number (optional) - A timestamp in microseconds that will be reflected to `VideoFrame`.
  * `handle` [SharedTextureHandle](structures/shared-texture-handle.md) - The shared texture handle.
  * `disableCopy` boolean (optional) - If `true`, the native texture will not be copied, and you should use `allReleased` callback to keep the native texture alive. Could be useful if GPU memory comsumption is a concern.
  * `allReleased` Function (optional) - Called when all references are released. If `disableCopy` is `true`, you should keep native texture valid until this callback is called.

Imports the shared texture from the given options. A copy of the native texture will be made, so that you can release the native texture after the import is complete.

> [!NOTE]
> This method is only available in the main process.

Returns `Promise<SharedTextureImported>` - Resolves a [SharedTextureImported](structures/shared-texture-imported.md) when the import is complete.

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

Set a callback to receive imported shared textures from the main process.

> [!NOTE]
> This method is only available in the renderer process.

## Properties

The `sharedTexture` module has the following properties:

### `sharedTexture.subtle` _Experimental_

A [`SharedTextureSubtle`](structures/shared-texture-subtle.md) property, provides subtle APIs for interacting with shared texture for advanced users.
