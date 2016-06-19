# Plataformas soportadas

Las siguientes plataformas son soportadas por Electron:

### macOS

Sólo se proveen binarios de 64 bit para macOS.
La versión mínima soportada es macOS 10.8.

### Windows

Windows 7 y posteriores son soportados, las versiones antiguas no son soportadas (y no funcionan).

Se proveen binarios para las arquitecturas `x86` y `amd64` (x64).
Nota: La versión para `ARM` de Windows no está soportada aún.

### Linux

Los binarios preconstruidos para `ia32`(`i686`) y `x64`(`amd64`) son construidos sobre
Ubuntu 12.04, el binario para `arm` es construido sobre ARM v7 con la ABI hard-float
y NEON para Debian Wheezy.

La posibilidad de que un binario preconstruido se ejecute en una distribución determinada
depende de las librerías contra las que fue enlazado Electron.
Por ahora sólo se garantiza la ejecución en Ubuntu 12.04, aunque también se ha verificado
el funcionamiento de los binarios preconstruidos en las siguientes plataformas:

* Ubuntu 12.04 and later
* Fedora 21
* Debian 8
