# DownloadItem

`DownloadItem`は、Electronでアイテムのダウンロードを示すEventEmitterです。 `Session`モジュールの`will-download`イベントで使用され、ダウンロードしたアイテムをコントロールすることができます。

```javascript
// In the main process.
win.webContents.session.on('will-download', function(event, item, webContents) {
  // Set the save path, making Electron not to prompt a save dialog.
  item.setSavePath('/tmp/save.pdf');
  console.log(item.getMimeType());
  console.log(item.getFilename());
  console.log(item.getTotalBytes());
  item.on('updated', function() {
    console.log('Received bytes: ' + item.getReceivedBytes());
  });
  item.on('done', function(e, state) {
    if (state == "completed") {
      console.log("Download successfully");
    } else {
      console.log("Download is cancelled or interrupted that can't be resumed");
    }
  });
```

## イベント

### イベント: 'updated'

`downloadItem`が更新されたときに出力されます。

### イベント: 'done'

* `event` Event
* `state` String
  * `completed` - ダウンロードが成功で完了
  * `cancelled` - ダウンロードをキャンセル
  * `interrupted` - ファイルサーバーとの接続が切れてエラー

ダウンロードが終了状態になったときに出力されます。終了状態には、ダウンロードの完了、ダウンロードのキャンセル（`downloadItem.cancel()`経由）、レジュームできないダウンロードの中断などです。

## メソッド

`downloadItem`オブジェクトは次のメソッドを持ちます:

### `downloadItem.setSavePath(path)`

* `path` String - ダウンロードアイテムの保存ファイルパスを設定します。

APIはセッションの`will-download`コールバック関数のみで提供されます。API経由で保存パスを設定しなかった場合、Electronは保存パスを決めるための元のルーチン（通常は保存ダイアログ）を使用します。

### `downloadItem.pause()`

ダウンロードをポーズします。

### `downloadItem.resume()`

ポーズしたダウンロードを再開します。

### `downloadItem.cancel()`

ダウンロード操作をキャンセルします。

### `downloadItem.getURL()`

どのURLからアイテムをダウンロードするのかを示す`String`を返します。

### `downloadItem.getMimeType()`

mimeタイプを示す`String`を返します。

### `downloadItem.hasUserGesture()`

ダウンロードがユーザージェスチャーを持っているかどうかを示す`Boolean`を返します。

### `downloadItem.getFilename()`

ダウンロードアイテムのファイル名を示す`String`を返します。

**Note:** ファイル名はローカルディスクに実際に保存するものといつも同じとは限りません。ダウンロード保存ダイアログでユーザーがファイル名を変更していると、保存するファイルの実際の名前は異なります。

### `downloadItem.getTotalBytes()`

ダウンロードアイテムの合計バイトサイズを示す`Integer`を返します。
サイズが不明な場合、0を返します。

### `downloadItem.getReceivedBytes()`

ダウンロードしたアイテムの受信バイト数を示す`Integer`を返します。

### `downloadItem.getContentDisposition()`

レスポンスヘッダーからContent-Dispositionを示す`String`を返します。
