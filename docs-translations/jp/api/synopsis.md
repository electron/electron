# 概要

Electron では全ての [Node.js のビルトインモジュール](http://nodejs.org/api/) 利用可能です。また、サードパーティの Node モジュール ([ネイティブモジュール](../tutorial/using-native-node-modules.md)も含む) も完全にサポートされています。

Electron はネイティブのデスクトップアプリケーション開発のための幾つかの追加のビルトインモジュールも提供しています。メインプロセスでだけ使えるモジュールもあれば、レンダラプロセス(ウェブページ)でだけ使えるモジュール、あるいはメインプロセス、レンダラプロセスどちらでも使えるモジュールもあります。

基本的なルールは：[GUI][gui]、または低レベルのシステムに関連するモジュールはメインモジュールでだけ利用できるべきです。これらのモジュールを使用できるようにするためには [メインプロセス対レンダラプロセス](../tutorial/quick-start.md#メインプロセス)スクリプトの概念を理解する必要があります。

メインプロセススクリプトは普通の Node.js スクリプトのようなものです：

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadURL('https://github.com');
});
```

レンダラプロセスは Node モジュールを使うための追加機能を除いて、通常のウェブページとなんら違いはありません：

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const remote = require('electron').remote;
  console.log(remote.app.getVersion());
</script>
</body>
</html>
```

アプリケーションを実行については、[アプリを実行する](../tutorial/quick-start.md#アプリを実行する)を参照してください。

## 分割代入

CoffeeScript か Babel を使っているなら、[分割代入][desctructuring-assignment]でビルトインモジュールの使用をより簡単にできます：

```javascript
const {app, BrowserWindow} = require('electron')
```

しかし、素の JavaScript を使っている場合、Chrome が ES6 を完全サポートするまで待たなければいけません。

## Disable old styles of using built-in modules

v0.35.0 以前は全てのビルトインモジュールは `require('module-name')` の形式で使われなければいけません。この形式は[多くの欠点][issue-387]がありますが、古いアプリケーションとの互換性のためにまだサポートしています。

古い形式を完全に無効にするために、環境変数 `ELECTRON_HIDE_INTERNAL_MODULES` を設定できます：

```javascript
process.env.ELECTRON_HIDE_INTERNAL_MODULES = 'true'
```

もしくは `hideInternalModules` API を呼んでください：

```javascript
require('electron').hideInternalModules()
```

[gui]: https://en.wikipedia.org/wiki/Graphical_user_interface
[desctructuring-assignment]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
[issue-387]: https://github.com/electron/electron/issues/387
