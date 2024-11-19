# WebRequestFilter Object

* `urls` string[] (optional) _Deprecated_ - Array of [URL patterns](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Match_patterns) that will be used to filter out the requests that do not match the URL patterns. Empty array means all urls are matched.
* `includeUrls` string[] (optional) - Array of [URL patterns](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Match_patterns) that will be used to filter out the requests that do not match the URL patterns. Empty array means no urls are matched and using \<all-urls\> means all urls are matched.
* `excludeUrls` string[] (optional) - Array of [URL patterns](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Match_patterns) that will be used to filter out the requests that match the URL patterns.
* `types` string[] (optional) - Array of types that will be used to filter out the requests that do not match the types. When not specified, all types will be matched. Can be `mainFrame`, `subFrame`, `stylesheet`, `script`, `image`, `font`, `object`, `xhr`, `ping`, `cspReport`, `media` or `webSocket`.
