# PreloadScriptRegistration Object

* `type` string - Context type where the preload script will be executed.
  Possible values include `frame` or `service-worker`.
* `id` string (optional) - Unique ID of preload script. Defaults to a random UUID.
* `filePath` string - Path of the script file. Must be an absolute path.
* `matches` string[] (optional) - [Match patterns](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Match_patterns)
  for URLs the script runs in. Any scheme, including custom protocols, may be used.
  If omitted, the script runs for all URLs not excluded by other filters. Must not be empty.
* `excludeMatches` string[] (optional) - Match patterns for URLs the script will not run in.
* `includeGlobs` string[] (optional) - Glob patterns applied after `matches`. A URL must
  also match one of these globs. `*` matches any sequence of characters and `?` matches
  any single character. Must not be empty.
* `excludeGlobs` string[] (optional) - Glob patterns for URLs the script will not run in.
* `matchOriginAsFallback` boolean (optional) - Whether a document whose URL has no origin of
  its own (`about:`, `blob:`, `data:` or `filesystem:`) is matched by the origin it inherited
  from the frame that created it instead of by its URL. For example, an `about:blank` popup
  opened by `https://example.com/page` is matched as `https://example.com/`. When set, every
  pattern in `matches` must have a path of `/*`. Only applies to `frame` contexts. Default is
  `false`.
