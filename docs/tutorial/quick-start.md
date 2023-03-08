# Inicio Rapido

Esta guía te ayudará a través del proceso de creación de una app "Hola Mundo" básica en Electrón, similar a [`electron/electron-quick-start`][quick-start].

Al final de este tutorial, tu app abrirá una ventana de navegador que mostrará una página web con información de las versiones de Chromium, node.js y Electrón que se están ejecutando.

[quick-start]: https://github.com/electron/electron-quick-start

## Prerequisitos

Para usar Electron, debes instalar [Node.js][node-download]. Te recomendamos que uses la última versión `LTS` disponible.

> Por favor instala Node.js usando instaladores precompilados para su plataforma.
> De lo contrario, puedes encontrar problemas de incompatibilidad con diferentes herramientas de desarrollo.

Para comprobar que node.js se instaló correctamente, escribe los siguientes comandos en tu terminal:

```sh
node -v
npm -v
```

Estos deben imprimir las versiones de Node.js y npm respectivamente.

**Note:**  Como Electron incrusta Node.js en su binario, la versión de Node.js en la que se ejecuta tu código no está relacionada con la versión ejecutada en tu sistema.

[node-download]: https://nodejs.org/en/download/

## Crea tu aplicacion

### Bases del proyecto

Las aplicaciones Electron siguen la misma estructura general que otros proyectos en Node.js. Comienza creando una carpeta e iniciando un paquete npm.

```sh npm2yarn
mkdir my-electron-app && cd my-electron-app
npm init
```

El comando interactivo `init` te pedirá que configures algunos campos en su configuración. Hay algunas reglas que seguir para este tutorial:

