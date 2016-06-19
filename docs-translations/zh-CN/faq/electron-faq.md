# Electron 常见问题

## Electron 会在什么时候升级到最新版本的 Chrome？

通常来说，在稳定版的 Chrome 发布后两周内，我们会更新 Electron 内的 Chrome 版本。

我们只会使用 stable 版本的 Chrome。但如果在 beta 或 dev 版本中有一个重要的更新，我们会把补丁应用到现版本的 Chrome 上。

## Electron 会在什么时候升级到最新版本的 Node.js？

我们通常会在最新版的 Node.js 发布后一个月左右将 Electron 更新到这个版本的 Node.js。我们通过这种方式来避免新版本的 Node.js
带来的 bug（这种 bug 太常见了）。

Node.js 的新特性通常是由新版本的 V8 带来的。由于 Electron 使用的是 Chrome 浏览器中附带的 V8 引擎，所以 Electron 内往往已经
有了部分新版本 Node.js 才有的崭新特性。

## 如何在两个网页间共享数据？

在两个网页（渲染进程）间共享数据最简单的方法是使用浏览器中已经实现的 HTML5 API，比较好的方案是用 [Storage API][storage]，
[`localStorage`][local-storage]，[`sessionStorage`][session-storage] 或者 [IndexedDB][indexed-db]。

你还可以用 Electron 内的 IPC 机制实现。将数据存在主进程的某个全局变量中，然后在多个渲染进程中使用 `remote` 模块来访问它。

```javascript
// 在主进程中
global.sharedObject = {
  someProperty: 'default value'
};
```

```javascript
// 在第一个页面中
require('remote').getGlobal('sharedObject').someProperty = 'new value';
```

```javascript
// 在第二个页面中
console.log(require('remote').getGlobal('sharedObject').someProperty);
```

## 为什么应用的窗口、托盘在一段时间后不见了？

这通常是因为用来存放窗口、托盘的变量被垃圾收集了。

你可以参考以下两篇文章来了解为什么会遇到这个问题。

* [内存管理][memory-management]
* [变量作用域][variable-scope]

如果你只是要一个快速的修复方案，你可以用下面的方式改变变量的作用域，防止这个变量被垃圾收集。

从

```javascript
app.on('ready', function() {
  var tray = new Tray('/path/to/icon.png');
})
```

改为

```javascript
var tray = null;
app.on('ready', function() {
  tray = new Tray('/path/to/icon.png');
})
```

## 在 Electron 中，我为什么不能用 jQuery、RequireJS、Meteor、AngularJS？

因为 Electron 在运行环境中引入了 Node.js，所以在 DOM 中有一些额外的变量，比如 `module`、`exports` 和 `require`。这导致
了许多库不能正常运行，因为它们也需要将同名的变量加入运行环境中。

我们可以通过禁用 Node.js 来解决这个问题，用如下的方式：

```javascript
// 在主进程中
var mainWindow = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
});
```

假如你依然需要使用 Node.js 和 Electron 提供的 API，你需要在引入那些库之前将这些变量重命名，比如：

```html
<head>
<script>
// 重命名 Electron 提供的 require
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## 为什么 `require('electron').xxx` 的结果是 undefined？

在使用 Electron 的提供的模块时，你可能会遇到和以下类似的错误：

```
> require('electron').webFrame.setZoomFactor(1.0);
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

这是因为你在项目中或者在全局中安装了[npm 上获取的 `electron` 模块][electron-module]，它把 Electron 的内置模块覆写了。

你可以通过以下方式输出 `electron` 模块的路径来确认你是否使用了正确的模块。

```javascript
console.log(require.resolve('electron'));
```

确认一下它是不是像下面这样的：

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

假如输出的路径类似于 `node_modules/electron/index.js`，那么你需要移除或者重命名 npm 上的 `electron` 模块。

```bash
npm uninstall electron
npm uninstall -g electron
```

如果你依然遇到了这个问题，你可能需要检查一下拼写或者是否在错误的进程中调用了这个模块。比如，
`require('electron').app` 只能在主进程中使用, 然而 `require('electron').webFrame` 只能在渲染进程中使用。

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API
