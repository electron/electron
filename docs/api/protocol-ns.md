# protocol (NetworkService)

This document describes the new protocol APIs based on NetworkService.

Currently we don't have an estimate of when will we enable NetworkService by
default in Electron, but as Chromium is already removing non-NetworkService
code, the switch may come after a few major version releases.

The content of this document should be moved to `protocol.md` after it is
determined when to enable NetworkService in Electron.

> Register a custom protocol and intercept existing protocol requests.

Process: [Main](../glossary.md#main-process)

An example of implementing a protocol that has the same effect as the
`file://` protocol:

```javascript
const { app, protocol } = require('electron')
const path = require('path')

app.on('ready', () => {
  protocol.registerProtocol('atom', (request, callback) => {
    const url = request.url.substr(7)
    callback('file', { path: path.normalize(`${__dirname}/${url}`) })
  })
})
```

**Note:** All methods unless specified can only be used after the `ready` event
of the `app` module gets emitted.

## Using `protocol` with a custom `partition` or `session`

A protocol is registered to a specific Electron [`session`](./session.md)
object. If you don't specify a session, then your `protocol` will be applied to
the default session that Electron uses. However, if you define a `partition` or
`session` on your `browserWindow`'s `webPreferences`, then that window will use
a different session and your custom protocol will not work if you just use
`electron.protocol.XXX`.

To have your custom protocol work in combination with a custom session, you need
to register it to that session explicitly.

```javascript
const { session, app, protocol } = require('electron')
const path = require('path')

app.on('ready', () => {
  const partition = 'persist:example'
  const ses = session.fromPartition(partition)

  ses.protocol.registerProtocol('atom', (request, callback) => {
    const url = request.url.substr(7)
    callback('file', { path: path.normalize(`${__dirname}/${url}`) })
  })

  mainWindow = new BrowserWindow({ webPreferences: { partition } })
})
```

## Methods

The `protocol` module has the following methods:

### `protocol.registerSchemesAsPrivileged(customSchemes)`

* `customSchemes` [CustomScheme[]](structures/custom-scheme.md)

**Note:** This method can only be used before the `ready` event of the `app`
module gets emitted and can be called only once.

Registers the `scheme` as standard, secure, bypasses content security policy for
resources, allows registering ServiceWorker and supports fetch API. Specify a
privilege with the value of `true` to enable the capability.

An example of registering a privileged scheme, with bypassing Content Security
Policy:

```javascript
const { protocol } = require('electron')
protocol.registerSchemesAsPrivileged([
  { scheme: 'foo', privileges: { bypassCSP: true } }
])
```

A standard scheme adheres to what RFC 3986 calls [generic URI
syntax](https://tools.ietf.org/html/rfc3986#section-3). For example `http` and
`https` are standard schemes, while `file` is not.

Registering a scheme as standard, will allow relative and absolute resources to
be resolved correctly when served. Otherwise the scheme will behave like the
`file` protocol, but without the ability to resolve relative URLs.

For example when you load following page with custom protocol without
registering it as standard scheme, the image will not be loaded because
non-standard schemes can not recognize relative URLs:

```html
<body>
  <img src='test.png'>
</body>
```

Registering a scheme as standard will allow access to files through the
[FileSystem API][file-system-api]. Otherwise the renderer will throw a security
error for the scheme.

By default web storage apis (localStorage, sessionStorage, webSQL, indexedDB,
cookies) are disabled for non standard schemes. So in general if you want to
register a custom protocol to replace the `http` protocol, you have to register
it as a standard scheme.

### `protocol.registerFileProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `filePath` String (optional)

Registers a protocol of `scheme` that will send the file as a response. The
`handler` will be called with `handler(request, callback)` when a `request` is
going to be created with `scheme`.

To handle the `request`, the `callback` should be called with either the file's
path or an object that has a `path` property, e.g. `callback(filePath)` or
`callback({ path: filePath })`. The object may also have a `headers` property
which gives a map of headers to values for the response headers, e.g.
`callback({ path: filePath, headers: {"Content-Security-Policy": "default-src
'none'"]})`.

When `callback` is called with nothing, a number, or an object that has an
`error` property, the `request` will fail with the `error` number you
specified. For the available error numbers you can use, please see the
[net error list][net-error].

By default the `scheme` is treated like `http:`, which is parsed differently
than protocols that follow the "generic URI syntax" like `file:`.

### `protocol.registerBufferProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `buffer` (Buffer | [MimeTypedBuffer](structures/mime-typed-buffer.md)) (optional)

Registers a protocol of `scheme` that will send a `Buffer` as a response.

The usage is the same with `registerFileProtocol`, except that the `callback`
should be called with either a `Buffer` object or an object that has the `data`,
`mimeType`, and `charset` properties.

Example:

```javascript
protocol.registerBufferProtocol('atom', (request, callback) => {
  callback({ mimeType: 'text/html', data: Buffer.from('<h5>Response</h5>') })
})
```

### `protocol.registerStringProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `data` String (optional)

Registers a protocol of `scheme` that will send a `String` as a response.

The usage is the same with `registerFileProtocol`, except that the `callback`
should be called with either a `String` or an object that has the `data`,
`mimeType`, and `charset` properties.

### `protocol.registerHttpProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `redirectRequest` Object
      * `url` String
      * `method` String
      * `session` Object (optional)
      * `uploadData` Object (optional)
        * `contentType` String - MIME type of the content.
        * `data` String - Content to be sent.

Registers a protocol of `scheme` that will send an HTTP request as a response.

The usage is the same with `registerFileProtocol`, except that the `callback`
should be called with a `redirectRequest` object that has the `url`, `method`,
`referrer`, `uploadData` and `session` properties.

By default the HTTP request will reuse the current session. If you want the
request to have a different session you should set `session` to `null`.

For POST requests the `uploadData` object must be provided.

### `protocol.registerStreamProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `stream` (ReadableStream | [StreamProtocolResponse](structures/stream-protocol-response.md)) (optional)

Registers a protocol of `scheme` that will send a `Readable` as a response.

The usage is similar to the other `register{Any}Protocol`, except that the
`callback` should be called with either a `Readable` object or an object that
has the `data`, `statusCode`, and `headers` properties.

Example:

```javascript
const { protocol } = require('electron')
const { PassThrough } = require('stream')

function createStream (text) {
  const rv = new PassThrough() // PassThrough is also a Readable stream
  rv.push(text)
  rv.push(null)
  return rv
}

protocol.registerStreamProtocol('atom', (request, callback) => {
  callback({
    statusCode: 200,
    headers: {
      'content-type': 'text/html'
    },
    data: createStream('<h5>Response</h5>')
  })
})
```

It is possible to pass any object that implements the readable stream API (emits
`data`/`end`/`error` events). For example, here's how a file could be returned:

```javascript
protocol.registerStreamProtocol('atom', (request, callback) => {
  callback(fs.createReadStream('index.html'))
})
```

### `protocol.unregisterProtocol(scheme)`

* `scheme` String

Unregisters the custom protocol of `scheme`.

### `protocol.isProtocolRegistered(scheme)`

* `scheme` String

Returns `Boolean` - Whether `scheme` is already registered.

### `protocol.interceptFileProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `filePath` String

Intercepts `scheme` protocol and uses `handler` as the protocol's new handler
which sends a file as a response.

### `protocol.interceptStringProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `data` String (optional)

Intercepts `scheme` protocol and uses `handler` as the protocol's new handler
which sends a `String` as a response.

### `protocol.interceptBufferProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `buffer` Buffer (optional)

Intercepts `scheme` protocol and uses `handler` as the protocol's new handler
which sends a `Buffer` as a response.

### `protocol.interceptHttpProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `redirectRequest` Object
      * `url` String
      * `method` String
      * `session` Object (optional)
      * `uploadData` Object (optional)
        * `contentType` String - MIME type of the content.
        * `data` String - Content to be sent.

Intercepts `scheme` protocol and uses `handler` as the protocol's new handler
which sends a new HTTP request as a response.

### `protocol.interceptStreamProtocol(scheme, handler)`

* `scheme` String
* `handler` Function
  * `request` ProtocolRequest
  * `callback` Function
    * `stream` (ReadableStream | [StreamProtocolResponse](structures/stream-protocol-response.md)) (optional)

Same as `protocol.registerStreamProtocol`, except that it replaces an existing
protocol handler.

### `protocol.uninterceptProtocol(scheme)`

* `scheme` String

Remove the interceptor installed for `scheme` and restore its original handler.

### `protocol.isProtocolIntercepted(scheme)`

* `scheme` String

Returns `Boolean` - Whether `scheme` is already intercepted.

[net-error]: https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h
[file-system-api]: https://developer.mozilla.org/en-US/docs/Web/API/LocalFileSystem
