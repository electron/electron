# webContents

> Render and control web pages.

`webContents` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).
It is responsible for rendering and controlling a web page and is a property of
the [`BrowserWindow`](browser-window.md) object. An example of accessing the
`webContents` object:

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({width: 800, height: 1500})
win.loadURL('http://github.com')

let contents = win.webContents
console.log(contents)
```

## Methods

These methods can be accessed from the `webContents` module:

```js
const {webContents} = require('electron')
console.log(webContents)
```

### `webContents.getAllWebContents()`

Returns an array of all web contents. This will contain web contents for all
windows, webviews, opened devtools, and devtools extension background pages.

### `webContents.getFocusedWebContents()`

Returns the web contents that is focused in this application, otherwise
returns `null`.

## Class: WebContents

### Instance Events

#### Event: 'did-finish-load'

Emitted when the navigation is done, i.e. the spinner of the tab has stopped
spinning, and the `onload` event was dispatched.

#### Event: 'did-fail-load'

Returns:

* `event` Event
* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

This event is like `did-finish-load` but emitted when the load failed or was
cancelled, e.g. `window.stop()` is invoked.
The full list of error codes and their meaning is available [here](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).
Note that redirect responses will emit `errorCode` -3; you may want to ignore
that error explicitly.

#### Event: 'did-frame-finish-load'

Returns:

* `event` Event
* `isMainFrame` Boolean

Emitted when a frame has done navigation.

#### Event: 'did-start-loading'

Corresponds to the points in time when the spinner of the tab started spinning.

#### Event: 'did-stop-loading'

Corresponds to the points in time when the spinner of the tab stopped spinning.

#### Event: 'did-get-response-details'

Returns:

* `event` Event
* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

Emitted when details regarding a requested resource are available.
`status` indicates the socket connection to download the resource.

#### Event: 'did-get-redirect-request'

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

#### Event: 'dom-ready'

Returns:

* `event` Event

Emitted when the document in the given frame is loaded.

#### Event: 'page-favicon-updated'

Returns:

* `event` Event
* `favicons` Array - Array of URLs

Emitted when page receives favicon urls.

#### Event: 'new-window'

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

#### Event: 'will-navigate'

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

#### Event: 'did-navigate'

Returns:

* `event` Event
* `url` String

Emitted when a navigation is done.

This event is not emitted for in-page navigations, such as clicking anchor links
or updating the `window.location.hash`. Use `did-navigate-in-page` event for
this purpose.

#### Event: 'did-navigate-in-page'

Returns:

* `event` Event
* `url` String

Emitted when an in-page navigation happened.

When in-page navigation happens, the page URL changes but does not cause
navigation outside of the page. Examples of this occurring are when anchor links
are clicked or when the DOM `hashchange` event is triggered.

#### Event: 'crashed'

Emitted when the renderer process has crashed.

#### Event: 'plugin-crashed'

Returns:

* `event` Event
* `name` String
* `version` String

Emitted when a plugin process has crashed.

#### Event: 'destroyed'

Emitted when `webContents` is destroyed.

#### Event: 'devtools-opened'

Emitted when DevTools is opened.

#### Event: 'devtools-closed'

Emitted when DevTools is closed.

#### Event: 'devtools-focused'

Emitted when DevTools is focused / opened.

#### Event: 'certificate-error'

Returns:

* `event` Event
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
  * `subjectName` String - Subject's Common Name
  * `serialNumber` String - Hex value represented string
  * `validStart` Integer - Start date of the certificate being valid in seconds
  * `validExpiry` Integer - End date of the certificate being valid in seconds
  * `fingerprint` String - Fingerprint of the certificate
* `callback` Function

Emitted when failed to verify the `certificate` for `url`.

The usage is the same with [the `certificate-error` event of
`app`](app.md#event-certificate-error).

#### Event: 'select-client-certificate'

Returns:

* `event` Event
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
  * `subjectName` String - Subject's Common Name
  * `serialNumber` String - Hex value represented string
  * `validStart` Integer - Start date of the certificate being valid in seconds
  * `validExpiry` Integer - End date of the certificate being valid in seconds
  * `fingerprint` String - Fingerprint of the certificate
* `callback` Function

Emitted when a client certificate is requested.

The usage is the same with [the `select-client-certificate` event of
`app`](app.md#event-select-client-certificate).

#### Event: 'login'

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

#### Event: 'found-in-page'

Returns:

* `event` Event
* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - Indicates if more responses are to follow.
  * `activeMatchOrdinal` Integer (optional) - Position of the active match.
  * `matches` Integer (optional) - Number of Matches.
  * `selectionArea` Object (optional) - Coordinates of first match region.

Emitted when a result is available for
[`webContents.findInPage`](web-contents.md#webcontentsfindinpage) request.

#### Event: 'media-started-playing'

Emitted when media starts playing.

#### Event: 'media-paused'

Emitted when media is paused or done playing.

#### Event: 'did-change-theme-color'

Emitted when a page's theme color changes. This is usually due to encountering
a meta tag:

```html
<meta name='theme-color' content='#ff0000'>
```

#### Event: 'update-target-url'

Returns:

* `event` Event
* `url` String

Emitted when mouse moves over a link or the keyboard moves the focus to a link.

#### Event: 'cursor-changed'

Returns:

* `event` Event
* `type` String
* `image` NativeImage (optional)
* `scale` Float (optional) - scaling factor for the custom cursor
* `size` Object (optional) - the size of the `image`
  * `width` Integer
  * `height` Integer
* `hotspot` Object (optional) - coordinates of the custom cursor's hotspot
  * `x` Integer - x coordinate
  * `y` Integer - y coordinate

Emitted when the cursor's type changes. The `type` parameter can be `default`,
`crosshair`, `pointer`, `text`, `wait`, `help`, `e-resize`, `n-resize`,
`ne-resize`, `nw-resize`, `s-resize`, `se-resize`, `sw-resize`, `w-resize`,
`ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `col-resize`,
`row-resize`, `m-panning`, `e-panning`, `n-panning`, `ne-panning`, `nw-panning`,
`s-panning`, `se-panning`, `sw-panning`, `w-panning`, `move`, `vertical-text`,
`cell`, `context-menu`, `alias`, `progress`, `nodrop`, `copy`, `none`,
`not-allowed`, `zoom-in`, `zoom-out`, `grab`, `grabbing`, `custom`.

