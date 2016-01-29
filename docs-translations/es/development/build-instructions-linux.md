#Instrucciones de Compilación (Linux)

Siga las siguientes pautas para la construcción de Electron en Linux.
#Requisitos previos

   *  Python 2.7.x. Algunas distribuciones como CentOS siguen utilizando Python 2.6.x por lo que puede que tenga que comprobar su versión de Python con  `Python -V`.
   *  Node.js v0.12.x. Hay varias formas de instalar Node. Puede descargar el código fuente de Node.js y compilar desde las fuentes. Si lo hace, permite la instalación de Node en el directorio personal como usuario estándar. O intentar de  repositorios como NodeSource.
   *  Clang 3.4 o mayor.
   *  Cabeceras de desarrollo de GTK + y libnotify.
   
En Ubuntu, instalar las siguientes bibliotecas:

`$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib`

En Fedora, instale las siguientes bibliotecas:

`$ sudo yum install clang dbus-devel gtk2-devel libnotify-devel libgnome-keyring-devel \
                   xorg-x11-server-utils libcap-devel cups-devel libXtst-devel \
                   alsa-lib-devel libXrandr-devel GConf2-devel nss-devel`

Otras distribuciones pueden ofrecer paquetes similares para la instalación, a través de gestores de paquetes como el pacman. O  puede compilarlo a partir del código fuente.

#Si utiliza máquinas virtuales para la construcción

Si usted planea construir Electron en una máquina virtual, necesitará un  dispositivo de al menos 25 gigabytes de tamaño.

#Obteniendo el codigo

`$ git clone https://github.com/atom/electron.git`

#Bootstrapping (Arranque)

The bootstrap script will download all necessary build dependencies and create the build project files. You must have Python 2.7.x for the script to succeed. Downloading certain files can take a long time. Notice that we are using ninja to build Electron so there is no Makefile generated.

El script de bootstrap descargará todas las dependencias necesarias para construcción  y creara los archivos del proyecto de construcción. Debe tener Python 2.7.x para que la secuencia de comandos tenga éxito. La descarga de determinados archivos puede llevar mucho tiempo. Nótese que estamos usando`ninja` para construir Electron por lo que no hay `Makefile` generado.

    $ cd electron
    $ ./script/bootstrap.py -v

#compilación cruzada

Si usted quiere construir para un `arm` objetivo también debe instalar las siguientes dependencias:

`$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \ g++-arm-linux-gnueabihf`

And to cross compile for arm or ia32 targets, you should pass the --target_arch parameter to the bootstrap.py script:
cruzar y  compilar para `arm` o `ia32` objetivos, debe pasar el parámetro `--target_arch` al script `bootstrap.py`:
`$ ./script/bootstrap.py -v --target_arch=arm`

#Construcción

Si a usted le gustaría construir dos objetivos de `Release` y `Debug`:

    `$ ./script/build.py`


Este script causará que el ejecutable de Electron se muy grande para ser colocado en el directorio `out / R`. El tamaño del archivo es de más de 1,3 gigabytes. Esto sucede porque el binario de destino lanzamiento contiene los símbolos de depuración. Para reducir el tamaño de archivo, ejecute el script `create-dist.py`:

`$ ./script/create-dist.py`

This will put a working distribution with much smaller file sizes in the dist directory. After running the create-dist.py script, you may want to remove the 1.3+ gigabyte binary which is still in out/R.

Esto pondrá una distribución a trabajar con tamaños de archivo mucho más pequeños en el directorio `dist`. Después de ejecutar el script create-dist.py, es posible que desee quitar el binario 1.3+ gigabyte que todavía está en  `out/R`.

También se puede construir sólo el objetivo  `Debug`:
`$ ./script/build.py -c D`

Después de la construcción está hecho, usted puede encontrar el `Electron` de depuración binario bajo `out / D`.

#Limpieza

Para limpiar los archivos de creación:

`$ ./script/clean.py`

#Solución de problemas
Asegúrese de que ha instalado todas las dependencias de construcción.

#Error al cargar bibliotecas compartidas: libtinfo.so.5

Prebulit clang will try to link to libtinfo.so.5. Depending on the host architecture, symlink to appropriate libncurses:
preconstruir `clang`  intentará enlazar a `libtinfo.so.5`. Dependiendo de la arquitectura anfitrión, enlace simbólico apropiado a `libncurses` :

`$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5`

#Pruebas
Pon a prueba tus cambios que ajustan al estilo de codificación proyecto mediante:

`$ ./script/cpplint.py`

prueba de funcionalidad utilizando:

`$ ./script/test.py`
