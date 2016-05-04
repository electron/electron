# process

Electronの`process`オブジェクトは上流nodeの1つから次のような違いがあります。

* `process.type` String - プロセスの種類で、`browser` (例　メインプロセス)または `renderer`を設定できます。
* `process.versions.electron` String - Electronのバージョン
* `process.versions.chrome` String - Chromiumのバージョン
* `process.resourcesPath` String - JavaScriptのソースコードのパスを設定します。
* `process.mas` Boolean - Mac app Store用のビルドで、値は`true`です。ほかのビルドの場合は`undefined`です。

## イベント

### イベント: 'loaded'

Electronは内部の初期化スクリプトをロードしたとき出力され、ウェブページまたはメインスクリプトのロードが始まります。

Node統合がオフになっているとき、削除したNodeグローバルシンボルをグローバルスコープへ戻してプリロードスクリプトで使用できます。

```js
// preload.js
var _setImmediate = setImmediate;
var _clearImmediate = clearImmediate;
process.once('loaded', function() {
  global.setImmediate = _setImmediate;
  global.clearImmediate = _clearImmediate;
});
```

## プロパティ

### `process.noAsar`

これを`true`に設定すると、Nodeのビルトインモジュールで、`asar`アーカイブのサポートを無効にできます。

## メソッド

`process`オブジェクトは次のめっそどを持ちます。

### `process.hang()`

現在のプロセスがハングしているメインスレッドが原因で発生します。

### `process.setFdLimit(maxDescriptors)` _OS X_ _Linux_

* `maxDescriptors` Integer

現在のプロセスで、`maxDescriptors`またはOSハード制限のどちらか低いほうで、ソフトファイルディスクリプターを設定します。
