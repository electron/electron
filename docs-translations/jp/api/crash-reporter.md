# crashReporter

`crash-reporter`モジュールはアプリのクラッシュレポートを送信することができます。

リモートサーバーに自動的にクラッシュレポートを登録する例です。

```javascript
const crashReporter = require('electron').crashReporter;

crashReporter.start({
  productName: 'YourName',
  companyName: 'YourCompany',
  submitURL: 'https://your-domain.com/url-to-submit',
  autoSubmit: true
});
```

## メソッド

`crash-reporter`モジュールは次のメソッドを持ちます:

### `crashReporter.start(options)`

`options` Object, properties:

* `productName` String, デフォルト: Electron.
* `companyName` String (**必須**)
* `submitURL` String, (**必須**)
  * クラッシュレポートがPOSTで送信されるURL
* `autoSubmit` Boolean, デフォルト: `true`.
  * ユーザーの判断なくクラッシュレポートを送信します
* `ignoreSystemCrashHandler` Boolean, デフォルト: `false`.
* `extra` Object
  * あなたが定義できるオブジェクトは、レポートと一緒に送信されます。
  * 文字列プロパティのみが正しく送信されます。
  * オブジェクトのネストはサポートしていません。

他の`crashReporter`APIを使用する前にこのメソッドをコールする必要があります。

**Note:** macOSでは、Electronは、WindowsとLinux上の`breakpad` とは異なる、新しい`crashpad`クライアントを使用します。クラッシュ収集機能を有効にするために、メインプロセスや、クラッシュレポートを収集したいそれぞれのレンダラープロセスで、`crashpad`を初期化するために`crashReporter.start`APIをコールする必要があります。

### `crashReporter.getLastCrashReport()`

日付と最後のクラッシュレポートのIDを返します。もしなんのクラッシュレポートも送信されていないか、クラッシュレポーターが起動していない場合、`null`を返します。

### `crashReporter.getUploadedReports()`

滑ってのアップロードされたクラッシュレポートが返されます。それぞれのレポートには日付とアップロードされたIDが含まれます。

## crash-reporter Payload

クラッシュレポーターは`POST`で`submitURL` に次のデーターが送信されます。

* `ver` String - Electronのバージョン
* `platform` String - 例： 'win32'.
* `process_type` String - 例： 'renderer'.
* `guid` String - 例： '5e1286fc-da97-479e-918b-6bfb0c3d1c72'
* `_version` String - `package.json`でのバージョン
* `_productName` String - `crashReporter`でのプロダクト名 `オプション`
  object.
* `prod` String - 基盤となる製品の名前。この場合は、Electronです。
* `_companyName` String - `crashReporter`での会社名 `オプション`
  object.
* `upload_file_minidump` File - ファイル形式のクラッシュレポート
* `crashReporter`での`extra`オブジェクトのすべてのレベル1のプロパティ
  `オプション` object
