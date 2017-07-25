# Wspierane platformy
Poniższe platformy są wspierane przez Electron:

### macOS

Only 64bit binaries are provided for macOS, and the minimum macOS version
supported is macOS 10.9.
Jedynie 64 bitowe pliki binarne są dostarczane dla systemu macOS. Minimalna wspierana wersja to macOS 10.9. 

### Windows
Windows 7 i późniejsze są wspierane, wcześniejsze nie są wspierane i mogą nie działać. Zarówno pliki binarne `ia32` (`x86`) oraz `x64` (`amd64`) są dostarczane dla systemu Windows. 

Weź pod uwagę, że wersja `ARM` dla systemów Windows nie jest wspierana.

### Linux
Pre-budowane pliki binarne `ia32` (`x86`) oraz `x64` (`amd64`) są budowane na Ubuntu 12.04, wersja `arm` plików binarnych jest zbudowana naprzeciw ARM v7 z `hard-float ABI` oraz NEON dla Debian Wheezy. 

Pre-budowane pliki binarne mogą zostać uruchomione na dystrybucjach, które zawierają biblioteki wykorzystywane przez Electron. Tak więc, Ubuntu 12.04 gwarantuje działanie. Poniższe platformy także są zweryfikowane pod kątem uruchamiania pre-budowanych plików binarnych Electron:

* Ubuntu 12.04 and later
* Fedora 21
* Debian 8
