# Opening windows from the renderer

Often, developers will want to control how windows are created from trusted or
untrusted content within a renderer. Windows can be created from the renderer in two ways:

1. clicking on links or submitting forms adorned with `target=_blank`
2. JavaScript calling `window.open()`

By default, this results in the creation of a
[`BrowserWindowProxy`](browser-window-proxy.md), a light wrapper around
`BrowserWindow`.

However, when the `contextIsolation` or `sandbox` (or directly,
`nativeWindowOpen`) options are set, a `Window` instance is created, as you'd
expect in the browser. For same-origin content, the new window is created within
the same process, enabling the parent to affect the DOM directly. This can be
very useful for app sub-windows that act as preference panels, or similar, as
the parent can render to the sub-window directly, as if it were a `div` in the
parent.


### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (optional)
* `features` String (optional) -

Returns [`BrowserWindowProxy`](browser-window-proxy.md) | [`Window`](https://developer.mozilla.org/en-US/docs/Web/API/Window)

A comma-separated key-value list, following the standard format of the browser.
Electron will parse `BrowserWindowConstructorOptions` out of this list where
possible, for convenience. For full control, consider
`webContents.setWindowOpenOverride`. A subset of `WebPreferences` can be set
directly, unnested, from the features string: `zoomFactor`, `nodeIntegration`,
`preload`, `javascript`, `contextIsolation`, and `webviewTag`.

For example:
```js
window.open('https://github.com', '_blank', 'frame=false,nodeIntegration=no')
```

**Notes:**

* Node integration will always be disabled in the opened `window` if it is
  disabled on the parent window.
* Context isolation will always be enabled in the opened `window` if it is
  enabled on the parent window.
* JavaScript will always be disabled in the opened `window` if it is disabled on
  the parent window.
* Non-standard features (that are not handled by Chromium or Electron) given in
  `features` will be passed to any registered `webContent`'s `did-create-window`
  event handler in the `additionalFeatures` argument.

To customize or cancel the creation of the window, you can optionally set an
override handler with `webContents.setWindowOpenOverride()`. Returning `false`
cancels the window, while returning an object sets the
`BrowserWindowConstructorOptions` used when creating the window. Note that this
is more powerful than passing options through the feature string, as the
renderer has more limited privileges in deciding security preferences than the
main process.

### `BrowserWindowProxy` example

```javascript
// main.js
const mainWindow = new BrowserWindow()

mainWindow.webContents.setWindowOpenOverride(({ url }) => {
  if (url.startsWith('https://github.com/')) {
    return true
  }
  return false
})

mainWindow.webContents.on('did-create-window', (window) => {
  // For example...
  window.webContents('will-navigate', (e) => {
    e.preventDefault()
  })
})
```

```javascript
// renderer.js
const windowProxy = window.open('https://github.com/', null, 'minimizable=false')
windowProxy.postMessage('hi', '*')
```

### Native `Window` example

```javascript
// main.js
const mainWindow = new BrowserWindow({
  webPreferences: {
    nativeWindowOpen: true
  }
})

mainWindow.webContents.setWindowOpenOverride(({ url }) => {
  if (url === 'about:blank') {
    return {
      frame: false,
      fullscreenable: false,
      backgroundColor: 'black',
      webPreferences: {
        preload: 'my-preload-script.js'
      }
    }
  }
  return false
})
```

```javascript
// renderer.js
const childWindow = window.open('about:blank')
childWindow.document.write('hello world')
```
