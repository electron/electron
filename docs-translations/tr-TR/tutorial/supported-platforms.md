# Desteklenen platformlar

Aşağıdaki platformlar Electron tarafından desteklenmektedir:

### macOS

MacOS için sadece 64bit mimariler sağlanmakta olup, desteklenen minimum macOS versiyonu 10.9 dur.

### Windows

Windows 7 ve gelişmiş versiyonlar desteklenmektedir, eski işletim sistemleri desteklenmemektedir
(ve çalışmayacaktır).

Windows `ia32` (`x86`) ve `x64` (`amd64`) mimarileri desteklenmektedir.
Unutmayın, `ARM` mimarisi al₺tında çalışan Windows işletim sistemleri şuan için desteklenmemektedir.

### Linux

Electron `ia32` (`i686`) ve `x64` (`amd64`) Prebuilt mimarileri Ubuntu 12.04 üzerinde kurulmuştur,
`arm` mimarisi ARM v7 ye karşılık olarak, hard-float ABI ve NEON Debian Wheezy ile kurulmuştur.

Prebuilt
Prebuilt mimarisi ancak Electron'nun yapı platformu ile bağlantılı olan kütüphaneleri içeren dağıtımlar ile çalışır.
Bu yüzden sadece Ubuntu 12.04 üzerinde çalışması garanti ediliyor, fakat aşagidaki platformlarında
Electron Prebuilt mimarisini çalıştıra bileceği doğrulanmıştır.

* Ubuntu 12.04 ve sonrası
* Fedora 21
* Debian 8
