# クイックスタート

Electron ではリッチなネイティブ API を持ったランタイムを提供することによってピュアな JavaScript でデスクトップアプリケーションをつくることができます。ウェブサーバーの代わりにデスクトップアプリケーションに焦点をあてた Node.js ランタイムであるといえばわかりやすいかもしれません。

Electron は JavaScript を GUI ライブラリにバインディングしません。その代わりに、Electron はウェブページを GUI として使用します。なので Electron は JavaScript によってコントロールされる最小のChromium ブラウザでもあるともいえます。

### メインプロセス

Electron では、`package.json` の `main` スクリプトで実行されるプロセスを __メインプロセス__ と呼びます。メインプロセスで実行されるスクリプトがウェブページを作ることによって GUI を表示することができます。

### レンダラープロセス

Electron はウェブページを表示させるために Chromium を使用しているので、Chromium のマルチプロセスアーキテクチャも使用されることになります。Electron で実行されるウェブページはそれぞれ自身のプロセスで実行されます。それを __レンダラープロセス__ と呼びます。

通常のブラウザでは、ウェブページはサンドボックス環境で実行されネイティブなリソースへのアクセスができません。Electron ではウェブページから Node.js の API を使えるためオペレーティングシステムと低レベルなやりとりが可能です。

### メインプロセスとレンダラープロセスの違い

メインプロセスは `BrowserWindow` インスタンスを作ることによってウェブページをつくります。それぞれの `BrowserWindow` インスタンスはそれ自身のレンダラープロセス上でウェブページを実行します。`BrowserWindow` インスタンスが破棄されると、対応するレンダラープロセスも終了されます。

メインプロセスはすべてのウェブページとそれに対応するレンダラープロセスを管理しています。それぞれのレンダラープロセスは隔離されているので、自身の中で実行されているウェブページの面倒だけをみます。

ウェブページでは、GUI 関連の API を呼ぶことはできません。なぜならば、ウェブページからネイティブ GUI リソースを扱うことは非常に危険であり、簡単にリソースをリークしてしまうからです。もしウェブページ内でGUI を操作したい場合には、ウェブページのレンダラープロセスはメインプロセスにそれらの操作をするように伝える必要があります。

Electron では、メインプロセスとレンダラープロセスとのコミュニケーションをするために [ipc](../api/ipc-renderer.md) モジュールを提供しています。またそれと、RPC 形式の通信を行う [remote](../api/remote.md) モジュールもあります。

Electron では、メインプロセスとレンダラープロセスとのコミュニケーションをするには幾つかのほうほうがあります。メッセージを送信する[`ipcRenderer`](../api/ipc-renderer.md)モジュールと[`ipcMain`](../api/ipc-main.md)モジュールのように、RPC 形式の通信を行う[remote](../api/remote.md)モジュールです。[ウェブページ間のデータを共有する方法][share-data]にFAQエントリーがあります。

## Electronアプリを作成する

一般的に Electron アプリの構成は次のようになります：

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json` の形式は Node モジュールとまったく同じです。 `main` フィールドで指定するスクリプトはアプリの起動スクリプトであり、メインプロセスを実行します。 `package.json` の例は次のようになります：

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__注記__： `package.json` に `main` が存在しない場合、Electron は `index.js` のロードを試みます。

`main.js` ではウィンドウを作成してシステムイベントを管理します。典型的な例は次のようになります：

```javascript
'use strict';

const electron = require('electron');
const app = electron.app;  // Module to control application life.
const BrowserWindow = electron.BrowserWindow;  // Module to create native browser window.

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
app.on('ready', function() {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // and load the index.html of the app.
  mainWindow.loadURL('file://' + __dirname + '/index.html');

  // Open the DevTools.
  mainWindow.webContents.openDevTools();

  // Emitted when the window is closed.
  mainWindow.on('closed', function() {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });
});
```

最後に表示するウェブページ `index.html` は次のようになります：

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using node <script>document.write(process.versions.node)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    and Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## アプリを実行する

最初の `main.js`、`index.html`、`package.json` を作ったら、手元でアプリを実行し、思った通りに動くのを確認したいでしょう。

### electron-prebuilt

`electron-prebuilt` を `npm` でグローバルインストールしているなら、アプリのソースディレクトリ内で以下を実行するだけで済みます：

```bash
electron .
```

ローカルにインストールしているなら以下を実行してください：

```bash
./node_modules/.bin/electron .
```

### 手動ダウンロードした Electron バイナリを使う場合

もしも Electron を手動ダウンロードしているなら、同梱されているバイナリであなたのアプリを直接実行できます。

#### Windows

```bash
$ .\electron\electron.exe your-app\
```

#### Linux

```bash
$ ./electron/electron your-app/
```

#### OS X

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` は Electron のリリースパッケージの一部で、[ここ](https://github.com/electron/electron/releases) からダウンロードできます。

### Run as a distribution

アプリケーションを作り終えたら、[Application distribution](./application-distribution.md) ガイドにしたがってディストリビューションを作成し、そしてパッケージされたアプリケーションとして実行することが可能です。

### 試してみよう

このチュートリアルのコードは [`atom/electron-quick-start`](https://github.com/electron/electron-quick-start) リポジトリから clone して実行できます。

**注記**：例を試すには、[Git](https://git-scm.com) と [Node.js](https://nodejs.org/en/download/) ([npm](https://npmjs.org) もこれに含まれています) が必要です。

```bash
# Clone the repository
$ git clone https://github.com/electron/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies and run the app
$ npm install && npm start
```

[share-data]: ../faq/electron-faq.md#how-to-share-data-between-web-pages
