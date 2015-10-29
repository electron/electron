# webContents

`webContents` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

It is responsible for rendering and controlling a web page and is a property of
the [`BrowserWindow`](browser-window.md) object. An example of accessing the
`webContents` object:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({width: 800, height: 1500});
win.loadUrl("http://github.com");

var webContents = win.webContents;
```

## Events

The `webContents` object emits the following events:

### Event: 'did-finish-load'

Emitted when the navigation is done, i.e. the spinner of the tab has stopped
spinning, and the `onload` event was dispatched.

### Event: 'did-fail-load'

Returns:

* `event` Event
* `errorCode` Integer
* `errorDescription` String
* `validatedUrl` String

This event is like `did-finish-load` but emitted when the load failed or was
cancelled, e.g. `window.stop()` is invoked.
The full list of error codes and their meaning is available [here](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

### Event: 'did-frame-finish-load'

Returns:

* `event` Event
* `isMainFrame` Boolean

Emitted when a frame has done navigation.

### Event: 'did-start-loading'

Corresponds to the points in time when the spinner of the tab started spinning.

### Event: 'did-stop-loading'

Corresponds to the points in time when the spinner of the tab stopped spinning.

### Event: 'did-get-response-details'

Returns:

* `event` Event
* `status` Boolean
* `newUrl` String
* `originalUrl` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

Emitted when details regarding a requested resource are available.
`status` indicates the socket connection to download the resource.

### Event: 'did-get-redirect-request'

Returns:

* `event` Event
* `oldUrl` String
* `newUrl` String
* `isMainFrame` Boolean
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

Emitted when a redirect is received while requesting a resource.

### Event: 'dom-ready'

Returns:

* `event` Event

Emitted when the document in the given frame is loaded.

### Event: 'page-favicon-updated'

Returns:

* `event` Event
* `favicons` Array - Array of Urls

Emitted when page receives favicon urls.

### Event: 'new-window'

Returns:

* `event` Event
* `url` String
* `frameName` String
* `disposition` String - Can be `default`, `foreground-tab`, `background-tab`,
  `new-window` and `other`.
* `options` Object - The options which will be used for creating the new
  `BrowserWindow`.

Emitted when the page requests to open a new window for a `url`. It could be
requested by `window.open` or an external link like `<a target='_blank'>`.

By default a new `BrowserWindow` will be created for the `url`.

Calling `event.preventDefault()` will prevent creating new windows.

### Event: 'will-navigate'

Returns:

* `event` Event
* `url` String

Emitted when a user or the page wants to start navigation. It can happen when the
`window.location` object is changed or a user clicks a link in the page.

This event will not emit when the navigation is started programmatically with
APIs like `webContents.loadUrl` and `webContents.back`.

Calling `event.preventDefault()` will prevent the navigation.

### Event: 'crashed'

Emitted when the renderer process has crashed.

### Event: 'plugin-crashed'

Returns:

* `event` Event
* `name` String
* `version` String

Emitted when a plugin process has crashed.

### Event: 'destroyed'

Emitted when `webContents` is destroyed.

### Event: 'devtools-opened'

Emitted when DevTools is opened.

### Event: 'devtools-closed'

Emitted when DevTools is closed.

### Event: 'devtools-focused'

Emitted when DevTools is focused / opened.

### Event: 'login'

Returns:

* `event` Event
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

Emitted when `webContents` wants to do basic auth.

The usage is the same with [the `login` event of `app`](app.md#event-login).

## Instance Methods

The `webContents` object has the following instance methods:

### `webContents.session`

Returns the `session` object used by this webContents.

See [session documentation](session.md) for this object's methods.

### `webContents.loadUrl(url[, options])`

* `url` URL
* `options` Object (optional), properties:
  * `httpReferrer` String - A HTTP Referrer url.
  * `userAgent` String - A user agent originating the request.
  * `extraHeaders` String - Extra headers separated by "\n"

Loads the `url` in the window, the `url` must contain the protocol prefix,
e.g. the `http://` or `file://`.

### `webContents.getUrl()`

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({width: 800, height: 600});
win.loadUrl("http://github.com");

