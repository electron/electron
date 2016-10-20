# dialog

`dialog`モジュールは、ファイルやアラートを開くようなネイティブシステムダイアログを表示するためのAPIを提供します。そのため、ネイティブアプリケーションのようにウェブアプリケーションに同じユーザー体験を提供できます。

複数のファイルやディレクトリを選択するためのダイアログを表示する例です:

```javascript
const {dialog} = require('electron')
console.log(dialog.showOpenDialog({properties: ['openFile', 'openDirectory', 'multiSelections']}))
```

**Note for macOS**: シートとしてダイアログを表示したい場合、唯一しなければならないことは、`browserWindow`パラメーターを参照する`BrowserWindow`を提供することです。

## メソッド

`dialog`モジュールは次のメソッドを持っています:

### `dialog.showOpenDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (オプション)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
  * `properties` Array - ダイアログが使うべき機能を含め、`openFile`と`openDirectory`、`multiSelections`、`createDirectory`を含められます。
* `callback` Function (オプション)

成功したら、このメソッドはユーザーが選択したファイルパスの配列を返し、さうでなければ`undefined`を返します。

ユーザーが選択できる種類を制限したいときに、`filters`で表示したり選択できるファイル種別の配列を指定します。

```javascript
{
  filters: [
    { name: 'Images', extensions: ['jpg', 'png', 'gif'] },
    { name: 'Movies', extensions: ['mkv', 'avi', 'mp4'] },
    { name: 'Custom File Type', extensions: ['as'] },
    { name: 'All Files', extensions: ['*'] }
  ]
}
```

`extensions`配列は、ワイルドカードやドットなしで拡張子を指定すべきです（例えば、`'png'`は良いですが、`'.png'` と `'*.png'`はダメです）。すべてのファイルを表示するために、`'*'`ワイルドカードを使用します（それいがいのワイルドカードはサポートしていません）。

`callback`を通すと、APIは非同期に読み出し、結果は`callback(filenames)`経由で通します。

**Note:** WindowsとLinuxでは、オープンダイアログがファイル選択とディレクトリ選択の両方を選択することはできません。プラットフォーム上で `properties`に`['openFile', 'openDirectory']`を設定すると、ディレクトリ選択が表示されます。

### `dialog.showSaveDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (オプション)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
* `callback` Function (オプション)

成功すると、このメソッドはユーザーが選択したファイルのパスが返され、そうでなければ`undefined`が返されます。

`filters`が表示できるファイル種別配列を指定します。例えば、`dialog.showOpenDialog`を参照してください。

`callback`を通すと、APIは非同期でコールされ、結果は`callback(filename)`経由で通します。

### `dialog.showMessageBox([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (オプション)
* `options` Object
  * `type` String - `"none"`と `"info"`、 `"error"`、 `"question"`、`"warning"`を設定できます。Windowsでは、 "icon"オプションを使用してアイコンを設定しない限り、"question"は"info"として同じアイコンを表示します。
  * `buttons` Array - ボタン用のテキスト配列。
  * `defaultId` Integer - メッセージボックスを開くとき、既定で選択されるボタン配列でのボタンインデックスです
  * `title` String - メッセージボックスのタイトルで、いくつかのプラットフォームでは表示されません。
  * `message` String - メッセージボックスのコンテンツ。
  * `detail` String - メッセージの外部情報
  * `icon` [NativeImage](native-image.md)
  * `cancelId` Integer - ダイアログのボタンをクリックする代わりにユーザーがダイアログをキャンセルしたときに返す値です。既定では、ラベルの "cancel"や"no"を持つボタンのインデックスまたは、そのようなボタンが無ければ0を返します。macOSやWindowsでは、 すでに指定されているかどうかは関係なく、"Cancel"ボタンのインデックスはいつでも `cancelId`が使われます。
  * `noLink` Boolean - Windowsでは、Electronは、 ("Cancel" または "Yes"のような)共通ボタンである`buttons`の一つを見つけようとし、ダイアログ内のコマンドリンクとして表示します。この挙動が気に入らない場合は、 `noLink` を `true`に設定できます。
* `callback` Function

メッセージボックスを表示し、メッセージボックスが閉じるまでプロセスをブロックします。クリックされたボタンのインデックスを返します。

`callback`が通されると、APIは非同期にコールし、結果は`callback(response)`経由で通されます。

### `dialog.showErrorBox(title, content)`

エラーメッセージを表示するモデルダイアログを表示します。

 `app`モジュールが`ready`イベントを出力する前に、このAPIは安全にコールできます。スタートアップの早い段階でエラーを報告するのに通常は使われます。Linuxで、アプリの`ready`イベントの前にコールすると、メッセージは標準エラーに出力され、GUIダイアログは表示されません。
