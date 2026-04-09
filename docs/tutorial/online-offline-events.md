# Online/Offline Event Detection

## Overview

Online and offline event detection can be implemented in both the main and renderer processes:

- **Renderer process**: Use the [`navigator.onLine`](http://html5index.org/Offline%20-%20NavigatorOnLine.html) attribute and [online/offline events](https://developer.mozilla.org/en-US/docs/Online_and_offline_events), part of standard HTML5 API.
- **Main process**: Use the [`net.isOnline()`](../api/net.md#netisonline) method or the [`net.online`](../api/net.md#netonline-readonly) property.

The `navigator.onLine` attribute returns:

- `false` if all network requests are guaranteed to fail (e.g. when disconnected from the network).
- `true` in all other cases.

Since many cases return `true`, you should treat with care situations of
getting false positives, as we cannot always assume that `true` value means
that Electron can access the Internet. For example, in cases when the computer
is running a virtualization software that has virtual Ethernet adapters in "always
connected" state. Therefore, if you want to determine the Internet access
status of Electron, you should develop additional means for this check.

## Main Process Detection

In the main process, you can use the `net` module to detect online/offline status:

```js
const { net } = require('electron')

// Method 1: Using net.isOnline()
const isOnline = net.isOnline()
console.log('Online status:', isOnline)

// Method 2: Using net.online property
console.log('Online status:', net.online)
```

Both `net.isOnline()` and `net.online` return the same boolean value with the same reliability characteristics as `navigator.onLine` - they provide a strong indicator when offline (`false`), but a `true` value doesn't guarantee successful internet connectivity.

> [!NOTE]
> The `net` module is only available after the app emits the `ready` event.

## Renderer Process Example

Starting with an HTML file `index.html`, this example will demonstrate how the `navigator.onLine` API can be used to build a connection status indicator.

```html title="index.html"
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
    <meta http-equiv="Content-Security-Policy" content="script-src 'self' 'unsafe-inline';" />
</head>
<body>
    <h1>Connection status: <strong id='status'></strong></h1>
    <script src="renderer.js"></script>
</body>
</html>
```

In order to mutate the DOM, create a `renderer.js` file that adds event listeners to the `'online'` and `'offline'` `window` events. The event handler sets the content of the `<strong id='status'>` element depending on the result of `navigator.onLine`.

```js title='renderer.js'
const updateOnlineStatus = () => {
  document.getElementById('status').innerHTML = navigator.onLine ? 'online' : 'offline'
}

window.addEventListener('online', updateOnlineStatus)
window.addEventListener('offline', updateOnlineStatus)

updateOnlineStatus()
```

Finally, create a `main.js` file for main process that creates the window.

```js title='main.js'
const { app, BrowserWindow } = require('electron')

const createWindow = () => {
  const onlineStatusWindow = new BrowserWindow()

  onlineStatusWindow.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
```

After launching the Electron application, you should see the notification:

![Connection status](../images/connection-status.png)

> [!NOTE]
> If you need to check the connection status in the main process, you can use [`net.isOnline()`](../api/net.md#netisonline) directly instead of communicating from the renderer process via [IPC](../api/ipc-renderer.md).
