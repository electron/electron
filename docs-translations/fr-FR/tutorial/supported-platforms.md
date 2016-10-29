# Plateformes supportées

Les plateformes suivantes sont supportées par Electron:

### macOS

Seulement les binaires en 64bit  sont fournis sur macOS, et la version minimale de macOS suportée est macOS 10.9.

### Windows

Windows 7 et suivants sont supportés, les systèmes d'exploitation plus ancien de sont pas supporté
(et ne marchent pas).

Les binaires `ia32` (`x86`) et `x64` (`amd64`) sont fournis pour Windows.
Veuillez noter que la version `ARM` de Windows n'est pas supporté à ce jour.

### Linux

Les binaires précompilés `ia32` (`i686`) et `x64` (`amd64`) d'Electron sont compilés sous
Ubuntu 12.04, le binaire `arm` est compilé à partir d'une version ARM v7 hard-float ABI et
NEON pour Debian Wheezy.

Le binaire précompilé peut tourner sur une distribution si la distribution contient les librairies liées à Electron, sur la plateforme de compilation. Donc il est ganranti de pouvoir le faire marcher seulement sur Ubuntu 12.04, mais les plateformes suivantes sont aussi vérifiées comme capable de faire marcher la version précompilé des binaires d'Electron :

* Ubuntu 12.04 et suivantes
* Fedora 21
* Debian 8
