# `<webview>` 标签

使用 `webview` 标签来把 'guest' 内容（例如 web pages ）嵌入到你的 Electron app 中. guest内容包含在 `webview` 容器中.一个嵌入你应用的page控制着guest内容如何布局摆放和表达含义.

与 `iframe` 不同, `webview` 和你的应用运行的是不同的进程. 它不拥有渲染进程的权限，并且应用和嵌入内容之间的交互全部都是异步的.因为这能保证应用的安全性不受嵌入内容的影响.

## 例子

把一个 web page 嵌入到你的app，首先添加 `webview` 标签到你的app待嵌入page(展示 guest content). 在一个最简单的 `webview` 中，它包含了  web page 的文件路径和一个控制 `webview` 容器展示效果的css样式:

```html
<webview id="foo" src="https://www.github.com/" style="display:inline-block; width:640px; height:480px"></webview>
```

如果想随时控制 guest 内容，可以添加 JavaScript 脚本来监听 `webview` 事件使用  `webview` 方法来做出响应. 这里是2个事件监听的例子：一个监听 web page 准备加载，另一个监听 web page 停止加载，并且在加载的时候显示一条 "loading..." 信息:

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

## 标签属性

`webview` 标签有下面一些属性 :

### `src`

```html
<webview src="https://www.github.com/"></webview>
```

将一个可用的url做为这个属性的初始值添加到顶部导航.

如果把当前页的src添加进去将加载当前page.

 `src`同样可以填 data urls，例如
`data:text/plain,Hello, world!`.

### `autosize`

```html
<webview src="https://www.github.com/" autosize="on" minwidth="576" minheight="432"></webview>
```

如果这个属性的值为 "on" ， `webview` 容器将会根据属性`minwidth`, `minheight`, `maxwidth`, 和
`maxheight` 的值在它们之间自适应. 只有在 `autosize` 开启的时候这个约束才会有效. 当 `autosize` 开启的时候， `webview` 容器的 size 只能在上面那四个属性值之间.

### `nodeintegration`

```html
<webview src="http://www.google.com/" nodeintegration></webview>
```

如果设置了这个属性， `webview` 中的 guest page 将整合node，并且拥有可以使用系统底层的资源，例如 `require` 和 `process` .

### `plugins`

```html
<webview src="https://www.github.com/" plugins></webview>
```

如果这个属性的值为 "on" ， `webview` 中的 guest page 就可以使用浏览器插件。

### `preload`

```html
<webview src="https://www.github.com/" preload="./test.js"></webview>
```

在 guest page 中的其他脚本执行之前预加载一个指定的脚本。规定预加载脚本的url须如 `file:` 或者 `asar:`，因为它在是 guest page 中通过通过 `require` 命令加载的。

如果  guest page 没有整合 node ，这个脚本将试图使用真个 Node APIs ，但是在这个脚本执行完毕时，之前由node插入的全局对象会被删除。


### `httpreferrer`

```html
<webview src="https://www.github.com/" httpreferrer="http://cheng.guru"></webview>
```

为 guest page 设置 referrer URL。

### `useragent`

```html
<webview src="https://www.github.com/" useragent="Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko"></webview>
```

在 guest page 加载之前为其设置用户代理。如果页面已经加载了，可以使用 `setUserAgent` 方法来改变用户代理。

### `disablewebsecurity`

```html
<webview src="https://www.github.com/" disablewebsecurity></webview>
```

如果这个属性的值为 "on" ， guest page会禁用web安全控制.

### partition

```html
<webview src="https://github.com" partition="persist:github"></webview>
<webview src="http://electron.atom.io" partition="electron"></webview>
```

为page设置session。如果初始值为 `partition` ,这个 page 将会为app中的所有 page 应用同一个持续有效的 session。如果没有 `persist:` 前缀, 这个 page 将会使用一个历史 session 。通过分配使用相同的 `partition`, 所有的page都可以分享相同的session。如果  `partition` 没有设置，那app将使用默认的session.

这个值只能在在第一个渲染进程之前设置修改，之后修改的话会无效并且抛出一个DOM异常.

### `allowpopups`

```html
<webview src="https://www.github.com/" allowpopups></webview>
```

如果这个属性的值为 "on" ，将允许 guest page 打开一个新窗口。

### `blinkfeatures`

```html
<webview src="https://www.github.com/" blinkfeatures="PreciseMemoryInfo, CSSVariables"></webview>
```

