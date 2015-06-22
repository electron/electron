# `<webview>` 태그

Use the `webview` tag to embed 'guest' content (such as web pages) in your
Electron app. The guest content is contained within the `webview` container;
an embedder page within your app controls how the guest content is laid out and
rendered.

Different from the `iframe`, the `webview` runs in a separate process than your
app; it doesn't have the same permissions as your web page and all interactions
between your app and embedded content will be asynchronous. This keeps your app
safe from the embedded content.

## 예제

To embed a web page in your app, add the `webview` tag to your app's embedder
page (this is the app page that will display the guest content). In its simplest
form, the `webview` tag includes the `src` of the web page and css styles that
control the appearance of the `webview` container:

```html
<webview id="foo" src="https://www.github.com/" style="display:inline-block; width:640px; height:480px"></webview>
```

If you want to control the guest content in any way, you can write JavaScript
that listens for `webview` events and responds to those events using the
`webview` methods. Here's sample code with two event listeners: one that listens
for the web page to start loading, the other for the web page to stop loading,
and displays a "loading..." message during the load time:

```html
<script>
  onload = function() {
    var webview = document.getElementById("foo");
    var indicator = document.querySelector(".indicator");

    var loadstart = function() {
      indicator.innerText = "loading...";
    }
    var loadstop = function() {
      indicator.innerText = "";
    }
    webview.addEventListener("did-start-loading", loadstart);
    webview.addEventListener("did-stop-loading", loadstop);
  }
</script>
```

## 태그 속성

### src

```html
<webview src="https://www.github.com/"></webview>
```

Returns the visible URL. Writing to this attribute initiates top-level
navigation.

Assigning `src` its own value will reload the current page.

The `src` attribute can also accept data URLs, such as
`data:text/plain,Hello, world!`.

### autosize

```html
<webview src="https://www.github.com/" autosize="on" minwidth="576" minheight="432"></webview>
```

If "on", the `webview` container will automatically resize within the
bounds specified by the attributes `minwidth`, `minheight`, `maxwidth`, and
`maxheight`. These contraints do not impact the `webview` UNLESS `autosize` is
enabled. When `autosize` is enabled, the `webview` container size cannot be less
than the minimum values or greater than the maximum.

### nodeintegration

```html
<webview src="http://www.google.com/" nodeintegration></webview>
```

If "on", the guest page in `webview` will have node integration and can use node
APIs like `require` and `process` to access low level system resources.

### plugins

```html
<webview src="https://www.github.com/" plugins></webview>
```

If "on", the guest page in `webview` will be able to use browser plugins.

### preload

```html
<webview src="https://www.github.com/" preload="./test.js"></webview>
```

Specifies a script that will be loaded before other scripts run in the guest
page. The protocol of script's URL must be either `file:` or `asar:`, because it
will be loaded by `require` in guest page under the hood.

When the guest page doesn't have node integration this script will still have
access to all Node APIs, but global objects injected by Node will be deleted
after this script has done execution.

### httpreferrer

```html
<webview src="https://www.github.com/" httpreferrer="http://cheng.guru"></webview>
```

Sets the referrer URL for the guest page.

### useragent

```html
<webview src="https://www.github.com/" useragent="Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko"></webview>
```

Sets the user agent for the guest page before the page is navigated to. Once the page is loaded, use the `setUserAgent` method to change the user agent.

### disablewebsecurity

```html
<webview src="https://www.github.com/" disablewebsecurity></webview>
```

If "on", the guest page will have web security disabled.

## API

The webview element must be loaded before using the methods.  
**Example**
```javascript
webview.addEventListener("dom-ready", function(){
  webview.openDevTools();
});
```

### `<webview>`.getUrl()

Returns URL of guest page.

### `<webview>`.getTitle()

Returns the title of guest page.

### `<webview>`.isLoading()

Returns whether guest page is still loading resources.

### `<webview>`.isWaitingForResponse()

Returns whether guest page is waiting for a first-response for the main resource
of the page.

### `<webview>`.stop()

Stops any pending navigation.

### `<webview>`.reload()

Reloads guest page.

### `<webview>`.reloadIgnoringCache()

Reloads guest page and ignores cache.

### `<webview>`.canGoBack()

Returns whether guest page can go back.

### `<webview>`.canGoForward()

Returns whether guest page can go forward.

### `<webview>`.canGoToOffset(offset)

* `offset` Integer

Returns whether guest page can go to `offset`.

### `<webview>`.clearHistory()

Clears the navigation history.

### `<webview>`.goBack()

Makes guest page go back.

### `<webview>`.goForward()

Makes guest page go forward.

### `<webview>`.goToIndex(index)

* `index` Integer

Navigates to the specified absolute index.

### `<webview>`.goToOffset(offset)

* `offset` Integer

Navigates to the specified offset from the "current entry".

### `<webview>`.isCrashed()

Whether the renderer process has crashed.

### `<webview>`.setUserAgent(userAgent)

