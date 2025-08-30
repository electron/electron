# sharedTexture

> Import shared textures into Electron, converts platform specific handles into [`VideoFrame`](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame). Support all Web rendering systems, and can be transferred across Electron processes. Read [here](https://github.com/electron/electron/blob/main/shell/common/api/shared_texture/README.md) for more information.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Methods

The `sharedTexture` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in the future.

### `sharedTexture.importSharedTexture(options)` _Experimental_

* `options` Object - Options for importing shared texture.
  * `textureInfo` [SharedTextureImportTextureInfo](structures/shared-texture-import-texture-info.md) - The information of shared texture to import.
  * `allReferenceReleased` Function (optional) - Called when all references in all processes are released, you should keep native texture valid until this callback is called.

Imports the shared texture from the given options. A copy of the native texture will be made, so that you can release the native texture after the import is complete.

> [!NOTE]
> This method is only available in the main process.

Returns `SharedTextureImported` - The imported shared texture.

### `sharedTexture.sendSharedTexture(options, ...args)` _Experimental_

* `options` Object - Options for sending shared texture. One and only one of the `webContents` or `webFrameMain` should be provided as the target.
  * `webContents` [WebContents](web-contents.md) (optional) - The webContents to transfer the shared texture to.
  * `webFrameMain` [WebFrameMain](web-frame-main.md) (optional) - The webFrameMain to transfer the shared texture to. You'll need to enable `nodeIntegrationInSubFrames` for this, since this feature requires [IPC](https://www.electronjs.org/docs/latest/api/web-frame-main#frameipc-readonly) between main and the frame.
  * `importedSharedTexture` [SharedTextureImported](structures/shared-texture-imported.md) - The imported shared texture.
* `...args` any[] - Additional arguments to pass to the renderer process.

Send the imported shared texture to a renderer process. You must register receiver at renderer process before calling this method. This method has a 1000ms timeout, ensure the receiver is set and the renderer process is alive before calling this method.

> [!NOTE]
> This method is only available in the main process.

Returns `Promise<void>` - Resolves when the transfer is complete.

### `sharedTexture.setSharedTextureReceiver(callback)` _Experimental_

* `callback` Function\<Promise\<void\>\> - The function to receive the imported shared texture.
  * `receivedSharedTextureData` Object - The data received from the main process.
    * `importedSharedTexture` [SharedTextureImported](structures/shared-texture-imported.md) - The imported shared texture.
  * `...args` any[] - Additional arguments passed from the main process.

Set a callback to receive imported shared textures from the main process.

> [!NOTE]
> This method is only available in the renderer process.

## Properties

The `sharedTexture` module has the following properties:

### `sharedTexture.subtle` _Experimental_

A [`SharedTextureSubtle`](structures/shared-texture-subtle.md) property, provides subtle APIs for interacting with shared texture for advanced users.
