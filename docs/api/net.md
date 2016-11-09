# net

> Issue HTTP/HTTPS requests using Chromium's native networking library

Process: [Main](../tutorial/quick-start.md#main-process)

The `net` module is a client-side API for issuing HTTP(S) requests. It is
similar to the [HTTP](https://nodejs.org/api/http.html) and
[HTTPS](https://nodejs.org/api/https.html) modules of Node.js but uses
Chromium's native networking library instead of the Node.js implementation,
offering better support for web proxies.

The following is a non-exhaustive list of why you may consider using the `net`
module instead of the native Node.js modules:

* Automatic management of system proxy configuration, support of the wpad
  protocol and proxy pac configuration files.
* Automatic tunneling of HTTPS requests.
* Support for authenticating proxies using basic, digest, NTLM, Kerberos or
  negotiate authentication schemes.
* Support for traffic monitoring proxies: Fiddler-like proxies used for access
  control and monitoring.

The `net` module API has been specifically designed to mimic, as closely as
possible, the familiar Node.js API. The API components including classes,
methods, properties and event names are similar to those commonly used in
Node.js.

For instance, the following example quickly shows how the `net` API might be
used:

```javascript
const {app} = require('electron')
app.on('ready', () => {
  const {net} = require('electron')
  const request = net.request('https://github.com')
  request.on('response', (response) => {
    console.log(`STATUS: ${response.statusCode}`)
    console.log(`HEADERS: ${JSON.stringify(response.headers)}`)
    response.on('data', (chunk) => {
      console.log(`BODY: ${chunk}`)
    })
    response.on('end', () => {
      console.log('No more data in response.')
    })
  })
  request.end()
})
```

By the way, it is almost identical to how you would normally use the
[HTTP](https://nodejs.org/api/http.html)/[HTTPS](https://nodejs.org/api/https.html)
modules of Node.js

The `net` API can be used only after the application emits the `ready` event.
Trying to use the module before the `ready` event will throw an error.

## Methods

The `net` module has the following methods:

### `net.request(options)`

* `options` (Object | String) - The `ClientRequest` constructor options.

Returns `ClientRequest`

Creates a `ClientRequest` instance using the provided `options` which are
directly forwarded to the `ClientRequest` constructor. The `net.request` method
would be used to issue both secure and insecure HTTP requests according to the
specified protocol scheme in the `options` object.

## Class: ClientRequest

> Make HTTP/HTTPS requests.

Process: [Main](../tutorial/quick-start.md#main-process)

`ClientRequest` implements the [Writable Stream](https://nodejs.org/api/stream.html#stream_writable_streams)
interface and is therefore an [EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter).

### `new ClientRequest(options)`

* `options` (Object | String) - If `options` is a String, it is interpreted as
  the request URL. If it is an object, it is expected to fully specify an HTTP
  request via the following properties:
  * `method` String (optional) - The HTTP request method. Defaults to the GET
    method.
  * `url` String (optional) - The request URL. Must be provided in the absolute
    form with the protocol scheme specified as http or https.
  * `session` Object (optional) - The [`Session`](session.md) instance with
    which the request is associated.
  * `partition` String (optional) - The name of the [`partition`](session.md)
    with which the request is associated. Defaults to the empty string. The
    `session` option prevails on `partition`. Thus if a `session` is explicitly
    specified, `partition` is ignored.
  * `protocol` String (optional) - The protocol scheme in the form 'scheme:'.
    Currently supported values are 'http:' or 'https:'. Defaults to 'http:'.
  * `host` String (optional) - The server host provided as a concatenation of
    the hostname and the port number 'hostname:port'
  * `hostname` String (optional) - The server host name.
  * `port` Integer (optional) - The server's listening port number.
  * `path` String (optional) - The path part of the request URL.

`options` properties such as `protocol`, `host`, `hostname`, `port` and `path`
strictly follow the Node.js model as described in the
[URL](https://nodejs.org/api/url.html) module.

For instance, we could have created the same request to 'github.com' as follows:

```javascript
const request = net.request({
  method: 'GET',
  protocol: 'https:',
  hostname: 'github.com',
  port: 443,
  path: '/'
})
```

### Instance Events

#### Event: 'response'

Returns:

* `response` IncomingMessage - An object representing the HTTP response message.

#### Event: 'login'

Returns:

* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

Emitted when an authenticating proxy is asking for user credentials.

The `callback` function is expected to be called back with user credentials:

* `username` String
* `password` String

```javascript
request.on('login', (authInfo, callback) => {
  callback('username', 'password')
})
```

Providing empty credentials will cancel the request and report an authentication
error on the response object:

```javascript
request.on('response', (response) => {
  console.log(`STATUS: ${response.statusCode}`)
  response.on('error', (error) => {
    console.log(`ERROR: ${JSON.stringify(error)}`)
  })
})
request.on('login', (authInfo, callback) => {
  callback()
})
```

#### Event: 'finish'

Emitted just after the last chunk of the `request`'s data has been written into
the `request` object.

#### Event: 'abort'

Emitted when the `request` is aborted. The `abort` event will not be fired if
the `request` is already closed.

#### Event: 'error'

Returns:

* `error` Error - an error object providing some information about the failure.

Emitted when the `net` module fails to issue a network request. Typically when
the `request` object emits an `error` event, a `close` event will subsequently
follow and no response object will be provided.

#### Event: 'close'

Emitted as the last event in the HTTP request-response transaction. The `close`
event indicates that no more events will be emitted on either the `request` or
`response` objects.

### Instance Properties

#### `request.chunkedEncoding`

A Boolean specifying whether the request will use HTTP chunked transfer encoding
or not. Defaults to false. The property is readable and writable, however it can
be set only before the first write operation as the HTTP headers are not yet put
on the wire. Trying to set the `chunkedEncoding` property after the first write
will throw an error.

Using chunked encoding is strongly recommended if you need to send a large
request body as data will be streamed in small chunks instead of being
internally buffered inside Electron process memory.

### Instance Methods

#### `request.setHeader(name, value)`

* `name` String - An extra HTTP header name.
* `value` String - An extra HTTP header value.

Adds an extra HTTP header. The header name will issued as it is without
lowercasing. It can be called only before first write. Calling this method after
the first write will throw an error.

#### `request.getHeader(name)`

* `name` String - Specify an extra header name.

Returns String - The value of a previously set extra header name.

#### `request.removeHeader(name)`

* `name` String - Specify an extra header name.

Removes a previously set extra header name. This method can be called only
before first write. Trying to call it after the first write will throw an error.

#### `request.write(chunk[, encoding][, callback])`

* `chunk` (String | Buffer) - A chunk of the request body's data. If it is a
  string, it is converted into a Buffer using the specified encoding.
* `encoding` String (optional) - Used to convert string chunks into Buffer
  objects. Defaults to 'utf-8'.
* `callback` Function (optional) - Called after the write operation ends.

`callback` is essentially a dummy function introduced in the purpose of keeping
similarity with the Node.js API. It is called asynchronously in the next tick
after `chunk` content have been delivered to the Chromium networking layer.
Contrary to the Node.js implementation, it is not guaranteed that `chunk`
content have been flushed on the wire before `callback` is called.

Adds a chunk of data to the request body. The first write operation may cause
the request headers to be issued on the wire. After the first write operation,
it is not allowed to add or remove a custom header.

#### `request.end([chunk][, encoding][, callback])`

* `chunk` (String | Buffer) (optional)
* `encoding` String (optional)
* `callback` Function (optional)

Sends the last chunk of the request data. Subsequent write or end operations
will not be allowed. The `finish` event is emitted just after the end operation.

#### `request.abort()`

Cancels an ongoing HTTP transaction. If the request has already emitted the
`close` event, the abort operation will have no effect. Otherwise an ongoing
event will emit `abort` and `close` events. Additionally, if there is an ongoing
response object,it will emit the `aborted` event.

## Class: IncomingMessage

> Handle responses to HTTP/HTTPS requests.

Process: [Main](../tutorial/quick-start.md#main-process)

`IncomingMessage` implements the [Readable Stream](https://nodejs.org/api/stream.html#stream_readable_streams)
interface and is therefore an [EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter).

### Instance Events

#### Event: 'data'

Returns:

* `chunk` Buffer - A chunk of response body's data.

The `data` event is the usual method of transferring response data into
applicative code.

#### Event: 'end'

Indicates that response body has ended.

#### Event: 'aborted'

Emitted when a request has been canceled during an ongoing HTTP transaction.

#### Event: 'error'

Returns:

`error` Error - Typically holds an error string identifying failure root cause.

Emitted when an error was encountered while streaming response data events. For
instance, if the server closes the underlying while the response is still
streaming, an `error` event will be emitted on the response object and a `close`
event will subsequently follow on the request object.

### Instance Properties

An `IncomingMessage` instance has the following readable properties:

#### `response.statusCode`

An Integer indicating the HTTP response status code.

#### `response.statusMessage`

A String representing the HTTP status message.

#### `response.headers`

An Object representing the response HTTP headers. The `headers` object is
formatted as follows:

* All header names are lowercased.
* Each header name produces an array-valued property on the headers object.
* Each header value is pushed into the array associated with its header name.

#### `response.httpVersion`

A String indicating the HTTP protocol version number. Typical values are '1.0'
or '1.1'. Additionally `httpVersionMajor` and `httpVersionMinor` are two
Integer-valued readable properties that return respectively the HTTP major and
minor version numbers.