* `entry point` debe ser `main.js`.
* `author` y `description` pueden ser cualquier valor, pero son necesarios para
[empaquetar aplicaciones](#package-and-distribute-your-application).

Tu archivo `package.json` debería verse algo así:

```json
{
  "name": "my-electron-app",
  "version": "1.0.0",
  "description": "Hello World!",
  "main": "main.js",
  "author": "Jane Doe",
  "license": "MIT"
}
```

Luego, instala el paquete `electron` en las `devDependencies` de tu aplicación.

```sh npm2yarn
npm install --save-dev electron
```

> Nota: Si encuentra algún problema con la instalación de Electron, por favor
> consulte la guía [Instalación avanzada].

Finalmente, querrás ser capaz de ejecutar Electron. En el campo [`scripts`] de tu configuración `package.json`, añade un comando `start` como se muestra a continuación:

```json
{
  "scripts": {
    "start": "electron ."
  }
}
```

Este comando `start` te permitirá abrir tu aplicación en modo desarrollo.

```sh npm2yarn
npm start
```

> Nota: Este script le dice a Electron que se ejecute en la carpeta raíz de su proyecto.
> En esta etapa, tu aplicación arrojará un error diciendo que no puede encontrar una aplicación para ejecutar.

[instalación avanzada]: ./installation.md
[package-scripts]: https://docs.npmjs.com/cli/v7/using-npm/scripts

### Ejecutar el proceso principal

El punto de entrada de cualquier aplicación de Electron es su script `main` Este script controla el
**proceso principal** que se ejecuta en un entorno de Node.js completo y es responsable de controlar el ciclo de vida de tu aplicación, mostrando interfaces nativas, realizando operaciones privilegiadas y gestionando procesos renderizadores (más sobre eso más adelante).

Durante la ejecución, Electron buscará este script en el campo [`main`][package-json-main]
de la configuración `package.json` de la configuración
[Inicializacion del projecto](#scaffold-the-project).

Para inicializar el script `main`, cree un archivo vacío llamado `main.js` en la carpeta raíz de tu proyecto.

> Nota: ¡Si vuelves a ejecutar el script `start` en este momento, tu aplicación
> no debería de arrojar errores! Sin embargo, aún no hará nada porque no hemos añadido ningún código a `main.js`.

[package-json-main]: https://docs.npmjs.com/cli/v7/configuring-npm/package-json#main

### Crear una página web

Antes de poder crear una ventana para nuestra aplicación, necesitamos crear el contenido que se cargará en ella. En Electron, cada ventana muestra contenidos web que pueden cargarse desde un archivo HTML local o desde una URL remota.

Para este tutorial estarás haciendo el primero. Crea un archivo `index.html` en la carpeta raíz de tu proyecto:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using Node.js <span id="node-version"></span>,
    Chromium <span id="chrome-version"></span>,
    and Electron <span id="electron-version"></span>.
  </body>
</html>
```

> Nota: Al observar este documento HTML, puede observar que los números de versión
> están ausentes en el texto del body. Los insertaremos manualmente más tarde usando JavaScript.

### Abriendo tu página web en una ventana del navegador

Ahora que tienes una página web, cárgala en una ventana de la aplicación. Para hacerlo, necesitarás dos módulos de Electron:

* El módulo [`app`][app], que controla el ciclo de vida de eventos de tu aplicación.
* El módulo [`BrowserWindow`][browser-window], que crea y administra las ventanas de la aplicación.

Ya que el proceso principal ejecuta Node.js, puede importarlos como módulos [CommonJS][commonjs]
en la parte superior de su archivo:

```js
const { app, BrowserWindow } = require('electron')
```

A continuación, añade una función `createWindow()` que cargue `index.html` en una nueva instancia de `BrowserWindow`.

```js
const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')
}
```

A continuación, llama a esta función `createWindow()` para abrir tu ventana.

En Electron, las ventanas del navegador sólo se pueden crear después de que se dispare el evento [`ready`][app-ready] del módulo `app`
Puedes esperar a este evento utilizando la API
[`app.whenReady()`][app-when-ready]. Llama a `createWindow()` después de que `whenReady()`
resuelva su Promise.

```js
app.whenReady().then(() => {
  createWindow()
})
```

> Nota: Llegados a este punto, su aplicación Electron debería abrir correctamente
> una ventana que muestre su página web.

[app]: ../api/app.md
[browser-window]: ../api/browser-window.md
[commonjs]: https://nodejs.org/docs/latest/api/modules.html#modules_modules_commonjs_modules
[app-ready]: ../api/app.md#event-ready
[app-when-ready]: ../api/app.md#appwhenready

### Gestione el ciclo de vida de sus ventanas

Aunque ahora se puede abrir una ventana del navegador, necesitará un poco de código boilerplate adicional
para que se sienta más nativo a cada plataforma. Las ventanas de aplicación se comportan de forma diferente en
cada OS, y Electron pone la responsabilidad en los desarrolladores para aplicar estas
convenciones en su aplicación.

En general, puedes utilizar el atributo [`platform`][node-platform] del global `process`
para ejecutar código específico para determinados sistemas operativos.

#### Salir de la aplicación cuando se cierran todas las ventanas (Windows & Linux)

En Windows y Linux, salir de todas las ventanas generalmente cierra una aplicación por completo.

Para implementar esto, escucha el evento [`'window-all-closed'`][window-all-closed] del módulo `app` y llama a [`app.quit()`][app-quit] si el usuario no está en macOS (`darwin`).

```js
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
```

[node-platform]: https://nodejs.org/api/process.html#process_process_platform
[window-all-closed]: ../api/app.md#event-window-all-closed
[app-quit]: ../api/app.md#appquit

#### Abrir una ventana si no hay ninguna abierta (macOS)

Mientras que las aplicaciones de Linux y Windows se cierran cuando no tienen ninguna ventana abierta, las de macOS generalmente
continúan ejecutándose incluso sin ninguna ventana abierta, y activar la aplicación cuando no hay ventanas
cuando no hay ventanas disponibles debería abrir una nueva.

Para implementar esta característica, escucha el evento [`activate`][activate] del módulo `app`
y llama al método `createWindow()` existente si no hay ninguna ventana abierta.

Debido a que las ventanas no se pueden crear antes del evento `ready`, sólo debes escuchar los eventos
`activate` después de que tu aplicación se haya inicializado. Para ello, adjunta tu evento listener
dentro de la llamada de retorno existente `whenReady()`.

[activate]: ../api/app.md#event-activate-macos

```js
app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})
```

> Nota: Llegados a este punto, los controles de las ventanas deberían ser totalmente funcionales.

### Accede a Node.js desde el renderizador con un script de precarga

Ahora, lo último que hay que hacer es imprimir los números de versión de Electron y sus
dependencias en su página web.

Acceder a esta información es trivial en el proceso principal a través del objeto global de Node `process`. Sin embargo, no puedes editar el DOM desde el proceso principal
porque no tiene acceso al contexto `document` del renderizador.
Están en procesos completamente diferentes.

> Nota: Si necesita profundizar más en los procesos Electron, consulte el documento
> [Modelo de proceso][Process Model].

Aquí es donde resulta útil adjuntar un **script de precarga** al renderizador.
Un script de precarga se ejecuta antes de que el proceso de renderizado se cargue, y tiene acceso tanto a
globales del renderizador (ej. `window` y `document`) y al entorno Node.js.

Crea un nuevo script llamado `preload.js` como tal:

```js
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const dependency of ['chrome', 'node', 'electron']) {
    replaceText(`${dependency}-version`, process.versions[dependency])
  }
})
```

El código anterior accede al objeto Node.js `process.versions` y ejecuta una función helper básica `replaceText` para insertar los números de versión en el documento HTML.

Para adjuntar este script a tu proceso de renderizado, pasa la ruta a tu script de precarga
la opción `webPreferences.preload` de su constructor `BrowserWindow`.

```js
// incluye el módulo 'path' de Node.js al principio de tu archivo
const path = require('path')

