# Offscreen Rendering

Offscreen rendering lets you obtain the content of a browser window in a bitmap,
so it can be rendered anywhere, for example on a texture in a 3D scene. The
offscreen rendering in Electron uses a similar approach than the [Chromium
Embedded Framework](https://bitbucket.org/chromiumembedded/cef) project.

Two modes of rendering can be used and only the dirty area is passed in the
`'paint'` event to be more efficient. The rendering can be stopped, continued
and the frame rate can be set. The specified frame rate is a top limit value,
when there is nothing happening on a webpage, no frames are generated. The
maximum frame rate is 60, because above that there is no benefit, just
performance loss.

## Two modes of rendering

### GPU accelerated

GPU accelerated rendering means that the GPU is used for composition. Because of
that the frame has to be copied from the GPU which requires more performance,
thus this mode is quite a bit slower than the other one. The benefit of this
mode that WebGL and 3D CSS animations are supported.

### Software output device

This mode uses a software output device for rendering in the CPU, so the frame
generation is much faster, thus this mode is preferred over the GPU accelerated
one.

To enable this mode GPU acceleration has to be disabled by calling the
[`app.disableHardwareAcceleration()`][disablehardwareacceleration] API.

## Usage

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
