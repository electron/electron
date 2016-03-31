#Instrucciones de Compilación (Mac)
Siga las siguientes pautas para la construcción de Electron en OS X.

#Requisitos previos 

      `OS X >= 10.8`
      `Xcode >= 5.1`
      `node.js (external)`


Si está utilizando  Python descargado de Homebrew, también es necesario instalar los siguientes módulos de python:
    `pyobjc`

#Obtener el Código

`$ git clone https://github.com/electron/electron.git`

#Bootstrapping (arranque)

The bootstrap script will download all necessary build dependencies and create the build project files. Notice that we're using ninja to build Electron so there is no Xcode project generated.

El script de bootstrap  descargará todas las dependencias  de construcción necesarias y creara los archivos del proyecto de compilación. notemos que estamos usando `ninja` para construir Electron por lo que no hay un proyecto de Xcode generado.

`$ cd electron`
`$ ./script/bootstrap.py -v`

#Construcción
Construir ambos objetivos de `Release` y  `Debug`:

`$ ./script/build.py`

También sólo se puede construir el objetivo de `Debug`:
`$ ./script/build.py -c D`

Después de la construcción está hecho, usted puede encontrar `Electron.app` bajo `out / D.`

#Soporte de 32bit

Electron sólo puede construirse para un objetivo de 64 bits en OS X y no hay un plan para apoyar a 32 bit OS X en el futuro.

#Pruebas

Pon a prueba tus cambios ajustandose al estilo de codificación del proyecto mediante:
`$ ./script/cpplint.py`

Prueba la funcionalidad usando:

`$ ./script/test.py`
