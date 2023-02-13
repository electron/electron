# IpcMainInvokeEvent Object extends `Event`

* `processId` Integer - The internal ID of the renderer process that sent this message
* `frameId` Integer - The ID of the renderer frame that sent this message
* `sender` WebContents - Returns the `webContents` that sent the message
* `senderFrame` WebFrameMain _Readonly_ - The frame that sent this message
