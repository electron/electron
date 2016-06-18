# Distribución de la Aplicación

Para distribuir tu aplicación con Electron, el directorio que contiene la
aplicación deberá llamarse `app`, y ser colocado debajo del directorio de
recursos de Electron (en OSX es `Electron.app/Contents/Resources/`, en Linux y
Windows es `resources/`), de esta forma:

En macOS:

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

Luego ejecutar `Electron.app` (o `electron` en Linux, `electron.exe` en Windows),
y Electron será iniciado como tu aplicación. El directorio `electron` será
entonces tu distribución que recibirán los usuarios finales.

## Empaquetando tu aplicación en un archivo

Además de distribuir tu aplicación al copiar todos los archivos de código fuente,
también puedes empaquetar tu aplicación como un archivo [asar](https://github.com/atom/asar)
y de esta forma evitar exponer del código fuente de tu aplicación a los usuarios.

Para utilizar un archivo `asar` en reemplazo del directorio `app`, debes de
renombrar el archivo a `app.asar`, y colocarlo por debajo el directorio de recursos
de Electron (ver en seguida), Electron intentará leer el archivo y arrancar desde el.

En macOS:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

En Windows y Linux:

```text
electron/resources/
└── app.asar
```

Más detalles en [Empaquetado de Aplicaciones](application-packaging.md).

## Redefinición con Binarios Descargados

Luego de empaquetar tu aplicación en Electron, querrás redefinir Electron antes
de distribuirlo a los usuarios.

### Windows

Puedes renombrar `electron.exe` a cualquier nombre que desees, y editar su ícono
y otra información con herramientas como [rcedit](https://github.com/atom/rcedit).

### OSX

Puedes renombrar `Electron.app` a cualquier nombre que desees, y tendrás que
renombrar los campos `CFBundleDisplayName`, `CFBundleIdentifier` y `CFBundleName`
en los siguientes archivos:

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

También puedes renombrar el helper de la aplicación para evitar que aparezca
como `Electron Helper` en el Monitor de Actividades. Pero asegurate de renombrar
el nombre de archivo del ejecutable.

La estructura de una aplicación renombrada será:

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

## Redefinición mediante la recompilación de Electron desde el código fuente

También es posible redefinir Electron cambiando el nombre del producto y
compilandolo desde sus fuentes. Para realizar esto necesitas modificar el
archivo `atom.gyp` y realizar una compilación desde cero.

### grunt-build-atom-shell

La modificación a mano del código de Electron y su compilación puede resultar
complicada, por lo cual se ha generado una tarea Grunt para manejar esto de
forma automaticamente:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

Esta tarea se encargará de modificar el archivo `.gyp`, compilar el código desde
las fuentes, y luego reconstruir los módulos nativos de la aplicación para que
coincidan con el nuevo nombre del ejecutable.
