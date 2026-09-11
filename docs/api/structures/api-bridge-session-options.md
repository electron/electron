# ApiBridgeSessionOptions Object

* `origin` string | string[] - The origins of the documents that get the API,
  such as `app://my-app` or `['app://my-app', 'https://my-app.com']`.
  `file://` is not accepted: every local file has that origin.
* `frames` string (optional) - Which frames get the API. Can be `main` or
  `all`. Defaults to `main`, the top frame of each window; `all` also covers
  iframes of the page that show one of the origins.
* `popups` boolean (optional) - Whether windows that a page opens with
  `window.open()` get the API. Defaults to `false`.
* `guests` boolean (optional) - Whether `<webview>` tags get the API. Defaults
  to `false`.
