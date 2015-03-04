# `window.open` function

When `window.open` is called to create a new window in web page, a new instance
of `BrowserWindow` will be created for the `url`, and a proxy will be returned
to `window.open` to let the page to have limited control over it.

The proxy only has some limited standard functionality implemented to be
compatible with traditional web pages, for full control of the created window
you should create a `BrowserWindow` directly.

## window.open(url, [frameName[, features]])

* `url` String
* `frameName` String
* `features` String

Creates a new window and returns an instance of `BrowserWindowProxy` class.

## window.opener.postMessage(message, targetOrigin)

* `message` String
* `targetOrigin` String

Sends a message to the parent window with the specified origin or `*` for no
origin preference.

## Class: BrowserWindowProxy

### BrowserWindowProxy.blur()

Removes focus from the child window.

### BrowserWindowProxy.close()

Forcefully closes the child window without calling its unload event.

### BrowserWindowProxy.closed

Set to true after the child window gets closed.

### BrowserWindowProxy.eval(code)

* `code` String

Evaluates the code in the child window.

### BrowserWindowProxy.focus()

Focuses the child window (brings the window to front).

### BrowserWindowProxy.postMessage(message, targetOrigin)

* `message` String
* `targetOrigin` String

Sends a message to the child window with the specified origin or `*` for no
origin preference.

In addition to these methods, the child window implements `window.opener` object
with no properties and a single method:
