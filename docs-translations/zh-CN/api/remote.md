# remote

`remote` 模块提供了一种在渲染进程（网页）和主进程之间进行进程间通讯（IPC）的简便途径。

Electron中, 与GUI相关的模块（如 `dialog`, `menu` 等)只存在于主进程，而不在渲染进程中
。为了能从渲染进程中使用它们，需要用`ipc`模块来给主进程发送进程间消息。使用 `remote` 
模块，可以调用主进程对象的方法，而无需显式地发送进程间消息，这类似于 Java 的 [RMI][rmi]。
下面是从渲染进程创建一个浏览器窗口的例子：

```javascript
const remote = require('electron').remote
const BrowserWindow = remote.BrowserWindow

var win = new BrowserWindow({ width: 800, height: 600 })
win.loadURL('https://github.com')
```

**注意:** 反向操作（从主进程访问渲染进程），可以使用[webContents.executeJavascript](web-contents.md#webcontentsexecutejavascriptcode-usergesture).

## 远程对象

`remote`模块返回的每个对象（包括函数）都代表了主进程中的一个对象（我们称之为远程对象或者远程函数）。
当调用远程对象的方法、执行远程函数或者使用远程构造器（函数）创建新对象时，其实就是在发送同步的进程间消息。

在上面的例子中， `BrowserWindow` 和 `win` 都是远程对象，然而
`new BrowserWindow` 并没有在渲染进程中创建 `BrowserWindow` 对象。
而是在主进程中创建了 `BrowserWindow` 对象，并在渲染进程中返回了对应的远程对象，即
`win` 对象。

请注意只有 [可枚举属性](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Enumerability_and_ownership_of_properties) 才能通过 remote 进行访问.

## 远程对象的生命周期

Electron 确保在渲染进程中的远程对象存在（换句话说，没有被垃圾收集），那主进程中的对应对象也不会被释放。
当远程对象被垃圾收集之后，主进程中的对应对象才会被取消关联。

如果远程对象在渲染进程泄露了（即，存在某个表中但永远不会释放），那么主进程中的对应对象也一样会泄露，
所以你必须小心不要泄露了远程对象。If the remote object is leaked in the renderer process (e.g. stored in a map but
never freed), the corresponding object in the main process will also be leaked,
so you should be very careful not to leak remote objects.

不过，主要的值类型如字符串和数字，是传递的副本。

## 给主进程传递回调函数

在主进程中的代码可以从渲染进程——`remote`模块——中接受回调函数，但是使用这个功能的时候必须非常非常小心。Code in the main process can accept callbacks from the renderer - for instance
the `remote` module - but you should be extremely careful when using this
feature.

首先，为了避免死锁，传递给主进程的回调函数会进行异步调用。所以不能期望主进程来获得传递过去的回调函数的返回值。First, in order to avoid deadlocks, the callbacks passed to the main process
are called asynchronously. You should not expect the main process to
get the return value of the passed callbacks.

比如，你不能主进程中给`Array.map`传递来自渲染进程的函数。

```javascript
// 主进程 mapNumbers.js
exports.withRendererCallback = function (mapper) {
  return [1, 2, 3].map(mapper)
}

exports.withLocalCallback = function () {
  return exports.mapNumbers(function (x) {
    return x + 1
  })
}
```

```javascript
// 渲染进程
var mapNumbers = require('remote').require('./mapNumbers')

var withRendererCb = mapNumbers.withRendererCallback(function (x) {
  return x + 1
})

var withLocalCb = mapNumbers.withLocalCallback()

console.log(withRendererCb, withLocalCb) // [true, true, true], [2, 3, 4]
```

如你所见，渲染器回调函数的同步返回值没有按预期产生，与主进程中的一模一样的回调函数的返回值不同。

其次，传递给主进程的函数会持续到主进程对他们进行垃圾回收。

例如，下面的代码第一眼看上去毫无问题。给远程对象的`close`事件绑定了一个回调函数：

```javascript
remote.getCurrentWindow().on('close', function () {
  // blabla...
})
```

但记住主进程会一直保持对这个回调函数的引用，除非明确的卸载它。如果不卸载，每次重新载入窗口都会再次绑定，这样每次重启就会泄露一个回调函数。

更严重的是，由于前面安装了回调函数的上下文已经被释放，所以当主进程的 `close` 事件触发的时候，会抛出异常。

为了避免这个问题，要确保对传递给主进程的渲染器的回调函数进行清理。可以清理事件处理器，或者明确告诉主进行取消来自已经退出的渲染器进程中的回调函数。

## 访问主进程中的内置模块

在主进程中的内置模块已经被添加为`remote`模块中的属性，所以可以直接像使用`electron`模块一样直接使用它们。

```javascript
const app = remote.app
```

## 方法

`remote` 模块有以下方法：

### `remote.require(module)`

* `module` String

返回在主进程中执行 `require(module)` 所返回的对象。

### `remote.getCurrentWindow()`

返回该网页所属的 [`BrowserWindow`](browser-window.md) 对象。

### `remote.getCurrentWebContents()`

返回该网页的 [`WebContents`](web-contents.md) 对象

### `remote.getGlobal(name)`

* `name` String

返回在主进程中名为 `name` 的全局变量(即 `global[name]`) 。

### `remote.process`

返回主进程中的 `process` 对象。等同于
`remote.getGlobal('process')` 但是有缓存。

[rmi]: http://en.wikipedia.org/wiki/Java_remote_method_invocation
