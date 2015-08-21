# protocol

The `protocol` module can register a custom protocol or intercept an existing
protocol.

An example of implementing a protocol that has the same effect with the
`file://` protocol:

```javascript
var app = require('app');
var path = require('path');

app.on('ready', function() {
    var protocol = require('protocol');
    protocol.registerFileProtocol('atom', function(request, callback) {
      var url = request.url.substr(7);
      callback({path: path.normalize(__dirname + '/' + url)});
    }, function (error) {
      if (error)
        console.error('Failed to register protocol')
    });
});
```

**Note:** This module can only be used after the `ready` event was emitted.

## protocol.registerStandardSchemes(schemes)

* `schemes` Array - Custom schemes to be registered as standard schemes.

A standard scheme adheres to what RFC 3986 calls
[generic URI syntax](https://tools.ietf.org/html/rfc3986#section-3). This
includes `file:` and `filesystem:`.

## protocol.registerFileProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Registers a protocol of `scheme` that will send file as response, the `handler`
will be called with `handler(request, callback)` when a `request` is going to be
created with `scheme`, and `completion` will be called with `completion(null)`
when `scheme` is successfully registered, or `completion(error)` when failed.

To handle the `request`, the `callback` should be called with either file's path
or an object that has `path` property, e.g. `callback(filePath)` or
`callback({path: filePath})`.

When `callback` is called with nothing, or a number, or an object that has
`error` property, the `request` will be failed with the `error` number you
specified. For the available error numbers you can use, please check:
https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h

By default the scheme is treated like `http:`, which is parsed differently
from protocols that follows "generic URI syntax" like `file:`, so you probably
want to call `protocol.registerStandardSchemes` to make your scheme treated as
standard scheme.

## protocol.registerBufferProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Registers a protocol of `scheme` that will send `Buffer` as response, the
`callback` should be called with either an `Buffer` object, or an object that
has `data`, `mimeType`, `chart` properties.

Example:

```javascript
protocol.registerBufferProtocol('atom', function(request, callback) {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')});
}, function (error) {
  if (error)
    console.error('Failed to register protocol')
});
```

## protocol.registerStringProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Registers a protocol of `scheme` that will send `String` as response, the
`callback` should be called with either a `String`, or an object that
has `data`, `mimeType`, `chart` properties.

## protocol.registerHttpProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Registers a protocol of `scheme` that will send a HTTP request as response, the
`callback` should be called with an object that has `url`, `method`, `referer`,
`session` properties.

By default the HTTP request will reuse current session, if you want the request
to have different session you should specify `session` to `null`.

## protocol.unregisterProtocol(scheme[, completion])

* `scheme` String
* `completion` Function

Unregisters the custom protocol of `scheme`.

## protocol.isProtocolHandled(scheme, callback)

* `scheme` String
* `callback` Function

The `callback` will be called with a boolean that indicates whether there is
already a handler for `scheme`.

## protocol.interceptFileProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Intercepts `scheme` protocol and use `handler` as the protocol's new handler
which sends file as response.

## protocol.interceptStringProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Intercepts `scheme` protocol and use `handler` as the protocol's new handler
which sends String as response.

## protocol.interceptBufferProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Intercepts `scheme` protocol and use `handler` as the protocol's new handler
which sends `Buffer` as response.

## protocol.interceptHttpProtocol(scheme, handler[, completion])

* `scheme` String
* `handler` Function
* `completion` Function

Intercepts `scheme` protocol and use `handler` as the protocol's new handler
which sends a new HTTP request as response.

## protocol.uninterceptProtocol(scheme[, completion])

* `scheme` String
* `completion` Function

Remove the interceptor installed for `scheme` and restore its original handler.
