# Utilizando el plugin Pepper Flash

El plugin Pepper Flash es soportado ahora. Para utilizar pepper flash en Electron, debes especificar la ubicación del plugin manualmente y activarlo en tu aplicación.

## Preparar una copia del plugin Flash

En OSX y Linux, el detalle del plugin puede encontrarse accediendo a `chrome://plugins` en el navegador. Su ubicación y versión son útiles para el soporte. También puedes copiarlo a otro lugar.

## Agrega la opción a Electron

Puedes agregar la opción `--ppapi-flash-path` y `ppapi-flash-version`  o utilizar el método `app.commandLine.appendSwitch` antes del evento ready de la aplicación.
También puedes agregar la opción `plugins` de `browser-window`. Por ejemplo,

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the javascript object is GCed.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// Specify flash path.
// On Windows, it might be /path/to/pepflashplayer.dll
// On macOS, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.loadURL('file://' + __dirname + '/index.html');
  // Something else
});
```

## Activar el plugin flash en una etiqueta `<webview>`
Agrega el atributo `plugins`.
```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