If the `type` parameter is `custom`, the `image` parameter will hold the custom
cursor image in a `NativeImage`, and `scale`, `size` and `hotspot` will hold 
additional information about the custom cursor.

#### Event: 'context-menu'

Returns:

* `event` Event
* `params` Object
  * `x` Integer - x coordinate
  * `y` Integer - y coordinate
  * `linkURL` String - URL of the link that encloses the node the context menu
    was invoked on.
  * `linkText` String - Text associated with the link. May be an empty
    string if the contents of the link are an image.
  * `pageURL` String - URL of the top level page that the context menu was
    invoked on.
  * `frameURL` String - URL of the subframe that the context menu was invoked
    on.
  * `srcURL` String - Source URL for the element that the context menu
    was invoked on. Elements with source URLs are images, audio and video.
  * `mediaType` String - Type of the node the context menu was invoked on. Can
    be `none`, `image`, `audio`, `video`, `canvas`, `file` or `plugin`.
  * `hasImageContents` Boolean - Whether the context menu was invoked on an image
    which has non-empty contents.
  * `isEditable` Boolean - Whether the context is editable.
  * `selectionText` String - Text of the selection that the context menu was
    invoked on.
  * `titleText` String - Title or alt text of the selection that the context
    was invoked on.
  * `misspelledWord` String - The misspelled word under the cursor, if any.
  * `frameCharset` String - The character encoding of the frame on which the
    menu was invoked.
  * `inputFieldType` String - If the context menu was invoked on an input
    field, the type of that field. Possible values are `none`, `plainText`,
    `password`, `other`.
  * `menuSourceType` String - Input source that invoked the context menu.
    Can be `none`, `mouse`, `keyboard`, `touch`, `touchMenu`.
  * `mediaFlags` Object - The flags for the media element the context menu was
    invoked on. See more about this below.
  * `editFlags` Object - These flags indicate whether the renderer believes it is
    able to perform the corresponding action. See more about this below.

The `mediaFlags` is an object with the following properties:

