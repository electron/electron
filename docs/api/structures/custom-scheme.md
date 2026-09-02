# CustomScheme Object

* `scheme` string - Custom schemes to be registered with options.
* `privileges` Object (optional)
  * `standard` boolean (optional) - Default false.
  * `secure` boolean (optional) - Default false.
  * `bypassCSP` boolean (optional) - Default false.
  * `allowServiceWorkers` boolean (optional) - Default false.
  * `supportFetchAPI` boolean (optional) - Default false.
  * `corsEnabled` boolean (optional) - Whether cross-origin requests to this
    scheme are permitted. When true, responses served for the scheme are not
    checked for `Access-Control-Allow-Origin`, so contents served over it are
    readable from any origin, so only enable it for schemes whose contents are
    safe to expose. When false, cross-origin requests from web contents fail
    regardless of response headers; requests from the main process (e.g.
    `net.fetch`) are unaffected. Default false.
  * `stream` boolean (optional) - Default false.
  * `codeCache` boolean (optional) - Enable V8 code cache for the scheme, only
    works when `standard` is also set to true. Default false.
  * `allowExtensions` boolean (optional) - Allow Chrome extensions to be used
    on pages served over this protocol. Default false.
