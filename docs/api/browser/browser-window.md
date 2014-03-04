# browser-window

The `BrowserWindow` class gives you ability to create a browser window, an
example is:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('destroyed', function() {
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

### Event: 'page-title-updated'

* `event` Event

Emitted when the document changed its title, calling `event.preventDefault()`
would prevent the native window's title to change.

### Event: 'loading-state-changed'

* `event` Event
* `isLoading` Boolean

Emitted when the window is starting or is done loading a page.

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

Emitted when the window is closed. At the time of this event, window is not
destroyed yet so you can still do some operations to the window (but you
shouldn't!).

### Event: 'destroyed'

Emitted when the memory taken by the native window is released. Usually you
should dereference the javascript object when received this event.

### Event: 'unresponsive'

Emiited when the web page becomes unresponsive.

### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

### Event: 'crashed'

Emitted when the renderer process is crashed.

### Event: 'blur'

Emiited when window loses focus.

### Class Method: BrowserWindow.getAllWindows()

Returns an array of all opened browser windows.

### Class Method: BrowserWindow.getFocusedWindow()

Returns the window that is focused in this application.

### Class Method: BrowserWindow.fromProcessIdAndRoutingId(processId, routingId)

* `processId` Integer
* `routingId` Integer

Find a window according to its `processId` and `routingId`.

### BrowserWindow.destroy()

Destroy the window and free the memory without closing it.

**Note:** Usually you should always call `Window.close()` to close the window,
**which will emit `beforeunload` and `unload` events for DOM. Only use
**`Window.destroy()` when the window gets into a very bad state and you want
**to force closing it.

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

### BrowserWindow.flashFlame()

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
[remote](../renderer/remote.md) if you are going to use this API in renderer
process.

### BrowserWindow.getPageTitle()

Returns the title of web page.

### BrowserWindow.isLoading()

Returns whether web page is still loading resources.

### BrowserWindow.isWaitingForResponse()

Returns whether web page is waiting for a first-response for the main resource
of the page.

### BrowserWindow.stop()

Stops any pending navigation.

### BrowserWindow.getProcessId()

Returns window's process ID. The process ID and routing ID can be used
together to locate a window.

### BrowserWindow.getRoutingId()

Returns window's routing ID. The process ID and routing ID can be used
together to locate a window.

### BrowserWindow.loadUrl(url)

* `url` URL

Loads the `url` in the window, the `url` must contains the protocol prefix,
e.g. the `http://` or `file://`.

### BrowserWindow.getUrl()

Returns URL of current web page.

### BrowserWindow.canGoBack()

Returns whether the web page can go back.

### BrowserWindow.canGoForward()

Returns whether the web page can go forward.

### BrowserWindow.canGoToOffset(offset)

* `offset` Integer

Returns whether the web page can go to `offset`.

### BrowserWindow.goBack()

Makes the web page go back.

### BrowserWindow.goForward()

Makes the web page go forward.

### BrowserWindow.goToIndex(index)

* `index` Integer

Navigates to the specified absolute index.

### BrowserWindow.goToOffset(offset)

* `offset` Integer

Navigates to the specified offset from the "current entry".

### BrowserWindow.reload()

Reloads current window.

### BrowserWindow.reloadIgnoringCache()

Reloads current window and ignores cache.
