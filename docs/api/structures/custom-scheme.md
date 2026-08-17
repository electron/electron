# CustomScheme Object

* `scheme` string - Custom schemes to be registered with options.
* `privileges` Object (optional)
  * `standard` boolean (optional) - Default false.
  * `secure` boolean (optional) - Default false.
  * `bypassCSP` boolean (optional) - Default false.
  * `allowServiceWorkers` boolean (optional) - Default false.
  * `supportFetchAPI` boolean (optional) - Default false.
  * `corsEnabled` boolean (optional) - Whether other origins are allowed to
    request this scheme. When `true`, a page from any other origin may
    `fetch()` or `XMLHttpRequest` this scheme and read the response. Electron
    does not validate `Access-Control-Allow-Origin` on responses to custom
    schemes, so this privilege grants cross-origin access rather than enforcing
    the CORS protocol. Leave it `false` for schemes that serve data other
    origins should not be able to read. Default false.
  * `stream` boolean (optional) - Default false.
  * `codeCache` boolean (optional) - Enable V8 code cache for the scheme, only
    works when `standard` is also set to true. Default false.
  * `allowExtensions` boolean (optional) - Allow Chrome extensions to be used
    on pages served over this protocol. Default false.
