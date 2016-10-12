# DevTools扩展

为了使调试更容易，Electron 原生支持 [Chrome DevTools Extension][devtools-extension]。

对于大多数DevTools的扩展，你可以直接下载源码，然后通过 `BrowserWindow.addDevToolsExtension` API 加载它们。Electron会记住已经加载了哪些扩展，所以你不需要每次创建一个新window时都调用 `BrowserWindow.addDevToolsExtension` API。

** 注：React DevTools目前不能直接工作，详情留意 [https://github.com/electron/electron/issues/915](https://github.com/electron/electron/issues/915) **

例如，要用[React DevTools Extension](https://github.com/facebook/react-devtools)，你得先下载他的源码：

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

参考 [`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md) 来编译这个扩展源码。

然后你就可以在任意页面的 DevTools 里加载 React DevTools 了，通过控制台输入如下命令加载扩展：

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome')
```

要卸载扩展，可以调用 `BrowserWindow.removeDevToolsExtension` API (扩展名作为参数传入)，该扩展在下次打开DevTools时就不会加载了：

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools')
```

## DevTools 扩展的格式

理论上，Electron 可以加载所有为 chrome 浏览器编写的 DevTools 扩展，但它们必须存放在文件夹里。那些以 `crx` 形式发布的扩展是不能被加载的，除非你把它们解压到一个文件夹里。

## 后台运行(background pages)

Electron 目前并不支持 chrome 扩展里的后台运行(background pages)功能，所以那些依赖此特性的 DevTools 扩展在 Electron 里可能无法正常工作。

## `chrome.*` APIs

有些 chrome 扩展使用了 `chrome.*`APIs，而且这些扩展在 Electron 中需要额外实现一些代码才能使用，所以并不是所有的这类扩展都已经在 Electron 中实现完毕了。

考虑到并非所有的 `chrome.*`APIs 都实现完毕，如果 DevTools 正在使用除了 `chrome.devtools.*` 之外的其它 APIs，这个扩展很可能无法正常工作。你可以通过报告这个扩展的异常信息，这样做方便我们对该扩展的支持。

[devtools-extension]: https://developer.chrome.com/extensions/devtools
