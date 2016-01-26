# remote

 `remote`モジュールは、レンダラープロセス（ウェブページ）とメインプロセス間でインタープロセスコミュニケーション（IPC）をする簡単な方法を提供します。

Electronでは、GUI関連モジュール（`dialog`や`menu`など）はメインプロセスのみに提供されており、レンダラープロセスには提供されていません。レンダラープロセスからそれらを使うために、`ipc`モジュールはメインプロセスにインタープロセスメッセージを送信するのに必要です。`remote`モジュールで、Javaの[RMI][rmi]と同じように、はっきりとインタープロセスメッセージを送信しなくてもメインプロセスオブジェクトのメソッドを呼び出せます。

レンダラープロセスからブラウザーウィンドウを作成する例：

```javascript
const remote = require('electron').remote;
const BrowserWindow = remote.BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL('https://github.com');
```

**Note:** 逆には（メインプロセスからレンダラープロセスにアクセスする）、[webContents.executeJavascript](web-contents.md#webcontentsexecutejavascriptcode-usergesture)が使えます。

## Remote オブジェクト

`remote`モジュールから返されるそれぞれのオブジェクト（関数含む）はメインプロセスでオブジェクトを示します（リモートオブジェクトまたはリモート関数と呼ばれます）。リモートプロジェクトのメソッドを実行したり、リモート関数をコールしたり、リモートコンストラクター（関数）で新しいオブジェクトを生成したりしたとき、実際に非同期にインタープロセスメッセージが送信されます。

上の例では、`BrowserWindow` と `win` はリモートオブジェクトで、`new BrowserWindow`はレンダラープロセスで `BrowserWindow`を作成しません。代わりに、メインプロセスで`BrowserWindow` オブジェクトが作成され、レンダラープロセスで対応するリモートオブジェクトを返し、すなわち`win`オブジェクトです。

リモート経由でのみアクセスできる [enumerable properties](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Enumerability_and_ownership_of_properties)に注意してください。

## Remote オブジェクトのライフタイム

Electronは、レンダラープロセスのリモートオブジェクトが生きている限り（言い換えれば、ガベージコレクションされません）、対応するメインプロセスのオブジェクトは解放されないことを確認してください。リモートオブジェクトがガベージコレクションされたとき、対応するメインプロセスのオブジェクトが間接参照されます。

レンダラープロセスでリモートオブジェクトがリークした場合（マップに格納されているが解放されない）、メインプロセスで対応するオブジェクトもリークするので、リモートオブジェクトがリークしないように細心の注意を払うべきです。

文字列や数字のようなプライマリ値は、コピーして送信します。

## メインプロセスにコールバックを通す

メインプロセスのコードは、レンダラーからコールバックを受け取ることができます。例えば、`remote`モジュールです。この機能をとても慎重に使うべきです。

最初に、デッドロックを避けるために、メインプロセスに渡されたコールバックは非同期に呼び出されます。メインプロセスは、渡されたコールバックの戻り値を取得することを期待すべきではありません。

例えば、メインプロセスでコールされた`Array.map`で、レンダラープロセスから関数を使用することはできません。:

```javascript
// main process mapNumbers.js
exports.withRendererCallback = function(mapper) {
  return [1,2,3].map(mapper);
}

exports.withLocalCallback = function() {
  return exports.mapNumbers(function(x) {
    return x + 1;
  });
}
```

```javascript
// renderer process
var mapNumbers = require("remote").require("./mapNumbers");

var withRendererCb = mapNumbers.withRendererCallback(function(x) {
  return x + 1;
})

var withLocalCb = mapNumbers.withLocalCallback()

console.log(withRendererCb, withLocalCb) // [true, true, true], [2, 3, 4]
```

見ることができるように、レンダラーコールバックの同期戻り値は予想されなかったとして、メインプロセスで生きている同一のコールバックの戻り値と一致しません。

第二に、メインプロセスに渡されるコールバックはメインプロセスがガベージコレクトするまで存続します。

例えば、次のコードは一目で無害なコードのように思えます。リモートオブジェクト上で`close`イベントのコールバックをインストールしています。

```javascript
remote.getCurrentWindow().on('close', function() {
  // blabla...
});
```

りかし、明確にアンインストールするまでメインプロセスによってコールバックは参照されることを覚えておいてください。アンインストールしない場合、ウィンドウをリロードするたびに、コールバックは再度インストールされ、それぞれの再起動時にコールバックあリークします。

さらに悪いことに、前にインストールされたコールバックのコンテキストは解放されるので、`close`イベントを出力されると、メインプロセスで例外が発生します。

この問題を避けるために、メインプロセスに渡されたレンダラーコールバックへの参照をクリーンアップを確認します。これにはイベントハンドラーのクリンアップも含まれ、存在しているレンダラープロセスから来るコールバックを確実にメインプロセスが守るように確認します。

## メインプロセスで組み込みモジュールにアクセスする

メインプロセスの組み込みモジュールは、`remote`モジュールでゲッターとして追加されるので、`electron`モジュールのように直接それらを使用できます。

```javascript
const app = remote.app;
```

## メソッド

`remote`モジュールは次のメソッドを持ちます。

### `remote.require(module)`

* `module` String

メインプロセスで、`require(module)`で返されるオブジェクトを返します。

### `remote.getCurrentWindow()`

このウェブページに属する[`BrowserWindow`](browser-window.md) オブジェクトを返します。

### `remote.getCurrentWebContents()`

このウェブページの[`WebContents`](web-contents.md) オブジェクトを返します。

### `remote.getGlobal(name)`

* `name` String

メインプロセスで、`name`のグローバル変数（例えば、`global[name]`）を返します。


### `remote.process`

メインプロセスで`process`オブジェクトを返します。これは`remote.getGlobal('process')`と同様ですが、キャッシュされます。

[rmi]: http://en.wikipedia.org/wiki/Java_remote_method_invocation
