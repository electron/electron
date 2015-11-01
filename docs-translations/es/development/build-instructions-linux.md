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

#Cross compilation

If you want to build for an arm target you should also install the following dependencies:

$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf

And to cross compile for arm or ia32 targets, you should pass the --target_arch parameter to the bootstrap.py script:

$ ./script/bootstrap.py -v --target_arch=arm

Building

If you would like to build both Release and Debug targets:

$ ./script/build.py

This script will cause a very large Electron executable to be placed in the directory out/R. The file size is in excess of 1.3 gigabytes. This happens because the Release target binary contains debugging symbols. To reduce the file size, run the create-dist.py script:

$ ./script/create-dist.py

This will put a working distribution with much smaller file sizes in the dist directory. After running the create-dist.py script, you may want to remove the 1.3+ gigabyte binary which is still in out/R.

You can also build the Debug target only:

$ ./script/build.py -c D

After building is done, you can find the electron debug binary under out/D.
Cleaning

To clean the build files:

$ ./script/clean.py

Troubleshooting

Make sure you have installed all of the build dependencies.
Error While Loading Shared Libraries: libtinfo.so.5

Prebulit clang will try to link to libtinfo.so.5. Depending on the host architecture, symlink to appropriate libncurses:

$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5

Tests

Test your changes conform to the project coding style using:

$ ./script/cpplint.py

Test functionality using:

$ ./script/test.py
