# 概要

> どうやってNode.jsとElectronのAPIを使うか。

Electron では全ての [Node.js のビルトインモジュール](http://nodejs.org/api/) 利用可能です。また、サードパーティの Node モジュール ([ネイティブモジュール](../tutorial/using-native-node-modules.md)も含む) も完全にサポートされています。

Electron はネイティブのデスクトップアプリケーション開発のための幾つかの追加のビルトインモジュールも提供しています。メインプロセスでだけ使えるモジュールもあれば、レンダラプロセス(ウェブページ)でだけ使えるモジュール、あるいはメインプロセス、レンダラプロセスどちらでも使えるモジュールもあります。

基本的なルールは：[GUI][gui]、または低レベルのシステムに関連するモジュールはメインモジュールでだけ利用できるべきです。これらのモジュールを使用できるようにするためには [メインプロセス対レンダラプロセス](../tutorial/quick-start.md#メインプロセス)スクリプトの概念を理解する必要があります。

メインプロセススクリプトは普通の Node.js スクリプトのようなものです：

```javascript
const {app, BrowserWindow} = require('electron');

let win = null;

app.on('ready', () => {
  win = new BrowserWindow({width: 800, height: 600});
  win.loadURL('https://github.com');
});
```

レンダラプロセスは Node モジュールを使うための追加機能を除いて、通常のウェブページとなんら違いはありません：

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {app} = require('electron').remote;
  console.log(app.getVersion());
</script>
</body>
</html>
```

アプリケーションを実行については、[アプリを実行する](../tutorial/quick-start.md#アプリを実行する)を参照してください。

## 分割代入

0.37の時点で、 、[分割代入][desctructuring-assignment]でビルトインモジュールの使用をより簡単にできます：

```javascript
const {app, BrowserWindow} = require('electron');
```

もし`electron`モジュール全体が必要であれば、requireして、それぞれのモジュールを`electron`からアクセスすることができます。

```javascript
const electron = require('electron');
const {app, BrowserWindow} = electron;
```

これは、次のコードと同じ意味を持ちます。

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;
```

[gui]: https://en.wikipedia.org/wiki/Graphical_user_interface
[desctructuring-assignment]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
[issue-387]: https://github.com/electron/electron/issues/387
