# ipc (主行程)

當在主行程裡使用 `ipc` 模組，這個模組負責處理來自渲染行程（網頁）的非同步與同步訊息。
來自渲染器的訊息將會被注入到這個模組裡。

## 傳送訊息

同樣的也可以透過主行程來傳送訊息到渲染行程，更多資訊請參考 [WebContents.send](browser-window.md#webcontentssendchannel-args)

- 當傳送一個訊息， 事件名稱為 `channel`
- 回覆同步訊息，你需要設定 `event.returnValue`
- 送出一個非同步訊息回給發送端，你可以使用 `event.sender.send(...)`

這裏是一個傳送和處理訊息的範例，在渲染行程與主行程之間：

```javascript
// 在主行程裡
var ipc = require('ipc')
ipc.on('asynchronous-message', function (event, arg) {
  console.log(arg)  // 輸出 "ping"
  event.sender.send('asynchronous-reply', 'pong')
})

ipc.on('synchronous-message', function (event, arg) {
  console.log(arg)  // 輸出 "ping"
  event.returnValue = 'pong'
})
```

```javascript
// 在渲染行程裡 (網頁).
var ipc = require('ipc')
console.log(ipc.sendSync('synchronous-message', 'ping')) // 輸出 "pong"

ipc.on('asynchronous-reply', function (arg) {
  console.log(arg) // 輸出 "pong"
})
ipc.send('asynchronous-message', 'ping')
```

## 聆聽訊息

`ipc` 模組擁有下列幾個方法去聆聽事件：

### `ipc.on(channel, callback)`

* `channel` String - 事件名稱
* `callback` Function

當一個事件發生 `callback` 會帶著 `event` 物件和一個訊息 `arg`

## IPC 事件

`event` 物件被傳入 `callback` 具有以下幾個方法：

### `Event.returnValue`

設定這個值可以回傳一個同步訊息

### `Event.sender`

回傳 `WebContents` 可以送出訊息

### `Event.sender.send(channel[, arg1][, arg2][, ...])`

* `channel` String - 事件名稱
* `arg` (選用)

此傳送訊息是非同步的訊息，至渲染行程，可以是一個一系列的參數 `arg` 可以是任何型態。