这个属性的值为一个用逗号分隔的列表，它的值指定特性被启用。你可以从[setFeatureEnabledFromString][blink-feature-string]函数找到完整的支持特性。

## 方法

 `webview` 的方法集合:

**注意:** webview 元素必须在使用这些方法之前加载完毕。

**例如**

```javascript
webview.addEventListener('dom-ready', function () {
  webview.openDevTools()
})
```

### `<webview>.loadURL(url[, options])`

* `url` URL
* `options` Object (可选)
  * `httpReferrer` String - 一个http类型的url.
  * `userAgent` String -用于发起请求的用户代理.
  * `extraHeaders` String - 额外的headers,用 "\n"分隔.

加载 webview 中的 `url`，`url` 必须包含协议前缀，例如 `http://` 或 `file://`.

### `<webview>.getURL()`

从 guest page 中返回 url.

### `<webview>.getTitle()`

从 guest page 中返回 title.

### `<webview>.isLoading()`

返回一个 guest page 是否仍在加载资源的布尔值.

### `<webview>.isWaitingForResponse()`

返回一个 guest page 是否正在等待page的主要资源做出回应的布尔值.


### `<webview>.stop()`

停止渲染.

### `<webview>.reload()`

重新加载 guest page.

### `<webview>.reloadIgnoringCache()`

忽视缓存，重新加载 guest page.

### `<webview>.canGoBack()`

返回一个 guest page 是否能够回退的布尔值.

### `<webview>.canGoForward()`

返回一个 guest page 是否能够前进的布尔值.

### `<webview>.canGoToOffset(offset)`

* `offset` Integer

返回一个 guest page 是否能够前进到 `offset` 的布尔值.

### `<webview>.clearHistory()`

清除导航历史.

### `<webview>.goBack()`

guest page 回退.

### `<webview>.goForward()`

guest page 前进.

### `<webview>.goToIndex(index)`

* `index` Integer

guest page 导航到指定的绝对位置.

### `<webview>.goToOffset(offset)`

* `offset` Integer

guest page 导航到指定的相对位置.

### `<webview>.isCrashed()`

返回一个 渲染进程是否崩溃 的布尔值.

### `<webview>.setUserAgent(userAgent)`

* `userAgent` String

重新设置用户代理.

### `<webview>.getUserAgent()`

返回用户代理名字，返回类型：`String`.

### `<webview>.insertCSS(css)`

* `css` String

插入css.

### `<webview>.executeJavaScript(code, userGesture, callback)`

* `code` String
* `userGesture` Boolean - 默认 `false`.
* `callback` Function (可选) - 回调函数.
  * `result`

评估  `code` ，如果 `userGesture` 值为 true ，它将在这个page里面创建用户手势. HTML APIs ，如 `requestFullScreen`,它需要用户响应，那么将自动通过这个参数优化.

### `<webview>.openDevTools()`

为 guest page 打开开发工具调试窗口.

### `<webview>.closeDevTools()`

为 guest page 关闭开发工具调试窗口.

### `<webview>.isDevToolsOpened()`

返回一个 guest page 是否打开了开发工具调试窗口的布尔值.

### `<webview>.isDevToolsFocused()`

返回一个 guest page 是否聚焦了开发工具调试窗口的布尔值.

### `<webview>.inspectElement(x, y)`

* `x` Integer
* `y` Integer

开始检查 guest page 在 (`x`, `y`) 位置的元素.

### `<webview>.inspectServiceWorker()`

在 guest page 中为服务人员打开开发工具.

### `<webview>.setAudioMuted(muted)`

* `muted` Boolean
设置 guest page 流畅(muted).

### `<webview>.isAudioMuted()`

返回一个 guest page 是否流畅的布尔值.

### `<webview>.undo()`

在page中编辑执行 `undo` 命令.

### `<webview>.redo()`

在page中编辑执行 `redo` 命令.

### `<webview>.cut()`

在page中编辑执行 `cut` 命令.

### `<webview>.copy()`

在page中编辑执行 `copy` 命令.

### `<webview>.paste()`

在page中编辑执行 `paste` 命令.

### `<webview>.pasteAndMatchStyle()`

在page中编辑执行 `pasteAndMatchStyle` 命令.

### `<webview>.delete()`

在page中编辑执行 `delete` 命令.

### `<webview>.selectAll()`

