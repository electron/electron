# browser-window

The `BrowserWindow` class gives you ability to create a browser window, an
example is:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('closed', function() {
  win = null;
});

win.loadUrl('https://github.com');
win.show();
```

You can also create a window without chrome by using
[Frameless Window](frameless-window.md) API.

## Class: BrowserWindow

`BrowserWindow` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

### new BrowserWindow(options)

* `options` Object
  * `x` Integer - Window's left offset to screen
  * `y` Integer - Window's top offset to screen
  * `center` Boolean - Show window in the center of the screen
  * `min-width` Integer - Minimum width
  * `min-height` Integer - Minimum height
  * `max-width` Integer - Maximum width
  * `max-height` Integer - Maximum height
  * `resizable` Boolean - Whether window is resizable
  * `always-on-top` Boolean - Whether the window should always stay on top of other windows
  * `fullscreen` Boolean - Whether the window should show in fullscreen
  * `kiosk` Boolean - The kiosk mode
  * `title` String - Default window title
  * `show` Boolean - Whether window should be shown when created
  * `frame` Boolean - Specify `false` to create a
    [Frameless Window](frameless-window.md)
  * `node-integration` String - Can be `all`, `except-iframe`,
    `manual-enable-iframe` or `disable`.
  * `accept-first-mouse` Boolean - Whether the web view accepts a single
     mouse-down event that simultaneously activates the window

Creates a new `BrowserWindow` with native properties set by the `options`.
Usually you only need to set the `width` and `height`, other properties will
have decent default values.

By default the `node-integration` option is `except-iframe`, which means node
integration is disabled in all iframes, . You can also set it to `all`, with
which node integration is available to the main page and all its iframes, or
`manual-enable-iframe`, which is like `except-iframe`, but would enable iframes
whose name is suffixed by `-enable-node-integration`. And setting to `disable`
would disable the node integration in both the main page and its iframes.

An example of enable node integration in iframe with `node-integration` set to
`manual-enable-iframe`:

```html
<!-- iframe with node integration enabled -->
<iframe name="gh-enable-node-integration" src="https://github.com"></iframe>

