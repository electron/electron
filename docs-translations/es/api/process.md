# process

El objeto `process` en Electron tiene las siguientes diferencias con respecto
al node convencional:

* `process.type` String - El tipo del proceso puede ser `browser` (ej. proceso
   principal) o `renderer`.
* `process.versions.electron` String - Versión de Electron.
* `process.versions.chrome` String - Versión de Chromium.
* `process.resourcesPath` String - Ruta al código fuente JavaScript.

## Events

### Event: 'loaded'

Se emite cuando Electron ha cargado su script de inicialización interna y
está comenzando a cargar la página web o el script principal.

Puede ser usado por el script precargado para añadir de nuevo los símbolos globales
de Node eliminados, al alcance global cuando la integración de Node está apagada:

```js
// preload.js
var _setImmediate = setImmediate;
var _clearImmediate = clearImmediate;
process.once('loaded', function() {
  global.setImmediate = _setImmediate;
  global.clearImmediate = _clearImmediate;
});
```

## Methods

El objeto `process` tiene los siguientes métodos:

### `process.hang`

Interrumpe el hilo principal del proceso actual.


### process.setFdLimit(maxDescriptors) _OS X_ _Linux_

* `maxDescriptors` Integer

Establece el límite dinámico del descriptor del archivo en `maxDescriptors`
o en el límite estricto del Sistema Operativo, el que sea menor para el
proceso actual.
