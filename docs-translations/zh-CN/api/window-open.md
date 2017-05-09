# `window.open` 函数

> 通过链接打开一个新窗口。

当在界面中使用 `window.open` 来创建一个新的窗口时候，将会创建一个 `BrowserWindow` 的实例，并且将返回一个标识，这个界面通过标识来对这个新的窗口进行有限的控制。

这个标识对传统的web界面来说，通过它能对子窗口进行有限的功能性兼容控制。
想要完全的控制这个窗口，可以直接创建一个 `BrowserWindow`。

新创建的 `BrowserWindow` 默认为继承父窗口的属性参数，想重写属性的话可以在 `features` 中设置他们。

### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (可选)
* `features` String (可选)

创建一个新的 window 并且返回一个 [`BrowserWindowProxy`](browser-window-proxy.md) 类的实例。

 `features` 遵循标准浏览器的格式，但是每个 feature 应该作为 `BrowserWindow` 参数的一个字段。

**注意：**
* 如果在父窗口禁用 Node integration，那么在新打开的 `window` 中将始终禁用。
* 非标准功能（不由 Chromium 或 Electron 处理）的
   `features` 将被传递给任何注册的 `webContent` 的 `new-window` 事件
   在 `additionalFeatures` 参数的处理程序。

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

通过指定位置或用 `*` 来代替没有明确位置来向父窗口发送信息。
