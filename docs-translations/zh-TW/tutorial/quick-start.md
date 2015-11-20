# 快速入門

## 簡介

Electron 可以讓你使用純 JavaScript 提供豐富的原生的 APIs 來創造桌面應用程式。
你可以把它視為一個 Node.js 的變體，專注於桌面應用程式而不是 web 伺服器。

這不表示 Electron 是一個用 JavaScript 去綁定 GUI 函式庫。取而代之的，Electron 是使用網頁介面來作為它的 GUI ，
所以你可以把它看作是一個被 JavaScript 所控制且精簡化的 Chromium 瀏覽器。

## 主行程

在 Electron 裡，有個行程會去運行 `package.json` 裡的 `main` 腳本被稱為 __主行程__ 。
這個腳本運行在主行程裏可以顯示一個 GUI 就由創建一個 web 頁面。

## 渲染行程

因為 Electron 使用 Chromium 來顯示網頁，所以 Chromium 的多行程架構也被充分利用。
每一個網頁在 Electron 裏執行各自的行程，被稱為 __渲染行程__。

在一般瀏覽器中，網頁通常會在沙盒環境下運行，並且不允許存取原生資源。然而，
Electron 的用戶擁有在網頁中呼叫 Node.js APIs 的能力，允許低級別操作與作業系統的交互作用。

## 主行程與渲染行程的區別

主行程創造網頁透過創造 `BroswerWindow` 實例。每一個 `BroswerWindow` 實例都在自己的渲染行程裡運行著一個網頁。
當一個 `BroswerWindow` 實例被銷毀，對應的渲染行程也會被終止。主行程管理所有網頁和與之對應的渲染行程。
每一個渲染行程都是相互獨立的，並且只關心他們自己的網頁。

在網頁中，是不允許呼叫原生 GUI 相關 APIs 因為管理原生 GUI 資源在網頁上是非常危險而且容易造成資源洩露。
如果你想要在網頁中呼叫 GUI 相關的 APIs 的操作，網頁的渲染行程必須與主行程進行通訊，請求主行程進行相關的操作。

在 Electron，我們提供用於在主行程與渲染行程之間通訊的 [ipc](../api/ipc-renderer.md) 模組。並且也有一個遠端模組使用 RPC 通訊方式 [remote](../api/remote.md)。

# 打造你第一個 Electron 應用

一般來說，一個 Electron 應用程式的目錄結構像下面這樣：

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json` 的格式與 Node 的模組完全一樣，並且有個腳本被指定為 `main` 是用來啟動你的應用程式，它運行在主行程上。
你應用裡的 一個範例在你的 `package.json` 看起來可能像這樣：

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__注意__：如果 `main` 沒有在 `package.json` 裏， Electron會嘗試載入 `index.js`。

`main.js` 用於創建視窗和處理系統事件，一個典型的例子如下：

``` javascript
var app = require('app'); // 控制應用程式生命週期的模組。
var BrowserWindow = require('browser-window'); // 創造原生瀏覽器窗口的模組

// 對我們的伺服器傳送異常報告。
require('crash-reporter').start();

// 保持一個對於 window 物件的全域的引用，不然，當 JavaScript 被GC，
// window 會被自動地關閉
var mainWindow = null;

// 當所有窗口被關閉了，退出。
app.on('window-all-closed', function() {
  // 在OS X 上，通常使用者在明確地按下 Cmd + Q 之前
  // 應用會保持活動狀態
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// 當Electron 完成了初始化並且準備創建瀏覽器視窗的時候
// 這個方法就被調用
app.on('ready', function() {
  // 創建瀏覽器視窗
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // 載入應用程式的 index.html
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // 打開開發者工具
  mainWindow.webContents.openDevTools();

  // 當window 被關閉，這個事件會被觸發
  mainWindow.on('closed', function() {
    // 取消引用 window 物件，如果你的應用程式支援多視窗的話，
    // 通常會把多個 window 物件存放在一個數組裡面，
    // 但這次不是。
    mainWindow = null;
  });
});
```

最後，你想顯示的 `index.html` ：

```html
<!DOCTYPE html>
<html>
  <head>
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

# 運行你的應用程式

一旦你創造了最初的 `main.js` ， `index.html` 和 `package.json` 這幾個文件，你可能會想嘗試在本地運行並測試，看看是不是和期望的那樣正常運行。

## electron-prebuild
如果你已經使用 `npm` 安裝了全域的 `electron-prebuilt`，那麼你只需要按照如下方式直接運行你的應用程式：

```bash
electron .
```

如果你是局部性地安裝，那運行：

```bash
./node_modules/.bin/electron .
```

## 手動下載 Electron Binary

如果你手動下載了 Electron ，你可以直接使的 Binary 直接運行你的應用程式。

### Windows

``` bash
$ .\electron\electron.exe your-app\
```

### Linux

``` bash
$ ./electron/electron your-app/
```

### OS X

``` bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` 裡面是 Electron 釋出包，你可以在[這裡](https://github.com/atom/electron/releases)下載到。

# 作為版本發行
在你完成了你的應用程式後，你可以依照 [應用部署](https://github.com/atom/electron/blob/master/docs/tutorial/application-distribution.md) 指南發布一個版本，並且運行已經打包好的應用程式。

# 試試這個範例

Clone 與執行本篇教學的程式碼，它們都放在 [`atom/electron-quick-start`](https://github.com/atom/electron-quick-start) 這個 repository。

**Note**: 執行這個範例需要 [Git](https://git-scm.com) 以及 [Node.js](https://nodejs.org/en/download/) (其中包括 [npm](https://npmjs.org)) 在你的作業系統。

```bash
# Clone the repository
$ git clone https://github.com/atom/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies and run the app
$ npm install && npm start
```