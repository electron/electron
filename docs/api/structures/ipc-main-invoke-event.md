# IpcMainInvokeEvent Object extends `Event`

* `processId` Integer - The internal ID of the renderer process that sent this message
* `frameId` Integer - The ID of the renderer frame that sent this message
* `sender` [WebContents](../web-contents.md) - Returns the `webContents` that sent the message
* `senderFrame` [WebFrameMain](../web-frame-main.md) _Readonly_ - The frame that sent this message. If the initiating page had a cross-site navigation, this frame will be disposed and throw upon accessing its properties.
