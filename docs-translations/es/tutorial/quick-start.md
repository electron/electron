## Introducción

Electron permite la creación de aplicaciones de escritorio utilizando JavaScript puro, a través de un runtime con APIs nativas. Puedes verlo como una variante de io.js, enfocado a aplicaciones de escritorio, en vez de servidores web.

Esto no significa que Electron sea un binding de librerías GUI para JavaScript.
Electron utiliza páginas web como su GUI, por lo cual puedes verlo como un navegador Chromium mínimo,
controlado por JavaScript.

### El proceso principal (main process)

En Electron, el proceso que ejecuta el script `main` del `package.json` se llama __el proceso principal__.
El script que corre en el proceso principal puede crear páginas para mostrar la GUI.

### El proceso renderer (renderer process)

Dado que Electron utiliza Chromium para mostrar las páginas web,
también es utilizada la arquitectura multiproceso de Chromium.
Cada página web en Electron se ejecuta en su propio proceso,
el cual es llamado __el proceso renderer__.

En los navegadores normales, las páginas web usualmente se ejecutan en un entorno
sandbox y no tienen acceso a los recursos nativos. Los usuarios de Electron tienen el poder
de utilizar las APIs de io.js en las páginas web, permitiendo interacciones de bajo nivel con el sistema operativo.

### Diferencias entre el proceso principal y el proceso renderer

El proceso principal crea páginas web mediante instancias de `BrowserWindow`. Cada instancia de `BrowserWindow`  ejecuta su propia página web y su propio proceso renderer.
Cuando una instancia de `BrowserWindow` es destruida, también su proceso renderer correspondiente acaba.

El proceso principal gestiona las páginas web y sus correspondientes procesos renderer.
Cada proceso renderer es aislado y sólo considera relevante la página web que corre en él.

En las páginas web, no está permitido llamar a APIs relacionadas a la GUI nativa
porque la gestión de los recursos GUI nativos es peligrosa, y tiende a que ocurran leaks de memoria.
Si deseas realizar operaciones GUI en una página web, el proceso renderer de la página web debe comunicarse
con el proceso principal, y solicitar a este que realice esas operaciones.

En Electron, hemos proveído el módulo [ipc](../api/ipc-renderer.md) para la comunicación
entre el proceso principal y el proceso renderer. Y también hay un módulo [remote](../api/remote.md)
para comunicación al estilo RPC.

## Escribe tu primera aplicación Electron

Generalmente, una aplicación Electron tendrá la siguiente estructura:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

El formato de `package.json` es exactamente el mismo que cualquier módulo Node,
y el script especificado en el campo `main` será el script de arranque de tu aplicación,
a ser ejecutado por el proceso principal. Un ejemplo de `package.json` podría verse así:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

El `main.js` debería crear las ventanas y gestionar los eventos del sistema, un ejemplo típico sería:

```javascript
var app = require('app');  // Módulo para controlar el ciclo de vida de la aplicación.
var BrowserWindow = require('browser-window');  // Módulo para crear uan ventana de navegador.

// Mantener una referencia global al objeto window, si no lo haces, esta ventana
// se cerrará automáticamente cuando el objeto JavaScript sea recolectado (garbage collected):
var mainWindow = null;

// Salir de todas las ventanas cuando se cierren.
app.on('window-all-closed', function() {
  // En macOS es común que las aplicaciones y su barra de menú
  // se mantengan activas hasta que el usuario cierre la aplicación
  // explícitamente utilizando Cmd + Q
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// Este método será llamado cuando Electron haya finalizado la inicialización
// y esté listo para crear ventanas de navegador.
app.on('ready', function() {
  // Crear la ventana.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // cargar el index.html de nuestra aplicación.
  mainWindow.loadURL('file://' + __dirname + '/index.html');

  // Desplegar devtools.
  mainWindow.openDevTools();

  // Evento emitido cuando se cierra la ventana.
  mainWindow.on('closed', function() {
    // Eliminar la referencia del objeto window.
    // En el caso de soportar multiples ventanas, es usual almacenar
    // los objetos window en un array, este es el momento en el que debes eliminar el elemento correspondiente.
    mainWindow = null;
  });
});
```

Finalmente el `index.html` es la página web que mostraremos:

```html
<!DOCTYPE html>
<html>
  <head>
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using io.js <script>document.write(process.version)</script>
    and Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Ejecutar la aplicación

Cuando termines de escribir tu aplicación, puedes distribuirla
siguiendo la [guía de distribución](./application-distribution-es.md)
y luego ejecutar la aplicación empaquetada. También puedes utilizar el binario de Electron
para ejecutar tu aplicación de forma directa.

En Windows:

```bash
$ .\electron\electron.exe your-app\
```

En Linux:

```bash
$ ./electron/electron your-app/
```

En macOS:

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` es parte del paquete de release de Electron, puedes descargarlo [aquí](https://github.com/electron/electron/releases).
