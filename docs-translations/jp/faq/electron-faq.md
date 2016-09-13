# Electron FAQ

## Electronは、いつ最新のChromeにアップグレードされますか?

ElectronのChromeバージョンは、通常、新しいChromeのstabeleバージョンがリリースされた後、1～2週間以内に上げられます。ただし、この期間というのは保障されてはおらず、またバージョンアップでの作業量に左右されます。

また、Chromeのstableチャンネルのみを使用し、もし、重要な修正がbetaまたはdevチャンネルにある場合、それをバックポートします。

もっと知りたければ、[セキュリティについて](../tutorial/security.md)をご参照ください。

## Electronは、いつ最新のNode.jsにアップグレードされますか?

Node.js の新しいバージョンがリリースされたとき、私たちは Electron の Node.js を更新するのを通常約1か月待ちます。そのようにして、とても頻繁に発生している、新しい Node.js バージョンによって取り込まれたバグによる影響を避けることができます。

通常、Node.js の新しい機能は V8 のアップグレードによってもたらされますが、Electron は Chrome ブラウザーに搭載されている V8 を使用しているので、新しい Node.js に入ったばかりのピカピカに新しい JavaScript 機能は Electron ではたいてい既に導入されています。

## ウェブページ間のデータを共有する方法は?

ウェブページ（レンダラープロセス）間のデータを共有するために最も単純な方法は、ブラウザで、すでに提供されているHTML5 APIを使用することです。もっとも良い方法は、[Storage API][storage]、[`localStorage`][local-storage]、[`sessionStorage`][session-storage]、[IndexedDB][indexed-db]です。

```javascript
// In the main process.
global.sharedObject = {
  someProperty: 'default value'
};
```

```javascript
// In page 1.
require('remote').getGlobal('sharedObject').someProperty = 'new value';
```

```javascript
// In page 2.
console.log(require('remote').getGlobal('sharedObject').someProperty);
```

## 何分か経つと、アプリの Window/tray が消えてしまいます

これは、Window/trayを格納するのに使用している変数がガベージコレクトされたときに発生します。

この問題に遭遇した時には、次のドキュメントを読むことをお勧めします。

* [Memory Management][memory-management]
* [Variable Scope][variable-scope]

もし簡単に修正したい場合は、コードを以下のように修正して変数をグローバルにすると良いでしょう：

変更前：

```javascript
app.on('ready', function() {
  var tray = new Tray('/path/to/icon.png');
})
```

変更後：

```javascript
var tray = null;
app.on('ready', function() {
  tray = new Tray('/path/to/icon.png');
})
```

## ElectronでjQuery/RequireJS/Meteor/AngularJSを使用できません

Electronに組み込まれているNode.jsの影響で, `module`, `exports`, `require`のようなシンボルがDOMに追加されています。このため、いくつかのライブラリでは同名のシンボルを追加しようとして問題が発生することがあります。

これを解決するために、Electronに組み込まれているnodeを無効にすることができます。

```javascript
// In the main process.
var win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
});
```

しかし、Node.jsとElectron APIを使用した機能を維持したい場合は、ほかのライブラリを読み込む前に、ページのシンボルをリネームする必要があります。

```html
<head>
<script>
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## `require('electron').xxx` は定義されていません。

Electronの組み込みモジュールを使うとに、次のようなエラーに遭遇するかもしれません。

```
> require('electron').webFrame.setZoomFactor(1.0);
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

これは、ローカルまたはグローバルのどちらかで [npm `electron` module][electron-module] をインストールしたことが原因で、Electronの組み込みモジュールを上書きしてしまいます。

正しい組み込みモジュールを使用しているかを確認するために、`electron`モジュールのパスを出力します。

```javascript
console.log(require.resolve('electron'));
```

そして、次の形式かどうかを確認します。

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

もし、`node_modules/electron/index.js`　のような形式の場合は、npm `electron` モジュールを削除するか、それをリネームします。

```bash
npm uninstall electron
npm uninstall -g electron
```

しかし、組み込みモジュールを使用しているのに、まだこのエラーが出る場合、不適切なプロセスでモジュールを使用しようとしている可能性が高いです。例えば、`electron.app`はメインプロセスのみで使え、一方で`electron.webFrame`はレンダラープロセスのみに提供されています。

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API
