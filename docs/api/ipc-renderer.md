# ipc (renderer)

The `ipc` module provides a few methods so you can send synchronous and
asynchronous messages from the render process (web page) to the main process. You can also receive messages returned from the main process.

**Note**: If you want to make use of modules in the main process from the renderer
process, you might consider using the [remote](remote.md) module.

See [ipc (main process)](ipc-main-process.md) for code examples.

## Methods

The `ipc` module has the following methods for sending messages:

**Note**: When using these methods to send a `message` you must also listen for it in the main process with [`ipc (main process)`](ipc-main-process.md).

### `ipc.send(channel[, message])`

* `channel` String - The event name.
* `message` (optional)

Send a `message` (any type) to the main process asynchronously via a `channel`. The main process handles it by listening for the `channel` event with `ipc`.

### `ipc.sendSync(channel[, message])`

* `channel` String - The event name.
* `message` (optional)

Send a `message` (any type) to the main process synchronously via a `channel`. A result is returned from the main process.

The main process handles it by listening for the `channel` event with `ipc` and replies by setting the `event.returnValue`.

**Note:** Sending a synchronous message will block the whole renderer process so
using this method is not recommended.

### `ipc.sendToHost(channel[, message])`

* `channel` String - The event name.
* `message` (optional)

Like `ipc.send` but the message will be sent to the host page in a `<webview>` instead of the main process.
