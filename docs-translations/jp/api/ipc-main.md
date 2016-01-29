# ipcMain

`ipcMain`モジュールは[EventEmitter](https://nodejs.org/api/events.html)クラスのインスタンスです。メインプロセスで使うと、レンダラ―プロセス（ウェブページ）から非同期、同期敵にメッセージ送信できます。レンダラ―から送信されたメッセージはこのモジュールに出力されます。

## メッセージ送信

メインプロレスからレンダラ―プロセスへメッセージ送信を可能にします。さらなる情報は、[webContents.send](web-contents.md#webcontentssendchannel-arg1-arg2-) を参照してください。

* メッセージを送信するとき、イベント名は`channel`です。
* 同期的にメッセージを返信するために、`event.returnValue`を設定する必要があります。
* 送信者に非同期に戻して送信するために、 `event.sender.send(...)`を使えます。

レンダラーとメインプロセス間でメッセージの送信とハンドリングをする例です:

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

## メッセージ受信

`ipcMain`モジュールが持つイベント受信用のメソッドです。

### `ipcMain.on(channel, callback)`

* `channel` String - イベント名
* `callback` Function

イベントが発生すると、`callback`が`event`と任意の引数でコールされます。

### `ipcMain.removeListener(channel, callback)`

* `channel` String - イベント名
* `callback` Function - 使用したのと同じ関数への参照です
  `ipcMain.on(channel, callback)`

一度メッセージを受信すると、もうコールバックをアクティブにしたくなく、何らかの理由でメッセージ送信を単に止めるには、この関数が指定したチャンネルのコールバックハンドラーを削除します。

### `ipcMain.removeAllListeners(channel)`

* `channel` String - The event name.

このipcチャンネルの *全ての* ハンドラーを削除します。

### `ipcMain.once(channel, callback)`

ハンドラーの実行のために`ipcMain.on()`の代わりにこれを使うと、一度だけ発生することを意味し、`callback`の一回のコールの後にアクティブにしないのと同じです。

## IPC Event

`callback`に渡される`event`オブジェクトには次のメソッドがあります。

### `event.returnValue`

値にこれを設定すると同期メッセージを返します。

### `event.sender`

送信したメッセージ`webContents`を返すと、非同期にメッセージを返信するために`event.sender.send`をコールできます。さらなる情報は、[webContents.send](web-contents.md#webcontentssendchannel-arg1-arg2-) を見てください。
