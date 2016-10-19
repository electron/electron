# net

> Issue HTTP/HTTPS requests

The `net` module is a client-side API for issuing HTTP requests. It is similar to the [HTTP](https://nodejs.org/api/http.html) and [HTTPS](https://nodejs.org/api/https.html) modules of Node.js but uses Chromium networking library instead of the Node.js stack offering therefore a much grater support regarding web proxies.

Following is a non-exhaustive list of why you may need to use the `net` module instead of Node.js [HTTP](https://nodejs.org/api/http.html):
* Automatic management of system proxy configuration, support of the wpad protocol and proxy pac configuration files.
* Automatic tunneling of HTTPS requests.
* Support for authenticating proxies using basic, digest, NTLM, Kerberos or negotiate authentication schemes.
* Support for traffic monitoring proxies: Fiddler-like proxies used for access control and monitoring.

The following example quickly shows how the net API mgiht be used:
```javascript
const {app} = require('electron')

app.on('ready', () => {
const {net} = require('electron')
const request = net.request('https://github.com')
request.on('response', (response) => {
  console.log(`STATUS: ${response.statusCode}`);  
  console.log(`HEADERS: ${JSON.stringify(response.headers)}`);
  response.on('data', (chunk) => {
    console.log(`BODY: ${chunk}`)
  })
  response.on('end', () => {
    console.log('No more data in response.');
  })
})
request.end()
})
```

## Methods

The `net` module has the following methods:

### `net.request(options)`

Create a `ClientRequest` instance using the provided `options` object.

Returns `ClientRequest`

## Class: ClientRequest

### `new ClientRequest(options)`

* `options` Object or String - If `options` is a String, it is interpreted as the request URL. If it is an object, it is expected to fully specify an HTTP request via the following properties:
  * `method` String (optional) - The HTTP request method. Defaults to the GET method.
  * `url` String (required) - The request URL. Must be provided in the absolute form with the protocol scheme specified as http or https.
  * `protocol` String (optional) - The protocol scheme in the form 'scheme:'. Current supported values are 'http:' or 'https:'. Defaults to 'http:'.
  * `host` String (optional) - The server host provided as a concatenation of a hostname and a port number 'hostname:port'
  * `hostname` String (optional) - The server host name.
  * `port` Integer (optional) - The server's listening port number.
  * `path` String (optional) - The path part of the request URL. 
  
`options` properties `protocol`, `host`, `hostname`, `port` and `path` strictly follow the Node.js model as described in the [URL](https://nodejs.org/api/url.html) module.

### Instance Events

#### Event: 'response'

Returns:

* `response` IncomingMessage - An object representing the HTTP response message.

#### Event: 'login'

Returns:

* `callback` Function

Emitted when an authenticating proxy is asking for user credentials.

The `callback` function is expected to be called back with user credentials:

* `usrename` String
* `password` String

Providing empty credentials will cancel the request.

#### Event: 'finish'

Emitted just after the last chunk of the `request`'s data has been written into the `request` object.

#### Event: 'abort'

Emitted when the `request` is aborted. The abort event will not be fired if the `request` is already closed.

#### Event: 'error'

Returns:

* `error` Error - an error object providing some information about the failure.

Emitted when the `net` module fails to issue a network request. Typically when the `request` object emits an error event, a close event will subsequently follow and no response object will be provided.

#### Event: 'close'

Emitted as the last event in the HTTP request-response transaction. The close event indicates that no more events will be emitted on either the `request` or `response` objects.

### Instance Methods

#### `request.setHeader(name, value)`

* `name` String - An extra header name.
* `value` String - An extra header value.

Adds an extra HTTP header. The header name will issued as it is without lowercasing.

#### `request.getHeader(name)`

* `name` String - Specify an extra header name.

Returns String - The value of a previously set extra header name.

#### `request.removeHeader(name)`

* `name` String - Specify an extra header name.

Removes a previously set extra header name. 

#### `request.write(chunk, [encoding, callback])`

* `chunk` String or Buffer - A chunk of the request body' data. If it is a string, it is converted into a Buffer object using the specified encoding.
* `encoding` String (optional) - Used to convert string chunks into Buffer objects. Defaults to 'utf-8'.
* `callback` Function (optional) - Called after the write operation ends.

Adds a chunk of data to the request body. Generally, the first write operation causes the request headers to be issued on the wire. 
After the first write operation, it is not allowed to add or remove a custom header.

#### `request.end([chunk, encoding, callback])`

* `chunk` String or Buffer (optional)
* `encoding` String (optional)
* `callback` Function (optional)

Sends the last chunk of the request data. Subsequent write or end operations will not be allowed. The finish event is emitted just after the end operation.

#### `request.abort()`

Cancels an ongoing HTTP transaction. If the request has already closed, the abort operation will have no effect. 
Otherwise an ongoing event will emit abort and close events. Additionally, if there is an ongoing response object,
it will emit the aborted event.

### Instance Properties

#### `request.chunkedEncoding`

## Class: IncomingMessage

### Instance Events

#### Event 'data'

#### Event 'end'

#### Event 'aborted'

#### Event 'error'

### Instance properties

#### `response.statusCode`

#### `response.statusMessage`

#### `response.headers`

#### `response.httpVersion`








