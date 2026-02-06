# sharedTexture

> Import shared textures into Electron and converts platform specific handles into [`VideoFrame`](https://developer.mozilla.org/en-US/docs/Web/API/VideoFrame). Supports all Web rendering systems, and can be transferred across Electron processes. Read [here](https://github.com/electron/electron/blob/main/shell/common/api/shared_texture/README.md) for more information.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

## Methods

The `sharedTexture` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in the future.

### `sharedTexture.importSharedTexture(options)` _Experimental_

* `options` Object - Options for importing shared textures.
  * `textureInfo` [SharedTextureImportTextureInfo](structures/shared-texture-import-texture-info.md) - The information of the shared texture to import.
  * `allReferencesReleased` Function (optional) - Called when all references in all processes are released. You should keep the imported texture valid until this callback is called.

Imports the shared texture from the given options.

> [!NOTE]
> This method is only available in the main process.

Returns [`SharedTextureImported`](structures/shared-texture-imported.md) - The imported shared texture.

### `sharedTexture.sendSharedTexture(options, ...args)` _Experimental_

* `options` Object - Options for sending shared texture.
  * `frame` [WebFrameMain](web-frame-main.md) - The target frame to transfer the shared texture to. For `WebContents`, you can pass `webContents.mainFrame`. If you provide a `webFrameMain` that is not a main frame, you'll need to enable `webPreferences.nodeIntegrationInSubFrames` for this, since this feature requires [IPC](https://www.electronjs.org/docs/latest/api/web-frame-main#frameipc-readonly) between main and the frame.
  * `importedSharedTexture` [SharedTextureImported](structures/shared-texture-imported.md) - The imported shared texture.
* `...args` any[] - Additional arguments to pass to the renderer process.

Send the imported shared texture to a renderer process. You must register a receiver at renderer process before calling this method. This method has a 1000ms timeout. Ensure the receiver is set and the renderer process is alive before calling this method.

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