<!-- iframe with node integration disabled -->
<iframe src="http://jandan.net"></iframe>
```

And in atom-shell, the security limitation of iframe is stricter than normal
browser, by default iframe is sandboxed with all permissions except the
`allow-same-origin`, which means iframe could not access parent's js context.

If you want to enable things like `parent.window.process.exit()` in iframe,
you should explicitly set `sandbox` to `none`:

```html
<iframe sandbox="none" src="https://github.com"></iframe>
```

### Event: 'page-title-updated'

* `event` Event

Emitted when the document changed its title, calling `event.preventDefault()`
would prevent the native window's title to change.

### Event: 'close'

* `event` Event

Emitted when the window is going to be closed. It's emitted before the
`beforeunload` and `unload` event of DOM, calling `event.preventDefault()`
would cancel the close.

Usually you would want to use the `beforeunload` handler to decide whether the
window should be closed, which will also be called when the window is
reloaded. In atom-shell, returning an empty string or `false` would cancel the
close. An example is:

```javascript
window.onbeforeunload = function(e) {
  console.log('I do not want to be closed');

  // Unlike usual browsers, in which a string should be returned and the user is
  // prompted to confirm the page unload. atom-shell gives the power completely
  // to the developers, return empty string or false would prevent the unloading
  // now. You can also use the dialog API to let user confirm it.
  return false;
};
```

### Event: 'closed'

Emitted when the window is closed. After you have received this event you should
remove the reference to the window and avoid using it anymore.

### Event: 'unresponsive'

Emitted when the web page becomes unresponsive.

### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

### Event: 'blur'

Emitted when window loses focus.

### Class Method: BrowserWindow.getAllWindows()

Returns an array of all opened browser windows.

### Class Method: BrowserWindow.getFocusedWindow()

Returns the window that is focused in this application.

### Class Method: BrowserWindow.fromWebContents(webContents)

* `webContents` WebContents

Find a window according to the `webContents` it owns

### BrowserWindow.webContents

The `WebContents` object this window owns, all web page related events and
operations would be done via it.

### BrowserWindow.devToolsWebContents

Get the `WebContents` of devtools of this window.

### BrowserWindow.destroy()

Force closing the window, the `unload` and `beforeunload` event won't be emitted
for the web page, and `close` event would also not be emitted for this window,
but it would guarantee the `closed` event to be emitted.

You should only use this method when the web page has crashed.

### BrowserWindow.close()

Try to close the window, this has the same effect with user manually clicking
the close button of the window. The web page may cancel the close though, see
the [close event](window#event-close).

### BrowserWindow.focus()

Focus on the window.

### BrowserWindow.isFocused()

Returns whether the window is focused.

### BrowserWindow.show()

Shows the window.

### BrowserWindow.hide()

Hides the window.

### BrowserWindow.isVisible()

Returns whether the window is visible to the user.

### BrowserWindow.maximize()

Maximizes the window.

### BrowserWindow.unmaximize()

Unmaximizes the window.

### BrowserWindow.minimize()

Minimizes the window. On some platforms the minimized window will be shown in
the Dock.

### BrowserWindow.restore()

Restores the window from minimized state to its previous state.

### BrowserWindow.setFullScreen(flag)

* `flag` Boolean

Sets whether the window should be in fullscreen mode.

### BrowserWindow.isFullScreen()

Returns whether the window is in fullscreen mode.

### BrowserWindow.setSize(width, height)

* `width` Integer
* `height` Integer

Resizes the window to `width` and `height`.

### BrowserWindow.getSize()

Returns an array that contains window's width and height.

### BrowserWindow.setMinimumSize(width, height)

* `width` Integer
* `height` Integer

Sets the minimum size of window to `width` and `height`.

### BrowserWindow.getMinimumSize()

Returns an array that contains window's minimum width and height.

### BrowserWindow.setMaximumSize(width, height)

* `width` Integer
* `height` Integer

Sets the maximum size of window to `width` and `height`.

### BrowserWindow.getMaximumSize()

Returns an array that contains window's maximum width and height.

### BrowserWindow.setResizable(resizable)

* `resizable` Boolean

Sets whether the window can be manually resized by user.

### BrowserWindow.isResizable()

Returns whether the window can be manually resized by user.

### BrowserWindow.setAlwaysOnTop(flag)

* `flag` Boolean

Sets whether the window should show always on top of other windows. After
setting this, the window is still a normal window, not a toolbox window which
can not be focused on.

### BrowserWindow.isAlwaysOnTop()

Returns whether the window is always on top of other windows.

### BrowserWindow.center()

Moves window to the center of the screen.

### BrowserWindow.setPosition(x, y)

* `x` Integer
* `y` Integer

Moves window to `x` and `y`.

### BrowserWindow.getPosition()

Returns an array that contains window's current position.

### BrowserWindow.setTitle(title)

* `title` String

Changes the title of native window to `title`.

### BrowserWindow.getTitle()

Returns the title of the native window.

**Note:** The title of web page can be different from the title of the native
**window.

### BrowserWindow.flashFrame()

Flashes the window to attract user's attention.

### BrowserWindow.setKiosk(flag)

* `flag` Boolean

Enters or leaves the kiosk mode.

### BrowserWindow.isKiosk()

Returns whether the window is in kiosk mode.

### BrowserWindow.openDevTools()

Opens the developer tools.

### BrowserWindow.closeDevTools()

Closes the developer tools.

### BrowserWindow.inspectElement(x, y)

* `x` Integer
* `y` Integer

Starts inspecting element at position (`x`, `y`).

### BrowserWindow.focusOnWebView()

### BrowserWindow.blurWebView()

### BrowserWindow.capturePage([rect, ]callback)

* `rect` Object - The area of page to be captured
  * `x`
  * `y`
  * `width`
  * `height`
* `callback` Function

Captures the snapshot of page within `rect`, upon completion `callback` would be
called with `callback(image)`, the `image` is a `Buffer` that stores the PNG
encoded data of the snapshot. Omitting the `rect` would capture the whole
visible page.

You can write received `image` directly to a `.png` file, or you can base64
encode it and use data URL to embed the image in HTML.

**Note:** Be sure to read documents on remote buffer in
[remote](remote.md) if you are going to use this API in renderer
process.

### BrowserWindow.loadUrl(url)

Same with `webContents.loadUrl(url)`.

## Class: WebContents

A `WebContents` is responsible for rendering and controlling a web page.

`WebContents` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

### Event: 'crashed'

Emitted when the renderer process is crashed.

### Event: 'did-finish-load'

Emitted when the navigation is done, i.e. the spinner of the tab will stop
spinning, and the onload event was dispatched.

### Event: 'did-start-loading'

### Event: 'did-stop-loading'

### WebContents.loadUrl(url)

* `url` URL

Loads the `url` in the window, the `url` must contains the protocol prefix,
e.g. the `http://` or `file://`.

### WebContents.getUrl()

Returns URL of current web page.

### WebContents.getTitle()

Returns the title of web page.

### WebContents.isLoading()

Returns whether web page is still loading resources.

### WebContents.isWaitingForResponse()

Returns whether web page is waiting for a first-response for the main resource
of the page.

### WebContents.stop()

Stops any pending navigation.

### WebContents.reload()

Reloads current page.

### WebContents.reloadIgnoringCache()

Reloads current page and ignores cache.

### WebContents.canGoBack()

Returns whether the web page can go back.

### WebContents.canGoForward()

Returns whether the web page can go forward.

### WebContents.canGoToOffset(offset)

* `offset` Integer

Returns whether the web page can go to `offset`.

### WebContents.goBack()

Makes the web page go back.

### WebContents.goForward()

Makes the web page go forward.

### WebContents.goToIndex(index)

* `index` Integer

Navigates to the specified absolute index.

### WebContents.goToOffset(offset)

* `offset` Integer

Navigates to the specified offset from the "current entry".

### WebContents.IsCrashed()

Whether the renderer process has crashed.

### WebContents.executeJavaScript(code)

* `code` String

Evaluate `code` in page.

### WebContents.send(channel[, args...])

* `channel` String

Send `args..` to the web page via `channel` in asynchronous message, the web
page can handle it by listening to the `channel` event of `ipc` module.

An example of sending messages from browser side to web pages:

```javascript
// On browser side.
var window = null;
app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadUrl('file://' + __dirname + '/index.html');
  window.webContents.on('did-finish-load', function() {
    window.webContents.send('ping', 'whoooooooh!');
  });
});
```

```html
// index.html
<html>
<body>
  <script>
    require('ipc').on('ping', function(message) {
      console.log(message);  // Prints "whoooooooh!"
    });
  </script>
</body>
</html>
```

**Note:**

1. The IPC message handler in web pages do not have a `event` parameter, which
   is different from the handlers on browser side.
2. There is no way to send synchronous messages from browser side to web pages,
   because it would be very easy to cause dead locks.
