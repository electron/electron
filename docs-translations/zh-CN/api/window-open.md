# `window.open` 函数

当在界面中使用 `window.open` 来创建一个新的窗口时候，将会创建一个 `BrowserWindow` 的实例，并且将返回一个标识，这个界面通过标识来对这个新的窗口进行有限的控制.

这个标识对传统的web界面来说，通过它能对子窗口进行有限的功能性兼容控制.
想要完全的控制这个窗口，可以直接创建一个 `BrowserWindow` .

新创建的 `BrowserWindow` 默认为继承父窗口的属性参数，想重写属性的话可以在 `features` 中设置他们.

### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (可选)
* `features` String (可选)

创建一个新的window并且返回一个 `BrowserWindowProxy` 类的实例.

 `features` 遵循标准浏览器的格式，但是每个feature 应该作为 `BrowserWindow` 参数的一个字段.

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

通过指定位置或用 `*` 来代替没有明确位置来向父窗口发送信息.

## Class: BrowserWindowProxy

`BrowserWindowProxy` 由`window.open` 创建返回，并且提供了对子窗口的有限功能性控制.

### `BrowserWindowProxy.blur()`

子窗口的失去焦点.
### `BrowserWindowProxy.close()`

强行关闭子窗口，忽略卸载事件.

### `BrowserWindowProxy.closed`

在子窗口关闭之后恢复正常.

### `BrowserWindowProxy.eval(code)`

* `code` String

评估子窗口的代码.

### `BrowserWindowProxy.focus()`

子窗口获得焦点(让其显示在最前).

### `BrowserWindowProxy.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String


通过指定位置或用 `*` 来代替没有明确位置来向子窗口发送信息.

除了这些方法，子窗口还可以无特性和使用单一方法来实现  `window.opener` 对象.