在page中编辑执行 `selectAll` 命令.

### `<webview>.unselect()`

在page中编辑执行 `unselect` 命令.

### `<webview>.replace(text)`

* `text` String

在page中编辑执行 `replace` 命令.

### `<webview>.replaceMisspelling(text)`

* `text` String

在page中编辑执行 `replaceMisspelling` 命令.

### `<webview>.insertText(text)`

* `text` String

插入文本.

### `<webview>.findInPage(text[, options])`

* `text` String - 搜索内容,不能为空.
* `options` Object (可选)
  * `forward` Boolean - 向前或向后, 默认为 `true`.
  * `findNext` Boolean - 是否查找的第一个结果,
    默认为 `false`.
  * `matchCase` Boolean - 是否区分大小写,
    默认为 `false`.
  * `wordStart` Boolean - 是否只查找首字母.
    默认为 `false`.
  * `medialCapitalAsWordStart` Boolean - 当配合 `wordStart`的时候,接受一个文字中的匹配项，要求匹配项是以大写字母开头后面跟小写字母或者没有字母。可以接受一些其他单词内部匹配, 默认为 `false`.

发起一个请求来寻找页面中的所有匹配 `text` 的地方并且返回一个 `Integer`来表示这个请求用的请求Id. 这个请求结果可以通过订阅[`found-in-page`](web-view-tag.md#event-found-in-page) 事件来取得.

### `<webview>.stopFindInPage(action)`

* `action` String - 指定一个行为来接替停止
  [`<webview>.findInPage`](web-view-tag.md#webviewtagfindinpage) 请求.
  * `clearSelection` - 转变为一个普通的 selection.
  * `keepSelection` - 清除 selection.
  * `activateSelection` - 聚焦并点击 selection node.

使用 `action` 停止 `findInPage` 请求.

### `<webview>.print([options])`

打印输出 `webview` 的 web page. 类似 `webContents.print([options])`.

### `<webview>.printToPDF(options, callback)`

以pdf格式打印输出 `webview` 的 web page. 类似 `webContents.printToPDF(options, callback)`.

### `<webview>.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

通过 `channel` 向渲染进程发出异步消息，你也可以发送任意的参数。
渲染进程通过`ipcRenderer` 模块监听 `channel` 事件来控制消息.

例子
[webContents.send](web-contents.md#webcontentssendchannel-args).

### `<webview>.sendInputEvent(event)`

* `event` Object

向 page 发送输入事件.

查看 [webContents.sendInputEvent](web-contents.md##webcontentssendinputeventevent)
关于 `event` 对象的相信介绍.

### `<webview>.getWebContents()`

返回和这个 `webview` 相关的 [WebContents](web-contents.md).

## DOM 事件

`webview` 可用下面的 DOM 事件:

### Event: 'load-commit'

返回:

* `url` String
* `isMainFrame` Boolean

加载完成触发. 这个包含当前文档的导航和副框架的文档加载，但是不包含异步资源加载.

### Event: 'did-finish-load'

在导航加载完成时触发，也就是tab 的 spinner停止spinning，并且加载事件处理.

### Event: 'did-fail-load'

Returns:

* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String

类似 `did-finish-load` ，在加载失败或取消是触发，例如提出    `window.stop()`.

### Event: 'did-frame-finish-load'

返回:

* `isMainFrame` Boolean

当一个 frame 完成 加载时触发.

### Event: 'did-start-loading'

开始加载时触发.

### Event: 'did-stop-loading'

停止家在时触发.

### Event: 'did-get-response-details'

返回:

* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

当获得返回详情的时候触发.

`status` 指示 socket 连接来下载资源.

### Event: 'did-get-redirect-request'

返回:

* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean

当重定向请求资源被接收的时候触发.

### Event: 'dom-ready'

当指定的frame文档加载完毕时触发.

### Event: 'page-title-updated'

返回:

* `title` String
* `explicitSet` Boolean

当导航中的页面title被设置时触发.
在title通过文档路径异步加载时`explicitSet`为false.

### Event: 'page-favicon-updated'

返回:

* `favicons` Array - Array of URLs.

当page收到了图标url时触发.

### Event: 'enter-html-full-screen'

当通过HTML API使界面进入全屏时触发.

### Event: 'leave-html-full-screen'

当通过HTML API使界面退出全屏时触发.

### Event: 'console-message'

返回:

* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

当客户端输出控制台信息的时候触发.

下面示例代码将所有信息输出到内置控制台，没有考虑到输出等级和其他属性。

```javascript
webview.addEventListener('console-message', function (e) {
  console.log('Guest page logged a message:', e.message)
})
```

### Event: 'found-in-page'

返回:

* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - 指明下面是否还有更多的回应.
  * `activeMatchOrdinal` Integer (可选) - 活动匹配位置
  * `matches` Integer (optional) - 匹配数量.
  * `selectionArea` Object (optional) - 整合第一个匹配域.

在请求[`webview.findInPage`](web-view-tag.md#webviewtagfindinpage)结果有效时触发.

```javascript
webview.addEventListener('found-in-page', function (e) {
  if (e.result.finalUpdate)
    webview.stopFindInPage('keepSelection')
})

const rquestId = webview.findInPage('test')
```

### Event: 'new-window'

返回:

* `url` String
* `frameName` String
* `disposition` String - 可以为 `default`, `foreground-tab`, `background-tab`,
  `new-window` 和 `other`.
* `options` Object - 参数应该被用作创建新的
  `BrowserWindow`.

在 guest page 试图打开一个新的浏览器窗口时触发.

下面示例代码在系统默认浏览器中打开了一个新的url.

```javascript
webview.addEventListener('new-window', function (e) {
  require('electron').shell.openExternal(e.url)
})
```

### Event: 'will-navigate'

返回:

* `url` String

当用户或page尝试开始导航时触发.
它能在 `window.location` 变化或者用户点击连接的时候触发.

这个事件在以 APIS 编程方式开始导航时不会触发，例如 `<webview>.loadURL` 和 `<webview>.back`.

在页面内部导航跳转也将不回触发这个事件，例如点击锚链接或更新 `window.location.hash`.使用 `did-navigate-in-page` 来实现页内跳转事件.

使用 `event.preventDefault()` 并不会起什么用.

### Event: 'did-navigate'

返回:

* `url` String

当导航结束时触发.

在页面内部导航跳转也将不回触发这个事件，例如点击锚链接或更新 `window.location.hash`.使用 `did-navigate-in-page` 来实现页内跳转事件.

### Event: 'did-navigate-in-page'

返回:

* `url` String

当页内导航发生时触发.
当业内导航发生时，page url改变了，但是不会跳出 page . 例如在锚链接被电击或DOM `hashchange` 事件发生时触发.

### Event: 'close'

在 guest page试图关闭自己的时候触发.

下面的示例代码指示了在客户端试图关闭自己的时候将改变导航连接为`about:blank`.

```javascript
webview.addEventListener('close', function () {
  webview.src = 'about:blank'
})
```

### Event: 'ipc-message'

返回:

* `channel` String
* `args` Array

在 guest page 向嵌入页发送一个异步消息的时候触发.

你可以很简单的使用 `sendToHost` 方法和 `ipc-message` 事件在 guest page 和 嵌入页(embedder page)之间通信:

```javascript
// In embedder page.
webview.addEventListener('ipc-message', function (event) {
  console.log(event.channel)
  // Prints "pong"
})
webview.send('ping')
```

```javascript
// In guest page.
var ipcRenderer = require('electron').ipcRenderer
ipcRenderer.on('ping', function () {
  ipcRenderer.sendToHost('pong')
})
```

### Event: 'crashed'

在渲染进程崩溃的时候触发.

### Event: 'gpu-crashed'

在GPU进程崩溃的时候触发.

### Event: 'plugin-crashed'

返回:

* `name` String
* `version` String

在插件进程崩溃的时候触发.

### Event: 'destroyed'

在界面内容销毁的时候触发.

### Event: 'media-started-playing'

在媒体准备播放的时候触发.

### Event: 'media-paused'

在媒体暂停播放或播放放毕的时候触发.

### Event: 'did-change-theme-color'

在页面的主体色改变的时候触发. 
在使用 meta 标签的时候这就很常见了:

```html
<meta name='theme-color' content='#ff0000'>
```

### Event: 'devtools-opened'

在开发者工具打开的时候触发.

### Event: 'devtools-closed'

在开发者工具关闭的时候触发.

### Event: 'devtools-focused'

在开发者工具获取焦点的时候触发.

[blink-feature-string]: https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/platform/RuntimeEnabledFeatures.cpp&sq=package:chromium&type=cs&l=527