* `inError` Boolean - Whether the media element has crashed.
* `isPaused` Boolean - Whether the media element is paused.
* `isMuted` Boolean - Whether the media element is muted.
* `hasAudio` Boolean - Whether the media element has audio.
* `isLooping` Boolean - Whether the media element is looping.
* `isControlsVisible` Boolean - Whether the media element's controls are
  visible.
* `canToggleControls` Boolean - Whether the media element's controls are
  toggleable.
* `canRotate` Boolean - Whether the media element can be rotated.

The `editFlags` is an object with the following properties:

* `canUndo` Boolean - Whether the renderer believes it can undo.
* `canRedo` Boolean - Whether the renderer believes it can redo.
* `canCut` Boolean - Whether the renderer believes it can cut.
* `canCopy` Boolean - Whether the renderer believes it can copy
* `canPaste` Boolean - Whether the renderer believes it can paste.
* `canDelete` Boolean - Whether the renderer believes it can delete.
* `canSelectAll` Boolean - Whether the renderer believes it can select all.

Emitted when there is a new context menu that needs to be handled.

#### Event: 'select-bluetooth-device'

Returns:

* `event` Event
* `devices` [Objects]
  * `deviceName` String
  * `deviceId` String
* `callback` Function
  * `deviceId` String

Emitted when bluetooth device needs to be selected on call to
`navigator.bluetooth.requestDevice`. To use `navigator.bluetooth` api
`webBluetooth` should be enabled.  If `event.preventDefault` is not called,
first available device will be selected. `callback` should be called with
`deviceId` to be selected, passing empty string to `callback` will
cancel the request.

```javascript
const {app, webContents} = require('electron')
app.commandLine.appendSwitch('enable-web-bluetooth')

app.on('ready', () => {
  webContents.on('select-bluetooth-device', (event, deviceList, callback) => {
    event.preventDefault()
    let result = deviceList.find((device) => {
      return device.deviceName === 'test'
    })
    if (!result) {
      callback('')
    } else {
      callback(result.deviceId)
    }
  })
})
```

#### Event: 'view-painted'

Emitted when a page's view is repainted.

### Instance Methods

#### `contents.loadURL(url[, options])`

* `url` URL
* `options` Object (optional)
  * `httpReferrer` String - A HTTP Referrer url.
  * `userAgent` String - A user agent originating the request.
  * `extraHeaders` String - Extra headers separated by "\n"

Loads the `url` in the window, the `url` must contain the protocol prefix,
e.g. the `http://` or `file://`. If the load should bypass http cache then
use the `pragma` header to achieve it.

```javascript
const {webContents} = require('electron')
const options = {extraHeaders: 'pragma: no-cache\n'}
webContents.loadURL('https://github.com', options)
```

#### `contents.downloadURL(url)`

* `url` URL

Initiates a download of the resource at `url` without navigating. The
`will-download` event of `session` will be triggered.

#### `contents.getURL()`

Returns URL of the current web page.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

let currentURL = win.webContents.getURL()
console.log(currentURL)
```

#### `contents.getTitle()`

Returns the title of the current web page.

#### `contents.isFocused()`

Returns a Boolean, whether the web page is focused.

#### `contents.isLoading()`

Returns whether web page is still loading resources.

#### `contents.isLoadingMainFrame()`

Returns whether the main frame (and not just iframes or frames within it) is
still loading.

#### `contents.isWaitingForResponse()`

Returns whether the web page is waiting for a first-response from the main
resource of the page.

#### `contents.stop()`

Stops any pending navigation.

#### `contents.reload()`

Reloads the current web page.

#### `contents.reloadIgnoringCache()`

Reloads current page and ignores cache.

#### `contents.canGoBack()`

Returns whether the browser can go back to previous web page.

#### `contents.canGoForward()`

Returns whether the browser can go forward to next web page.

#### `contents.canGoToOffset(offset)`

* `offset` Integer

Returns whether the web page can go to `offset`.

#### `contents.clearHistory()`

Clears the navigation history.

#### `contents.goBack()`

Makes the browser go back a web page.

#### `contents.goForward()`

Makes the browser go forward a web page.

#### `contents.goToIndex(index)`

* `index` Integer

Navigates browser to the specified absolute web page index.

#### `contents.goToOffset(offset)`

* `offset` Integer

Navigates to the specified offset from the "current entry".

#### `contents.isCrashed()`

Whether the renderer process has crashed.

#### `contents.setUserAgent(userAgent)`

* `userAgent` String

Overrides the user agent for this web page.

#### `contents.getUserAgent()`

Returns a `String` representing the user agent for this web page.

#### `contents.insertCSS(css)`

* `css` String

Injects CSS into the current web page.

#### `contents.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (optional)
* `callback` Function (optional) - Called after script has been executed.
  * `result`

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

