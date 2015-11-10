# ipcRenderer

The `ipcRenderer` module provides a few methods so you can send synchronous and
asynchronous messages from the render process (web page) to the main process.
You can also receive replies from the main process.

See [ipcMain](ipc-main.md) for code examples.

## Listening for Messages

The `ipcRenderer` module has the following method to listen for events:

### `ipcRenderer.on(channel, callback)`

* `channel` String - The event name.
* `callback` Function

When the event occurs the `callback` is called with an `event` object and
arbitrary arguments.

## Sending Messages

The `ipcRenderer` module has the following methods for sending messages:

### `ipcRenderer.send(channel[, arg1][, arg2][, ...])`

* `channel` String - The event name.
* `arg` (optional)

Send an event to the main process asynchronously via a `channel`, you can also
send arbitrary arguments. The main process handles it by listening for the
`channel` event with `ipcMain`.

### `ipcRenderer.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String - The event name.
* `arg` (optional)

Send an event to the main process synchronously via a `channel`, you can also
send arbitrary arguments. The main process handles it by listening for the
`channel` event with `ipcMain`.

The main process handles it by listening for the `channel` event with `ipc` and
replies by setting the `event.returnValue`.

__Note:__ Sending a synchronous message will block the whole renderer process,
unless you know what you are doing you should never use it.

### `ipcRenderer.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String - The event name.
* `arg` (optional)

Like `ipcRenderer.send` but the event will be sent to the `<webview>` element in
the host page instead of the main process.
