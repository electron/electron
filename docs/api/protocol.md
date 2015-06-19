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
    });
});
```

**Note:** This module can only be used after the `ready` event
was emitted.

## protocol.registerProtocol(scheme, handler)

* `scheme` String
* `handler` Function

Registers a custom protocol of `scheme`, the `handler` would be called with
`handler(request)` when the a request with registered `scheme` is made.

You need to return a request job in the `handler` to specify which type of
response you would like to send.

## protocol.unregisterProtocol(scheme)

* `scheme` String

Unregisters the custom protocol of `scheme`.

## protocol.registerStandardSchemes(value)

* `value` Array

`value` is an array of custom schemes to be registered to the standard.

## protocol.isHandledProtocol(scheme)

* `scheme` String

Returns whether the `scheme` can be handled already.

## protocol.interceptProtocol(scheme, handler)

* `scheme` String
* `handler` Function

Intercepts an existing protocol with `scheme`, returning `null` or `undefined`
in `handler` would use the original protocol handler to handle the request.

## protocol.uninterceptProtocol(scheme)

* `scheme` String

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