var currentUrl = win.webContents.getUrl();
```

Returns URL of the current web page.

### `webContents.getTitle()`

Returns the title of the current web page.

### `webContents.isLoading()`

Returns whether web page is still loading resources.

### `webContents.isWaitingForResponse()`

Returns whether the web page is waiting for a first-response from the main
resource of the page.

### `webContents.stop()`

Stops any pending navigation.

### `webContents.reload()`

Reloads the current web page.

### `webContents.reloadIgnoringCache()`

Reloads current page and ignores cache.

### `webContents.canGoBack()`

Returns whether the browser can go back to previous web page.

### `webContents.canGoForward()`

Returns whether the browser can go forward to next web page.

### `webContents.canGoToOffset(offset)`

* `offset` Integer

Returns whether the web page can go to `offset`.

### `webContents.clearHistory()`

Clears the navigation history.

### `webContents.goBack()`

Makes the browser go back a web page.

### `webContents.goForward()`

Makes the browser go forward a web page.

### `webContents.goToIndex(index)`

* `index` Integer

Navigates browser to the specified absolute web page index.

### `webContents.goToOffset(offset)`

* `offset` Integer

Navigates to the specified offset from the "current entry".

### `webContents.isCrashed()`

Whether the renderer process has crashed.

### `webContents.setUserAgent(userAgent)`

* `userAgent` String

Overrides the user agent for this web page.

### `webContents.getUserAgent()`

Returns a `String` representing the user agent for this web page.

### `webContents.insertCSS(css)`

* `css` String

Injects CSS into the current web page.

### `webContents.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (optional)

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

### `webContents.setAudioMuted(muted)`

+ `muted` Boolean

Mute the audio on the current web page.

### `webContents.isAudioMuted()`

Returns whether this page has been muted.

### `webContents.undo()`

Executes the editing command `undo` in web page.

### `webContents.redo()`

Executes the editing command `redo` in web page.

### `webContents.cut()`

Executes the editing command `cut` in web page.

### `webContents.copy()`

Executes the editing command `copy` in web page.

### `webContents.paste()`

Executes the editing command `paste` in web page.

### `webContents.pasteAndMatchStyle()`

Executes the editing command `pasteAndMatchStyle` in web page.

### `webContents.delete()`

Executes the editing command `delete` in web page.

### `webContents.selectAll()`

Executes the editing command `selectAll` in web page.

### `webContents.unselect()`

Executes the editing command `unselect` in web page.

### `webContents.replace(text)`

* `text` String

Executes the editing command `replace` in web page.

### `webContents.replaceMisspelling(text)`

* `text` String

Executes the editing command `replaceMisspelling` in web page.

### `webContents.hasServiceWorker(callback)`

* `callback` Function

Checks if any ServiceWorker is registered and returns a boolean as
response to `callback`.

### `webContents.unregisterServiceWorker(callback)`

* `callback` Function

Unregisters any ServiceWorker if present and returns a boolean as
response to `callback` when the JS promise is fulfilled or false
when the JS promise is rejected.

### `webContents.print([options])`

`options` Object (optional), properties:

* `silent` Boolean - Don't ask user for print settings, defaults to `false`
* `printBackground` Boolean - Also prints the background color and image of
  the web page, defaults to `false`.

Prints window's web page. When `silent` is set to `false`, Electron will pick
up system's default printer and default settings for printing.

Calling `window.print()` in web page is equivalent to calling
`webContents.print({silent: false, printBackground: false})`.

**Note:** On Windows, the print API relies on `pdf.dll`. If your application
doesn't need the print feature, you can safely remove `pdf.dll` to reduce binary
size.

### `webContents.printToPDF(options, callback)`

`options` Object, properties:

* `marginsType` Integer - Specify the type of margins to use
  * 0 - default
  * 1 - none
  * 2 - minimum
* `pageSize` String - Specify page size of the generated PDF.
  * `A4`
  * `A3`
  * `Legal`
  * `Letter`
  * `Tabloid`
* `printBackground` Boolean - Whether to print CSS backgrounds.
* `printSelectionOnly` Boolean - Whether to print selection only.
* `landscape` Boolean - `true` for landscape, `false` for portrait.

`callback` Function - `function(error, data) {}`

* `error` Error
* `data` Buffer - PDF file content.

Prints window's web page as PDF with Chromium's preview printing custom
settings.

By default, an empty `options` will be regarded as:

```javascript
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

```javascript
var BrowserWindow = require('browser-window');
var fs = require('fs');

var win = new BrowserWindow({width: 800, height: 600});
win.loadUrl("http://github.com");

win.webContents.on("did-finish-load", function() {
  // Use default printing options
  win.webContents.printToPDF({}, function(error, data) {
    if (error) throw error;
    fs.writeFile("/tmp/print.pdf", data, function(error) {
      if (error)
        throw error;
      console.log("Write PDF successfully.");
    })
  })
});
```

### `webContents.addWorkSpace(path)`

* `path` String

Adds the specified path to DevTools workspace.

### `webContents.removeWorkSpace(path)`

* `path` String

Removes the specified path from DevTools workspace.

### `webContents.openDevTools([options])`

* `options` Object (optional). Properties:
  * `detach` Boolean - opens DevTools in a new window

Opens the developer tools.

### `webContents.closeDevTools()`

Closes the developer tools.

### `webContents.isDevToolsOpened()`

Returns whether the developer tools are opened.

### `webContents.toggleDevTools()`

Toggles the developer tools.

### `webContents.isDevToolsFocused()`

Returns whether the developer tools is focused.

### `webContents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

Starts inspecting element at position (`x`, `y`).

### `webContents.inspectServiceWorker()`

Opens the developer tools for the service worker context.

### `webContents.send(channel[, args...])`

* `channel` String
* `args...` (optional)

Send `args...` to the web page via `channel` in an asynchronous message, the web
page can handle it by listening to the `channel` event of the `ipc` module.

An example of sending messages from the main process to the renderer process:

```javascript
// In the main process.
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
<!-- index.html -->
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

1. The IPC message handler in web pages does not have an `event` parameter,
   which is different from the handlers in the main process.
2. There is no way to send synchronous messages from the main process to a
   renderer process, because it would be very easy to cause dead locks.

### `webContents.enableDeviceEmulation(parameters)`

`parameters` Object, properties:

* `screenPosition` String - Specify the screen type to emulate
    (default: `desktop`)
  * `desktop`
  * `mobile`
* `screenSize` Object - Set the emulated screen size (screenPosition == mobile)
  * `width` Integer - Set the emulated screen width
  * `height` Integer - Set the emulated screen height
* `viewPosition` Object - Position the view on the screen
    (screenPosition == mobile) (default: `{x: 0, y: 0}`)
  * `x` Integer - Set the x axis offset from top left corner
  * `y` Integer - Set the y axis offset from top left corner
* `deviceScaleFactor` Integer - Set the device scale factor (if zero defaults to
    original device scale factor) (default: `0`)
* `viewSize` Object - Set the emulated view size (empty means no override)
  * `width` Integer - Set the emulated view width
  * `height` Integer - Set the emulated view height
* `fitToView` Boolean - Whether emulated view should be scaled down if
    necessary to fit into available space (default: `false`)
* `offset` Object - Offset of the emulated view inside available space (not in
    fit to view mode) (default: `{x: 0, y: 0}`)
  * `x` Float - Set the x axis offset from top left corner
  * `y` Float - Set the y axis offset from top left corner
* `scale` Float - Scale of emulated view inside available space (not in fit to
    view mode) (default: `1`)

Enable device emulation with the given parameters.

### `webContents.disableDeviceEmulation()`

Disable device emulation enabled by `webContents.enableDeviceEmulation`.

### `webContents.sendInputEvent(event)`

* `event` Object
  * `type` String (**required**) - The type of the event, can be `mouseDown`,
    `mouseUp`, `mouseEnter`, `mouseLeave`, `contextMenu`, `mouseWheel`,
    `keyDown`, `keyUp`, `char`.
  * `modifiers` Array - An array of modifiers of the event, can
    include `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

Sends an input `event` to the page.

For keyboard events, the `event` object also have following properties:

* `keyCode` String (**required**) - A single character that will be sent as
  keyboard event. Can be any UTF-8 character.

For mouse events, the `event` object also have following properties:

* `x` Integer (**required**)
* `y` Integer (**required**)
* `button` String - The button pressed, can be `left`, `middle`, `right`
* `globalX` Integer
* `globalY` Integer
* `movementX` Integer
* `movementY` Integer
* `clickCount` Integer

For the `mouseWheel` event, the `event` object also have following properties:

* `deltaX` Integer
* `deltaY` Integer
* `wheelTicksX` Integer
* `wheelTicksY` Integer
* `accelerationRatioX` Integer
* `accelerationRatioY` Integer
* `hasPreciseScrollingDeltas` Boolean
* `canScroll` Boolean

### `webContents.beginFrameSubscription(callback)`

* `callback` Function

Begin subscribing for presentation events and captured frames, the `callback`
will be called with `callback(frameBuffer)` when there is a presentation event.

The `frameBuffer` is a `Buffer` that contains raw pixel data. On most machines,
the pixel data is effectively stored in 32bit BGRA format, but the actual
representation depends on the endianness of the processor (most modern
processors are little-endian, on machines with big-endian processors the data
is in 32bit ARGB format).

### `webContents.endFrameSubscription()`

End subscribing for frame presentation events.

## Instance Properties

`WebContents` objects also have the following properties:

### `webContents.devToolsWebContents`

Get the `WebContents` of DevTools for this `WebContents`.

**Note:** Users should never store this object because it may become `null`
when the DevTools has been closed.

### `webContents.savePage(fullPath, saveType, callback)`

* `fullPath` String - The full file path.
* `saveType` String - Specify the save type.
  * `HTMLOnly` - Save only the HTML of the page.
  * `HTMLComplete` - Save complete-html page.
  * `MHTML` - Save complete-html page as MHTML.
* `callback` Function - `function(error) {}`.
  * `error` Error

Returns true if the process of saving page has been initiated successfully.

```javascript
win.loadUrl('https://github.com');

win.webContents.on('did-finish-load', function() {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete', function(error) {
    if (!error)
      console.log("Save page successfully");
  });
});
```
