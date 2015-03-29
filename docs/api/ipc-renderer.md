# ipc (renderer)

The `ipc` module provides a few methods so you can send synchronous and
asynchronous messages to the main process, and also receive messages sent from
main process. If you want to make use of modules of main process from renderer
process, you might consider using the [remote](remote.md) module.

See [ipc (main process)](ipc-main-process.md) for examples.

## ipc.send(channel[, args...])

Send `args..` to the renderer via `channel` in asynchronous message, the main
process can handle it by listening to the `channel` event of `ipc` module.

## ipc.sendSync(channel[, args...])

Send `args..` to the renderer via `channel` in synchronous message, and returns
the result sent from main process. The main process can handle it by listening to
the `channel` event of `ipc` module, and returns by setting `event.returnValue`.

**Note:** Usually developers should never use this API, since sending
synchronous message would block the whole renderer process.

## ipc.sendToHost(channel[, args...])

Like `ipc.send` but the message will be sent to the host page instead of the
main process.

This is mainly used by the page in `<webview>` to communicate with host page.
