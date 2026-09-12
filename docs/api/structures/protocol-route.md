# ProtocolRoute Object

* `match` Object (optional) - Which URLs on the scheme this route serves. A
  route without `match` serves every URL.
  * `host` string (optional) - Host to match, for standard schemes. Omit to
    match any host.
  * `path` string (optional) - Path prefix to match, starting with `/`.
    Defaults to `/`.
* `source` Object - What the matched URLs are served from.
  * `type` string - Must be `directory`.
  * `root` string - Absolute path of the directory. The part of the URL's path
    after `match.path` is resolved inside it; paths that would leave it are not
    served. May point into an `asar` archive.
  * `index` string (optional) - File served for a URL whose path ends in `/`.
    Defaults to `index.html`; an empty string disables it.
  * `headers` Record\<string, string\> (optional) - Response headers added to
    every file served by this route.