//  modifica la función createWindow() existente
const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  win.loadFile('index.html')
}
// ...
```

Aquí se utilizan dos conceptos de Node.js:

* La cadena [`__dirname`][dirname] apunta a la ruta del script actualmente en ejecución
  (en este caso, la carpeta raíz del proyecto).
* La API [`path.join`][path-join] une múltiples segmentos de ruta, creando una
  cadena de ruta combinada que funciona en todas las plataformas.

Utilizamos una ruta relativa al archivo JavaScript que se está ejecutando en ese momento para que su ruta relativa funcione tanto en el modo de desarrollo como en el modo empaquetado.

[Process Model]: ./process-model.md
[dirname]: https://nodejs.org/api/modules.html#modules_dirname
[path-join]: https://nodejs.org/api/path.html#path_path_join_paths

### Bonus: Añada funcionalidad a sus contenidos web

Llegados a este punto, puede que te estés preguntando cómo añadir más funcionalidad a tu aplicación.

Para cualquier interacción con los contenidos de su web, querrá añadir scripts a su
proceso de renderizado. Como el renderizador se ejecuta en un entorno web normal, puede añadir una 
etiqueta `<script>` justo antes de la etiqueta de cierre `</body>` de su archivo `index.html` para incluir
cualquier script arbitrario que desee:

```html
<script src="./renderer.js"></script>
```

El código contenido en `renderer.js` puede entonces utilizar las mismas APIs de JavaScript y herramientas
que usas para el típico desarrollo front-end, como usar [`webpack`][webpack] para empaquetar y minificar tu código o [React][react] para gestionar tus interfaces de usuario.

[webpack]: https://webpack.js.org
[react]: https://reactjs.org

### Recapitulemos

Después de seguir los pasos anteriores, debería tener una aplicación
Electron completamente funcional:

![La aplicación Electron más sencilla](../images/simplest-electron-app.png)

El código completo está disponible a continuación:
```js
// main.js

