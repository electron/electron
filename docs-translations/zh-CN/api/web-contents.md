# webContents

`webContents` 是一个
[事件发出者](http://nodejs.org/api/events.html#events_class_events_eventemitter).

它负责渲染并控制网页，也是 [`BrowserWindow`](browser-window.md) 对象的属性.一个使用 `webContents` 的例子:

```javascript
const BrowserWindow = require('electron').BrowserWindow

var win = new BrowserWindow({width: 800, height: 1500})
win.loadURL('http://github.com')

var webContents = win.webContents
```

## 事件

`webContents` 对象可发出下列事件:

### Event: 'did-finish-load'

当导航完成时发出事件，`onload` 事件也完成.

### Event: 'did-fail-load'

返回:

* `event` Event
* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

这个事件类似 `did-finish-load` ，但是是在加载失败或取消加载时发出, 例如， `window.stop()` 请求结束.错误代码的完整列表和它们的含义都可以在 [here](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h) 找到.

### Event: 'did-frame-finish-load'

返回:

* `event` Event
* `isMainFrame` Boolean

当一个 frame 导航完成的时候发出事件.

### Event: 'did-start-loading'

当 tab 的spinner 开始 spinning的时候.

### Event: 'did-stop-loading'

当 tab 的spinner 结束 spinning的时候.

### Event: 'did-get-response-details'

返回:

* `event` Event
* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

当有关请求资源的详细信息可用的时候发出事件.
`status` 标识了 socket链接来下载资源.

### Event: 'did-get-redirect-request'

返回:

* `event` Event
* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

当在请求资源时收到重定向的时候发出事件.

### Event: 'dom-ready'

返回:

* `event` Event

当指定 frame 中的 文档加载完成的时候发出事件.

### Event: 'page-favicon-updated'

返回:

* `event` Event
* `favicons` Array - Array of URLs

当 page 收到图标 url 的时候发出事件.

### Event: 'new-window'

返回:

* `event` Event
* `url` String
* `frameName` String
* `disposition` String - 可为 `default`, `foreground-tab`, `background-tab`,
  `new-window` 和 `other`.
* `options` Object - 创建新的 `BrowserWindow`时使用的参数.

当 page 请求打开指定 url 窗口的时候发出事件.这可以是通过 `window.open` 或一个外部连接如 `<a target='_blank'>` 发出的请求.

默认指定 `url` 的 `BrowserWindow` 会被创建.

调用 `event.preventDefault()` 可以用来阻止打开窗口.

### Event: 'will-navigate'

返回:

* `event` Event
* `url` String

当用户或 page 想要开始导航的时候发出事件.它可在当 `window.location` 对象改变或用户点击 page 中的链接的时候发生.

当使用 api(如 `webContents.loadURL` 和 `webContents.back`) 以编程方式来启动导航的时候，这个事件将不会发出.

它也不会在页内跳转发生， 例如点击锚链接或更新 `window.location.hash`.使用 `did-navigate-in-page` 事件可以达到目的.

调用 `event.preventDefault()` 可以阻止导航.

### Event: 'did-navigate'

返回:

* `event` Event
* `url` String

当一个导航结束时候发出事件.

页内跳转时不会发出这个事件，例如点击锚链接或更新 `window.location.hash`.使用 `did-navigate-in-page` 事件可以达到目的.

### Event: 'did-navigate-in-page'

返回:

* `event` Event
* `url` String

当页内导航发生的时候发出事件.

当页内导航发生的时候，page 的url 改变，但是不会跳出界面.例如当点击锚链接时或者 DOM 的 `hashchange` 事件发生.

### Event: 'crashed'

当渲染进程崩溃的时候发出事件.

### Event: 'plugin-crashed'

返回:

* `event` Event
* `name` String
* `version` String

当插件进程崩溃时候发出事件.

### Event: 'destroyed'

当 `webContents` 被删除的时候发出事件.

### Event: 'devtools-opened'

当开发者工具栏打开的时候发出事件.

### Event: 'devtools-closed'

当开发者工具栏关闭时候发出事件.

### Event: 'devtools-focused'

当开发者工具栏获得焦点或打开的时候发出事件.

### Event: 'certificate-error'

返回:

* `event` Event
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` Buffer - PEM encoded data
  * `issuerName` String
* `callback` Function

当验证证书或 `url` 失败的时候发出事件.

使用方法类似 [`app` 的 `certificate-error` 事件](app.md#event-certificate-error).

### Event: 'select-client-certificate'

返回:

* `event` Event
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
* `callback` Function

当请求客户端证书的时候发出事件.

使用方法类似 [`app` 的 `select-client-certificate` 事件](app.md#event-select-client-certificate).

### Event: 'login'

返回:

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

当 `webContents` 想做基本验证的时候发出事件.

使用方法类似 [the `login` event of `app`](app.md#event-login).

### Event: 'found-in-page'

返回:

* `event` Event
* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - 标识是否还有更多的值可以查看.
  * `activeMatchOrdinal` Integer (可选) - 活动匹配位置
  * `matches` Integer (可选) - 匹配数量.
  * `selectionArea` Object (可选) - 协调首个匹配位置.

当使用 [`webContents.findInPage`] 进行页内查找并且找到可用值得时候发出事件.

### Event: 'media-started-playing'

当媒体开始播放的时候发出事件.

### Event: 'media-paused'

当媒体停止播放的时候发出事件.

### Event: 'did-change-theme-color'

当page 的主题色时候发出事件.这通常由于引入了一个 meta 标签 :

```html
<meta name='theme-color' content='#ff0000'>
```

### Event: 'cursor-changed'

返回:

* `event` Event
* `type` String
* `image` NativeImage (可选)
* `scale` Float (可选)

当鼠标的类型发生改变的时候发出事件. `type` 的参数可以是  `default`,
`crosshair`, `pointer`, `text`, `wait`, `help`, `e-resize`, `n-resize`,
`ne-resize`, `nw-resize`, `s-resize`, `se-resize`, `sw-resize`, `w-resize`,
`ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `col-resize`,
`row-resize`, `m-panning`, `e-panning`, `n-panning`, `ne-panning`, `nw-panning`,
`s-panning`, `se-panning`, `sw-panning`, `w-panning`, `move`, `vertical-text`,
`cell`, `context-menu`, `alias`, `progress`, `nodrop`, `copy`, `none`,
`not-allowed`, `zoom-in`, `zoom-out`, `grab`, `grabbing`, `custom`.

如果 `type` 参数值为 `custom`, `image` 参数会在一个`NativeImage` 中控制自定义鼠标图片, 并且 `scale` 会控制图片的缩放比例.

## 实例方法

`webContents` 对象有如下的实例方法:

### `webContents.loadURL(url[, options])`

* `url` URL
* `options` Object (可选)
  * `httpReferrer` String - A HTTP Referrer url.
  * `userAgent` String - 产生请求的用户代理
  * `extraHeaders` String - 以 "\n" 分隔的额外头

在窗口中加载 `url` , `url` 必须包含协议前缀,
比如 `http://` 或 `file://`. 如果加载想要忽略 http 缓存，可以使用 `pragma` 头来达到目的.

```javascript
const options = {'extraHeaders': 'pragma: no-cache\n'}
webContents.loadURL(url, options)
```

### `webContents.downloadURL(url)`

* `url` URL

初始化一个指定 `url` 的资源下载，不导航跳转. `session` 的 `will-download` 事件会触发.

### `webContents.getURL()`

返回当前page 的 url.

```javascript
var win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

var currentURL = win.webContents.getURL()
```

### `webContents.getTitle()`

返回当前page 的 标题.

### `webContents.isLoading()`

返回一个布尔值，标识当前页是否正在加载.

### `webContents.isWaitingForResponse()`

返回一个布尔值，标识当前页是否正在等待主要资源的第一次响应.

### `webContents.stop()`

停止还为开始的导航.

### `webContents.reload()`

重载当前页.

### `webContents.reloadIgnoringCache()`

重载当前页，忽略缓存.

### `webContents.canGoBack()`

返回一个布尔值，标识浏览器是否能回到前一个page.

### `webContents.canGoForward()`

返回一个布尔值，标识浏览器是否能前往下一个page.

### `webContents.canGoToOffset(offset)`

* `offset` Integer

返回一个布尔值，标识浏览器是否能前往指定 `offset` 的page.

### `webContents.clearHistory()`

清除导航历史.

### `webContents.goBack()`

让浏览器回退到前一个page.

### `webContents.goForward()`

让浏览器回前往下一个page.

### `webContents.goToIndex(index)`

* `index` Integer

让浏览器回前往指定 `index` 的page.

### `webContents.goToOffset(offset)`

* `offset` Integer

导航到相对于当前页的偏移位置页.

### `webContents.isCrashed()`

渲染进程是否崩溃.

### `webContents.setUserAgent(userAgent)`

* `userAgent` String

重写本页用户代理.

### `webContents.getUserAgent()`

返回一个 `String` ，标识本页用户代理信息.

### `webContents.insertCSS(css)`

* `css` String

为当前页插入css.

### `webContents.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (可选)
* `callback` Function (可选) - 脚本执行完成后调用的回调函数.
  * `result`

评估 page  `代码`.

浏览器窗口中的一些 HTML API ，例如 `requestFullScreen`，只能被用户手势请求.设置 `userGesture` 为 `true` 可以取消这个限制.

### `webContents.setAudioMuted(muted)`

* `muted` Boolean

减缓当前页的 audio 的播放速度.

### `webContents.isAudioMuted()`

返回一个布尔值，标识当前页是否减缓了 audio 的播放速度.

### `webContents.undo()`

执行网页的编辑命令 `undo` . 

### `webContents.redo()`

执行网页的编辑命令 `redo` . 

### `webContents.cut()`

执行网页的编辑命令 `cut` . 

### `webContents.copy()`

执行网页的编辑命令 `copy` . 

### `webContents.paste()`

执行网页的编辑命令 `paste` . 

### `webContents.pasteAndMatchStyle()`

执行网页的编辑命令 `pasteAndMatchStyle` . 

### `webContents.delete()`

执行网页的编辑命令 `delete` . 

### `webContents.selectAll()`

执行网页的编辑命令 `selectAll` . 

### `webContents.unselect()`

执行网页的编辑命令 `unselect` . 

### `webContents.replace(text)`

* `text` String

执行网页的编辑命令 `replace` . 

### `webContents.replaceMisspelling(text)`

* `text` String

执行网页的编辑命令 `replaceMisspelling` . 

### `webContents.insertText(text)`

* `text` String

插入 `text` 到获得了焦点的元素.

### `webContents.findInPage(text[, options])`

* `text` String - 查找内容, 不能为空.
* `options` Object (可选)
  * `forward` Boolean - 是否向前或向后查找, 默认为 `true`.
  * `findNext` Boolean - 当前操作是否是第一次查找或下一次查找,
    默认为 `false`.
  * `matchCase` Boolean - 查找是否区分大小写,
    默认为 `false`.
  * `wordStart` Boolean -是否仅以首字母查找.
     默认为 `false`.
  * `medialCapitalAsWordStart` Boolean - 是否结合 `wordStart`,如果匹配是大写字母开头，后面接小写字母或无字母，那么就接受这个词中匹配.接受几个其它的合成词匹配,  默认为 `false`.

发起请求，在网页中查找所有与 `text` 相匹配的项，并且返回一个 `Integer` 来表示这个请求用的请求Id.这个请求结果可以通过订阅
 [`found-in-page`](web-contents.md#event-found-in-page) 事件来取得.

### `webContents.stopFindInPage(action)`

* `action` String - 指定一个行为来接替停止
  [`webContents.findInPage`] 请求.
  * `clearSelection` - 转变为一个普通的 selection.
  * `keepSelection` - 清除 selection.
  * `activateSelection` - 获取焦点并点击 selection node.

使用给定的 `action` 来为 `webContents` 停止任何 `findInPage` 请求.

```javascript
webContents.on('found-in-page', function (event, result) {
  if (result.finalUpdate)
    webContents.stopFindInPage('clearSelection')
})

const requestId = webContents.findInPage('api')
```

### `webContents.hasServiceWorker(callback)`

* `callback` Function

检查是否有任何 ServiceWorker 注册了，并且返回一个布尔值，来作为 `callback`响应的标识.

### `webContents.unregisterServiceWorker(callback)`

* `callback` Function

如果存在任何 ServiceWorker ，则全部注销，并且当JS承诺执行行或JS拒绝执行而失败的时候，返回一个布尔值，它标识了相应的 `callback`.

### `webContents.print([options])`

* `options` Object (可选)
  * `silent` Boolean - 不需要请求用户的打印设置. 默认为 `false`.
  * `printBackground` Boolean - 打印背景和网页图片. 默认为 `false`.

打印窗口的网页. 当设置 `silent` 为 `false` 的时候，Electron 将使用系统默认的打印机和打印方式来打印.

在网页中调用 `window.print()` 和 调用 `webContents.print({silent: false, printBackground: false})`具有相同的作用.

**注意:** 在 Windows, 打印 API 依赖于 `pdf.dll`. 如果你的应用不使用任何的打印, 你可以安全删除 `pdf.dll` 来减少二进制文件的size.

### `webContents.printToPDF(options, callback)`

* `options` Object
  * `marginsType` Integer - 指定使用的 margin type. 默认 margin 使用 0, 无 margin 使用 1, 最小化 margin 使用 2.
  * `pageSize` String - 指定生成的PDF文件的page size. 可以是 `A3`,
    `A4`, `A5`, `Legal`, `Letter` 和 `Tabloid`.
  * `printBackground` Boolean - 是否打印 css 背景.
  * `printSelectionOnly` Boolean - 是否只打印选中的部分.
  * `landscape` Boolean - landscape 为 `true`, portrait 为 `false`.
* `callback` Function

打印窗口的网页为 pdf ，使用 Chromium 预览打印的自定义设置.

完成时使用 `callback(error, data)` 调用 `callback` . `data` 是一个 `Buffer` ，包含了生成的 pdf 数据.

默认，空的 `options` 被视为 :

```javascript
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

```javascript
const BrowserWindow = require('electron').BrowserWindow
const fs = require('fs')

var win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

win.webContents.on('did-finish-load', function () {
  // Use default printing options
  win.webContents.printToPDF({}, function (error, data) {
    if (error) throw error
    fs.writeFile('/tmp/print.pdf', data, function (error) {
      if (error)
        throw error
      console.log('Write PDF successfully.')
    })
  })
})
```

### `webContents.addWorkSpace(path)`

* `path` String

添加指定的路径给开发者工具栏的 workspace.必须在 DevTools 创建之后使用它 :

```javascript
mainWindow.webContents.on('devtools-opened', function () {
  mainWindow.webContents.addWorkSpace(__dirname)
})
```

### `webContents.removeWorkSpace(path)`

* `path` String

从开发者工具栏的 workspace 删除指定的路径.

### `webContents.openDevTools([options])`

* `options` Object (可选)
  * `detach` Boolean - 在一个新窗口打开开发者工具栏

打开开发者工具栏.

### `webContents.closeDevTools()`

关闭开发者工具栏.

### `webContents.isDevToolsOpened()`

返回布尔值，开发者工具栏是否打开.

### `webContents.isDevToolsFocused()`

返回布尔值，开发者工具栏视图是否获得焦点.

### `webContents.toggleDevTools()`

Toggles 开发者工具.

### `webContents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

在 (`x`, `y`) 开始检测元素.

### `webContents.inspectServiceWorker()`

为 service worker 上下文打开开发者工具栏.

### `webContents.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

通过 `channel` 发送异步消息给渲染进程，你也可发送任意的参数.参数应该在 JSON 内部序列化，并且此后没有函数或原形链被包括了.

渲染进程可以通过使用 `ipcRenderer` 监听 `channel` 来处理消息.

例子，从主进程向渲染进程发送消息 :

```javascript
// 主进程.
var window = null
app.on('ready', function () {
  window = new BrowserWindow({width: 800, height: 600})
  window.loadURL('file://' + __dirname + '/index.html')
  window.webContents.on('did-finish-load', function () {
    window.webContents.send('ping', 'whoooooooh!')
  })
})
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

* `screenPosition` String - 指定需要模拟的屏幕
    (默认 : `desktop`)
  * `desktop`
  * `mobile`
* `screenSize` Object - 设置模拟屏幕 size (screenPosition == mobile)
  * `width` Integer - 设置模拟屏幕 width
  * `height` Integer - 设置模拟屏幕 height
* `viewPosition` Object - 在屏幕放置 view
    (screenPosition == mobile) (默认: `{x: 0, y: 0}`)
  * `x` Integer - 设置偏移左上角的x轴 
  * `y` Integer - 设置偏移左上角的y轴 
* `deviceScaleFactor` Integer - 设置设备比例因子 (如果为0，默认为原始屏幕比例) (默认: `0`)
* `viewSize` Object - 设置模拟视图 size (空表示不覆盖)
  * `width` Integer - 设置模拟视图 width
  * `height` Integer - 设置模拟视图 height
* `fitToView` Boolean - 如果有必要的话，是否把模拟视图按比例缩放来适应可用空间  (默认: `false`)
* `offset` Object - 可用空间内的模拟视图偏移 (不在适应模式) (默认: `{x: 0, y: 0}`)
  * `x` Float - 设置相对左上角的x轴偏移值
  * `y` Float - 设置相对左上角的y轴偏移值
* `scale` Float - 可用空间内的模拟视图偏移 (不在适应视图模式) (默认: `1`)

使用给定的参数来开启设备模拟.

### `webContents.disableDeviceEmulation()`

使用 `webContents.enableDeviceEmulation` 关闭设备模拟.

### `webContents.sendInputEvent(event)`

* `event` Object
  * `type` String (**必需**) - 事件类型，可以是 `mouseDown`,
    `mouseUp`, `mouseEnter`, `mouseLeave`, `contextMenu`, `mouseWheel`,
    `mouseMove`, `keyDown`, `keyUp`, `char`.
  * `modifiers` Array - 事件的 modifiers 数组, 可以是
    include `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

向 page 发送一个输入 `event` .

对键盘事件来说，`event` 对象还有如下属性 :

* `keyCode` String (**必需**) - 特点是将作为键盘事件发送. 可用的 key codes [Accelerator](accelerator.md).


对鼠标事件来说，`event` 对象还有如下属性 :

* `x` Integer (**required**)
* `y` Integer (**required**)
* `button` String - button 按下, 可以是 `left`, `middle`, `right`
* `globalX` Integer
* `globalY` Integer
* `movementX` Integer
* `movementY` Integer
* `clickCount` Integer

对鼠标滚轮事件来说，`event` 对象还有如下属性 :

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

开始订阅 提交 事件和捕获数据帧，当有 提交 事件时，使用 `callback(frameBuffer)` 调用 `callback`.

`frameBuffer` 是一个包含原始像素数据的 `Buffer`,像素数据是按照 32bit BGRA 格式有效存储的，但是实际情况是取决于处理器的字节顺序的(大多数的处理器是存放小端序的，如果是在大端序的处理器上，数据是 32bit ARGB 格式).

### `webContents.endFrameSubscription()`

停止订阅帧提交事件.

### `webContents.savePage(fullPath, saveType, callback)`

* `fullPath` String - 文件的完整路径.
* `saveType` String - 指定保存类型.
  * `HTMLOnly` - 只保存html.
  * `HTMLComplete` - 保存整个 page 内容.
  * `MHTML` - 保存完整的 html 为 MHTML.
* `callback` Function - `function(error) {}`.
  * `error` Error

如果保存界面过程初始化成功，返回 true.

```javascript
win.loadURL('https://github.com')

win.webContents.on('did-finish-load', function () {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete', function (error) {
    if (!error)
      console.log('Save page successfully')
  })
})
```

## 实例属性

`WebContents` 对象也有下列属性:

### `webContents.session`

返回这个 `webContents` 使用的  [session](session.md) 对象.

### `webContents.hostWebContents`

返回这个 `webContents` 的父 `webContents` .

### `webContents.devToolsWebContents`

获取这个 `WebContents` 的开发者工具栏的 `WebContents` .

**注意:** 用户不可保存这个对象，因为当开发者工具栏关闭的时候它的值为 `null` .

### `webContents.debugger`

调试 API 为 [remote debugging protocol][rdp] 提供交替传送.

```javascript
try {
  win.webContents.debugger.attach('1.1')
} catch (err) {
  console.log('Debugger attach failed : ', err)
};

win.webContents.debugger.on('detach', function (event, reason) {
  console.log('Debugger detached due to : ', reason)
})

win.webContents.debugger.on('message', function (event, method, params) {
  if (method == 'Network.requestWillBeSent') {
    if (params.request.url == 'https://www.github.com')
      win.webContents.debugger.detach()
  }
})

win.webContents.debugger.sendCommand('Network.enable')
```

#### `webContents.debugger.attach([protocolVersion])`

* `protocolVersion` String (可选) - 请求调试协议版本.

添加 `webContents` 调试.

#### `webContents.debugger.isAttached()`

返回一个布尔值，标识是否已经给 `webContents` 添加了调试.

#### `webContents.debugger.detach()`

删除 `webContents` 调试.

#### `webContents.debugger.sendCommand(method[, commandParams, callback])`

* `method` String - 方法名, 应该是由远程调试协议定义的方法.
* `commandParams` Object (可选) - 请求参数为 JSON 对象.
* `callback` Function (可选) - Response
  * `error` Object - 错误消息，标识命令失败.
  * `result` Object - 回复在远程调试协议中由 'returns'属性定义的命令描述.

发送给定命令给调试目标.

### Event: 'detach'

* `event` Event
* `reason` String - 拆分调试器原因.

在调试 session 结束时发出事件.这在 `webContents` 关闭时或 `webContents` 请求开发者工具栏时发生.

### Event: 'message'

* `event` Event
* `method` String - 方法名.
* `params` Object - 在远程调试协议中由 'parameters' 属性定义的事件参数.

每当调试目标发出事件时发出.

[rdp]: https://developer.chrome.com/devtools/docs/debugger-protocol
[`webContents.findInPage`]: web-contents.md#webcontentsfindinpagetext-options
