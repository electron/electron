# webContents

`webContents` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

It is responsible for rendering and controlling a web page and is a property of
the [`BrowserWindow`](browser-window.md) object. An example of accessing the
`webContents` object:

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({width: 800, height: 1500});
win.loadURL("http://github.com");

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
* `validatedURL` String

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
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

Emitted when details regarding a requested resource are available.
`status` indicates the socket connection to download the resource.

### Event: 'did-get-redirect-request'

Returns:

* `event` Event
* `oldURL` String
* `newURL` String
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
* `favicons` Array - Array of URLs

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

Emitted when a user or the page wants to start navigation. It can happen when
the `window.location` object is changed or a user clicks a link in the page.

This event will not emit when the navigation is started programmatically with
APIs like `webContents.loadURL` and `webContents.back`.

It is also not emitted for in-page navigations, such as clicking anchor links
or updating the `window.location.hash`. Use `did-navigate-in-page` event for
this purpose.

Calling `event.preventDefault()` will prevent the navigation.

### Event: 'did-navigate'

Returns:

* `event` Event
* `url` String

Emitted when a navigation is done.

This event is not emitted for in-page navigations, such as clicking anchor links
or updating the `window.location.hash`. Use `did-navigate-in-page` event for
this purpose.

### Event: 'did-navigate-in-page'

Returns:

* `event` Event
* `url` String

Emitted when an in-page navigation happened.

When in-page navigation happens, the page URL changes but does not cause
navigation outside of the page. Examples of this occurring are when anchor links
are clicked or when the DOM `hashchange` event is triggered.

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

### Event: 'certificate-error'

Returns:

* `event` Event
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` Buffer - PEM encoded data
  * `issuerName` String
* `callback` Function

Emitted when failed to verify the `certificate` for `url`.

The usage is the same with [the `certificate-error` event of
`app`](app.md#event-certificate-error).

### Event: 'select-client-certificate'

Returns:

* `event` Event
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
* `callback` Function

Emitted when a client certificate is requested.

The usage is the same with [the `select-client-certificate` event of
`app`](app.md#event-select-client-certificate).

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

### Event: 'found-in-page'

Returns:

* `event` Event
* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - Indicates if more responses are to follow.
  * `matches` Integer (Optional) - Number of Matches.
  * `selectionArea` Object (Optional) - Coordinates of first match region.

Emitted when a result is available for
[`webContents.findInPage`](web-contents.md#webcontentsfindinpage) request.

### Event: 'media-started-playing'

Emitted when media starts playing.

### Event: 'media-paused'

Emitted when media is paused or done playing.

### Event: 'did-change-theme-color'

Emitted when a page's theme color changes. This is usually due to encountering a meta tag:

```html
<meta name='theme-color' content='#ff0000'>
```

## Instance Methods

The `webContents` object has the following instance methods:

### `webContents.loadURL(url[, options])`

* `url` URL
* `options` Object (optional), properties:
  * `httpReferrer` String - A HTTP Referrer url.
  * `userAgent` String - A user agent originating the request.
  * `extraHeaders` String - Extra headers separated by "\n"

Loads the `url` in the window, the `url` must contain the protocol prefix,
e.g. the `http://` or `file://`. If the load should bypass http cache then
use the `pragma` header to achieve it.

```javascript
const options = {"extraHeaders" : "pragma: no-cache\n"}
webContents.loadURL(url, options)
```

### `webContents.downloadURL(url)`

* `url` URL

Initiates a download of the resource at `url` without navigating. The
`will-download` event of `session` will be triggered.

### `webContents.getURL()`

Returns URL of the current web page.

```javascript
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL("http://github.com");

var currentURL = win.webContents.getURL();
```

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

* `muted` Boolean

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

### `webContents.insertText(text)`

* `text` String

Inserts `text` to the focused element.

### `webContents.findInPage(text[, options])`

