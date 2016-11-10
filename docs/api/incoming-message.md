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
