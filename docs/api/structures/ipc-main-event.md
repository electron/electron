# IpcMainEvent Object extends `Event`

* `type` String - Possible values include `frame`
* `processId` Integer - The internal ID of the renderer process that sent this message
* `frameId` Integer - The ID of the renderer frame that sent this message
* `returnValue` any - Set this to the value to be returned in a synchronous message
* `sender` [WebContents](../web-contents.md) - Returns the `webContents` that sent the message
* `senderFrame` [WebFrameMain](../web-frame-main.md) | null _Readonly_ - The frame that sent this message. May be `null` if accessed after the frame has either navigated or been destroyed.
* `ports` [MessagePortMain](../message-port-main.md)[] - A list of MessagePorts that were transferred with this message
* `reply` Function - A function that will send an IPC message to the renderer frame that sent the original message that you are currently handling.  You should use this method to "reply" to the sent message in order to guarantee the reply will go to the correct process and frame.
  * `channel` string
  * `...args` any[]
