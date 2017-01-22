# 简介

> 如何使用 Node.js 和 Electron APIs。

所有的[Node.js's built-in modules](https://nodejs.org/api/)在 Electron 中都可用，并且所有的 node 的第三方组件也可以放心使用（包括[自身的模块](../tutorial/using-native-node-modules.md)）。

Electron 也提供了一些额外的内置组件来开发传统桌面应用。一些组件只可以在主进程中使用，一些只可以在渲染进程中使用（ web 页面），但是也有部分可以在这2种进程中都可使用。

基本规则：GUI模块或者系统底层的模块只可以在主进程中使用。要使用这些模块，你应当很熟悉[主进程vs渲染进程](../tutorial/quick-start.md#main-process)脚本的概念。

主进程脚本看起来像个普通的 Node.js 脚本：

```javascript
const {app, BrowserWindow} = require('electron')
let win = null

app.on('ready', () => {
  win = new BrowserWindow({width: 800, height: 600})
  win.loadURL('https://github.com')
})
```

渲染进程和传统的 web 界面一样，除了它具有使用 node 模块的能力：

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {app} = require('electron').remote
  console.log(app.getVersion())
</script>
</body>
</html>
```

如果想运行应用，参考 [Run your app](../tutorial/quick-start.md#run-your-app)。

## 解构任务

在 v0.37 版本之后，你可以使用[destructuring assignment][destructuring-assignment] 来更加简单地使用内置模块。

```javascript
const {app, BrowserWindow} = require('electron')

let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

如果你需要整个 `electron` 模块，你可以加载并使用
从 `electron` 解构访问各个模块。

```javascript
const electron = require('electron')
const {app, BrowserWindow} = electron

let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

这相当于以下代码：

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow
let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

[gui]: https://en.wikipedia.org/wiki/Graphical_user_interface
[destructuring-assignment]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
