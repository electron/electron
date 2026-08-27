# HeaderRule Object

* `urls` string[] - Array of [URL patterns](https://developer.chrome.com/docs/extensions/develop/concepts/match-patterns)
  the rule applies to. A rule that sets a request header must use patterns that
  name a host, such as `https://*.example.com/*`; `<all_urls>` and patterns
  whose host is only a wildcard are rejected for such rules.
* `excludeUrls` string[] (optional) - Array of URL patterns the rule does not
  apply to.
* `types` string[] (optional) - Resource types the rule applies to. Can be
  `mainFrame`, `subFrame`, `stylesheet`, `script`, `image`, `font`, `object`,
  `xhr`, `ping`, `cspReport`, `media` or `webSocket`. All types when omitted.
* `requestHeaders` Record\<string, string | null\> (optional) - Request headers
  to set (string value) or remove (`null`) on matching requests.
* `responseHeaders` Record\<string, string | string[] | null\> (optional) -
  Response headers to set (replacing any existing values) or remove (`null`) on
  matching responses.
