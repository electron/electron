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

### `net.request`

## Class: ClientRequest

### Instance Events

#### Event: 'response'

#### Event: 'login'

#### Event: 'finish'

#### Event: 'abort'

#### Event: 'error'

#### Event: 'close'

### Instance Methods

#### `request.setHeader(name, value)`

#### `request.getHeader(name)`

#### `request.removeHeader(name)`

#### `request.write(chunk, [encoding], [callback])`

#### `request.end(chunk, [encoding], [callback])`

#### `request.abort()`

### Instance Properties

#### `request.chunkedEncoding`

## Class: IncomingMessage

#### Event:





