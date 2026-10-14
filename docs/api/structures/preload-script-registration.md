# PreloadScriptRegistration Object

* `type` string - Context type where the preload script will be executed.
  Possible values include `frame` or `service-worker`.
* `id` string (optional) - Unique ID of preload script. Defaults to a random UUID.
* `filePath` string - Path of the script file. Must be an absolute path.
* `matches` string[] (optional) - [Match patterns](https://developer.chrome.com/docs/extensions/develop/concepts/match-patterns)
  for URLs the script runs in. Any scheme, including custom protocols, may be used.
  If omitted, the script runs for all URLs not excluded by other filters.
* `excludeMatches` string[] (optional) - Match patterns for URLs the script will not run in.
* `includeGlobs` string[] (optional) - Glob patterns applied after `matches`. A URL must
  also match one of these globs. `*` matches any sequence of characters and `?` matches
  any single character.
* `excludeGlobs` string[] (optional) - Glob patterns for URLs the script will not run in.
