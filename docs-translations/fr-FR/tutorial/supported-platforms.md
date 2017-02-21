# Plateformes supportées

Les plateformes suivantes sont supportées par Electron:

### macOS

Seuls les binaires en 64bit sont fournis sur macOS, et la version minimale de macOS suportée est macOS 10.9.

### Windows

Les systèmes d'exploitations Windows 7 et supérieur sont supportés. Les versions antérieures ne sont pas supportées (et ne marchent pas).

Les binaires `ia32` (`x86`) et `x64` (`amd64`) sont fournis pour Windows.
Veuillez noter que la version `ARM` de Windows n'est pas supportée à ce jour.

### Linux

Les binaires précompilés `ia32` (`i686`) et `x64` (`amd64`) d'Electron sont compilés sous
Ubuntu 12.04, le binaire `arm` est compilé à partir d'une version ARM v7 hard-float ABI et
NEON pour Debian Wheezy.

Pour que les binaires pré-compilés puissent s'exécuter sur une certaine distribution, il faut que cette distribution inclut les librairies dont Electron a besoin. C'est à dire que seulement Ubuntu 12.04 est guaranti de fonctionner, même si les plateformes suivantes sont aussi verifiées et capables d'exécuter les binaires pré-compilés d'Electron:

* Ubuntu 12.04 et suivantes
* Fedora 21
* Debian 8
