# 온라인/오프라인 이벤트 감지

온라인/오프라인 이벤트는 다음 예제와 같이 렌더러 프로세스에서 표준 HTML5 API를 이용하여
구현할 수 있습니다.

_main.js_

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var onlineStatusWindow;
app.on('ready', function() {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html');
});
```

_online-status.html_

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

메인 프로세스에서 이 이벤트를 처리할 필요가 있는 경우 이벤트를 메인 프로세스로 보낼 수
있습니다. 메인 프로세스는 `navigator` 객체를 가지고 있지 않기 때문에 이 이벤트를 직접
사용할 수는 없습니다.

대신 다음 예제와 같이 Electron의 inter-process communication(ipc) 유틸리티를
사용하면 이벤트를 메인 프로세스로 전달할 수 있습니다.

_main.js_

```javascript
const electron = require('electron');
const app = electron.app;
const ipcMain = electron.ipcMain;
const BrowserWindow = electron.BrowserWindow;

var onlineStatusWindow;
app.on('ready', function() {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html');
});

ipcMain.on('online-status-changed', function(event, status) {
  console.log(status);
});
```

_online-status.html_

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
