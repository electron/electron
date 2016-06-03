# ipcRenderer

`ipcRenderer` 模块是一个
[EventEmitter](https://nodejs.org/api/events.html) 类的实例. 它提供了有限的方法，你可以从渲染进程向主进程发送同步或异步消息. 也可以收到主进程的相应.

查看 [ipcMain](ipc-main.md) 代码例子.

## 消息监听

`ipcRenderer`  模块有下列方法来监听事件:

### `ipcRenderer.on(channel, listener)`

* `channel` String
* `listener` Function

监听 `channel`, 当有新消息到达，使用 `listener(event, args...)` 调用 `listener` .

### `ipcRenderer.once(channel, listener)`

* `channel` String
* `listener` Function

为这个事件添加一个一次性 `listener` 函数.这个 `listener` 将在下一次有新消息被发送到 `channel` 的时候被请求调用，之后就被删除了.

### `ipcRenderer.removeListener(channel, listener)`

* `channel` String
* `listener` Function

从指定的 `channel` 中的监听者数组删除指定的 `listener` .

### `ipcRenderer.removeAllListeners([channel])`

* `channel` String (optional)

删除所有的监听者，或者删除指定 `channel` 中的全部.

## 发送消息

`ipcRenderer` 模块有如下方法来发送消息:

### `ipcRenderer.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

通过 `channel` 向主进程发送异步消息，也可以发送任意参数.参数会被JSON序列化，之后就不会包含函数或原型链.

主进程通过使用 `ipcMain` 模块来监听 `channel`，从而处理消息.

### `ipcRenderer.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

通过 `channel` 向主进程发送同步消息，也可以发送任意参数.参数会被JSON序列化，之后就不会包含函数或原型链.

主进程通过使用 `ipcMain` 模块来监听 `channel`，从而处理消息,
通过 `event.returnValue` 来响应.

__注意:__ 发送同步消息将会阻塞整个渲染进程,除非你知道你在做什么，否则就永远不要用它 .

### `ipcRenderer.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (可选)

类似 `ipcRenderer.send` ，但是它的事件将发往 host page 的 `<webview>` 元素，而不是主进程.