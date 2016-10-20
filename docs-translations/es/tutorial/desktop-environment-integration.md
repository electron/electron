# Integración con el entorno de escritorio

Los sistemas operativos proveen diferentes características para integrar aplicaciones
en sus entornos de escritorio. Por ejemplo, en Windows, las aplicaciones pueden agregar accesos directos
en la JumpList de la barra de tareas, y en Mac, las aplicaciones pueden agregar un menú personalizado en el dock.

Esta guía explica cómo integrar tu aplicación en esos entornos de escritorio a través de las APIs de Electron.

## Documentos recientes (Windows y macOS)

Windows y macOS proveen un acceso sencillo a la lista de documentos recientes.

__JumpList:__

![JumpList, Archivos recientes](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__Menú Dock:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

Para agregar un archivo a la lista de documentos recientes, puedes utilizar:
[app.addRecentDocument][addrecentdocument] API:

```javascript
var app = require('app')
app.addRecentDocument('/Users/USERNAME/Desktop/work.type')
```

También puedes utilizar [app.clearRecentDocuments](clearrecentdocuments) para vaciar la lista de documentos recientes:

```javascript
app.clearRecentDocuments()
```

### Notas sobre Windows

Para activar esta característica en Windows, tu aplicación debe registrar un handler
para el tipo de archivo que quieres utilizar, de lo contrario el archivo no aparecerá
en la JumpList, aún después de agregarlo. Puedes encontrar más información sobre el proceso de
registrar tu aplicación en [Application Registration][app-registration].

Cuando un usuario haga click en un archivo de la JumpList, una nueva instancia de tu aplicación
se iniciará, la ruta del archivo se agregará como un argumento de la línea de comandos.

### Notas sobre macOS

Cuando un archivo es solicitado desde el menú de documentos recientes, el evento `open-file`
del módulo `app` será emitido.

## Menú dock personalizado (macOS)

macOS permite a los desarrolladores definir un menú personalizado para el dock,
el cual usualmente contiene algunos accesos directos a las características más comunes
de tu aplicación:

__Menú dock de Terminal.app:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

Para establecer tu menú dock, puedes utilizar la API `app.dock.setMenu`, la cual sólo está disponible para macOS:

```javascript
var app = require('app')
var Menu = require('menu')
var dockMenu = Menu.buildFromTemplate([
  {label: 'New Window', click: function () { console.log('New Window') }},
  {label: 'New Window with Settings',
    submenu: [
      {label: 'Basic'},
      {label: 'Pro'}
    ]
  },
  {label: 'New Command...'}
])
app.dock.setMenu(dockMenu)
```

## Tareas de usuario (Windows)

En Windows puedes especificar acciones personalizadas en la categoría `Tasks` del JumpList,
tal como menciona MSDN:


> Las aplicaciones definen tareas basadas en las características del programa
> y las acciones clave que se esperan de un usuario. Las tareas deben ser
> libres de contexto, es decir, la aplicación no debe encontrarse en ejecución
> para que estas acciones funcionen. También deberían ser las acciones estadísticamente
> más comunes que un usuario normal realizaría en tu aplicación, como por ejemplo,
> redactar un mensaje de correo electrónico, crear un documento en el procesador de textos,
> ejecutar una aplicación en cierto modo, o ejecutar alguno de sus subcomandos. Una aplicación
> no debería popular el menú con características avanzadas que el usuario estándar no necesita
> ni con acciones que sólo se realizan una vez, como por ejemplo, el registro. No utilices
> las tareas para mostrar elementos promocionales como actualizaciones u ofertas especiales.
>
> Es recomendable que la lista de tareas sea estática. Debe mantenerse a pesar
> de los cambios de estado de la aplicación. Aunque exista la posibilidad de variar
> el contenido de la lista dinámicamente, debes considerar que podría ser confuso
> para un usuario que no espera que el destino de la lista cambie.

__Tareas de Internet Explorer:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

A diferencia del menú dock en macOS, el cual es un menú real, las tareas de usuario en Windows
funcionan como accesos directos de aplicación, que al ser clickeados, lanzan el programa
con argumentos específicos.

Para establecer las tareas de usuario en tu aplicación, puedes utilizar:
[app.setUserTasks][setusertaskstasks] API:

```javascript
var app = require('app')
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
])
```

Para purgar la lista de tareas, puedes llamar a `app.setUserTasks` con un array vacío:

```javascript
app.setUserTasks([])
```

Las tareas de usuario aún serán visibles después de cerrar tu aplicación, por lo cual
el ícono y la ruta del programa deben existir hasta que la aplicación sea desinstalada.

## Accesos directos en el lanzador Unity (Linux)

En Unity, es posible agregar algunas entradas personalizadas, modificando el archivo `.desktop`,
ver  [Adding shortcuts to a launcher][unity-launcher].

__Accesos directos de Audacious:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## Barra de progreso en la barra de tareas (Windows y Unity)

En Windows, un botón en la barra de tareas puede utilizarse para mostrar una barra de progreso. Esto permite
que una ventana muestre la información de progreso al usuario, sin que el usuario tenga la ventana de la aplicación activa.

El entorno de escritorio Unity también posee una característica similar que permite mostrar una barra de progreso en el lanzador.

__Barra de progreso en un botón de la barra de herramientas:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

__Barra de progreso en el lanzador Unity:__

![Unity Launcher](https://cloud.githubusercontent.com/assets/639601/5081747/4a0a589e-6f0f-11e4-803f-91594716a546.png)

Para establecer la barra de progreso de una ventana, puedes utilizar
[BrowserWindow.setProgressBar][setprogressbar] API:

```javascript
var window = new BrowserWindow()
window.setProgressBar(0.5)
```

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks
[setprogressbar]: ../api/browser-window.md#browserwindowsetprogressbarprogress
[setrepresentedfilename]: ../api/browser-window.md#browserwindowsetrepresentedfilenamefilename
[setdocumentedited]: ../api/browser-window.md#browserwindowsetdocumenteditededited
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
