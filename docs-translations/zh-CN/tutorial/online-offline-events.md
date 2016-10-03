# 在线/离线事件探测
使用标准 HTML5 APIs 可以实现在线和离线事件的探测，就像以下例子：

*main.js*
```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var onlineStatusWindow
app.on('ready', function () {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html')
})
```

*online-status.html*
```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var alertOnlineStatus = function() {
        window.alert(navigator.onLine ? 'online' : 'offline');
      };

      window.addEventListener('online',  alertOnlineStatus);
      window.addEventListener('offline',  alertOnlineStatus);

      alertOnlineStatus();
    </script>
  </body>
</html>
```

也会有人想要在主进程也有回应这些事件的实例。然后主进程没有 `navigator` 对象因此不能直接探测在线还是离线。使用 Electron 的进程间通讯工具，事件就可以在主进程被使用，就像下面的例子：

*main.js*
```javascript
const electron = require('electron')
const app = electron.app
const ipcMain = electron.ipcMain
const BrowserWindow = electron.BrowserWindow

var onlineStatusWindow
app.on('ready', function () {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html')
})

ipcMain.on('online-status-changed', function (event, status) {
  console.log(status)
})
```

*online-status.html*
```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      const ipcRenderer = require('electron').ipcRenderer;
      var updateOnlineStatus = function() {
        ipcRenderer.send('online-status-changed', navigator.onLine ? 'online' : 'offline');
      };

      window.addEventListener('online',  updateOnlineStatus);
      window.addEventListener('offline',  updateOnlineStatus);

      updateOnlineStatus();
    </script>
  </body>
</html>
```
