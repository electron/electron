# クイックスタート

## 導入

ElectronではリッチなネイティブAPIを持ったランタイムを提供することによってピュアなJavaScriptでデスクトップアプリケーションをつくることができます。ウェブサーバーの代わりにデスクトップアプリケーションに焦点をあてたio.jsランタイムであるといえばわかりやすいかもしれません。

ElectronはJavaScriptをGUIライブラリにバインディングしません。その代わりに、ElectronはウェブページをGUIとして使用します。なのでElectronはJavaScriptによってコントロールされる最小のChromiumブラウザでもあるともいえます。

### メインプロセス

Electronでは、`package.json` の `main`で実行されるプロセスを __メインプロセス__ と呼びます。メインスクリプトではGUIにウェブページを表示することができるプロセスを実行します。

### レンダラープロセス

Electronはウェブページを表示させるためにChromiumを使用しているので、Chromiumのマルチプロセスアーキテクチャが使用されることになります。Electronで実行されるウェブページはそれぞれ自身のプロセスで実行されます。それを __レンダラープロセス__ と呼びます。

通常、ブラウザのウェブページはサンドボックス環境で実行されネイティブなリソースへのアクセスができません。Electronではウェブページからio.jsのAPIを使って、ネイティブリソースへの権限が与えられます。そのおかげでウェブページの中からJavaScriptを使って低レベルなオペレーティングシステムとのインタラクションが可能になります。

### メインプロセスとレンダラープロセスの違い

メインプロセスは `BrowserWindow` インスタンスを作ることによってウェブページをつくります。それぞれの `BrowserWindow` インスタンスはそれ自身の レンダラープロセス上でウェブページを実行します。`BrowserWindow` インスタンスが破棄されると、対応するレンダラープロセスも終了されます。

メインプロセスはすべてのウェブページとそれに対応するレンダラープロセスを管理しています。それぞれのレンダラープロセスは分離しているのでウェブページで実行されていることだけを気に留めておいてください。

ウェブページでは、GUI関連のAPIを呼ぶことはできません。なぜならば、ウェブページで管理しているネイティブのGUIリソースは非常に危険で簡単にリークしてしまうからです。もしウェブページ内でGUIを操作したい場合には、メインプロセスと通信をする必要があります。

Electronでは、メインプロセスとレンダラープロセスとのコミュニケーションをするために[ipc](../api/ipc-renderer.md)モジュールを提供しています。またそれと、RPC形式の通信を行う[remote](../api/remote.md)モジュールもあります。

## Electronアプリを作成する

一般的に  Electronアプリの構成は次のようになります：

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json`の形式はNodeモジュールとまったく同じです。 `main`　フィールドでアプリを起動するためのスクリプトを特定し、メインプロセスで実行します。 `package.json`の例は次のようになります：

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

`main.js` ではウィンドウを作成してシステムイベントを管理します。典型的な例は次のようになります：

```javascript
var app = require('app');  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.

// Report crashes to our server.
require('crash-reporter').start();

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the javascript object is GCed.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// This method will be called when Electron has done everything
// initialization and ready for creating browser windows.
app.on('ready', function() {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // Open the devtools.
  mainWindow.openDevTools();

  // Emitted when the window is closed.
  mainWindow.on('closed', function() {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });
});
```

最後に表示するウェブページ`index.html`は次のようになります：


```html
<!DOCTYPE html>
<html>
  <head>
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using io.js <script>document.write(process.version)</script>
    and Electron <script>document.write(process.versions['electron'])</script>.
  </body>
</html>
```

## アプリを実行する

アプリケーションを作り終えたら、[Application distribution](./application-distribution.md)ガイドにしたがってディストリビューションを作成します、そしてパッケージされたアプリケーションとして配布することが可能です。またダウンロードしたElectronのバイナリをアプリケーション・ディレクトリを実行するために利用することもできます。

Windowsの場合：

```bash
$ .\electron\electron.exe your-app\
```

Linuxの場合：

```bash
$ ./electron/electron your-app/
```

OS Xの場合：

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` はElectronのリリースパッケージに含まれており、[ここ](https://github.com/atom/electron/releases) からダウンロードできます。
