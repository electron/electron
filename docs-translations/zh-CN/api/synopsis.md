# 简介

所有的[Node.js's built-in modules][1]在Electron中都可用，并且所有的node的第三方组件也可以放心使用（包括[自身的模块][2]）。

Electron也提供了一些额外的内置组件来开发传统桌面应用。一些组件只可以在主进程中使用，一些只可以在渲染进程中使用，但是也有部分可以在这2种进程中都可使用。

基本规则：GUI模块或者系统底层的模块只可以在主进程中使用。要使用这些模块，你应当很熟悉[主进程vs渲染进程][3]脚本的概念。

主进程脚本看起来像个普通的nodejs脚本

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadURL('https://github.com');
});
```

渲染进程和传统的web界面一样，除了它具有使用node模块的能力：

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const remote = require('electron').remote;
  console.log(remote.app.getVersion());
</script>
</body>
</html>
```

如果想运行应用，参考 `Run your app` 。

## 解构任务

如果你使用的是CoffeeScript或Babel，你可以使用[destructuring assignment][4]来让使用内置模块更简单:

```javascript
const {app, BrowserWindow} = require('electron');
```

然而如果你使用的是普通的JavaScript，你就需要等到Chrome支持ES6了。

##使用内置模块时禁用旧样式

在版本v0.35.0之前，所有的内置模块都需要按造 `require('module-name')` 形式来使用，虽然它有很多[弊端][5]，我们仍然在老的应用中友好的支持它。

为了完整的禁用旧样式，你可以设置环境变量 `ELECTRON_HIDE_INTERNAL_MODULES ` :

```javascript
process.env.ELECTRON_HIDE_INTERNAL_MODULES = 'true'
```

或者调用 `hideInternalModules` API:

```javascript
require('electron').hideInternalModules()
```


 [1]:http://nodejs.org/api/
 [2]:https://github.com/heyunjiang/electron/blob/master/docs/tutorial/using-native-node-modules.md
 [3]:https://github.com/heyunjiang/electron/blob/master/docs/tutorial/quick-start.md#the-main-process
 [4]:https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
 [5]:https://github.com/electron/electron/issues/387