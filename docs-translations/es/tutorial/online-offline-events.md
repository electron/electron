# Detección del evento en línea/fuera de línea

La detección de estos eventos puede ser implementada en el proceso renderer utilizando las APIs HTML5 estándar,
como en este ejemplo:

_main.js_

```javascript
var app = require('app')
var BrowserWindow = require('browser-window')
var onlineStatusWindow

app.on('ready', function () {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html')
})
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

Existen casos en donde necesitas responder a estos eventos desde el proceso principal.
El proceso principal no posee un objeto `navigator`, por lo tanto no puede detectar estos eventos directamente.
Es posible reenviar el evento al proceso principal mediante la utilidad de intercomunicación entre procesos (ipc):

_main.js_

```javascript
var app = require('app')
var ipc = require('ipc')
var BrowserWindow = require('browser-window')
var onlineStatusWindow

app.on('ready', function () {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false })
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html')
})

ipc.on('online-status-changed', function (event, status) {
  console.log(status)
})
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
