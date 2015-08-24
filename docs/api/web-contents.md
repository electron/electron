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

var webContents = win.webContents

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

This event is like `did-finish-load` but emitted when the load failed or was
cancelled, e.g. `window.stop()` is invoked.

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

Emitted when details regarding a requested resource is available.
`status` indicates the socket connection to download the resource.

### Event: 'did-get-redirect-request'

Returns:

* `event` Event
* `oldUrl` String
* `newUrl` String
* `isMainFrame` Boolean

Emitted when a redirect was received while requesting a resource.

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
  `new-window` and `other`

Emitted when the page requests to open a new window for a `url`. It could be
requested by `window.open` or an external link like `<a target='_blank'>`.

By default a new `BrowserWindow` will be created for the `url`.

Calling `event.preventDefault()` can prevent creating new windows.

### Event: 'will-navigate'

Returns:

* `event` Event
* `url` String

Emitted when user or the page wants to start a navigation, it can happen when
`window.location` object is changed or user clicks a link in the page.

This event will not emit when the navigation is started programmatically with
APIs like `webContents.loadUrl` and `webContents.back`.

Calling `event.preventDefault()` can prevent the navigation.

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

## Instance Methods

The `webContents` object has the following instance methods:

### `webContents.session`

Returns the `Session` object used by this webContents.

See [session documentation](session.md) for this object's methods.

### `webContents.loadUrl(url[, options])`

* `url` URL
* `options` Object (optional). Properties:
  * `httpReferrer` String - A HTTP Referrer url
  * `userAgent` String - A user agent originating the request

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

Returns whether the web page is waiting for a first-response for the main
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

Overrides the user agent for this page.

### `webContents.getUserAgent()`

Returns a `String` representing the user agent for this page.

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

Executes editing command `undo` in page.

### `webContents.redo()`

Executes editing command `redo` in page.

### `webContents.cut()`

Executes editing command `cut` in page.

### `webContents.copy()`

Executes editing command `copy` in page.

### `webContents.paste()`

Executes editing command `paste` in page.

### `webContents.pasteAndMatchStyle()`

Executes editing command `pasteAndMatchStyle` in page.

### `webContents.delete()`

Executes editing command `delete` in page.

### `webContents.selectAll()`

Executes editing command `selectAll` in page.

### `webContents.unselect()`

Executes editing command `unselect` in page.

### `webContents.replace(text)`

* `text` String

Executes editing command `replace` in page.

### `webContents.replaceMisspelling(text)`

* `text` String

Executes editing command `replaceMisspelling` in page.

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

`options` Object (optional). Properties:

* `silent` Boolean - Don't ask user for print settings, defaults to `false`
* `printBackground` Boolean - Also prints the background color and image of
  the web page, defaults to `false`.

Prints window's web page. When `silent` is set to `false`, Electron will pick
up system's default printer and default settings for printing.

Calling `window.print()` in web page is equivalent to call
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
* `pageSize` String - Specify page size of the generated PDF
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
* `data` Buffer - PDF file content

Prints window's web page as PDF with Chromium's preview printing custom
settings.

By default, an empty `options` will be regarded as:

```javascript
{
  marginsType:0,
  printBackgrounds:false,
  printSelectionOnly:false,
  landscape:false
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
      if (err)
        throw error;
      console.log("Write PDF successfully.");
    })
  })
});
```

### `webContents.addWorkSpace(path)`

* `path` String

Adds the specified path to devtools workspace.

### `webContents.removeWorkSpace(path)`

* `path` String

Removes the specified path from devtools workspace.

### `webContents.send(channel[, args...])`

* `channel` String
* `args...` (optional)

Send `args...` to the web page via `channel` in an asynchronous message, the web
page can handle it by listening to the `channel` event of the `ipc` module.

An example of sending messages from the main process to the renderer process:

```javascript
// On the main process.
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

1. The IPC message handler in web pages does not have an `event` parameter, which
   is different from the handlers on the main process.
2. There is no way to send synchronous messages from the main process to a
   renderer process, because it would be very easy to cause dead locks.
