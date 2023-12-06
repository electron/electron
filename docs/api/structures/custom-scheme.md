# CustomScheme Object

* `scheme` string - Custom schemes to be registered with options.
* `privileges` Object (optional)
  * `standard` boolean (optional) - Default false.
  * `secure` boolean (optional) - Default false.
  * `bypassCSP` boolean (optional) - Default false.
  * `allowServiceWorkers` boolean (optional) - Default false.
  * `supportFetchAPI` boolean (optional) - Default false.
  * `corsEnabled` boolean (optional) - Default false.
  * `stream` boolean (optional) - Default false.
  * `codeCache` boolean (optional) - Enable V8 code cache for the scheme, only
    works when `standard` is also set to true. Default false.