#### `contents.setAudioMuted(muted)`

* `muted` Boolean

Mute the audio on the current web page.

#### `contents.isAudioMuted()`

Returns whether this page has been muted.

#### `contents.setZoomFactor(factor)`

* `factor` Number - Zoom factor.

Changes the zoom factor to the specified factor. Zoom factor is
zoom percent divided by 100, so 300% = 3.0.

#### `contents.setZoomLevel(level)`

* `level` Number - Zoom level

Changes the zoom level to the specified level. The original size is 0 and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

#### `contents.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum zoom level.`

#### `contents.undo()`

Executes the editing command `undo` in web page.

#### `contents.redo()`

Executes the editing command `redo` in web page.

#### `contents.cut()`

Executes the editing command `cut` in web page.

#### `contents.copy()`

Executes the editing command `copy` in web page.

### `contents.copyImageAt(x, y)`

* `x` Integer
* `y` Integer

Copy the image at the given position to the clipboard.

#### `contents.paste()`

Executes the editing command `paste` in web page.

#### `contents.pasteAndMatchStyle()`

Executes the editing command `pasteAndMatchStyle` in web page.

#### `contents.delete()`

Executes the editing command `delete` in web page.

#### `contents.selectAll()`

Executes the editing command `selectAll` in web page.

#### `contents.unselect()`

Executes the editing command `unselect` in web page.

#### `contents.replace(text)`

* `text` String

Executes the editing command `replace` in web page.

#### `contents.replaceMisspelling(text)`

* `text` String

Executes the editing command `replaceMisspelling` in web page.

#### `contents.insertText(text)`

* `text` String

Inserts `text` to the focused element.

#### `contents.findInPage(text[, options])`

* `text` String - Content to be searched, must not be empty.
* `options` Object (optional)
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

