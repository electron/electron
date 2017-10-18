# Online/Offline Event Detection

Online and offline event detection can be implemented in the renderer process
using standard HTML5 NavigatorOnLine API.
The navigator.onLine attribute returns `false` if the user agent will not contact the network when the user follows links or when a script requests a remote page (or knows that such an attempt would fail) i.e. definitely offline (disconnected from the network) and must return `true` otherwise.
Since all other conditions return `true`, one has to be mindful of getting false positives, as we cannot assume `true` value necessarily means that Electron can access the internet. Such as in cases where the computer is running a virtualization software that has virtual ethernet adapters that are always “connected.”
Therefore, if you really want to determine the internet access status of Electron,
you should develop additional means for checking.

Example:

_main.js_

```javascript
const {app, BrowserWindow} = require('electron')

let onlineStatusWindow

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`)
})
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const alertOnlineStatus = () => {
    window.alert(navigator.onLine ? 'online' : 'offline')
  }

  window.addEventListener('online',  alertOnlineStatus)
  window.addEventListener('offline',  alertOnlineStatus)

  alertOnlineStatus()
</script>
</body>
</html>
```

There may be instances where you want to respond to these events in the
main process as well. The main process however does not have a
`navigator` object and thus cannot detect these events directly. Using
Electron's inter-process communication utilities, the events can be forwarded
to the main process and handled as needed, as shown in the following example.

_main.js_

```javascript
const {app, BrowserWindow, ipcMain} = require('electron')
let onlineStatusWindow

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`)
})

ipcMain.on('online-status-changed', (event, status) => {
  console.log(status)
})
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {ipcRenderer} = require('electron')
  const updateOnlineStatus = () => {
    ipcRenderer.send('online-status-changed', navigator.onLine ? 'online' : 'offline')
  }

  window.addEventListener('online',  updateOnlineStatus)
  window.addEventListener('offline',  updateOnlineStatus)

  updateOnlineStatus()
</script>
</body>
</html>
```
