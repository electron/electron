#Repaso del Sistema de construcción
Electron utiliza `gyp` para la generación de proyectos y` ninja` para la contrucción. Las Configuraciones del proyecto se pueden encontrar en los archivos `.gypi` y `.gyp `.

#Archivos Gyp 
los siguientes archivos `gyp` contienen las principales reglas para la contrucción en electron:

  * `atom.gyp` define en si como se compila en Electron.
  * `common.gypi` ajusta las configuraciones de generación de Node para  construir junto con Chromium.
  * `vendor/brightray/brightray.gyp` define cómo se construye `brightray` e incluye las configuraciones predeterminadas para linkear con Chromium.
  * `vendor/brightray/brightray.gypi` incluye configuraciones de generación generales sobre la construcción.

#Construir un componente
Desde Chromium es un proyecto bastante largo, la etapa de enlace final puede tomar pocos minutos, lo que hace que sea difícil para el desarrollo. Con el fin de resolver esto, Chromium introdujo el "componente de construcción", que se basa en construir cada componente como una libreria  compartida por separado, haciendo que se enlace muy rápido, pero sacrificando el tamaño del archivo y el rendimiento.

En Electron tomamos un enfoque muy similar: para versiones de `Debug` (depuración), el binario será linkeado a una versión de la libreria compartida de los componentes de Chromium para lograr un tiempo de enlace rápido; para versiones de `Release` (lanzamiento), el binario será linkeado a las versiones de las librerias estáticas, por lo que puede tener es posible tener un mejor tamaño binario  y rendimiento.

#Bootstrapping minimo (minimo arranque)
Todos los binarios pre-compilados de Chromium (`libchromiumcontent`) son descargados al ejecutar el script de arranque. Por defecto ambas librerias estáticas y librerias compartidas se descargarán y el tamaño final debe estar entre 800 MB y 2 GB dependiendo de la plataforma.

Por defecto, `libchromiumcontent` se descarga de Amazon Web Services. Si se establece la variable de entorno `LIBCHROMIUMCONTENT_MIRROR`, el bootstrap script se descargará de ella. `libchromiumcontent-qiniu-mirror` es un espejo para el` libchromiumcontent`. Si tiene problemas para acceder a AWS, puede cambiar la dirección de descarga a la misma a través de `exportación LIBCHROMIUMCONTENT_MIRROR = http: // 7xk3d2.dl1.z0.glb.clouddn.com /`

Si sólo desea construir en Electron rápidamente para pruebas o desarrollo, puede descargar sólo las versiones de librerias compartidas pasando el parámetro `--dev`:

`$ ./script/bootstrap.py --dev`
`$ ./script/build.py -c D`

#generación de proyecto de dos frases
Los enlaces de Electron con diferentes conjuntos de librerias en versiones `Release` y `Debug`. `gyp`, sin embargo, no es compatible con la configuración de los diferentes ajustes de enlace para diferentes configuraciones.

Para evitar que Electron utilice una variable de `gyp` `libchromiumcontent_component` para controlar qué configuraciones de enlace usar y sólo generar un objetivo cuando se ejecute `gyp`.

#Nombres de destino
A diferencia de la mayoría de los proyectos que utilizan `Release` y `Debug` como nombres de destino, Electron utiliza `R` y `D` en su lugar. Esto se debe a `gyp` bloquea aleatoriamente si sólo hay una configuración de `Release` o `Debug` definidas, y Electron sólo tiene  que  generar un objetivo a la vez como se ha indicado anteriormente.

Esto sólo afecta a los desarrolladores, si usted está construyendo Electron para rebranding no se ven afectados.
