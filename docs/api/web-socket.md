## Class: WebSocket extends `EventTarget`

> Create WebSocket connections from the main process using Chromium's native networking library

Process: [Main](../glossary.md#main-process)

`net.WebSocket` is a drop-in replacement for the [WHATWG `WebSocket`](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
interface that routes the connection through Chromium's network stack rather
than Node.js. Use it when you want a main-process WebSocket connection that:

* Uses the system or session proxy configuration (PAC, WPAD, authenticated proxies).
* Validates TLS certificates against the platform trust store and the session's
  certificate verification policy.
* Honors session-level configuration (custom CA, host resolution rules, etc.).
* Sends the session's cookies (when [`useSessionCookies`](#new-websocketurl-protocols) is enabled).

The class implements the standard [`WebSocket` interface](https://websockets.spec.whatwg.org/#the-websocket-interface)
(an [`EventTarget`](https://nodejs.org/api/events.html#class-eventtarget)), so
code written against the browser or Node.js global `WebSocket` works without
changes:

```js
const { app, net } = require('electron')

app.whenReady().then(() => {
  const ws = new net.WebSocket('wss://echo.websocket.events')
  ws.onopen = () => ws.send('hello')
  ws.onmessage = (event) => {
    console.log('received', event.data)
    ws.close()
  }
})
```

`net.WebSocket` can only be used after the application emits the `ready` event.

### `new WebSocket(url[, protocols])`

* `url` string - The URL to connect to. The scheme must be `ws:` or `wss:`
  (`http:` and `https:` are accepted and rewritten to their WebSocket
  equivalents, as in the browser).
* `protocols` string | string[] | [WebSocketOptions](structures/web-socket-options.md) (optional) -
  One or more WebSocket subprotocols, or an Electron-specific options object.

Passing an options object as the second argument is an Electron extension; the
two-argument form `new net.WebSocket(url, protocols)` is fully compatible with
the [WHATWG constructor](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/WebSocket).

### Static Properties

#### `WebSocket.CONNECTING` _Readonly_

An `Integer` constant equal to `0`, the `readyState` value while the opening
handshake is in progress.

#### `WebSocket.OPEN` _Readonly_

An `Integer` constant equal to `1`, the `readyState` value once the connection
is established.

#### `WebSocket.CLOSING` _Readonly_

An `Integer` constant equal to `2`, the `readyState` value while the closing
handshake is in progress.

#### `WebSocket.CLOSED` _Readonly_

An `Integer` constant equal to `3`, the `readyState` value once the connection
is closed.

### Instance Properties

#### `ws.url` _Readonly_

A `string` representing the resolved URL of the connection.

#### `ws.readyState` _Readonly_

An `Integer` representing the current state of the connection: one of
`WebSocket.CONNECTING` (`0`), `WebSocket.OPEN` (`1`), `WebSocket.CLOSING`
(`2`), or `WebSocket.CLOSED` (`3`).

#### `ws.bufferedAmount` _Readonly_

An `Integer` representing the number of bytes of application data that have
been queued via `send()` but not yet handed off to the network.

#### `ws.protocol` _Readonly_

A `string` containing the subprotocol selected by the server. The empty string
until the connection is open or if the server did not select a subprotocol.

#### `ws.extensions` _Readonly_

A `string` containing the extensions negotiated by the server (for example
`permessage-deflate`).

#### `ws.binaryType`

A `string` controlling how incoming binary messages are exposed on the
`message` event. Can be `nodebuffer`, `arraybuffer`, or `blob`. The default is
`nodebuffer`.

`'nodebuffer'` is an Electron extension that delivers binary messages as
[`Buffer`](https://nodejs.org/api/buffer.html) objects, which is generally
the most convenient representation in the main process. Set `binaryType` to
`'arraybuffer'` or `'blob'` for behavior identical to the renderer
`WebSocket`.

#### `ws.onopen`

A `Function | null` event handler for the `open` event. Equivalent to calling
`addEventListener('open', ...)`.

#### `ws.onmessage`

A `Function | null` event handler for the `message` event. Equivalent to
calling `addEventListener('message', ...)`.

#### `ws.onerror`

A `Function | null` event handler for the `error` event. Equivalent to calling
`addEventListener('error', ...)`.

#### `ws.onclose`

A `Function | null` event handler for the `close` event. Equivalent to calling
`addEventListener('close', ...)`.

### Instance Methods

#### `ws.send(data)`

* `data` string | ArrayBufferLike | ArrayBufferView | Blob - The
  data to send. Strings are sent as text frames; everything else is sent as a
  binary frame.

Enqueues `data` to be transmitted to the server. Throws an
`InvalidStateError` `DOMException` if `readyState` is `CONNECTING`.

#### `ws.close([code][, reason])`

* `code` Integer (optional) - The WebSocket close code. Must be `1000` or in
  the range `3000`–`4999`.
* `reason` string (optional) - A human-readable close reason. Must encode to
  no more than 123 bytes of UTF-8.

Closes the connection. Calling `close()` while still `CONNECTING` aborts the
handshake.

### Events

`net.WebSocket` is an [`EventTarget`](https://nodejs.org/api/events.html#class-eventtarget),
not an `EventEmitter`. Listen with `addEventListener()` or the corresponding
`on*` event handler property:

* `open` - Emitted when the connection is established and the opening handshake
  has completed. After this event, `protocol` and `extensions` reflect the
  values negotiated with the server.
* `message` - Emitted with a [`MessageEvent`](https://developer.mozilla.org/en-US/docs/Web/API/MessageEvent)
  when a message arrives. `event.data` is a `string` for text frames, or a
  `Buffer`, `ArrayBuffer`, or `Blob` (per [`binaryType`](#wsbinarytype)) for
  binary frames.
* `error` - Emitted when the connection fails. Always followed by a `close`
  event.
* `close` - Emitted with a [`CloseEvent`](https://developer.mozilla.org/en-US/docs/Web/API/CloseEvent)
  (`code`, `reason`, `wasClean`) when the connection is closed for any reason.
