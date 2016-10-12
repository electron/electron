# ipc (主进程)

在主进程使用`ipc`模块时,`ipc`负责捕获从渲染进程(网页)发送的同步或者是异步消息.

## 发送消息

主进程也可以向渲染进程发送信息,具体可以看[WebContents.send](web-contents.md#webcontentssendchannel-args).

- 当发送消息的时候,事件名字为`channel`.
- 回复一个同步消息的时候,你需要使用`event.returnValue`
- 回复一个异步消息的时候,使用`event.sender.send(...)`

下面是一个主进程和渲染进程的通信例子.

```javascript
// 在主进程中.
var ipc = require('ipc')
ipc.on('asynchronous-message', function (event, arg) {
  console.log(arg)  // 打印 "ping"
  event.sender.send('asynchronous-reply', 'pong')
})

ipc.on('synchronous-message', function (event, arg) {
  console.log(arg)  // 打印 "ping"
  event.returnValue = 'pong'
})
```

```javascript
// 在渲染进程(网页).
var ipc = require('ipc')
console.log(ipc.sendSync('synchronous-message', 'ping')) // 打印 "pong"

ipc.on('asynchronous-reply', function (arg) {
  console.log(arg) // 打印 "pong"
})
ipc.send('asynchronous-message', 'ping')
```

## 监听消息

`ipc`模块有下列几种方法来监听事件.

### `ipc.on(channel, callback)`

* `channel` - 事件名称.
* `callback` - 回调函数.

当事件发生的时候,会传入`callback` `event`和`arg`参数.

## IPC 事件

传入`callback`的`event`对象含有下列方法.

### `Event.returnValue`

在同步消息中,设置这个值将会被返回.

### `Event.sender`

返回一个可以发送消息的`WebContents`.

### `Event.sender.send(channel[.arg1][,arg2][,...])`

* `channel` - 事件名称.
* `arg` (选用)

这个可以发送一个可带参数的异步消息回渲染进程.
