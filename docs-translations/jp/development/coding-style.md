# コーディング規約

Electron 開発におけるコーディング規約です。

`npm run lint` を実行することで `cpplint` 及び `eslint` によって
検出された規約違反を検出することができます。

## C++ と Python

C++ と Python は Chromium の[コーディング規約](http://www.chromium.org/developers/coding-style)を踏襲しています。`script/cpplint.py` というスクリプトを実行することで、全てのファイルがこの規約に従っているかどうかチェックできます。

私達が今使っている Python のバージョンは Python 2.7 です。

C++ のコードは Chromium の抽象型をたくさん使っているので、それに精通していることをオススメします。Chromium の [Important Abstractions and Data Structures](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures) のドキュメントから始めるのがよいでしょう。ドキュメントでは、いくつかの特別な型や、スコープのある型(スコープ外に出ると自動でメモリが開放される型)、ログの仕組みなどが言及されています。

## JavaScript

* [standard](http://npm.im/standard) JavaScript style で書いてください。
* ファイルは改行で**終わらないでください**。Google の規約と合わないためです。
* ファイル名はアンダーバー `_` で結合せず、ハイフン `-` で結合してください。
  例えば `file_name.js` ではなく `file-name.js` です。なぜかというと
  [github/atom](https://github.com/github/atom) モジュールの名前が
  `module-name` 形式だからです。このルールは `.js` ファイルのみ適用されます。
* 必要に応じて新しい ES6/ES2015 の構文を使用してください。
  * [`const`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/const)
    を定数使用時に
  * [`let`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/let)
    を変数宣言時に
  * [Arrow functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/Arrow_functions)
    を `function () { }` の代わりに
  * [Template literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals)
    を `+` を使って文字結合する代わりに

## 名前付け

Electron の API は Node.js と同じ大文字小文字のルールを使用しています。

- モジュールがクラスの場合は、`BrowserWindow` のようにキャメルケースを使います。
- モジュールが API の一部なら `globalShortcut` のように小文字大文字を混ぜて使います(`mixedCase`)。
- `win.webContents` のように API が オブジェクトのプロパティで、かつ分けると複雑になる場合は、小文字大文字を混ぜて使います(`mixedCase`)。
- その他のモジュールでないAPIでは、`<webview> Tag` や `Process Object` のように通常通り名づけます。

新しい API を作る際は、jQuery のように一つの関数で getter/setter にするより、getter と setter を分ける方が望ましいです。
例えば `.text([text])` ではなく `.getText()` と `.setText(text)` にします。この辺りは [issue 46](https://github.com/electron/electron/issues/46) で議論されました。
