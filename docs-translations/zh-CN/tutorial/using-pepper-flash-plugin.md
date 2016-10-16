# 使用 Pepper Flash 插件

Electron 现在支持 Pepper Flash 插件。要在 Electron 里面使用 Pepper Flash 插件，你需
要手动设置 Pepper Flash 的路径和在你的应用里启用 Pepper Flash。

## 保留一份 Flash 插件的副本

在 macOS 和 Linux 上，你可以在 Chrome 浏览器的 `chrome://plugins` 页面上找到 Pepper
Flash 的插件信息。插件的路径和版本会对 Election 对其的支持有帮助。你也可以把插件
复制到另一个路径以保留一份副本。

## 添加插件在 Electron 里的开关

你可以直接在命令行中用 `--ppapi-flash-path` 和 `ppapi-flash-version` 或者
在 app 的准备事件前调用 `app.commandLine.appendSwitch` 这个 method。同时，
添加 `browser-window` 的插件开关。
例如：

```javascript
// Specify flash path. 设置 flash 路径
// On Windows, it might be /path/to/pepflashplayer.dll
// On macOS, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so')

// Specify flash version, for example, v17.0.0.169 设置版本号
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169')

app.on('ready', function () {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  })
  mainWindow.loadURL(`file://${__dirname}/index.html`)
  // Something else
})
```

## 使用 `<webview>` 标签启用插件

在 `<webview>` 标签里添加 `plugins` 属性。

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
