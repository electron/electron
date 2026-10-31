# PreloadScript Object

* `type` string - Context type where the preload script will be executed.
  Possible values include `frame` or `service-worker`.
* `id` string - Unique ID of preload script.
* `filePath` string - Path of the script file. Must be an absolute path.
* `matches` string[] (optional) - [Match patterns](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Match_patterns)
  for URLs the script runs in. A custom protocol can be used if its scheme is registered as
  `standard` with [`protocol.registerSchemesAsPrivileged`](../protocol.md#protocolregisterschemesasprivilegedcustomschemes).
  To filter documents served from a non-standard scheme, use `includeGlobs` or `excludeGlobs`,
  which match against the whole URL. If omitted, the script runs for all URLs not excluded by
  other filters.
* `excludeMatches` string[] (optional) - Match patterns for URLs the script will not run in.
* `includeGlobs` string[] (optional) - Glob patterns applied after `matches`. A URL must
  also match one of these globs. `*` matches any sequence of characters and `?` matches
  any single character.
* `excludeGlobs` string[] (optional) - Glob patterns for URLs the script will not run in.
* `matchOriginAsFallback` boolean (optional) - Whether a document whose URL has no origin of
  its own (`about:`, `blob:`, `data:` or `filesystem:`) is matched by the origin of the frame
  that created it instead of by its URL. This includes documents with an opaque origin, such
  as a `data:` frame or a sandboxed `<iframe srcdoc>`. For example, an `about:blank` popup
  opened by `https://example.com/page` is matched as `https://example.com/`. When set, every
  pattern in `matches` must have a path of `/*`. Only applies to `frame` contexts. Default is
  `false`.