Starts a request to find all matches for the `text` in the web page and returns
an `Integer` representing the request id used for the request. The result of
the request can be obtained by subscribing to
[`found-in-page`](web-contents.md#event-found-in-page) event.

#### `contents.stopFindInPage(action)`

* `action` String - Specifies the action to take place when ending
  [`webContents.findInPage`](web-contents.md#webcontentfindinpage) request.
  * `clearSelection` - Clear the selection.
  * `keepSelection` - Translate the selection into a normal selection.
  * `activateSelection` - Focus and click the selection node.

Stops any `findInPage` request for the `webContents` with the provided `action`.

```javascript
const {webContents} = require('electron')
webContents.on('found-in-page', (event, result) => {
  if (result.finalUpdate) webContents.stopFindInPage('clearSelection')
})

const requestId = webContents.findInPage('api')
console.log(requestId)
```

#### `contents.capturePage([rect, ]callback)`

* `rect` Object (optional) - The area of the page to be captured
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

Captures a snapshot of the page within `rect`. Upon completion `callback` will
be called with `callback(image)`. The `image` is an instance of
[NativeImage](native-image.md) that stores data of the snapshot. Omitting
`rect` will capture the whole visible page.

#### `contents.hasServiceWorker(callback)`

* `callback` Function

Checks if any ServiceWorker is registered and returns a boolean as
response to `callback`.

#### `contents.unregisterServiceWorker(callback)`

* `callback` Function

Unregisters any ServiceWorker if present and returns a boolean as
response to `callback` when the JS promise is fulfilled or false
when the JS promise is rejected.

#### `contents.print([options])`

* `options` Object (optional)
  * `silent` Boolean - Don't ask user for print settings. Default is `false`.
  * `printBackground` Boolean - Also prints the background color and image of
    the web page. Default is `false`.

Prints window's web page. When `silent` is set to `true`, Electron will pick
up system's default printer and default settings for printing.

Calling `window.print()` in web page is equivalent to calling
`webContents.print({silent: false, printBackground: false})`.

#### `contents.printToPDF(options, callback)`

* `options` Object
  * `marginsType` Integer - Specifies the type of margins to use. Uses 0 for
    default margin, 1 for no margin, and 2 for minimum margin.
  * `pageSize` String - Specify page size of the generated PDF. Can be `A3`,
    `A4`, `A5`, `Legal`, `Letter`, `Tabloid` or an Object containing `height`
    and `width` in microns.
  * `printBackground` Boolean - Whether to print CSS backgrounds.
  * `printSelectionOnly` Boolean - Whether to print selection only.
  * `landscape` Boolean - `true` for landscape, `false` for portrait.
* `callback` Function

Prints window's web page as PDF with Chromium's preview printing custom
settings.

The `callback` will be called with `callback(error, data)` on completion. The
`data` is a `Buffer` that contains the generated PDF data.

By default, an empty `options` will be regarded as:

```
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

An example of `webContents.printToPDF`:

```javascript
const {BrowserWindow} = require('electron')
const fs = require('fs')

let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

win.webContents.on('did-finish-load', () => {
  // Use default printing options
  win.webContents.printToPDF({}, (error, data) => {
    if (error) throw error
    fs.writeFile('/tmp/print.pdf', data, (error) => {
      if (error) throw error
      console.log('Write PDF successfully.')
    })
  })
})
```

#### `contents.addWorkSpace(path)`

* `path` String

Adds the specified path to DevTools workspace. Must be used after DevTools
creation:

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.webContents.on('devtools-opened', () => {
  win.webContents.addWorkSpace(__dirname)
})
```

#### `contents.removeWorkSpace(path)`

* `path` String

Removes the specified path from DevTools workspace.

#### `contents.openDevTools([options])`

* `options` Object (optional)
  * `mode` String - Opens the devtools with specified dock state, can be
  `right`, `bottom`, `undocked`, `detach`. Defaults to last used dock state.
  In `undocked` mode it's possible to dock back. In `detach` mode it's not.

Opens the devtools.

#### `contents.closeDevTools()`

Closes the devtools.

#### `contents.isDevToolsOpened()`

Returns whether the devtools is opened.

#### `contents.isDevToolsFocused()`

Returns whether the devtools view is focused .

#### `contents.toggleDevTools()`

Toggles the developer tools.

#### `contents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

Starts inspecting element at position (`x`, `y`).

#### `contents.inspectServiceWorker()`

Opens the developer tools for the service worker context.

#### `contents.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

Send an asynchronous message to renderer process via `channel`, you can also
send arbitrary arguments. Arguments will be serialized in JSON internally and
hence no functions or prototype chain will be included.

The renderer process can handle the message by listening to `channel` with the
`ipcRenderer` module.

An example of sending messages from the main process to the renderer process:

```javascript
// In the main process.
const {app, BrowserWindow} = require('electron')
let win = null

app.on('ready', () => {
  win = new BrowserWindow({width: 800, height: 600})
  win.loadURL(`file://${__dirname}/index.html`)
  win.webContents.on('did-finish-load', () => {
    win.webContents.send('ping', 'whoooooooh!')
  })
})
```

```html
<!-- index.html -->
<html>
<body>
  <script>
    require('electron').ipcRenderer.on('ping', (event, message) => {
      console.log(message)  // Prints 'whoooooooh!'
    })
  </script>
