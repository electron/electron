# Online/Offline Event Detection

Os eventos de detecção Online e Offline podem ser implementados no processo
de renderização utilizando a API padrão do HTML, como é mostrado no exemplo
a seguir:

_main.js_

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');
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

Pode haver casos onde você também deseja responder a estes eventos no processo principal.
Mas o processo principal não consegue detectar esses eventos diretamente, pois não possui
um objeto `navigator`. Utilizando a ferramentas para comunicação entre processos, os eventos
podem ser direcionados para o processo principal e manipulados quando necessário. Você
pode ver isto no exemplo abaixo:

_main.js_

```javascript
var app = require('app');
var ipc = require('ipc');
var BrowserWindow = require('browser-window');
var onlineStatusWindow;

app.on('ready', function() {
  onlineStatusWindow = new BrowserWindow({ width: 0, height: 0, show: false });
  onlineStatusWindow.loadURL('file://' + __dirname + '/online-status.html');
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
