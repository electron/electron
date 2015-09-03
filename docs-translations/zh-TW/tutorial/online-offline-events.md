# 在線/離線事件偵測

我們可以在渲染引擎 (renderer) 的行程裡用標準的 HTML5 API 來實作在線與離線事件的偵測。  
請參考以下範例:

_main.js_

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');
var onlineStatusWindow;

app.on('ready', function() {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadUrl('file://' + __dirname + '/online-status.html');
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

您也許有時候也會有想要在主行程裡回應這些事件的情況。然而，在主行程裡並沒有 `navigator` 這個物件，因此不能直接地偵測這些事件。
但我們可以使用 Electron 所提供的跨行程 (inter-process) 溝通的工具，事件就可以被傳送到主程序內並做您所需的處理。  
請參考以下範例:

_main.js_

```javascript
var app = require('app');
var ipc = require('ipc');
var BrowserWindow = require('browser-window');
var onlineStatusWindow;

app.on('ready', function() {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadUrl('file://' + __dirname + '/online-status.html');
});

ipc.on('online-status-changed', function(event, status) {
  console.log(status);
});
```

_online-status.html_

```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var ipc = require('ipc');
      var updateOnlineStatus = function() {
        ipc.send('online-status-changed', navigator.onLine ? 'online' : 'offline');
      };

      window.addEventListener('online',  updateOnlineStatus);
      window.addEventListener('offline',  updateOnlineStatus);

      updateOnlineStatus();
    </script>
  </body>
</html>
```
