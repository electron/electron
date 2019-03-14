# webContents

> Render and control web pages.

Process: [Main](../glossary.md#main-process)

`webContents` is an
[EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter).
It is responsible for rendering and controlling a web page and is a property of
the [`BrowserWindow`](browser-window.md) object. An example of accessing the
`webContents` object:

```javascript
const { BrowserWindow } = require('electron')

let win = new BrowserWindow({ width: 800, height: 1500 })
win.loadURL('http://github.com')

let contents = win.webContents
console.log(contents)
```

## Methods

These methods can be accessed from the `webContents` module:

```javascript
const { webContents } = require('electron')
console.log(webContents)
```

### `webContents.getAllWebContents()`

Returns `WebContents[]` - An array of all `WebContents` instances. This will contain web contents
for all windows, webviews, opened devtools, and devtools extension background pages.

### `webContents.getFocusedWebContents()`

Returns `WebContents` - The web contents that is focused in this application, otherwise
returns `null`.

### `webContents.fromId(id)`

* `id` Integer

Returns `WebContents` - A WebContents instance with the given ID.

## Class: WebContents

> Render and control the contents of a BrowserWindow instance.

Process: [Main](../glossary.md#main-process)

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
* `frameProcessId` Integer
* `frameRoutingId` Integer

This event is like `did-finish-load` but emitted when the load failed or was
cancelled, e.g. `window.stop()` is invoked.
The full list of error codes and their meaning is available [here](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

#### Event: 'did-frame-finish-load'

Returns:

* `event` Event
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted when a frame has done navigation.

#### Event: 'did-start-loading'

Corresponds to the points in time when the spinner of the tab started spinning.

#### Event: 'did-stop-loading'

Corresponds to the points in time when the spinner of the tab stopped spinning.

#### Event: 'dom-ready'

Returns:

* `event` Event

Emitted when the document in the given frame is loaded.

#### Event: 'page-favicon-updated'

Returns:

* `event` Event
* `favicons` String[] - Array of URLs.

Emitted when page receives favicon urls.

#### Event: 'new-window'

Returns:

* `event` Event
* `url` String
* `frameName` String
* `disposition` String - Can be `default`, `foreground-tab`, `background-tab`,
  `new-window`, `save-to-disk` and `other`.
* `options` Object - The options which will be used for creating the new
  [`BrowserWindow`](browser-window.md).
* `additionalFeatures` String[] - The non-standard features (features not handled
  by Chromium or Electron) given to `window.open()`.
* `referrer` [Referrer](structures/referrer.md) - The referrer that will be
  passed to the new window. May or may not result in the `Referer` header being
  sent, depending on the referrer policy.

Emitted when the page requests to open a new window for a `url`. It could be
requested by `window.open` or an external link like `<a target='_blank'>`.

By default a new `BrowserWindow` will be created for the `url`.

Calling `event.preventDefault()` will prevent Electron from automatically creating a
new [`BrowserWindow`](browser-window.md). If you call `event.preventDefault()` and manually create a new
[`BrowserWindow`](browser-window.md) then you must set `event.newGuest` to reference the new [`BrowserWindow`](browser-window.md)
instance, failing to do so may result in unexpected behavior. For example:

```javascript
myBrowserWindow.webContents.on('new-window', (event, url) => {
  event.preventDefault()
  const win = new BrowserWindow({ show: false })
  win.once('ready-to-show', () => win.show())
  win.loadURL(url)
  event.newGuest = win
})
```

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

#### Event: 'did-start-navigation'

Returns:

* `event` Event
* `url` String
* `isInPlace` Boolean
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted when any frame (including main) starts navigating. `isInplace` will be
`true` for in-page navigations.

#### Event: 'will-redirect'

Returns:

* `event` Event
* `url` String
* `isInPlace` Boolean
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted as a server side redirect occurs during navigation.  For example a 302
redirect.

This event will be emitted after `did-start-navigation` and always before the
`did-redirect-navigation` event for the same navigation.

Calling `event.preventDefault()` will prevent the navigation (not just the
redirect).

#### Event: 'did-redirect-navigation'

Returns:

* `event` Event
* `url` String
* `isInPlace` Boolean
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted after a server side redirect occurs during navigation.  For example a 302
redirect.

This event can not be prevented, if you want to prevent redirects you should
checkout out the `will-redirect` event above.

#### Event: 'did-navigate'

Returns:

* `event` Event
* `url` String
* `httpResponseCode` Integer - -1 for non HTTP navigations
* `httpStatusText` String - empty for non HTTP navigations

Emitted when a main frame navigation is done.

This event is not emitted for in-page navigations, such as clicking anchor links
or updating the `window.location.hash`. Use `did-navigate-in-page` event for
this purpose.

#### Event: 'did-frame-navigate'

Returns:

* `event` Event
* `url` String
* `httpResponseCode` Integer - -1 for non HTTP navigations
* `httpStatusText` String - empty for non HTTP navigations,
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted when any frame navigation is done.

This event is not emitted for in-page navigations, such as clicking anchor links
or updating the `window.location.hash`. Use `did-navigate-in-page` event for
this purpose.

#### Event: 'did-navigate-in-page'

Returns:

* `event` Event
* `url` String
* `isMainFrame` Boolean
* `frameProcessId` Integer
* `frameRoutingId` Integer

Emitted when an in-page navigation happened in any frame.

When in-page navigation happens, the page URL changes but does not cause
navigation outside of the page. Examples of this occurring are when anchor links
are clicked or when the DOM `hashchange` event is triggered.

#### Event: 'will-prevent-unload'

Returns:

* `event` Event

Emitted when a `beforeunload` event handler is attempting to cancel a page unload.

Calling `event.preventDefault()` will ignore the `beforeunload` event handler
and allow the page to be unloaded.

```javascript
const { BrowserWindow, dialog } = require('electron')
const win = new BrowserWindow({ width: 800, height: 600 })
win.webContents.on('will-prevent-unload', (event) => {
  const choice = dialog.showMessageBox(win, {
    type: 'question',
    buttons: ['Leave', 'Stay'],
    title: 'Do you want to leave this site?',
    message: 'Changes you made may not be saved.',
    defaultId: 0,
    cancelId: 1
  })
  const leave = (choice === 0)
  if (leave) {
    event.preventDefault()
  }
})
```

#### Event: 'crashed'

Returns:

* `event` Event
* `killed` Boolean

Emitted when the renderer process crashes or is killed.

#### Event: 'unresponsive'

Emitted when the web page becomes unresponsive.

#### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

#### Event: 'plugin-crashed'

Returns:

* `event` Event
* `name` String
* `version` String

Emitted when a plugin process has crashed.

#### Event: 'destroyed'

Emitted when `webContents` is destroyed.

#### Event: 'before-input-event'

Returns:

* `event` Event
* `input` Object - Input properties.
  * `type` String - Either `keyUp` or `keyDown`.
  * `key` String - Equivalent to [KeyboardEvent.key][keyboardevent].
  * `code` String - Equivalent to [KeyboardEvent.code][keyboardevent].
  * `isAutoRepeat` Boolean - Equivalent to [KeyboardEvent.repeat][keyboardevent].
  * `shift` Boolean - Equivalent to [KeyboardEvent.shiftKey][keyboardevent].
  * `control` Boolean - Equivalent to [KeyboardEvent.controlKey][keyboardevent].
  * `alt` Boolean - Equivalent to [KeyboardEvent.altKey][keyboardevent].
  * `meta` Boolean - Equivalent to [KeyboardEvent.metaKey][keyboardevent].

Emitted before dispatching the `keydown` and `keyup` events in the page.
Calling `event.preventDefault` will prevent the page `keydown`/`keyup` events
and the menu shortcuts.

To only prevent the menu shortcuts, use
[`setIgnoreMenuShortcuts`](#contentssetignoremenushortcutsignore-experimental):

```javascript
const { BrowserWindow } = require('electron')

let win = new BrowserWindow({ width: 800, height: 600 })

win.webContents.on('before-input-event', (event, input) => {
  // For example, only enable application menu keyboard shortcuts when
  // Ctrl/Cmd are down.
  win.webContents.setIgnoreMenuShortcuts(!input.control && !input.meta)
})
```

#### Event: 'enter-html-full-screen'

Emitted when the window enters a full-screen state triggered by HTML API.

#### Event: 'leave-html-full-screen'

Emitted when the window leaves a full-screen state triggered by HTML API.

#### Event: 'devtools-opened'

Emitted when DevTools is opened.

#### Event: 'devtools-closed'

Emitted when DevTools is closed.

#### Event: 'devtools-focused'

Emitted when DevTools is focused / opened.

#### Event: 'certificate-error'

Returns:

* `event` Event
* `url` String
* `error` String - The error code.
* `certificate` [Certificate](structures/certificate.md)
* `callback` Function
  * `isTrusted` Boolean - Indicates whether the certificate can be considered trusted.

Emitted when failed to verify the `certificate` for `url`.

The usage is the same with [the `certificate-error` event of
`app`](app.md#event-certificate-error).

#### Event: 'select-client-certificate'

Returns:

* `event` Event
* `url` URL
* `certificateList` [Certificate[]](structures/certificate.md)
* `callback` Function
  * `certificate` [Certificate](structures/certificate.md) - Must be a certificate from the given list.

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
  * `username` String
  * `password` String

Emitted when `webContents` wants to do basic auth.

The usage is the same with [the `login` event of `app`](app.md#event-login).

#### Event: 'found-in-page'

Returns:

* `event` Event
* `result` Object
  * `requestId` Integer
  * `activeMatchOrdinal` Integer - Position of the active match.
  * `matches` Integer - Number of Matches.
  * `selectionArea` Object - Coordinates of first match region.
  * `finalUpdate` Boolean

Emitted when a result is available for
[`webContents.findInPage`] request.

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

Returns:

* `event` Event
* `color` (String | null) - Theme color is in format of '#rrggbb'. It is `null` when no theme color is set.

#### Event: 'update-target-url'

Returns:

* `event` Event
* `url` String

Emitted when mouse moves over a link or the keyboard moves the focus to a link.

#### Event: 'cursor-changed'

Returns:

* `event` Event
* `type` String
* `image` [NativeImage](native-image.md) (optional)
* `scale` Float (optional) - scaling factor for the custom cursor.
* `size` [Size](structures/size.md) (optional) - the size of the `image`.
* `hotspot` [Point](structures/point.md) (optional) - coordinates of the custom cursor's hotspot.

Emitted when the cursor's type changes. The `type` parameter can be `default`,
`crosshair`, `pointer`, `text`, `wait`, `help`, `e-resize`, `n-resize`,
`ne-resize`, `nw-resize`, `s-resize`, `se-resize`, `sw-resize`, `w-resize`,
`ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `col-resize`,
`row-resize`, `m-panning`, `e-panning`, `n-panning`, `ne-panning`, `nw-panning`,
`s-panning`, `se-panning`, `sw-panning`, `w-panning`, `move`, `vertical-text`,
`cell`, `context-menu`, `alias`, `progress`, `nodrop`, `copy`, `none`,
`not-allowed`, `zoom-in`, `zoom-out`, `grab`, `grabbing` or `custom`.

If the `type` parameter is `custom`, the `image` parameter will hold the custom
cursor image in a [`NativeImage`](native-image.md), and `scale`, `size` and `hotspot` will hold
additional information about the custom cursor.

#### Event: 'context-menu'

Returns:

* `event` Event
* `params` Object
  * `x` Integer - x coordinate.
  * `y` Integer - y coordinate.
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
    Can be `none`, `mouse`, `keyboard`, `touch` or `touchMenu`.
  * `mediaFlags` Object - The flags for the media element the context menu was
    invoked on.
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
  * `editFlags` Object - These flags indicate whether the renderer believes it
    is able to perform the corresponding action.
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
* `devices` [BluetoothDevice[]](structures/bluetooth-device.md)
* `callback` Function
  * `deviceId` String

Emitted when bluetooth device needs to be selected on call to
`navigator.bluetooth.requestDevice`. To use `navigator.bluetooth` api
`webBluetooth` should be enabled. If `event.preventDefault` is not called,
first available device will be selected. `callback` should be called with
`deviceId` to be selected, passing empty string to `callback` will
cancel the request.

```javascript
const { app, BrowserWindow } = require('electron')

let win = null
app.commandLine.appendSwitch('enable-experimental-web-platform-features')

app.on('ready', () => {
  win = new BrowserWindow({ width: 800, height: 600 })
  win.webContents.on('select-bluetooth-device', (event, deviceList, callback) => {
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

#### Event: 'paint'

Returns:

* `event` Event
* `dirtyRect` [Rectangle](structures/rectangle.md)
* `image` [NativeImage](native-image.md) - The image data of the whole frame.

Emitted when a new frame is generated. Only the dirty area is passed in the
buffer.

```javascript
const { BrowserWindow } = require('electron')

let win = new BrowserWindow({ webPreferences: { offscreen: true } })
win.webContents.on('paint', (event, dirty, image) => {
  // updateBitmap(dirty, image.getBitmap())
})
win.loadURL('http://github.com')
```

#### Event: 'devtools-reload-page'

Emitted when the devtools window instructs the webContents to reload

#### Event: 'will-attach-webview'

Returns:

* `event` Event
* `webPreferences` Object - The web preferences that will be used by the guest
  page. This object can be modified to adjust the preferences for the guest
  page.
* `params` Object - The other `<webview>` parameters such as the `src` URL.
  This object can be modified to adjust the parameters of the guest page.

Emitted when a `<webview>`'s web contents is being attached to this web
contents. Calling `event.preventDefault()` will destroy the guest page.

This event can be used to configure `webPreferences` for the `webContents`
of a `<webview>` before it's loaded, and provides the ability to set settings
that can't be set via `<webview>` attributes.

**Note:** The specified `preload` script option will be appear as `preloadURL`
(not `preload`) in the `webPreferences` object emitted with this event.

#### Event: 'did-attach-webview'

Returns:

* `event` Event
* `webContents` WebContents - The guest web contents that is used by the
  `<webview>`.

Emitted when a `<webview>` has been attached to this web contents.

#### Event: 'console-message'

Returns:

* `event` Event
* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

Emitted when the associated window logs a console message. Will not be emitted
for windows with *offscreen rendering* enabled.

#### Event: 'preload-error'

Returns:

* `event` Event
* `preloadPath` String
* `error` Error

Emitted when the preload script `preloadPath` throws an unhandled exception `error`.

#### Event: 'ipc-message'

Returns:

* `event` Event
* `channel` String
* `...args` any[]

Emitted when the renderer process sends an asynchronous message via `ipcRenderer.send()`.

#### Event: 'ipc-message-sync'

Returns:

* `event` Event
* `channel` String
* `...args` any[]

Emitted when the renderer process sends a synchronous message via `ipcRenderer.sendSync()`.

#### Event: 'desktop-capturer-get-sources'

Returns:

* `event` Event

Emitted when `desktopCapturer.getSources()` is called in the renderer process.
Calling `event.preventDefault()` will make it return empty sources.

#### Event: 'remote-require'

Returns:

* `event` Event
* `moduleName` String

Emitted when `remote.require()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the module from being returned.
Custom value can be returned by setting `event.returnValue`.

#### Event: 'remote-get-global'

Returns:

* `event` Event
* `globalName` String

Emitted when `remote.getGlobal()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the global from being returned.
Custom value can be returned by setting `event.returnValue`.

#### Event: 'remote-get-builtin'

Returns:

* `event` Event
* `moduleName` String

Emitted when `remote.getBuiltin()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the module from being returned.
Custom value can be returned by setting `event.returnValue`.

#### Event: 'remote-get-current-window'

Returns:

* `event` Event

Emitted when `remote.getCurrentWindow()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the object from being returned.
Custom value can be returned by setting `event.returnValue`.

#### Event: 'remote-get-current-web-contents'

Returns:

* `event` Event

Emitted when `remote.getCurrentWebContents()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the object from being returned.
Custom value can be returned by setting `event.returnValue`.

#### Event: 'remote-get-guest-web-contents'

Returns:

* `event` Event
* `guestWebContents` [WebContents](web-contents.md)

Emitted when `<webview>.getWebContents()` is called in the renderer process.
Calling `event.preventDefault()` will prevent the object from being returned.
Custom value can be returned by setting `event.returnValue`.

### Instance Methods

#### `contents.loadURL(url[, options])`

* `url` String
* `options` Object (optional)
  * `httpReferrer` (String | [Referrer](structures/referrer.md)) (optional) - An HTTP Referrer url.
  * `userAgent` String (optional) - A user agent originating the request.
  * `extraHeaders` String (optional) - Extra headers separated by "\n".
  * `postData` ([UploadRawData[]](structures/upload-raw-data.md) | [UploadFile[]](structures/upload-file.md) | [UploadBlob[]](structures/upload-blob.md)) (optional)
  * `baseURLForDataURL` String (optional) - Base url (with trailing path separator) for files to be loaded by the data url. This is needed only if the specified `url` is a data url and needs to load other files.

Returns `Promise<void>` - the promise will resolve when the page has finished loading
(see [`did-finish-load`](web-contents.md#event-did-finish-load)), and rejects
if the page fails to load (see
[`did-fail-load`](web-contents.md#event-did-fail-load)).

Loads the `url` in the window. The `url` must contain the protocol prefix,
e.g. the `http://` or `file://`. If the load should bypass http cache then
use the `pragma` header to achieve it.

```javascript
const { webContents } = require('electron')
const options = { extraHeaders: 'pragma: no-cache\n' }
webContents.loadURL('https://github.com', options)
```

#### `contents.loadFile(filePath[, options])`

* `filePath` String
* `options` Object (optional)
  * `query` Object (optional) - Passed to `url.format()`.
  * `search` String (optional) - Passed to `url.format()`.
  * `hash` String (optional) - Passed to `url.format()`.

Returns `Promise<void>` - the promise will resolve when the page has finished loading
(see [`did-finish-load`](web-contents.md#event-did-finish-load)), and rejects
if the page fails to load (see [`did-fail-load`](web-contents.md#event-did-fail-load)).

Loads the given file in the window, `filePath` should be a path to
an HTML file relative to the root of your application.  For instance
an app structure like this:

```sh
| root
| - package.json
| - src
|   - main.js
|   - index.html
```

Would require code like this

```js
win.loadFile('src/index.html')
```

#### `contents.downloadURL(url)`

* `url` String

Initiates a download of the resource at `url` without navigating. The
`will-download` event of `session` will be triggered.

#### `contents.getURL()`

Returns `String` - The URL of the current web page.

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow({ width: 800, height: 600 })
win.loadURL('http://github.com')

let currentURL = win.webContents.getURL()
console.log(currentURL)
```

#### `contents.getTitle()`

Returns `String` - The title of the current web page.

#### `contents.isDestroyed()`

Returns `Boolean` - Whether the web page is destroyed.

#### `contents.focus()`

Focuses the web page.

#### `contents.isFocused()`

Returns `Boolean` - Whether the web page is focused.

#### `contents.isLoading()`

Returns `Boolean` - Whether web page is still loading resources.

#### `contents.isLoadingMainFrame()`

Returns `Boolean` - Whether the main frame (and not just iframes or frames within it) is
still loading.

#### `contents.isWaitingForResponse()`

Returns `Boolean` - Whether the web page is waiting for a first-response from the main
resource of the page.

#### `contents.stop()`

Stops any pending navigation.

#### `contents.reload()`

Reloads the current web page.

#### `contents.reloadIgnoringCache()`

Reloads current page and ignores cache.

#### `contents.canGoBack()`

Returns `Boolean` - Whether the browser can go back to previous web page.

#### `contents.canGoForward()`

Returns `Boolean` - Whether the browser can go forward to next web page.

#### `contents.canGoToOffset(offset)`

* `offset` Integer

Returns `Boolean` - Whether the web page can go to `offset`.

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

Returns `Boolean` - Whether the renderer process has crashed.

#### `contents.setUserAgent(userAgent)`

* `userAgent` String

Overrides the user agent for this web page.

#### `contents.getUserAgent()`

Returns `String` - The user agent for this web page.

#### `contents.insertCSS(css)`

* `css` String

Injects CSS into the current web page.

```js
contents.on('did-finish-load', function () {
  contents.insertCSS('html, body { background-color: #f00; }')
})
```

#### `contents.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (optional) - Default is `false`.
* `callback` Function (optional) - Called after script has been executed.
  * `result` Any

Returns `Promise<any>` - A promise that resolves with the result of the executed code
or is rejected if the result of the code is a rejected promise.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

```js
contents.executeJavaScript('fetch("https://jsonplaceholder.typicode.com/users/1").then(resp => resp.json())', true)
  .then((result) => {
    console.log(result) // Will be the JSON object from the fetch call
  })
```

**[Deprecated Soon](promisification.md)**

#### `contents.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (optional) - Default is `false`.

Returns `Promise<any>` - A promise that resolves with the result of the executed code
or is rejected if the result of the code is a rejected promise.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

```js
contents.executeJavaScript('fetch("https://jsonplaceholder.typicode.com/users/1").then(resp => resp.json())', true)
  .then((result) => {
    console.log(result) // Will be the JSON object from the fetch call
  })
```

#### `contents.setIgnoreMenuShortcuts(ignore)` _Experimental_

* `ignore` Boolean

Ignore application menu shortcuts while this web contents is focused.

#### `contents.setAudioMuted(muted)`

* `muted` Boolean

Mute the audio on the current web page.

#### `contents.isAudioMuted()`

Returns `Boolean` - Whether this page has been muted.

#### `contents.isCurrentlyAudible()`

Returns `Boolean` - Whether audio is currently playing.

#### `contents.setZoomFactor(factor)`

* `factor` Number - Zoom factor.

Changes the zoom factor to the specified factor. Zoom factor is
zoom percent divided by 100, so 300% = 3.0.

#### `contents.getZoomFactor()`

Returns `Number` - the current zoom factor.

#### `contents.setZoomLevel(level)`

* `level` Number - Zoom level.

Changes the zoom level to the specified level. The original size is 0 and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively. The formula for this is
`scale := 1.2 ^ level`.

#### `contents.getZoomLevel()`

Returns `Number` - the current zoom level.

#### `contents.setVisualZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum pinch-to-zoom level.

> **NOTE**: Visual zoom is disabled by default in Electron. To re-enable it, call:
>
> ```js
> contents.setVisualZoomLevelLimits(1, 3)
> ```

#### `contents.setLayoutZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum layout-based (i.e. non-visual) zoom level.

#### `contents.undo()`

Executes the editing command `undo` in web page.

#### `contents.redo()`

Executes the editing command `redo` in web page.

#### `contents.cut()`

Executes the editing command `cut` in web page.

#### `contents.copy()`

Executes the editing command `copy` in web page.

#### `contents.copyImageAt(x, y)`

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
  * `forward` Boolean (optional) - Whether to search forward or backward, defaults to `true`.
  * `findNext` Boolean (optional) - Whether the operation is first request or a follow up,
    defaults to `false`.
  * `matchCase` Boolean (optional) - Whether search should be case-sensitive,
    defaults to `false`.
  * `wordStart` Boolean (optional) - Whether to look only at the start of words.
    defaults to `false`.
  * `medialCapitalAsWordStart` Boolean (optional) - When combined with `wordStart`,
    accepts a match in the middle of a word if the match begins with an
    uppercase letter followed by a lowercase or non-letter.
    Accepts several other intra-word matches, defaults to `false`.

Returns `Integer` - The request id used for the request.

Starts a request to find all matches for the `text` in the web page. The result of the request
can be obtained by subscribing to [`found-in-page`](web-contents.md#event-found-in-page) event.

#### `contents.stopFindInPage(action)`

* `action` String - Specifies the action to take place when ending
  [`webContents.findInPage`] request.
  * `clearSelection` - Clear the selection.
  * `keepSelection` - Translate the selection into a normal selection.
  * `activateSelection` - Focus and click the selection node.

Stops any `findInPage` request for the `webContents` with the provided `action`.

```javascript
const { webContents } = require('electron')
webContents.on('found-in-page', (event, result) => {
  if (result.finalUpdate) webContents.stopFindInPage('clearSelection')
})

const requestId = webContents.findInPage('api')
console.log(requestId)
```

#### `contents.capturePage([rect, ]callback)`

* `rect` [Rectangle](structures/rectangle.md) (optional) - The bounds to capture
* `callback` Function
  * `image` [NativeImage](native-image.md)

Captures a snapshot of the page within `rect`. Upon completion `callback` will
be called with `callback(image)`. The `image` is an instance of [NativeImage](native-image.md)
that stores data of the snapshot. Omitting `rect` will capture the whole visible page.

**[Deprecated Soon](promisification.md)**

#### `contents.capturePage([rect])`

* `rect` [Rectangle](structures/rectangle.md) (optional) - The area of the page to be captured.

* Returns `Promise<NativeImage>` - Resolves with a [NativeImage](native-image.md)

Captures a snapshot of the page within `rect`. Omitting `rect` will capture the whole visible page.

#### `contents.getPrinters()`

Get the system printer list.

Returns [`PrinterInfo[]`](structures/printer-info.md).

#### `contents.print([options], [callback])`

* `options` Object (optional)
  * `silent` Boolean (optional) - Don't ask user for print settings. Default is `false`.
  * `printBackground` Boolean (optional) - Also prints the background color and image of
    the web page. Default is `false`.
  * `deviceName` String (optional) - Set the printer device name to use. Default is `''`.
* `callback` Function (optional)
  * `success` Boolean - Indicates success of the print call.

Prints window's web page. When `silent` is set to `true`, Electron will pick
the system's default printer if `deviceName` is empty and the default settings
for printing.

Calling `window.print()` in web page is equivalent to calling
`webContents.print({ silent: false, printBackground: false, deviceName: '' })`.

Use `page-break-before: always; ` CSS style to force to print to a new page.

#### `contents.printToPDF(options, callback)`

* `options` Object
  * `marginsType` Integer (optional) - Specifies the type of margins to use. Uses 0 for
    default margin, 1 for no margin, and 2 for minimum margin.
  * `pageSize` String | Size (optional) - Specify page size of the generated PDF. Can be `A3`,
    `A4`, `A5`, `Legal`, `Letter`, `Tabloid` or an Object containing `height`
    and `width` in microns.
  * `printBackground` Boolean (optional) - Whether to print CSS backgrounds.
  * `printSelectionOnly` Boolean (optional) - Whether to print selection only.
  * `landscape` Boolean (optional) - `true` for landscape, `false` for portrait.
* `callback` Function
  * `error` Error
  * `data` Buffer

Prints window's web page as PDF with Chromium's preview printing custom
settings.

The `callback` will be called with `callback(error, data)` on completion. The
`data` is a `Buffer` that contains the generated PDF data.

**[Deprecated Soon](promisification.md)**

#### `contents.printToPDF(options)`

* `options` Object
  * `marginsType` Integer (optional) - Specifies the type of margins to use. Uses 0 for
    default margin, 1 for no margin, and 2 for minimum margin.
  * `pageSize` String | Size (optional) - Specify page size of the generated PDF. Can be `A3`,
    `A4`, `A5`, `Legal`, `Letter`, `Tabloid` or an Object containing `height`
    and `width` in microns.
  * `printBackground` Boolean (optional) - Whether to print CSS backgrounds.
  * `printSelectionOnly` Boolean (optional) - Whether to print selection only.
  * `landscape` Boolean (optional) - `true` for landscape, `false` for portrait.

* Returns `Promise<Buffer>` - Resolves with the generated PDF data.

Prints window's web page as PDF with Chromium's preview printing custom
settings.

The `landscape` will be ignored if `@page` CSS at-rule is used in the web page.

By default, an empty `options` will be regarded as:

```javascript
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

Use `page-break-before: always; ` CSS style to force to print to a new page.

An example of `webContents.printToPDF`:

```javascript
const { BrowserWindow } = require('electron')
const fs = require('fs')

let win = new BrowserWindow({ width: 800, height: 600 })
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
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()
win.webContents.on('devtools-opened', () => {
  win.webContents.addWorkSpace(__dirname)
})
```

#### `contents.removeWorkSpace(path)`

* `path` String

Removes the specified path from DevTools workspace.

#### `contents.setDevToolsWebContents(devToolsWebContents)`

* `devToolsWebContents` WebContents

Uses the `devToolsWebContents` as the target `WebContents` to show devtools.

The `devToolsWebContents` must not have done any navigation, and it should not
be used for other purposes after the call.

By default Electron manages the devtools by creating an internal `WebContents`
with native view, which developers have very limited control of. With the
`setDevToolsWebContents` method, developers can use any `WebContents` to show
the devtools in it, including `BrowserWindow`, `BrowserView` and `<webview>`
tag.

Note that closing the devtools does not destroy the `devToolsWebContents`, it
is caller's responsibility to destroy `devToolsWebContents`.

An example of showing devtools in a `<webview>` tag:

```html
<html>
<head>
  <style type="text/css">
    * { margin: 0; }
    #browser { height: 70%; }
    #devtools { height: 30%; }
  </style>
</head>
<body>
  <webview id="browser" src="https://github.com"></webview>
  <webview id="devtools"></webview>
  <script>
    const browserView = document.getElementById('browser')
    const devtoolsView = document.getElementById('devtools')
    browserView.addEventListener('dom-ready', () => {
      const browser = browserView.getWebContents()
      browser.setDevToolsWebContents(devtoolsView.getWebContents())
      browser.openDevTools()
    })
  </script>
</body>
</html>
```

An example of showing devtools in a `BrowserWindow`:

```js
const { app, BrowserWindow } = require('electron')

let win = null
let devtools = null

app.once('ready', () => {
  win = new BrowserWindow()
  devtools = new BrowserWindow()
  win.loadURL('https://github.com')
  win.webContents.setDevToolsWebContents(devtools.webContents)
  win.webContents.openDevTools({ mode: 'detach' })
})
```

#### `contents.openDevTools([options])`

* `options` Object (optional)
  * `mode` String - Opens the devtools with specified dock state, can be
    `right`, `bottom`, `undocked`, `detach`. Defaults to last used dock state.
    In `undocked` mode it's possible to dock back. In `detach` mode it's not.
  * `activate` Boolean (optional) - Whether to bring the opened devtools window
    to the foreground. The default is `true`.

Opens the devtools.

When `contents` is a `<webview>` tag, the `mode` would be `detach` by default,
explicitly passing an empty `mode` can force using last used dock state.

#### `contents.closeDevTools()`

Closes the devtools.

#### `contents.isDevToolsOpened()`

Returns `Boolean` - Whether the devtools is opened.

#### `contents.isDevToolsFocused()`

Returns `Boolean` - Whether the devtools view is focused .

#### `contents.toggleDevTools()`

Toggles the developer tools.

#### `contents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

Starts inspecting element at position (`x`, `y`).

#### `contents.inspectSharedWorker()`

Opens the developer tools for the shared worker context.

#### `contents.inspectServiceWorker()`

Opens the developer tools for the service worker context.

#### `contents.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `...args` any[]

Send an asynchronous message to renderer process via `channel`, you can also
send arbitrary arguments. Arguments will be serialized in JSON internally and
hence no functions or prototype chain will be included.

The renderer process can handle the message by listening to `channel` with the
[`ipcRenderer`](ipc-renderer.md) module.

An example of sending messages from the main process to the renderer process:

```javascript
// In the main process.
const { app, BrowserWindow } = require('electron')
let win = null

app.on('ready', () => {
  win = new BrowserWindow({ width: 800, height: 600 })
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
      console.log(message) // Prints 'whoooooooh!'
    })
  </script>
</body>
</html>
```

#### `contents.sendToFrame(frameId, channel[, arg1][, arg2][, ...])`

* `frameId` Integer
* `channel` String
* `...args` any[]

Send an asynchronous message to a specific frame in a renderer process via
`channel`. Arguments will be serialized
as JSON internally and as such no functions or prototype chains will be included.

The renderer process can handle the message by listening to `channel` with the
[`ipcRenderer`](ipc-renderer.md) module.

If you want to get the `frameId` of a given renderer context you should use
the `webFrame.routingId` value.  E.g.

```js
// In a renderer process
console.log('My frameId is:', require('electron').webFrame.routingId)
```

You can also read `frameId` from all incoming IPC messages in the main process.

```js
// In the main process
ipcMain.on('ping', (event) => {
  console.info('Message came from frameId:', event.frameId)
})
```

#### `contents.enableDeviceEmulation(parameters)`

* `parameters` Object
  * `screenPosition` String - Specify the screen type to emulate
      (default: `desktop`):
    * `desktop` - Desktop screen type.
    * `mobile` - Mobile screen type.
  * `screenSize` [Size](structures/size.md) - Set the emulated screen size (screenPosition == mobile).
  * `viewPosition` [Point](structures/point.md) - Position the view on the screen
      (screenPosition == mobile) (default: `{ x: 0, y: 0 }`).
  * `deviceScaleFactor` Integer - Set the device scale factor (if zero defaults to
      original device scale factor) (default: `0`).
  * `viewSize` [Size](structures/size.md) - Set the emulated view size (empty means no override)
  * `scale` Float - Scale of emulated view inside available space (not in fit to
      view mode) (default: `1`).

Enable device emulation with the given parameters.

#### `contents.disableDeviceEmulation()`

Disable device emulation enabled by `webContents.enableDeviceEmulation`.

#### `contents.sendInputEvent(event)`

* `event` Object
  * `type` String (**required**) - The type of the event, can be `mouseDown`,
    `mouseUp`, `mouseEnter`, `mouseLeave`, `contextMenu`, `mouseWheel`,
    `mouseMove`, `keyDown`, `keyUp` or `char`.
  * `modifiers` String[] - An array of modifiers of the event, can
    include `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

Sends an input `event` to the page.
**Note:** The [`BrowserWindow`](browser-window.md) containing the contents needs to be focused for
`sendInputEvent()` to work.

For keyboard events, the `event` object also have following properties:

* `keyCode` String (**required**) - The character that will be sent
  as the keyboard event. Should only use the valid key codes in
  [Accelerator](accelerator.md).

For mouse events, the `event` object also have following properties:

* `x` Integer (**required**)
* `y` Integer (**required**)
* `button` String - The button pressed, can be `left`, `middle`, `right`.
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

* `onlyDirty` Boolean (optional) - Defaults to `false`.
* `callback` Function
  * `image` [NativeImage](native-image.md)
  * `dirtyRect` [Rectangle](structures/rectangle.md)

Begin subscribing for presentation events and captured frames, the `callback`
will be called with `callback(image, dirtyRect)` when there is a presentation
event.

The `image` is an instance of [NativeImage](native-image.md) that stores the
captured frame.

The `dirtyRect` is an object with `x, y, width, height` properties that
describes which part of the page was repainted. If `onlyDirty` is set to
`true`, `image` will only contain the repainted area. `onlyDirty` defaults to
`false`.

#### `contents.endFrameSubscription()`

End subscribing for frame presentation events.

#### `contents.startDrag(item)`

* `item` Object
  * `file` String or `files` Array - The path(s) to the file(s) being dragged.
  * `icon` [NativeImage](native-image.md) - The image must be non-empty on
    macOS.

Sets the `item` as dragging item for current drag-drop operation, `file` is the
absolute path of the file to be dragged, and `icon` is the image showing under
the cursor when dragging.

#### `contents.savePage(fullPath, saveType)`

* `fullPath` String - The full file path.
* `saveType` String - Specify the save type.
  * `HTMLOnly` - Save only the HTML of the page.
  * `HTMLComplete` - Save complete-html page.
  * `MHTML` - Save complete-html page as MHTML.

Returns `Promise<void>` - resolves if the page is saved.

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()

win.loadURL('https://github.com')

win.webContents.on('did-finish-load', async () => {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete').then(() => {
    console.log('Page was saved successfully.')
  }).catch(err => {
    console.log(err)
  })
})
```

#### `contents.showDefinitionForSelection()` _macOS_

Shows pop-up dictionary that searches the selected word on the page.

#### `contents.isOffscreen()`

Returns `Boolean` - Indicates whether *offscreen rendering* is enabled.

#### `contents.startPainting()`

If *offscreen rendering* is enabled and not painting, start painting.

#### `contents.stopPainting()`

If *offscreen rendering* is enabled and painting, stop painting.

#### `contents.isPainting()`

Returns `Boolean` - If *offscreen rendering* is enabled returns whether it is currently painting.

#### `contents.setFrameRate(fps)`

* `fps` Integer

If *offscreen rendering* is enabled sets the frame rate to the specified number.
Only values between 1 and 60 are accepted.

#### `contents.getFrameRate()`

Returns `Integer` - If *offscreen rendering* is enabled returns the current frame rate.

#### `contents.invalidate()`

Schedules a full repaint of the window this web contents is in.

If *offscreen rendering* is enabled invalidates the frame and generates a new
one through the `'paint'` event.

#### `contents.getWebRTCIPHandlingPolicy()`

Returns `String` - Returns the WebRTC IP Handling Policy.

#### `contents.setWebRTCIPHandlingPolicy(policy)`

* `policy` String - Specify the WebRTC IP Handling Policy.
  * `default` - Exposes user's public and local IPs. This is the default
  behavior. When this policy is used, WebRTC has the right to enumerate all
  interfaces and bind them to discover public interfaces.
  * `default_public_interface_only` - Exposes user's public IP, but does not
  expose user's local IP. When this policy is used, WebRTC should only use the
  default route used by http. This doesn't expose any local addresses.
  * `default_public_and_private_interfaces` - Exposes user's public and local
  IPs. When this policy is used, WebRTC should only use the default route used
  by http. This also exposes the associated default private address. Default
  route is the route chosen by the OS on a multi-homed endpoint.
  * `disable_non_proxied_udp` - Does not expose public or local IPs. When this
  policy is used, WebRTC should only use TCP to contact peers or servers unless
  the proxy server supports UDP.

Setting the WebRTC IP handling policy allows you to control which IPs are
exposed via WebRTC. See [BrowserLeaks](https://browserleaks.com/webrtc) for
more details.

#### `contents.getOSProcessId()`

Returns `Integer` - The operating system `pid` of the associated renderer
process.

#### `contents.getProcessId()`

Returns `Integer` - The Chromium internal `pid` of the associated renderer. Can
be compared to the `frameProcessId` passed by frame specific navigation events
(e.g. `did-frame-navigate`)

#### `contents.takeHeapSnapshot(filePath)`

* `filePath` String - Path to the output file.

Returns `Promise<void>` - Indicates whether the snapshot has been created successfully.

Takes a V8 heap snapshot and saves it to `filePath`.

#### `contents.setBackgroundThrottling(allowed)`

* `allowed` Boolean

Controls whether or not this WebContents will throttle animations and timers
when the page becomes backgrounded. This also affects the Page Visibility API.

#### `contents.getType()`

Returns `String` - the type of the webContent. Can be `backgroundPage`, `window`, `browserView`, `remote`, `webview` or `offscreen`.

### Instance Properties

#### `contents.id`

A `Integer` representing the unique ID of this WebContents.

#### `contents.session`

A [`Session`](session.md) used by this webContents.

#### `contents.hostWebContents`

A [`WebContents`](web-contents.md) instance that might own this `WebContents`.

#### `contents.devToolsWebContents`

A `WebContents` of DevTools for this `WebContents`.

**Note:** Users should never store this object because it may become `null`
when the DevTools has been closed.

#### `contents.debugger`

A [Debugger](debugger.md) instance for this webContents.

[keyboardevent]: https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent
