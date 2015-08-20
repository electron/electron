# Extensión DevTools

Para facilitar la depuración, Electron provee un soporte básico para la extensión 
[Chrome DevTools Extension][devtools-extension].

Para la mayoría de las extensiones devtools, simplemente puedes descargar el código fuente
y utilizar `BrowserWindow.addDevToolsExtension` para cargarlas, las extensiones cargadas
serán recordadas para que no sea necesario llamar a la función cada vez que creas una ventana.

Por ejemplo, para usar la extensión [React DevTools Extension](https://github.com/facebook/react-devtools), primero debes descargar el código fuente:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

Luego cargas la aplicación en Electron, abriendo devtools en cualquier ventana,
y ejecutando este código en la consola devtools:

```javascript
require('remote').require('browser-window').addDevToolsExtension('/some-directory/react-devtools');
```

Para remover una extensión, puedes utilizar `BrowserWindow.removeDevToolsExtension`
especificando el nombre, y esta ya no se cargará la siguiente vez que abras devtools:

```javascript
require('remote').require('browser-window').removeDevToolsExtension('React Developer Tools');
```

## Formato de las extensiones devtools

Idealmente todas las extensiones devtools escritas para Chrome pueden ser cargadas por Electron,
pero para ello deben estar en un directorio plano, las extensiones empaquetadas como `crx`
no pueden ser cargadas por Chrome a no ser que halles una forma de extraerlas a un directorio.

## Páginas en segundo plano (background)

Electron no soporta la característica de páginas en segundo plano de las extensiones de Chrome,
las extensiones que utilizan esta característica podrían no funcionar.

## APIs `chrome.*`

Algunas extensiones utilizan las APIs `chrome.*`, hemos realizado un esfuerzo
para implementar esas APIs en Electron, sin embargo no han sido implementadas en su totalidad.

Dado que no todas las funciones `chrome.*` han sido implementadas, si la extensión devtools está utilizando otras APIs más allá de `chrome.devtools.*`, es muy probable que no funcione. Puedes reportar fallos en el issue tracker para que podamos agregar soporte a esas APIs.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
