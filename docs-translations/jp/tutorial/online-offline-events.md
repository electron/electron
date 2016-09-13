# オンライン/オフライン イベントの検知

オンラインとオフラインイベントの検知は、以下の例で示すように、標準のHTML 5 APIを使用してレンダラプロセスに実装することができます。

_main.js_

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

let onlineStatusWindow;

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`);
});
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const alertOnlineStatus = () => {
    window.alert(navigator.onLine ? 'online' : 'offline');
  };

  window.addEventListener('online',  alertOnlineStatus);
  window.addEventListener('offline',  alertOnlineStatus);

  alertOnlineStatus();
</script>
</body>
</html>
```

メインプロセスでこれらのイベントに応答したいことがあるかもしれません。しかし、メインプロセスは `navigator` オブジェクトを持たないため、直接これらのイベントを検知することができません。Electronの inter-process communication ユーティリティを使用して、オンライン・オフラインイベントをメインプロセスに転送し、必要に応じて扱うことができます。次の例を見てみましょう。

_main.js_

```javascript
const electron = require('electron');
const app = electron.app;
const ipcMain = electron.ipcMain;
const BrowserWindow = electron.BrowserWindow;

let onlineStatusWindow;

app.on('ready', () => {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadURL(`file://${__dirname}/online-status.html`);
});

ipcMain.on('online-status-changed', (event, status) => {
  console.log(status);
});
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {ipcRenderer} = require('electron');
  const updateOnlineStatus = () => {
    ipcRenderer.send('online-status-changed', navigator.onLine ? 'online' : 'offline');
  };

  window.addEventListener('online',  updateOnlineStatus);
  window.addEventListener('offline',  updateOnlineStatus);

  updateOnlineStatus();
</script>
</body>
</html>
```
