# process

> processオブジェクトの拡張。

Electronの`process`オブジェクトは次のようなAPIで拡張されています。

## イベント

### イベント: 'loaded'

このイベントはElectronが内部の初期化スクリプトをロードし、ウェブページまたはメインスクリプトのロードが始まるときに発行されます。

Node統合がオフになっているとき、削除したNodeグローバルシンボルをグローバルスコープへ戻すために、プリロードスクリプトで使用できます。

```js
// preload.js
var _setImmediate = setImmediate
var _clearImmediate = clearImmediate
process.once('loaded', function () {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## プロパティ

### `process.noAsar`

これを`true`に設定すると、Nodeのビルトインモジュールで、`asar`アーカイブのサポートを無効にできます。

### `process.type` 

現在のプロセスのタイプで、`"browser"`(例: メインプロセス) または `"renderer"`の値をとります。

### `process.versions.electron` 

Electronのバージョン文字列です。

### `process.versions.chrome`

Chromeのバージョン文字列です。

### `process.resourcesPath`

リソースディレクトリのパスです。

### `process.mas`

Mac app Store用のビルドでは値は`true`になります。ほかのビルドの場合は`undefined`です。

### `process.windowsStore`

Windows Store App (appx)として動作中の場合は、値は`true`になります。それ以外では`undefined`です。

### `process.defaultApp`

アプリがdefault appのパラメータとして渡されて起動された場合は、値は`true`になります。

訳注: [issue #4690](https://github.com/electron/electron/issues/4690)が参考になります。


## メソッド

`process`オブジェクトは次のメソッドを持ちます。


### `process.crash()`

このプロセスのメインスレッドをクラッシュさせます。

### `process.hang()`

このプロセスのメインスレッドをハングさせます。

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

ファイルデスクリプタの最大数のソフトリミットを、`maxDescriptors`かOSのハードリミットの、どちらか低い方に設定します。

### `process.getProcessMemoryInfo()`

本プロセスのメモリ使用統計に関する情報を得ることのできるオブジェクトを返します。全ての報告値はキロバイト単位であることに注意してください。

* `workingSetSize` - 実際の物理メモリに固定されているメモリサイズです。
* `peakWorkingSetSize` - これまでに実際の物理メモリに固定された最大のメモリサイズです。
* `privateBytes` - JS heapやHTML contentなど、他のプロセスに共有されていないメモリサイズです。
* `sharedBytes` - プロセス間で共有されているメモリサイズです。例えばElectronそのものにより使用されたメモリがこれに当たります。

### `process.getSystemMemoryInfo()`

システム全体で使用されているメモリ使用統計に関する情報を得ることのできるオブジェクトを返します。全ての報告値はキロバイト単位であることに注意してください。

* `total` - システムで使用可能な全ての物理メモリのサイズ(KB)です。
* `free` - アプリケーションやディスクキャッシュで使用されていないメモリのサイズです。

Windows / Linux用:

* `swapTotal` - システムで使用可能なスワップメモリの総サイズです。
* `swapFree` - システムで使用可能なスワップメモリの空きサイズです。