* `userAgent` String

Overrides the user agent for guest page.

### `<webview>`.insertCSS(css)

* `css` String

Injects CSS into guest page.

### `<webview>`.executeJavaScript(code)

* `code` String

Evaluates `code` in guest page.

### `<webview>`.openDevTools()

Opens a devtools window for guest page.

### `<webview>`.closeDevTools()

Closes the devtools window of guest page.

### `<webview>`.isDevToolsOpened()

Returns whether guest page has a devtools window attached.

### `<webview>`.inspectElement(x, y)

* `x` Integer
* `y` Integer

Starts inspecting element at position (`x`, `y`) of guest page.

### `<webview>`.inspectServiceWorker()

Opens the devtools for the service worker context present in the guest page.

### `<webview>`.undo()

Executes editing command `undo` in page.

### `<webview>`.redo()

Executes editing command `redo` in page.

### `<webview>`.cut()

Executes editing command `cut` in page.

### `<webview>`.copy()

Executes editing command `copy` in page.

### `<webview>`.paste()

Executes editing command `paste` in page.

### `<webview>`.pasteAndMatchStyle()

Executes editing command `pasteAndMatchStyle` in page.

### `<webview>`.delete()

Executes editing command `delete` in page.

### `<webview>`.selectAll()

Executes editing command `selectAll` in page.

### `<webview>`.unselect()

Executes editing command `unselect` in page.

### `<webview>`.replace(text)

* `text` String

Executes editing command `replace` in page.

### `<webview>`.replaceMisspelling(text)

* `text` String

Executes editing command `replaceMisspelling` in page.

### `<webview>`.send(channel[, args...])

* `channel` String

Send `args..` to guest page via `channel` in asynchronous message, the guest
page can handle it by listening to the `channel` event of `ipc` module.

See [WebContents.send](browser-window-ko.md#webcontentssendchannel-args) for
examples.

## DOM 이벤트

### did-finish-load

Fired when the navigation is done, i.e. the spinner of the tab will stop
spinning, and the `onload` event was dispatched.

### did-fail-load

* `errorCode` Integer
* `errorDescription` String

This event is like `did-finish-load`, but fired when the load failed or was
cancelled, e.g. `window.stop()` is invoked.

### did-frame-finish-load

* `isMainFrame` Boolean

Fired when a frame has done navigation.

### did-start-loading

Corresponds to the points in time when the spinner of the tab starts spinning.

### did-stop-loading

Corresponds to the points in time when the spinner of the tab stops spinning.

### did-get-response-details

* `status` Boolean
* `newUrl` String
* `originalUrl` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

Fired when details regarding a requested resource is available.
`status` indicates socket connection to download the resource.

### did-get-redirect-request

* `oldUrl` String
* `newUrl` String
* `isMainFrame` Boolean

Fired when a redirect was received while requesting a resource.

### dom-ready

Fired when document in the given frame is loaded.

### page-title-set

* `title` String
* `explicitSet` Boolean

Fired when page title is set during navigation. `explicitSet` is false when title is synthesised from file
url.

### page-favicon-updated

* `favicons` Array - Array of Urls

Fired when page receives favicon urls.

### enter-html-full-screen

Fired when page enters fullscreen triggered by html api.

### leave-html-full-screen

Fired when page leaves fullscreen triggered by html api.

### console-message

* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

Fired when the guest window logs a console message.

The following example code forwards all log messages to the embedder's console
without regard for log level or other properties.

```javascript
webview.addEventListener('console-message', function(e) {
  console.log('Guest page logged a message:', e.message);
});
```

### new-window

* `url` String
* `frameName` String
* `disposition` String - Can be `default`, `foreground-tab`, `background-tab`,
  `new-window` and `other`

Fired when the guest page attempts to open a new browser window.

The following example code opens the new url in system's default browser.

```javascript
webview.addEventListener('new-window', function(e) {
  require('shell').openExternal(e.url);
});
```

### close

Fired when the guest page attempts to close itself.

The following example code navigates the `webview` to `about:blank` when the
guest attempts to close itself.

```javascript
webview.addEventListener('close', function() {
  webview.src = 'about:blank';
});
```

### ipc-message

* `channel` String
* `args` Array

Fired when the guest page has sent an asynchronous message to embedder page.

With `sendToHost` method and `ipc-message` event you can easily communicate
between guest page and embedder page:

```javascript
// In embedder page.
webview.addEventListener('ipc-message', function(event) {
  console.log(event.channel);
  // Prints "pong"
});
webview.send('ping');
```

```javascript
// In guest page.
var ipc = require('ipc');
ipc.on('ping', function() {
  ipc.sendToHost('pong');
});
```

### crashed

Fired when the renderer process is crashed.

### gpu-crashed

Fired when the gpu process is crashed.

### plugin-crashed

* `name` String
* `version` String

Fired when a plugin process is crashed.

### destroyed

Fired when the WebContents is destroyed.
