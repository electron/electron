# ipc (renderer)

The `ipc` module provides a few methods so you can send synchronous and
asynchronous messages to the browser, and also receive messages sent from
browser. If you want to make use of modules of browser from renderer, you
might consider using the [remote](remote.md) module.

See [ipc (browser)](ipc-browser.md) for examples.

## ipc.send(channel[, args...])

Send `args..` to the web page via `channel` in asynchronous message, the browser
process can handle it by listening to the `channel` event of `ipc` module.

## ipc.sendSync(channel[, args...])

Send `args..` to the web page via `channel` in synchronous message, and returns
the result sent from browser. The browser process can handle it by listening to
the `channel` event of `ipc` module, and returns by setting `event.returnValue`.

**Note:** Usually developers should never use this API, since sending
synchronous message would block the whole web page.

## ipc.sendToHost(channel[, args...])

Like `ipc.send` but the message will be sent to the host page instead of the
browser process.

This is mainly used by the page in `<webview>` to communicate with host page.
