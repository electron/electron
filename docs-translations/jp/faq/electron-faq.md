# Electron FAQ

## Electronは、いつ最新のChromeにアップグレードされますか?

ElectronのChromeバージョンは、通常、新しいChromeのstabeleバージョンがリリースされた後、1～2週間以内に上げられます。

また、Chromeのstableチャンネルのみを使用し、もし、重要な修正がbetaまたはdevチャンネルにある場合、それをバックポートします。

## Electronは、いつ最新のNode.jsにアップグレードされますか?

Node.js の新しいバージョンがリリースされたとき、私たちは Electron の Node.js を更新するのを通常約1か月待ちます。そのようにして、とても頻繁に発生している、新しい Node.js バージョンによって取り込まれたバグによる影響を避けることができます。

通常、Node.js の新しい機能は V8 のアップグレードによってもたらされますが、Electron は Chrome ブラウザーに搭載されている V8 を使用しているので、新しい Node.js に入ったばかりのピカピカに新しい JavaScript 機能は Electron ではたいてい既に導入されています。

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
var mainWindow = new BrowserWindow({
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

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
