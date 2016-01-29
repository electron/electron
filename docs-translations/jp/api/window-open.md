# `window.open` 関数

Webページで新しいウィンドウを作成するために`window.open`をコールすると、`url`用の`BrowserWindow`の新しいインスタンスが作成され、プロキシはそれ以上の制御を制限したページを許可する`window.open`を返します。

プロキシは伝統的なウェブページと互換性のある制限された標準的な機能が実装されています。新しいウィンドウの完全な制御のために、直接、`BrowserWindow`を作成する必要があります。

新しく作成された`BrowserWindow`は、既定で、親ウィンドウのオプションを継承し、`features`文字列で設定したオプションを継承しオーバーライドします。

### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (optional)
* `features` String (optional)

新しいウィンドウを作成し、`BrowserWindowProxy` クラスのインスタンを返します。

 `features`文字列は標準的なブラウザーのフォーマットに従いますが、それぞれの機能は`BrowserWindow`のオプションフィールドである必要があります。

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

優先的なオリジンなく特定のオリジンまたは`*`で親ウィンドウにメッセージが送信されます。

## Class: BrowserWindowProxy

`BrowserWindowProxy`オブジェクトは`window.open`から返り、子ウィンドウに制限された機能を提供します。

### `BrowserWindowProxy.blur()`

子ウィンドウからフォーカスを削除します。

### `BrowserWindowProxy.close()`

アンロードイベントをコールせず、強制的に子ウィンドウを閉じます。

### `BrowserWindowProxy.closed`

子ウィンドウがクローズした後、trueを設定します。

### `BrowserWindowProxy.eval(code)`

* `code` String

子ウィンドウでコードを評価します。

### `BrowserWindowProxy.focus()`

前面にウィンドウを出し、子ウィンドウにフォーカスします。

### `BrowserWindowProxy.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

優先的なオリジンなく特定のオリジンまたは`*`で子ウィンドウにメッセージが送信されます。

それらのメッセージに加え、子ウィンドウはプロパティとシグナルメッセージなしで`window.opener`オブジェクトを実装します。
