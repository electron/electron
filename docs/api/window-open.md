# `window.open` Function

> Open a new window and load a URL.

When `window.open` is called to create a new window in a web page, a new instance
of `BrowserWindow` will be created for the `url` and a proxy will be returned
to `window.open` to let the page have limited control over it.

The proxy has limited standard functionality implemented to be
compatible with traditional web pages. For full control of the new window
you should create a `BrowserWindow` directly.

The newly created `BrowserWindow` will inherit the parent window's options by
default. To override inherited options you can set them in the `features`
string.

### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (optional)
* `features` String (optional)

Returns `BrowserWindowProxy` - Creates a new window and returns an instance of `BrowserWindowProxy` class.

The `features` string follows the format of standard browser, but each feature
has to be a field of `BrowserWindow`'s options.

**Notes:**

* Node integration will always be disabled in the opened `window` if it is
  disabled on the parent window.
* Non-standard features (that are not handled by Chromium or Electron) given in
  `features` will be passed to any registered `webContent`'s `new-window` event
  handler in the `additionalFeatures` argument.

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

Sends a message to the parent window with the specified origin or `*` for no
origin preference.
