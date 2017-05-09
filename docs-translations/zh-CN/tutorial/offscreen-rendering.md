# 离屏渲染

离线渲染允许您在位图中获取浏览器窗口的内容，因此可以在任何地方渲染，例如在3D场景中的纹理。Electron中的离屏渲染使用与 [Chromium
Embedded Framework](https://bitbucket.org/chromiumembedded/cef) 项目类似的方法。

可以使用两种渲染模式，并且只有脏区通过 `'paint'` 事件才能更高效。渲染可以停止、继续，并且可以设置帧速率。 指定的帧速率是上限值，当网页上没有发生任何事件时，不会生成任何帧。 最大帧速率是60，因为再高没有好处，而且损失性能。

**注意:** 屏幕窗口始终创建为 [Frameless Window](../api/frameless-window.md).

## 两种渲染模式

### GPU加速

GPU加速渲染意味着使用GPU用于合成。因为帧必须从需要更多性能的GPU中复制，因此这种模式比另一个模式慢得多。这种模式的优点是支持WebGL和3D CSS动画。

### 软件输出设备

此模式使用软件输出设备在CPU中渲染，因此帧生成速度更快，因此此模式优先于GPU加速模式。

要启用此模式，必须通过调用 [`app.disableHardwareAcceleration()`][disablehardwareacceleration] API 来禁用GPU加速。

## 使用

``` javascript
const {app, BrowserWindow} = require('electron')

app.disableHardwareAcceleration()

let win
app.once('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      offscreen: true
    }
  })
  win.loadURL('http://github.com')
  win.webContents.on('paint', (event, dirty, image) => {
    // updateBitmap(dirty, image.getBitmap())
  })
  win.webContents.setFrameRate(30)
})
```

[disablehardwareacceleration]: ../api/app.md#appdisablehardwareacceleration
