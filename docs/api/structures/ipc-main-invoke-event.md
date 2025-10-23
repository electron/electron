# IpcMainInvokeEvent Object extends `Event`

* `type` String - Possible values include `frame`
* `processId` Integer - The internal ID of the renderer process that sent this message
* `frameId` Integer - The ID of the renderer frame that sent this message
* `sender` [WebContents](../web-contents.md) - Returns the `webContents` that sent the message
* `senderFrame` [WebFrameMain](../web-frame-main.md) | null _Readonly_ - The frame that sent this message. May be `null` if accessed after the frame has either navigated or been destroyed.
