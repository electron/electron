# webContents

> 渲染和控制网页。

可使用的进程: [主进程](../tutorial/quick-start.md#main-process)

`webContents` 是一个
[事件发出者](http://nodejs.org/api/events.html#events_class_events_eventemitter).
它负责渲染并控制网页，也是 [`BrowserWindow`](browser-window.md) 对象的属性.一个使用 `webContents` 的例子:

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({width: 800, height: 1500})
win.loadURL('http://github.com')

let contents = win.webContents
console.log(contents)
```

## 方法

这些方法可以从 `webContents` 模块访问：

```javascript
const {webContents} = require('electron')
console.log(webContents)
```

### `webContents.getAllWebContents()`

返回 `WebContents[]` - 所有 `WebContents` 实例的数组。这将包含Web内容
适用于所有 windows，webviews，打开的 devtools 和 devtools 扩展背景页面。

### `webContents.getFocusedWebContents()`

返回 `WebContents` - 在此应用程序中焦点的 Web 内容，否则返回`null`。

### `webContents.fromId(id)`

* `id` Integer

返回 `WebContents` - 一个给定 ID 的 WebContents 实例。

## Class: WebContents

> 渲染和控制浏览器窗口实例的内容。

可使用的进程: [主进程](../tutorial/quick-start.md#main-process)

### 实例事件


#### Event: 'did-finish-load'

当导航完成时，发出 `onload` 事件。

#### Event: 'did-fail-load'

返回：

* `event` Event
* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

这个事件类似 `did-finish-load` ，但是是在加载失败或取消加载时发出, 例如， `window.stop()` 被调用时。错误代码的完整列表和它们的含义都可以在 [这里](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h) 找到。

#### Event: 'did-frame-finish-load'

返回：

* `event` Event
* `isMainFrame` Boolean

当一个 frame 导航完成的时候发出事件。

#### Event: 'did-start-loading'

当 tab 的spinner 开始 spinning的时候。

#### Event: 'did-stop-loading'

当 tab 的spinner 结束 spinning的时候。

#### Event: 'did-get-response-details'

返回：

* `event` Event
* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

当有关请求资源的详细信息可用的时候发出事件。
`status` 标识了 socket 链接来下载资源。

#### Event: 'did-get-redirect-request'

返回：

* `event` Event
* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

当在请求资源时收到重定向的时候发出事件。

#### Event: 'dom-ready'

返回：

* `event` Event

当指定 frame 中的 文档加载完成的时候发出事件。

#### Event: 'page-favicon-updated'

返回：

* `event` Event
* `favicons` Array - Array of URLs

当 page 收到图标 url 的时候发出事件。

#### Event: 'new-window'

返回：

* `event` Event
* `url` String
* `frameName` String
* `disposition` String - 可为 `default`, `foreground-tab`, `background-tab`,
  `new-window`, `save-to-disk` 和 `other`
* `options` Object - 创建新的 `BrowserWindow`时使用的参数。
* `additionalFeatures` String[] - 非标准功能（功能未处理
   由 Chromium 或 Electron ）赋予 `window.open()`。

当 page 请求打开指定 url 窗口的时候发出事件.这可以是通过 `window.open` 或一个外部连接如 `<a target='_blank'>` 发出的请求。

默认指定 `url` 的 `BrowserWindow` 会被创建。

调用 `event.preventDefault()` 可以用来阻止打开窗口。

调用 `event.preventDefault()` 将阻止 Electron 自动创建
新 `BrowserWindow`。 如果调用 `event.preventDefault()` 并手动创建一个新的
`BrowserWindow`，那么你必须设置 `event.newGuest` 来引用新的 `BrowserWindow`
实例，如果不这样做可能会导致意外的行为。例如：

```javascript
myBrowserWindow.webContents.on('new-window', (event, url) => {
  event.preventDefault()
  const win = new BrowserWindow({show: false})
  win.once('ready-to-show', () => win.show())
  win.loadURL(url)
  event.newGuest = win
})
```

#### Event: 'will-navigate'

返回：

* `event` Event
* `url` String

当用户或 page 想要开始导航的时候发出事件。它可在当 `window.location` 对象改变或用户点击 page 中的链接的时候发生。

当使用 api（如 `webContents.loadURL` 和 `webContents.back`） 以编程方式来启动导航的时候，这个事件将不会发出。

它也不会在页内跳转发生，例如点击锚链接或更新 `window.location.hash`。使用 `did-navigate-in-page` 事件可以达到目的。

调用 `event.preventDefault()` 可以阻止导航。

#### Event: 'did-navigate'

返回：

* `event` Event
* `url` String

当一个导航结束时候发出事件。

页内跳转时不会发出这个事件，例如点击锚链接或更新 `window.location.hash`。使用 `did-navigate-in-page` 事件可以达到目的。

#### Event: 'did-navigate-in-page'

返回：

* `event` Event
* `url` String
* `isMainFrame ` Boolean

当页内导航发生的时候发出事件。

当页内导航发生的时候，page 的url 改变，但是不会跳出界面。例如当点击锚链接时或者 DOM 的 `hashchange` 事件发生。

#### Event: 'crashed'

返回：

* `event` Event
* `killed` Boolean

当渲染进程崩溃的时候发出事件。

#### Event: 'plugin-crashed'

返回：

* `event` Event
* `name` String
* `version` String

当插件进程崩溃时候发出事件。

#### Event: 'destroyed'

当 `webContents` 被删除的时候发出事件。

#### Event: 'before-input-event'

返回：

* `event` Event
* `input` Object - 属性
  * `type` String - `keyUp` 或者 `keyDown`
  * `key` String - [KeyboardEvent.key][keyboardevent]
  * `code` String - [KeyboardEvent.code][keyboardevent]
  * `isAutoRepeat` Boolean - [KeyboardEvent.repeat][keyboardevent]
  * `shift` Boolean - [KeyboardEvent.shiftKey][keyboardevent]
  * `control` Boolean - [KeyboardEvent.controlKey][keyboardevent]
  * `alt` Boolean - [KeyboardEvent.altKey][keyboardevent]
  * `meta` Boolean - [KeyboardEvent.metaKey][keyboardevent]

在 `keydown` 和 `keyup` 事件触发前发送。调用 `event.preventDefault` 方法可以阻止页面的 `keydown/keyup` 事件。

#### Event: 'devtools-opened'

当开发者工具栏打开的时候发出事件。

#### Event: 'devtools-closed'

当开发者工具栏关闭时候发出事件。

#### Event: 'devtools-focused'

当开发者工具栏获得焦点或打开的时候发出事件。

#### Event: 'certificate-error'

返回：

* `event` Event
* `url` URL
* `error` String - The error code
* `certificate` [Certificate](structures/certificate.md)
* `callback` Function
	* `isTrusted ` Boolean - 表明证书是否可以被认为是可信的

当验证 `certificate` 或 `url` 失败的时候发出事件。

使用方法类似 [`app` 的 `certificate-error` 事件](app.md#event-certificate-error)。


#### Event: 'select-client-certificate'

返回：

* `event` Event
* `url` URL
* `certificateList` [Certificate[]](structures/certificate.md)
* `callback` Function
  * `certificate` [Certificate](structures/certificate.md) - 证书必须来自于指定的列表

当请求客户端证书的时候发出事件。

使用方法类似 [`app` 的 `select-client-certificate` 事件](app.md#event-select-client-certificate)。

#### Event: 'login'

返回：

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

当 `webContents` 想做基本验证的时候发出事件.

使用方法类似 [the `login` event of `app`](app.md#event-login)。

#### Event: 'found-in-page'

返回：

* `event` Event
* `result` Object
  * `requestId` Integer
  * `activeMatchOrdinal` Integer - 活动匹配位置。
  * `matches` Integer - 匹配数量。
  * `selectionArea` Object - 协调首个匹配位置。
  * `finalUpdate` Boolean


当使用 [`webContents.findInPage`] 进行页内查找并且找到可用值得时候发出事件。

#### Event: 'media-started-playing'

当媒体开始播放的时候发出事件。

#### Event: 'media-paused'

当媒体停止播放的时候发出事件。

#### Event: 'did-change-theme-color'

当page 的主题色时候发出事件。这通常由于引入了一个 meta 标签：

```html
<meta name='theme-color' content='#ff0000'>
```

#### Event: 'update-target-url'

返回：

* `event` Event
* `url` String

当鼠标移到一个链接上或键盘焦点移动到一个链接上时发送。

#### Event: 'cursor-changed'

返回：

* `event` Event
* `type` String
* `image` NativeImage (可选)
* `scale` Float (可选) 自定义光标的比例
* `size` Object (可选) - `image`的大小
  * `width` Integer
  * `height` Integer
* `hotspot` Object (可选) - 自定义光标热点的坐标
  * `x` Integer - x 坐标
  * `y` Integer - y 坐标

当鼠标的类型发生改变的时候发出事件. `type` 的参数可以是  `default`,
`crosshair`, `pointer`, `text`, `wait`, `help`, `e-resize`, `n-resize`,
`ne-resize`, `nw-resize`, `s-resize`, `se-resize`, `sw-resize`, `w-resize`,
`ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `col-resize`,
`row-resize`, `m-panning`, `e-panning`, `n-panning`, `ne-panning`, `nw-panning`,
`s-panning`, `se-panning`, `sw-panning`, `w-panning`, `move`, `vertical-text`,
`cell`, `context-menu`, `alias`, `progress`, `nodrop`, `copy`, `none`,
`not-allowed`, `zoom-in`, `zoom-out`, `grab`, `grabbing`, `custom`.

如果 `type` 参数值为 `custom`、 `image` 参数会在一个`NativeImage` 中控制自定义鼠标图片，并且 `scale` 、`size` 和 `hotspot`会控制自定义光标的属性。

#### Event: 'context-menu'

Returns:

* `event` Event
* `params` Object
  * `x` Integer - x 坐标
  * `y` Integer - y 坐标
  * `linkURL` String - 菜单中调用的节点的 URL 链接.
  * `linkText` String - 链接的文本。当链接是一个图像时文本可以为空.
  * `pageURL` String - 菜单被调用时顶级页面的 URL 链接.
  * `frameURL` String - 菜单被调用时子级页面的 URL 链接.
  * `srcURL` String - 菜单被调用时元素的原始链接。带有链接的元素可以为图像、音频和视频。
  * `mediaType` String - 菜单被调用时的节点类型。可以为 `none`, `image`, `audio`, `video`, `canvas`, `file` or `plugin`.
  * `hasImageContents` Boolean - 菜单中是否含有图像.
  * `isEditable` Boolean - 菜单是否可以被编辑.
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

#### Event: 'paint'

Returns:

* `event` Event
* `dirtyRect` [Rectangle](structures/rectangle.md)
* `image` [NativeImage](native-image.md) - The image data of the whole frame.

Emitted when a new frame is generated. Only the dirty area is passed in the
buffer.

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({webPreferences: {offscreen: true}})
win.webContents.on('paint', (event, dirty, image) => {
  // updateBitmap(dirty, image.getBitmap())
})
win.loadURL('http://github.com')
```

#### Event: 'devtools-reload-page'
当 devtools 调试工具页面重新加载时发送

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
### 实例方法


#### `contents.loadURL(url[, options])`

* `url` URL
* `options` Object (可选)
  * `httpReferrer` String - HTTP 的 url 链接.
  * `userAgent` String - 产生请求的用户代理
  * `extraHeaders` String - 以 "\n" 分隔的额外头
  *  `postData` ([UploadRawData](structures/upload-raw-data.md) | [UploadFile](structures/upload-file.md) | [UploadFileSystem](structures/upload-file-system.md) | [UploadBlob](structures/upload-blob.md))[] - (optional)
  * `baseURLForDataURL` String (optional) - 由数据URL加载的文件的基本URL（带尾随路径分隔符）。只有当指定的URL是一个数据URL并且需要加载其他文件时才需要设置。

在窗口中加载 `url`。 `url` 必须包含协议前缀，
比如 `http://` 或 `file://`。如果加载想要忽略 http 缓存，可以使用 `pragma` 头来达到目的。

```javascript
const {webContents} = require('electron')
const options = {extraHeaders: 'pragma: no-cache\n'}
webContents.loadURL('https://github.com', options)
```

#### `contents.downloadURL(url)`

* `url` URL

初始化一个指定 `url` 的资源下载，不导航跳转。 `session` 的 `will-download` 事件会触发。

#### `contents.getURL()`

Returns `String` - 当前页面的 URL.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

let currentURL = win.webContents.getURL()
console.log(currentURL)
```

#### `contents.getTitle()`

Returns `String` - 当前页面的标题.

#### `contents.isDestroyed()`

Returns `Boolean` - 当前的页面是否被销毁了

#### `contents.isFocused()`

Returns `Boolean` - 焦点是否在当前页面上.

#### `contents.isLoading()`

Returns `Boolean` - 当前页是否正在加载资源。

#### `contents.isLoadingMainFrame()`

Returns `Boolean` - 是否主框架(而不仅是 iframe 或者在帧内)仍在加载中。

#### `contents.isWaitingForResponse()`

Returns `Boolean` - 当前页是否正在等待主要资源的第一次响应。

#### `contents.stop()`

停止还为开始的导航。

#### `contents.reload()`

重载当前页。

#### `contents.reloadIgnoringCache()`

重载当前页，忽略缓存。

#### `contents.canGoBack()`

Returns `Boolean` - 浏览器是否能回到前一个页面。

#### `contents.canGoForward()`

Returns `Boolean` - 浏览器是否能前往下一个页面。

#### `contents.canGoToOffset(offset)`

* `offset` Integer

Returns `Boolean` - 页面是否能前往 `offset`。

#### `contents.clearHistory()`

清除导航历史。

#### `contents.goBack()`

让浏览器回退到前一个页面。

#### `contents.goForward()`

让浏览器回前往下一个页面。

#### `contents.goToIndex(index)`

* `index` Integer

让浏览器回前往指定 `index` 的页面。

#### `contents.goToOffset(offset)`

* `offset` Integer

导航到相对于当前页的偏移位置页。

#### `contents.isCrashed()`

Returns `Boolean` - 渲染进程是否崩溃。

#### `contents.setUserAgent(userAgent)`

* `userAgent` String

重写本页用户代理。

#### `contents.getUserAgent()`

Returns `String` - 页面的用户代理信息。

#### `contents.insertCSS(css)`

* `css` String

为当前页插入css。

#### `contents.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (可选) - 默认值为 `false`
* `callback` Function (可选) - 脚本执行完成后调用的回调函数.
  * `result` Any

Returns `Promise` - 成功了返回 resolves，失败了返回 rejected

评估页面的 `code` 代码。

浏览器窗口中的一些 HTML API ，例如 `requestFullScreen`，只能被用户手势请求。设置 `userGesture` 为 `true` 可以取消这个限制。

如果执行的代码的结果是一个 promise,回调方法将为 promise 的 resolved 的值。我们建议您使用返回的 Promise 来处理导致结果的代码。

```js
contents.executeJavaScript('fetch("https://jsonplaceholder.typicode.com/users/1").then(resp => resp.json())', true)
  .then((result) => {
    console.log(result) // Will be the JSON object from the fetch call
  })
```
#### `contents.setAudioMuted(muted)`

* `muted` Boolean

设置当前页为静音。

#### `contents.isAudioMuted()`

Returns `Boolean` - 当前页是否为静音状态

#### `contents.setZoomFactor(factor)`

* `factor` Number - 缩放系数

改变缩放系数为指定的数值。缩放系数是缩放的百分比除以100，比如 300% = 3.0

#### `contents.getZoomFactor(callback)`

* `callback` Function
  * `zoomFactor` Number

发送一个请求来获取当前的缩放系数，`callback` 回调方法会在 `callback(zoomFactor)` 中调用.

#### `contents.setZoomLevel(level)`

* `level` Number - 缩放等级

改变缩放等级为指定的等级。原始大小为0，每升高或降低一次，代表 20% 的大小缩放。限制为原始大小的 300% 到 50%。

#### `contents.getZoomLevel(callback)`

* `callback` Function
  * `zoomLevel` Number

发送一个请求来获取当前的缩放等级，`callback` 回调方法会在 `callback(zoomLevel)` 中调用.
#### `contents.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

**废弃:** 使用 `setVisualZoomLevelLimits` 来代替. 这个方法将在 Electron 2.0 中移除.

#### `contents.setVisualZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

设置最大和最小的手势缩放等级。

#### `contents.setLayoutZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

设置最大和最小的 layout-based (i.e. non-visual) zoom level.

#### `contents.undo()`

在网页中执行编辑命令 `undo`。

#### `contents.redo()`

在网页中执行编辑命令 `redo`。

#### `contents.cut()`

在网页中执行编辑命令 `cut`。

#### `contents.copy()`

在网页中执行编辑命令 `copy`。

#### `contents.copyImageAt(x, y)`

* `x` Integer
* `y` Integer

拷贝剪贴板中指定位置的图像.

#### `contents.paste()`

在网页中执行编辑命令 `paste`。

#### `contents.pasteAndMatchStyle()`

在网页中执行编辑命令 `pasteAndMatchStyle`。

#### `contents.delete()`

在网页中执行编辑命令 `delete`。

#### `contents.selectAll()`

在网页中执行编辑命令 `selectAll`。

#### `contents.unselect()`

在网页中执行编辑命令 `unselect`。

#### `contents.replace(text)`

* `text` String

在网页中执行编辑命令 `replace`。

#### `contents.replaceMisspelling(text)`

* `text` String

在网页中执行编辑命令 `replaceMisspelling`。

#### `contents.insertText(text)`

* `text` String

插入 `text` 到获得了焦点的元素。

#### `contents.findInPage(text[, options])`

* `text` String - 查找内容，不能为空。
* `options` Object (可选)
  * `forward` Boolean - (可选) 是否向前或向后查找，默认为 `true`。
  * `findNext` Boolean - (可选) 当前操作是否是第一次查找或下一次查找，
    默认为 `false`。
  * `matchCase` Boolean - (可选) 查找是否区分大小写，
    默认为 `false`。
  * `wordStart` Boolean - (可选) 是否仅以首字母查找，
     默认为 `false`。
  * `medialCapitalAsWordStart` Boolean - (可选) 是否结合 `wordStart`，如果匹配是大写字母开头，后面接小写字母或无字母，那么就接受这个词中匹配。接受几个其它的合成词匹配，默认为 `false`。

发起请求，在网页中查找所有与 `text` 相匹配的项，并且返回一个 `Integer` 来表示这个请求用的请求 Id。这个请求结果可以通过订阅
 [`found-in-page`](web-contents.md#event-found-in-page) 事件来取得。

#### `contents.stopFindInPage(action)`

* `action` String - 指定一个行为来接替停止
  [`webContents.findInPage`] 请求。
  * `clearSelection` - 转变为一个普通的 selection。
  * `keepSelection` - 清除 selection。
  * `activateSelection` - 获取焦点并点击 selection node。

使用给定的 `action` 来为 `webContents` 停止任何 `findInPage` 请求。

```javascript
const {webContents} = require('electron')
webContents.on('found-in-page', (event, result) => {
  if (result.finalUpdate) webContents.stopFindInPage('clearSelection')
})

const requestId = webContents.findInPage('api')
console.log(requestId)
```

#### `contents.capturePage([rect, ]callback)`

* `rect` [Rectangle](structures/rectangle.md) (optional) - 页面被捕捉的区域
* `callback` Function
  * `image` [NativeImage](native-image.md)

捕捉页面 `rect` 区域的快照。在完成后 `callback` 方法会通过 `callback(image)` 调用 。`image` 是 [NativeImage](native-image.md) 的实例。省略 `rect` 参数会捕捉整个页面的可视区域。

#### `contents.hasServiceWorker(callback)`

* `callback` Function
  * `hasWorker` Boolean
  
检查是否有任何 ServiceWorker 注册了，并且返回一个布尔值，来作为 `callback`响应的标识。

#### `contents.unregisterServiceWorker(callback)`

* `callback` Function
  * `success` Boolean
  
如果存在任何 ServiceWorker，则全部注销，并且当JS承诺执行行或JS拒绝执行而失败的时候，返回一个布尔值，它标识了相应的 `callback`。

#### `contents.print([options])`

* `options` Object (可选)
  * `silent` Boolean - 不需要请求用户的打印设置. 默认为 `false`。
  * `printBackground` Boolean - 打印背景和网页图片. 默认为 `false`。

打印窗口的网页。当设置 `silent` 为 `true` 的时候，Electron 将使用系统默认的打印机和打印方式来打印。

在网页中调用 `window.print()` 和 调用 `webContents.print({silent: false, printBackground: false})`具有相同的作用。

使用 `page-break-before: always; ` CSS 的样式将强制打印到一个新的页面.

#### `contents.printToPDF(options, callback)`

* `options` Object
  * `marginsType` Integer - 指定使用的 margin type。默认 margin 使用 0，无 margin 使用 1，最小化 margin 使用 2。
  * `pageSize` String - 指定生成的PDF文件的page size. 可以是 `A3`、
    `A4`、 `A5`、 `Legal`、`Letter` 和 `Tabloid`。或者是一个包含 `height`
    和 `width` 的对象，单位是微米。
  * `printBackground` Boolean - 是否打印 css 背景。
  * `printSelectionOnly` Boolean - 是否只打印选中的部分。
  * `landscape` Boolean - landscape 为 `true`, portrait 为 `false`。
* `callback` Function
  * `error` Error
  * `data` Buffer

打印窗口的网页为 pdf，使用 Chromium 预览打印的自定义设置。

完成时使用 `callback(error, data)` 调用 `callback` 。 `data` 是一个 `Buffer` ，包含了生成的 pdf 数据、

默认，空的 `options` 被视为：

```javascript
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

使用 `page-break-before: always; ` CSS 的样式将强制打印到一个新的页面.

一个 `webContents.printToPDF` 的示例:

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

添加指定的路径给开发者工具栏的 workspace。必须在 DevTools 创建之后使用它：

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.webContents.on('devtools-opened', () => {
  win.webContents.addWorkSpace(__dirname)
})
```

#### `contents.removeWorkSpace(path)`

* `path` String

从开发者工具栏的 workspace 删除指定的路径。

#### `contents.openDevTools([options])`

* `options` Object (可选)
  * `mode` String - 打开开发者工具的状态属性。可以为 `right`, `bottom`, `undocked`, `detach`。默认值为上一次使用时的状态。在`undocked`模式可以把工具栏放回来，`detach`模式不可以。

打开开发者工具栏。

#### `contents.closeDevTools()`

关闭开发者工具栏。

#### `contents.isDevToolsOpened()`

Returns `Boolean` - 开发者工具栏是否打开。

#### `contents.isDevToolsFocused()`

Returns `Boolean` - 开发者工具栏视图是否获得焦点。

#### `contents.toggleDevTools()`

Toggles 开发者工具。

#### `contents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

在 (`x`, `y`) 开始检测元素。

#### `contents.inspectServiceWorker()`

为 service worker 上下文打开开发者工具栏。

#### `contents.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

通过 `channel` 发送异步消息给渲染进程，你也可发送任意的参数。参数应该在 JSON 内部序列化，并且此后没有函数或原形链被包括了。

渲染进程可以通过使用 `ipcRenderer` 监听 `channel` 来处理消息。

从主进程向渲染进程发送消息的示例：

```javascript
// 主进程.
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

`parameters` Object, 属性为:

* `screenPosition` String - 指定需要模拟的屏幕
    (默认 : `desktop`)
  * `desktop` - 桌面屏幕模式
  * `mobile` - 手机屏幕模式
* `screenSize` Object - 设置模拟屏幕的尺寸 (screenPosition == mobile)
  * `width` Integer - 设置模拟屏幕 width
  * `height` Integer - 设置模拟屏幕 height
* `viewPosition` Object - 屏幕中可视区域的位置
    (screenPosition == mobile) (默认: `{x: 0, y: 0}`)
  * `x` Integer - 设置偏移左上角的x轴
  * `y` Integer - 设置偏移左上角的y轴
* `deviceScaleFactor` Integer - 设置设备缩放比例系数 (如果为0，默认为原始屏幕比例) (默认: `0`)
* `viewSize` Object - 设置模拟视图的尺寸 (空表示不覆盖)
  * `width` Integer - 设置模拟视图 width
  * `height` Integer - 设置模拟视图 height
* `scale` Float - 可用空间内的模拟视图偏移 (不在适应视图模式) (默认: `1`)

使用给定的参数来开启设备模拟。

#### `contents.disableDeviceEmulation()`

关闭模拟器，使用 `webContents.enableDeviceEmulation` 来启用。

#### `contents.sendInputEvent(event)`

* `event` Object
  * `type` String (**必需**) - 事件类型，可以是 `mouseDown`,
    `mouseUp`, `mouseEnter`, `mouseLeave`, `contextMenu`, `mouseWheel`,
    `mouseMove`, `keyDown`, `keyUp`, `char`.
  * `modifiers` String[] - 事件的 modifiers 数组, 可以包含 `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

向页面发送一个输入 `event`。

对键盘事件来说，`event` 对象还有如下属性：

* `keyCode` String (**必需**) - 将字符串作为键盘事件发送。可用的 key codes [Accelerator](accelerator.md)。

对鼠标事件来说，`event` 对象还有如下属性：

* `x` Integer (**必需**)
* `y` Integer (**必需**)
* `button` String - button 按下, 可以是 `left`, `middle`, `right`
* `globalX` Integer
* `globalY` Integer
* `movementX` Integer
* `movementY` Integer
* `clickCount` Integer

对鼠标滚轮事件来说，`event` 对象还有如下属性：

* `deltaX` Integer
* `deltaY` Integer
* `wheelTicksX` Integer
* `wheelTicksY` Integer
* `accelerationRatioX` Integer
* `accelerationRatioY` Integer
* `hasPreciseScrollingDeltas` Boolean
* `canScroll` Boolean

#### `contents.beginFrameSubscription(callback)`
* `onlyDirty` Boolean (可选) - 默认值为 `false`
* `callback` Function
  * `frameBuffer` Buffer
  * `dirtyRect` [Rectangle](structures/rectangle.md)
  
开始订阅 提交 事件和捕获数据帧，当有提交事件时，使用 `callback(frameBuffer)` 调用 `callback`。

`frameBuffer` 是一个包含原始像素数据的 `Buffer`，像素数据是按照 32bit BGRA 格式有效存储的，但是实际情况是取决于处理器的字节顺序的（大多数的处理器是存放小端序的，如果是在大端序的处理器上，数据是 32bit ARGB 格式）。

`dirtyRect` 脏区域是一个包含 `x, y, width, height` 属性的对象，它们描述了一个页面中的重绘区域。如果 `onlyDirty` 被设置为`true`, `frameBuffer` 将只包含重绘区域。`onlyDirty` 的默认值为 `false`.

#### `contents.endFrameSubscription()`

停止订阅帧提交事件。

#### `contents.startDrag(item)`

* `item` Object
  * `file` String 或 `files` Array - 要拖拽的文件(可以为多个)的路径。
  * `icon` [NativeImage](native-image.md) - 在 macOS 中图标不能为空.

设置 `item` 作为当前拖拽操作的对象。`file` 是作为拖拽文件的绝对路径。`icon` 是拖拽时光标下面显示的图像。

#### `contents.savePage(fullPath, saveType, callback)`

* `fullPath` String - 文件的完整路径.
* `saveType` String - 指定保存类型.
  * `HTMLOnly` - 只保存html.
  * `HTMLComplete` - 保存整个 page 内容.
  * `MHTML` - 保存完整的 html 为 MHTML.
* `callback` Function - `(error) => {}`.
  * `error` Error

Returns `Boolean` - 如果保存界面过程初始化成功，返回 true。

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

在页面上显示搜索的弹窗。

#### `contents.setSize(options)`

设置页面的大小。This is only supported for `<webview>` guest contents.

* `options` Object
  * `normal` Object (可选) - Normal size of the page. This can be used in
    combination with the [`disableguestresize`](web-view-tag.md#disableguestresize)
    attribute to manually resize the webview guest contents.
    * `width` Integer
    * `height` Integer

#### `contents.isOffscreen()`

Returns `Boolean` - 表明是否设置了 *offscreen rendering*.

#### `contents.startPainting()`

如果设置了 *offscreen rendering* 并且没有绘制，开始绘制.

#### `contents.stopPainting()`

如果设置了 *offscreen rendering* 并且绘制了，停止绘制.

#### `contents.isPainting()`

Returns `Boolean` -  如果设置了 *offscreen rendering* ，返回当前是否正在绘制.

#### `contents.setFrameRate(fps)`

* `fps` Integer

如果设置了 *offscreen rendering*，设置帧频为制定数值。有效范围为1-60.

#### `contents.getFrameRate()`

Returns `Integer` - 如果设置了 *offscreen rendering*，返回当前的帧频

#### `contents.invalidate()`

全部重新绘制整个页面的内容。

如果设置了*offscreen rendering* ，让页面失效并且生成一个新的`'paint'`事件

#### `contents.getWebRTCIPHandlingPolicy()`

Returns `String` - Returns the WebRTC IP Handling Policy.

#### `contents.setWebRTCIPHandlingPolicy(policy)`

* `policy` String - Specify the WebRTC IP Handling Policy.
  * `default` - Exposes user's public and local IPs.  This is the default
  behavior.  When this policy is used, WebRTC has the right to enumerate all
  interfaces and bind them to discover public interfaces.
  * `default_public_interface_only` - Exposes user's public IP, but does not
  expose user's local IP.  When this policy is used, WebRTC should only use the
  default route used by http. This doesn't expose any local addresses.
  * `default_public_and_private_interfaces` - Exposes user's public and local
  IPs.  When this policy is used, WebRTC should only use the default route used
  by http. This also exposes the associated default private address. Default
  route is the route chosen by the OS on a multi-homed endpoint.
  * `disable_non_proxied_udp` - Does not expose public or local IPs.  When this
  policy is used, WebRTC should only use TCP to contact peers or servers unless
  the proxy server supports UDP.

Setting the WebRTC IP handling policy allows you to control which IPs are
exposed via WebRTC.  See [BrowserLeaks](https://browserleaks.com/webrtc) for
more details.

### 实例属性

`WebContents` 对象也有下列属性：

#### `contents.id`

表明 WebContents 唯一标示的整数

#### `contents.session`

返回这个 `webContents` 使用的  [session](session.md) 对象。

#### `contents.hostWebContents`

返回这个 `webContents` 的父 [`WebContents`](web-contents.md)。

#### `contents.devToolsWebContents`

获取这个 `WebContents` 的开发者工具栏的 `WebContents`。

**注意:** 用户不可保存这个对象，因为当开发者工具栏关闭的时候它的值为 `null`。

#### `contents.debugger`

webContents 的 [Debugger](debugger.md) 实例.

[keyboardevent]: https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent
