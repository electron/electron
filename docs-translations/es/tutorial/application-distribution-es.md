# Distribución de aplicaciones

Para distribuir tu aplicación con Electron, debes nombrar al directorio de tu aplicación
como `app`, y ponerlo bajo el directorio de recursos de Electron (en OSX es `Electron.app/Contents/Resources/`,
en Linux y Windows es `resources/`):

En OSX:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

En Windows y Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

Posteriormente ejecutas `Electron.app` (o `electron` en Linux, `electron.exe` en Windows),
y Electron iniciará la aplicación.  El directorio `electron` será la distribución que recibirán los usuarios finales.

## Empaquetando tu aplicación como un archivo

Además de copiar todos tus archivos fuente para la distribución, también puedes
empaquetar tu aplicación como un archivo [asar](https://github.com/atom/asar)
y de esta forma evitar la exposición del código fuente de tu aplicación a los usuarios.

Para usar un archivo `asar` en reemplazo de la carpeta `app`, debes renombrar
el archivo a `app.asar`, y ponerlo bajo el directorio de recursos de Electron (como arriba),
Electron intentará leer el archivo y ejecutar la aplicación desde él.

En OS X:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

En Windows y Linux:

```text
electron/resources/
└── app.asar
```

Más detalles en [Empaquetamiento de aplicaciones](application-packaging-es.md).

## Rebranding con binarios descargados

Luego de empaquetar tu aplicación con Electron, podría ser útil agregar tu marca
antes de realizar la distribución.

### Windows

Puedes renombrar `electron.exe` a cualquier nombre que desees, y editar su ícono y otras informaciones
con herramientas como [rcedit](https://github.com/atom/rcedit) o [ResEdit](http://www.resedit.net).

### OS X

Puedes renombrar `Electron.app` a cualquier nombre que desees. También debes modificar los campos 
`CFBundleDisplayName`, `CFBundleIdentifier` y `CFBundleName` en los siguientes archivos:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

También puedes renombrar el helper de la aplicación para evitar que aparezca como  `Electron Helper`
en el Monitor de Actividades.

La estructura de una aplicación renombrada sería así:

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

Puedes renombrar el ejectuable `electron` a cualquier nombre que desees.

## Rebranding desde el código fuente de Electron

También es posible agregar tu marca a Electron mediante un build personalizado.
Para realizar esto debes modificar el archivo `atom.gyp`.

### grunt-build-atom-shell

La modificación del código de Electron para agregar tu marca puede resultar complicada, una tarea Grunt
se ha creado para manejar esto de forma automatizada:

[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Esta tarea se encargará de modificar el archivo `.gyp`, compilar el código
y reconstruir los módulos nativos de la aplicación para que coincidan con el nuevo nombre.
