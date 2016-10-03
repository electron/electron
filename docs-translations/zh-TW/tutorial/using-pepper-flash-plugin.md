# 使用 Pepper Flash 外掛

Electron 現在支援 Pepper Flash 外掛，要在 Electron 中使用 Pepper Flash 外掛，你應該手動指定 Pepper Flash 外掛的位置，並在你的應用程式中啟用它。

## 準備一份 Flash 外掛

在 macOS 和 Linux 上，Pepper Flash 外掛的細節可以透過查看 Chrome 瀏覽器中的 `chrome://plugins` 來得知，它的位置和版本對於 Electron 的 Pepper Flash 支援都有很大的幫助，你可以把它複製一份到別的位置。

## 加入 Electron 開關

你可以直接加入 `--ppapi-flash-path` 和 `ppapi-flash-version` 到
Electron 命定列或是在應用程式的 ready 事件之前使用 `app.commandLine.appendSwitch` 方法，並且加入 `browser-window` 的 `plugins` 開關。

例如：

```javascript
// 指定 Flash 路徑
// Windows 中可能是 /path/to/pepflashplayer.dll
// macOS 中 /path/to/PepperFlashPlayer.plugin
// Linux 中 /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so')

// 指定 Flash 版本, 例如 v17.0.0.169
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

## 在一個 `<webview>` Tag 中啟用 Flash 外掛

把 `plugins` 屬性加入 `<webview>` tag。

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
