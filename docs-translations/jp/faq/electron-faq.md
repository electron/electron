# Electron FAQ

## Electronは、いつ最新のChromeにアップグレードされますか?

ElectronのChromeバージョンは、通常、新しいChromeのstabeleバージョンがリリースされた後、1～2週間以内に上げられます。

また、Chromeのstableチャンネルのみを使用し、もし、重要な修正がbetaまたはdevチャンネルにある場合、それをバックポートします。

## Electronは、いつ最新のNode.jsにアップグレードされますか?

Node.jsの新しいバージョンがリリースされたとき、とても頻繁に発生している新しいNode.jsバージョンで取り込まれたバグによる影響を避けるために、ElectronのNode.jsをアップグレードする前に、通常、約1カ月待ちます。

通常、Node.jsの新しい機能はV8のアップグレードによってもたらされ、 ElectronはChromeブラウザーに搭載されているV8を使用しているので、
新しいNode.jsの重要な新しいJavaScript機能はElectronでは、すでに導入されています。

## 数分後、アプリのWindow/trayが表示されなくなります

これは、Window/trayを格納するのに使用している変数がガベージ コレクションされたときに発生します。

この問題に遭遇した時には、次のドキュメントを読むことをお勧めします。

* [Memory Management][memory-management]
* [Variable Scope][variable-scope]

もし簡単に修正したい場合は、グローバル変数を作成し、コードを変更します。

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

Electronに組み込まれているNode.jsの影響で, `module`, `exports`, `require`のようなシンボルがDOMに追加されています。いくつかのライブラリで、追加しようとしているシンボルと同じ名前があり、これが原因で問題が発生します。
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
