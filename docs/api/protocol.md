# protocol

The `protocol` module can register a new protocol or intercept an existing
protocol, so you can customize the response to the requests for various protocols.

An example of implementing a protocol that has the same effect with the
`file://` protocol:

```javascript
var app = require('app');
var path = require('path');

app.on('ready', function() {
    var protocol = require('protocol');
    protocol.registerProtocol('atom', function(request) {
      var url = request.url.substr(7)
      return new protocol.RequestFileJob(path.normalize(__dirname + '/' + url));
    }, function (error, scheme) {
      if (!error)
        console.log(scheme, ' registered successfully')
    });
});
```

**Note:** This module can only be used after the `ready` event
was emitted.

## protocol.registerProtocol(scheme, handler, callback)

* `scheme` String
* `handler` Function
* `callback` Function

Registers a custom protocol of `scheme`, the `handler` would be called with
`handler(request)` when the a request with registered `scheme` is made.

You need to return a request job in the `handler` to specify which type of
response you would like to send.

By default the scheme is treated like `http:`, which is parsed differently
from protocols that follows "generic URI syntax" like `file:`, so you probably
want to call `protocol.registerStandardSchemes` to make your scheme treated as
standard scheme.

## protocol.unregisterProtocol(scheme, callback)

* `scheme` String
* `callback` Function

Unregisters the custom protocol of `scheme`.

## protocol.registerStandardSchemes(value)

* `value` Array

`value` is an array of custom schemes to be registered as standard schemes.

A standard scheme adheres to what RFC 3986 calls
[generic URI syntax](https://tools.ietf.org/html/rfc3986#section-3). This
includes `file:` and `filesystem:`.

## protocol.isHandledProtocol(scheme, callback)

* `scheme` String
* `callback` Function

`callback` returns a boolean whether the `scheme` can be handled already.

## protocol.interceptProtocol(scheme, handler, callback)

* `scheme` String
* `handler` Function
* `callback` Function

Intercepts an existing protocol with `scheme`, returning `null` or `undefined`
in `handler` would use the original protocol handler to handle the request.

## protocol.uninterceptProtocol(scheme, callback)

* `scheme` String
* `callback` Function

Unintercepts a protocol.

## Class: protocol.RequestFileJob(path)

* `path` String

Create a request job which would query a file of `path` and set corresponding
mime types.

## Class: protocol.RequestStringJob(options)

* `options` Object
  * `mimeType` String - Default is `text/plain`
  * `charset` String - Default is `UTF-8`
  * `data` String

Create a request job which sends a string as response.

## Class: protocol.RequestBufferJob(options)

* `options` Object
  * `mimeType` String - Default is `application/octet-stream`
  * `encoding` String - Default is `UTF-8`
  * `data` Buffer

Create a request job which sends a buffer as response.

## Class: protocol.RequestHttpJob(options)

* `options` Object
  * `session` [Session](browser-window.md#class-session) - By default it is
    the app's default session, setting it to `null` will create a new session
    for the requests
  * `url` String
  * `method` String - Default is `GET`
  * `referrer` String

Send a request to `url` and pipe the response back.

## Class: protocol.RequestErrorJob(code)

* `code` Integer

Create a request job which sets appropriate network error message to console.
Default message is `net::ERR_NOT_IMPLEMENTED`. Code should be in the following
range.

* Ranges:
  * 0- 99 System related errors
  * 100-199 Connection related errors
  * 200-299 Certificate errors
  * 300-399 HTTP errors
  * 400-499 Cache errors
  * 500-599 ?
  * 600-699 FTP errors
  * 700-799 Certificate manager errors
  * 800-899 DNS resolver errors

Check the [network error list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h) for code and message relations.