// Módulos para controlar la vida de la aplicación y crear una ventana de navegación nativa
const { app, BrowserWindow } = require('electron')
const path = require('path')

const createWindow = () => {
  // Crea la ventana del navegador.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  // y carga el index.html de la aplicación.
  mainWindow.loadFile('index.html')

  // Abra el DevTools.
  // mainWindow.webContents.openDevTools()
}

// Este método será llamado cuando Electron haya terminado
// inicialización y está listo para crear ventanas de navegador.
// Algunas APIs sólo se pueden utilizar después de que se produzca este evento.
app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    // En macOS es común volver a crear una ventana en la aplicación cuando al
    // icono del dock de les click y no hay otras ventanas abiertas.
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Salir cuando todas las ventanas están cerradas, excepto en macOS. Allí, es común
// que las aplicaciones y su barra de menú permanezcan activas hasta que el usuario las cierre
// explícitamente con Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

// En este archivo puedes incluir el resto del proceso principal específico de tu app
// También puedes ponerlos en archivos separados y requerirlos aquí.
```

```js
// preload.js

// Todas las API de Node.js están disponibles en el proceso de precarga.
// Tiene el mismo sandbox que una extensión de Chrome.
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const dependency of ['chrome', 'node', 'electron']) {
    replaceText(`${dependency}-version`, process.versions[dependency])
  }
})
```

```html
<!--index.html-->

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using Node.js <span id="node-version"></span>,
    Chromium <span id="chrome-version"></span>,
    and Electron <span id="electron-version"></span>.

    <!-- También puede requerir que otros archivos se ejecuten en este proceso -->
    <script src="./renderer.js"></script>
  </body>
</html>
```

```fiddle docs/fiddles/quick-start
```

Para resumir todos los pasos que hemos dado:

* Arrancamos una aplicación Node.js y añadimos Electron como dependencia.
* Creamos un script `main.js` que ejecuta nuestro proceso principal, que controla nuestra app
  y se ejecuta en un entorno Node.js. En este script, utilizamos los módulos `app` y
  `BrowserWindow` de Electron para crear una ventana de navegador que muestre el contenido web
  en un proceso separado (el renderizador).
* Para acceder a ciertas funcionalidades de Node.js en el renderizador, adjuntamos
  un script de precarga a nuestro constructor `BrowserWindow`.

## Empaquete y distribuya su aplicación

La forma más rápida de distribuir su aplicación recién creada es utilizar
[Electron Forge](https://www.electronforge.io).

1. Añade Electron Forge como una dependencia de desarrollo de tu aplicación, y utiliza su comando `import` para configurar
Forge:

   ```sh npm2yarn
   npm install --save-dev @electron-forge/cli
   npx electron-forge import

   ✔ Checking your system
   ✔ Initializing Git Repository
   ✔ Writing modified package.json file
   ✔ Installing dependencies
   ✔ Writing modified package.json file
   ✔ Fixing .gitignore

   We have ATTEMPTED to convert your app to be in a format that electron-forge understands.

   Thanks for using "electron-forge"!!!
   ```

2. Crea un distribuible utilizando el comando `make` de Forge:

   ```sh npm2yarn
   npm run make

   > my-electron-app@1.0.0 make /my-electron-app
   > electron-forge make

   ✔ Checking your system
   ✔ Resolving Forge Config
   We need to package your application before we can make it
   ✔ Preparing to Package Application for arch: x64
   ✔ Preparing native dependencies
   ✔ Packaging Application
   Making for the following targets: zip
   ✔ Making for target: zip - On platform: darwin - For arch: x64
   ```

   Electron Forge creates the `out` folder where your package will be located:

   ```plain
   // Example for macOS
   out/
   ├── out/make/zip/darwin/x64/my-electron-app-darwin-x64-1.0.0.zip
   ├── ...
   └── out/my-electron-app-darwin-x64/my-electron-app.app/Contents/MacOS/my-electron-app
   ```
