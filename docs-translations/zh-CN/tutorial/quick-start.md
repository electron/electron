# 快速入门

Electron 可以让你使用纯 JavaScript 调用丰富的原生 APIs 来创造桌面应用。你可以把它看作一个专注于桌面应用的 Node.js 的变体，而不是 Web 服务器。

这不意味着 Electron 是绑定了 GUI 库的 JavaScript。相反，Electron 使用 web 页面作为它的 GUI，所以你能把它看作成一个被 JavaScript 控制的，精简版的 Chromium 浏览器。

## 主进程
在 Electron 里，运行 `package.json` 里 `main` 脚本的进程被称为**主进程**。在主进程运行的脚本可以以创建 web 页面的形式展示 GUI。

## 渲染进程
由于 Electron 使用 Chromium 来展示页面，所以 Chromium 的多进程结构也被充分利用。每个 Electron 的页面都在运行着自己的进程，这样的进程我们称之为**渲染进程**。

在一般浏览器中，网页通常会在沙盒环境下运行，并且不允许访问原生资源。然而，Electron 用户拥有在网页中调用 io.js 的 APIs 的能力，可以与底层操作系统直接交互。

## 主进程与渲染进程的区别
主进程使用 `BrowserWindow` 实例创建页面。每个 `BrowserWindow` 实例都在自己的渲染进程里运行页面。当一个 `BrowserWindow` 实例被销毁后，相应的渲染进程也会被终止。

主进程管理所有页面和与之对应的渲染进程。每个渲染进程都是相互独立的，并且只关心他们自己的页面。

由于在页面里管理原生 GUI 资源是非常危险而且容易造成资源泄露，所以在页面调用 GUI 相关的 APIs 是不被允许的。如果你想在网页里使用 GUI 操作，其对应的渲染进程必须与主进程进行通讯，请求主进程进行相关的 GUI 操作。

在 Electron，我们提供几种方法用于主进程和渲染进程之间的通讯。像 `[ipcRenderer][1]` 和 `[ipcMain][2]` 模块用于发送消息， `[remote][3]` 模块用于 RPC 方式通讯。这些内容都可以在一个 FAQ 中查看 `［how to share data between web pages］[4]`。

# 打造你第一个 Electron 应用
大体上，一个 Electron 应用的目录结构如下：
````
your-app/
├── package.json
├── main.js
└── index.html
````
`package.json `的格式和 Node 的完全一致，并且那个被 `main` 字段声明的脚本文件是你的应用的启动脚本，它运行在主进程上。你应用里的 `package.json` 看起来应该像：
```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```
**注意**：如果 `main` 字段没有在 `package.json` 声明，Electron会优先加载 `index.js`。

`main.js` 应该用于创建窗口和处理系统事件，一个典型的例子如下：
```javascript
const electron = require('electron');
// 控制应用生命周期的模块。
const {app} = electron;
// 创建原生浏览器窗口的模块。
const {BrowserWindow} = electron;

// 保持一个对于 window 对象的全局引用，如果你不这样做，
// 当 JavaScript 对象被垃圾回收， window 会被自动地关闭
let win;

function createWindow() {
  // 创建浏览器窗口。
  win = new BrowserWindow({width: 800, height: 600});

  // 加载应用的 index.html。
  win.loadURL(`file://${__dirname}/index.html`);

  // 启用开发工具。
  win.webContents.openDevTools();

  // 当 window 被关闭，这个事件会被触发。
  win.on('closed', () => {
    // 取消引用 window 对象，如果你的应用支持多窗口的话，
    // 通常会把多个 window 对象存放在一个数组里面，
    // 与此同时，你应该删除相应的元素。
    win = null;
  });
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow();
  }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
```
最后，你想展示的 `index.html` ：
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

# 运行你的应用
一旦你创建了最初的 `main.js`， `index.html` 和 `package.json` 这几个文件，你可能会想尝试在本地运行并测试，看看是不是和期望的那样正常运行。

## electron-prebuilt
`[electron-prebuilt][5]` 是一个 `npm` 模块，包含所使用的 Electron 预编译版本。 
如果你已经用 `npm` 全局安装了它，你只需要按照如下方式直接运行你的应用：
```bash
electron .
```
如果你是局部安装，那运行：
```bash
./node_modules/.bin/electron .
```

## 手工下载 Electron 二进制文件
如果你手工下载了 Electron 的二进制文件，你也可以直接使用其中的二进制文件直接运行你的应用。
### Windows
```bash
$ .\electron\electron.exe your-app\
```
### Linux
```bash
$ ./electron/electron your-app/
```
### OS X
```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```
`Electron.app` 里面是 Electron 发布包，你可以在 `[这里][6]` 下载到。

# 以发行版本运行
在你完成了你的应用后，你可以按照 `[应用部署][7]` 指导发布一个版本，并且以已经打包好的形式运行应用。

# 参照下面例子
复制并且运行这个库 `[atom/electron-quick-start][8]`。

*注意：*运行时需要你的系统已经安装了 `[Git][9]` 和 `[Node.js][10]`（包含 `[npm][11]`）。

```bash
# Clone the repository
$ git clone https://github.com/electron/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies and run the app
$ npm install && npm start
```
  [1]: https://github.com/electron/electron/blob/v1.1.3/docs/api/ipc-renderer.md
  [2]: https://github.com/electron/electron/blob/v1.1.3/docs/api/ipc-main.md
  [3]: https://github.com/electron/electron/blob/v1.1.3/docs/api/remote.md
  [4]: https://github.com/electron/electron/blob/v1.1.3/docs/faq/electron-faq.md#how-to-share-data-between-web-pages
  [5]: https://github.com/electron-userland/electron-prebuilt
  [6]: https://github.com/electron/electron/releases
  [7]: https://github.com/electron/electron/blob/v1.1.3/docs/tutorial/application-distribution.md
  [8]: https://github.com/electron/electron-quick-start
  [9]: https://git-scm.com/
  [10]: https://nodejs.org/en/download/
  [11]: https://www.npmjs.com/