</body>
</html>
```

#### `contents.enableDeviceEmulation(parameters)`

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
* `deviceScaleFactor` Float - Set the device scale factor (if zero defaults to
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

#### `contents.disableDeviceEmulation()`

Disable device emulation enabled by `webContents.enableDeviceEmulation`.

#### `contents.sendInputEvent(event)`

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

* `keyCode` String (**required**) - The character that will be sent
  as the keyboard event. Should only use the valid key codes in
  [Accelerator](accelerator.md).

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

#### `contents.beginFrameSubscription([onlyDirty ,]callback)`

* `onlyDirty` Boolean (optional) - Defaults to `false`
* `callback` Function

Begin subscribing for presentation events and captured frames, the `callback`
will be called with `callback(frameBuffer, dirtyRect)` when there is a
presentation event.

The `frameBuffer` is a `Buffer` that contains raw pixel data. On most machines,
the pixel data is effectively stored in 32bit BGRA format, but the actual
representation depends on the endianness of the processor (most modern
processors are little-endian, on machines with big-endian processors the data
is in 32bit ARGB format).

The `dirtyRect` is an object with `x, y, width, height` properties that
describes which part of the page was repainted. If `onlyDirty` is set to
`true`, `frameBuffer` will only contain the repainted area. `onlyDirty`
defaults to `false`.

#### `contents.endFrameSubscription()`

End subscribing for frame presentation events.

#### `contents.startDrag(item)`

* `item` object
  * `file` String
  * `icon` [NativeImage](native-image.md)

Sets the `item` as dragging item for current drag-drop operation, `file` is the
absolute path of the file to be dragged, and `icon` is the image showing under
the cursor when dragging.

#### `contents.savePage(fullPath, saveType, callback)`

* `fullPath` String - The full file path.
* `saveType` String - Specify the save type.
  * `HTMLOnly` - Save only the HTML of the page.
  * `HTMLComplete` - Save complete-html page.
  * `MHTML` - Save complete-html page as MHTML.
* `callback` Function - `(error) => {}`.
  * `error` Error

Returns true if the process of saving page has been initiated successfully.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

win.loadURL('https://github.com')

win.webContents.on('did-finish-load', () => {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete', (error) => {
    if (!error) console.log('Save page successfully')
  })
})
```

#### `contents.showDefinitionForSelection()` _macOS_

Shows pop-up dictionary that searches the selected word on the page.

### Instance Properties

#### `contents.id`

The unique ID of this WebContents.

#### `contents.session`

Returns the [session](session.md) object used by this webContents.

#### `contents.hostWebContents`

Returns the `WebContents` that might own this `WebContents`.

#### `contents.devToolsWebContents`

Get the `WebContents` of DevTools for this `WebContents`.

**Note:** Users should never store this object because it may become `null`
when the DevTools has been closed.

#### `contents.debugger`

Get the debugger instance for this webContents.

## Class: Debugger

Debugger API serves as an alternate transport for [remote debugging protocol][rdp].

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

try {
  win.webContents.debugger.attach('1.1')
} catch (err) {
  console.log('Debugger attach failed : ', err)
}

win.webContents.debugger.on('detach', (event, reason) => {
  console.log('Debugger detached due to : ', reason)
})

win.webContents.debugger.on('message', (event, method, params) => {
  if (method === 'Network.requestWillBeSent') {
    if (params.request.url === 'https://www.github.com') {
      win.webContents.debugger.detach()
    }
  }
})

win.webContents.debugger.sendCommand('Network.enable')
```

### Instance Methods

#### `debugger.attach([protocolVersion])`

* `protocolVersion` String (optional) - Requested debugging protocol version.

Attaches the debugger to the `webContents`.

#### `debugger.isAttached()`

Returns a boolean indicating whether a debugger is attached to the `webContents`.

#### `debugger.detach()`

Detaches the debugger from the `webContents`.

#### `debugger.sendCommand(method[, commandParams, callback])`

* `method` String - Method name, should be one of the methods defined by the
   remote debugging protocol.
* `commandParams` Object (optional) - JSON object with request parameters.
* `callback` Function (optional) - Response
  * `error` Object - Error message indicating the failure of the command.
  * `result` Object - Response defined by the 'returns' attribute of
     the command description in the remote debugging protocol.

Send given command to the debugging target.

### Instance Events

#### Event: 'detach'

* `event` Event
* `reason` String - Reason for detaching debugger.

Emitted when debugging session is terminated. This happens either when
`webContents` is closed or devtools is invoked for the attached `webContents`.

#### Event: 'message'

* `event` Event
* `method` String - Method name.
* `params` Object - Event parameters defined by the 'parameters'
   attribute in the remote debugging protocol.

Emitted whenever debugging target issues instrumentation event.

[rdp]: https://developer.chrome.com/devtools/docs/debugger-protocol