* `text` String - Content to be searched, must not be empty.
* `options` Object (Optional)
  * `forward` Boolean - Whether to search forward or backward, defaults to `true`.
  * `findNext` Boolean - Whether the operation is first request or a follow up,
    defaults to `false`.
  * `matchCase` Boolean - Whether search should be case-sensitive,
    defaults to `false`.
  * `wordStart` Boolean - Whether to look only at the start of words.
    defaults to `false`.
  * `medialCapitalAsWordStart` Boolean - When combined with `wordStart`,
    accepts a match in the middle of a word if the match begins with an
    uppercase letter followed by a lowercase or non-letter.
    Accepts several other intra-word matches, defaults to `false`.

Starts a request to find all matches for the `text` in the web page and returns an `Integer`
representing the request id used for the request. The result of the request can be
obtained by subscribing to [`found-in-page`](web-contents.md#event-found-in-page) event.

### `webContents.stopFindInPage(action)`

* `action` String - Specifies the action to take place when ending
  [`webContents.findInPage`](web-contents.md#webcontentfindinpage) request.
  * `clearSelection` - Translate the selection into a normal selection.
  * `keepSelection` - Clear the selection.
  * `activateSelection` - Focus and click the selection node.

Stops any `findInPage` request for the `webContents` with the provided `action`.

```javascript
webContents.on('found-in-page', function(event, result) {
  if (result.finalUpdate)
    webContents.stopFindInPage("clearSelection");
});

const requestId = webContents.findInPage("api");
```

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
  * `A5`
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
const BrowserWindow = require('electron').BrowserWindow;
const fs = require('fs');

var win = new BrowserWindow({width: 800, height: 600});
win.loadURL("http://github.com");

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

Adds the specified path to DevTools workspace. Must be used after DevTools
creation:

```javascript
mainWindow.webContents.on('devtools-opened', function() {
  mainWindow.webContents.addWorkSpace(__dirname);
});
```

### `webContents.removeWorkSpace(path)`

* `path` String

Removes the specified path from DevTools workspace.

### `webContents.openDevTools([options])`

* `options` Object (optional). Properties:
  * `detach` Boolean - opens DevTools in a new window

Opens the devtools.

### `webContents.closeDevTools()`

Closes the devtools.

### `webContents.isDevToolsOpened()`

Returns whether the devtools is opened.

### `webContents.isDevToolsFocused()`

Returns whether the devtools view is focused .

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

### `webContents.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

Send an asynchronous message to renderer process via `channel`, you can also
send arbitrary arguments. The renderer process can handle the message by
listening to the `channel` event with the `ipcRenderer` module.

An example of sending messages from the main process to the renderer process:

```javascript
// In the main process.
var window = null;
app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadURL('file://' + __dirname + '/index.html');
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
    require('electron').ipcRenderer.on('ping', function(event, message) {
      console.log(message);  // Prints "whoooooooh!"
    });
  </script>
</body>
</html>
```

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
    `mouseMove`, `keyDown`, `keyUp`, `char`.
  * `modifiers` Array - An array of modifiers of the event, can
    include `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

Sends an input `event` to the page.

For keyboard events, the `event` object also have following properties:

* `keyCode` Char or String (**required**) - The character that will be sent
  as the keyboard event. Can be a single UTF-8 character, or the name of the
  key that generates the event. Accepted key names are `enter`, `backspace`,
  `delete`, `tab`, `escape`, `control`, `alt`, `shift`, `end`, `home`, `insert`,
  `left`, `up`, `right`, `down`, `pageUp`, `pageDown`, `printScreen`

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
win.loadURL('https://github.com');

win.webContents.on('did-finish-load', function() {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete', function(error) {
    if (!error)
      console.log("Save page successfully");
  });
});
```

## Instance Properties

`WebContents` objects also have the following properties:

### `webContents.session`

Returns the [session](session.md) object used by this webContents.

### `webContents.devToolsWebContents`

Get the `WebContents` of DevTools for this `WebContents`.

**Note:** Users should never store this object because it may become `null`
when the DevTools has been closed.
