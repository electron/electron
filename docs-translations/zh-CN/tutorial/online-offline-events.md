# 在线/离线事件探测

使用标准 HTML5 APIs 可以实现在线和离线事件的探测，就像以下例子：

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

也会有人想要在主进程也有回应这些事件的实例。然后主进程没有 `navigator` 对象因此不能直接探测在线还是离线。使用 Electron 的进程间通讯工具，事件就可以在主进程被使用，就像下面的例子：

*main.js*

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

*online-status.html*

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

**注意：** 如果 Electron 无法连接到局域网（LAN）或
一个路由器，它被认为是离线的; 所有其他条件返回 `true`。
所以，虽然你可以假设 Electron 是离线的，当 `navigator.onLine`
返回一个 `false' 值，你不能假设一个 'true' 值必然
意味着 Electron 可以访问互联网。你可能会获得虚假
连接，例如在计算机运行虚拟化软件的情况下，
具有始终“连接”的虚拟以太网适配器。
因此，如果您真的想确定 Electron 的互联网访问状态，
你应该开发额外的检查手段。
