# Sinopsis

Todos los [Módulos integrados de Node.js](http://nodejs.org/api/) se encuentran
disponibles en Electron y módulos de terceros son támbien totalmente compatibles
(incluyendo los [módulos nativos](../tutorial/using-native-node-modules.md)).

Electron también provee algunos módulos integrados adicionales para desarrollar
aplicaciones nativas de escritorio. Algunos módulos sólo se encuentran disponibles
en el proceso principal, algunos sólo en el proceso renderer (página web), y
algunos pueden ser usados en ambos procesos.

La regla básica es: Si un módulo es
[GUI](https://es.wikipedia.org/wiki/Interfaz_gráfica_de_usuario) o de bajo nivel,
entonces solo estará disponible en el proceso principal. Necesitas familiarizarte
con el concepto de [scripts para proceso principal vs scripts para proceso renderer]
(../tutorial/quick-start.md#the-main-process) para ser capaz de usar esos módulos.

El script del proceso principal es como un script normal de Node.js:

```javascript
var app = require('app')
var BrowserWindow = require('browser-window')

var window = null

app.on('ready', function () {
  window = new BrowserWindow({width: 800, height: 600})
  window.loadURL('https://github.com')
})
```

El proceso renderer no es diferente de una página web normal, excepto por la
capacidad extra de utilizar módulos de node:

```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var remote = require('remote');
      console.log(remote.require('app').getVersion());
    </script>
  </body>
</html>
```

Para ejecutar tu aplicación, lee [Ejecutar la aplicación](../tutorial/quick-start.md#run-your-app).
