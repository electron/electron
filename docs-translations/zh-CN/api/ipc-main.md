# ipcMain

`ipcMain` 模块是类
[EventEmitter](https://nodejs.org/api/events.html) 的实例.当在主进程中使用它的时候，它控制着由渲染进程(web page)发送过来的异步或同步消息.从渲染进程发送过来的消息将触发事件.

## 发送消息

同样也可以从主进程向渲染进程发送消息，查看更多 [webContents.send][web-contents-send] .

* 发送消息，事件名为 `channel`.
* 回应同步消息, 你可以设置 `event.returnValue`.
* 回应异步消息, 你可以使用
  `event.sender.send(...)`.

一个例子，在主进程和渲染进程之间发送和处理消息:

```javascript
// In main process.
const ipcMain = require('electron').ipcMain;
ipcMain.on('asynchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.sender.send('asynchronous-reply', 'pong');
});

ipcMain.on('synchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.returnValue = 'pong';
});
```

```javascript
// In renderer process (web page).
const ipcRenderer = require('electron').ipcRenderer;
console.log(ipcRenderer.sendSync('synchronous-message', 'ping')); // prints "pong"

ipcRenderer.on('asynchronous-reply', function(event, arg) {
  console.log(arg); // prints "pong"
});
ipcRenderer.send('asynchronous-message', 'ping');
```

## 监听消息

`ipcMain` 模块有如下监听事件方法:

### `ipcMain.on(channel, listener)`

* `channel` String
* `listener` Function

监听 `channel`, 当新消息到达，将通过 `listener(event, args...)` 调用 `listener`.

### `ipcMain.once(channel, listener)`

* `channel` String
* `listener` Function

为事件添加一个一次性用的`listener` 函数.这个 `listener` 只有在下次的消息到达 `channel` 时被请求调用，之后就被删除了.

### `ipcMain.removeListener(channel, listener)`

* `channel` String
* `listener` Function

为特定的 `channel` 从监听队列中删除特定的 `listener` 监听者.

### `ipcMain.removeAllListeners([channel])`

* `channel` String (可选)

删除所有监听者，或特指的 `channel` 的所有监听者.

## 事件对象

传递给 `callback` 的 `event` 对象有如下方法:

### `event.returnValue`

将此设置为在一个同步消息中返回的值.

### `event.sender`

返回发送消息的 `webContents` ，你可以调用 `event.sender.send`  来回复异步消息，更多信息 [webContents.send][web-contents-send].

[web-contents-send]: web-contents.md#webcontentssendchannel-arg1-arg2-