# ipcRenderer

`ipcRenderer`モジュールは[EventEmitter](https://nodejs.org/api/events.html) クラスのインスタンスです。レンダープロセス（ウェブページ）からメインプロセスに同期、非同期にメッセージを送信できるメソッドを提供します。メインプロセスから返答を受け取ることもできます。


コード例は [ipcMain](ipc-main.md) をみてください。

## メッセージの受信

`ipcRenderer`モジュールは、イベントを受信するための次のメソッドを持ちます：

### `ipcRenderer.on(channel, callback)`

* `channel` String - イベント名
* `callback` Function

イベントが発生したとき、任意の引数と `event`オブジェクトで`callback`をコールします。

### `ipcRenderer.removeListener(channel, callback)`

* `channel` String - イベント名
* `callback` Function - 使用したのと同じ関数への参照
  `ipcRenderer.on(channel, callback)`

一度メッセージを受信すると、もうコールバックをアクティブにしたくなく、何らかの理由でメッセージ送信を単に止めるには、この関数が指定したチャンネルのコールバックハンドラーを削除します。

### `ipcRenderer.removeAllListeners(channel)`

* `channel` String - The event name.

このipcチャンネルの *全ての* ハンドラーを削除します。

### `ipcMain.once(channel, callback)`

ハンドラーの実行のために`ipcMain.on()`の代わりにこれを使うと、一度だけ発生することを意味し、`callback`の一回のコールの後にアクティブにしないのと同じです。

## メッセージ送信

`ipcRenderer`モジュールは、イベントを送信するための次のメソッドを持ちます：

### `ipcRenderer.send(channel[, arg1][, arg2][, ...])`

* `channel` String - イベント名
* `arg` (optional)

`channel`

`channel`経由でメインプロセスに非同期にイベントを送信し、任意の引数を送信できます。メインプロセスは`ipcMain`で`channel`を受信することでハンドルします。

### `ipcRenderer.sendSync(channel[, arg1][, arg2][, ...])`

* `channel` String - イベント名
* `arg` (optional)

`channel`経由でメインプロセスに同期的にイベントを送信し、任意の引数を送信できます。

メインプロセスは`ipcMain`で`channel`を受信することでハンドルし、 `event.returnValue`を設定してリプライします。

__Note:__ 同期的なメッセージ送信をすると全てのレンダラ―プロセスがブロックされるので、何をしているか理解できない限り、これを使うべきではありません。

### `ipcRenderer.sendToHost(channel[, arg1][, arg2][, ...])`

* `channel` String - イベント名.
* `arg` (optional)

`ipcRenderer.send`のようですが、メインプロセスの代わりにホストに`<webview>`エレメントにイベントを送信します。
