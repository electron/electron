# Plataformas Suportadas

As plataformas suportadas por Electron são:

### macOS

Somente binários em 64bit são construídos para macOS e a versão mínima suportada é macOS 10.9.

### Windows
Suporte para Windows 7 ou superior, versões anteriores não são suportados (e não ira funcionar)

Binários em `x86` e `amd64` (x64) são construídos para Windows. Atenção: Versão `ARM` do Windows não é suportada agora.

### Linux
Binario pré-construído `ia32`(`i686`) e binario `x64`(`amd64`) são construídas no Ubuntu 12.04, binario `arm` está construído contra ARM v7 com hard-float ABI e NEION para Debian Wheezy.

Se o pré-compilador poderá ser executado por uma distribuição, depende se a distribuição inclui as bibliotecas que o Eletron está vinculando na plataforma de construção, por este motivo apenas Ubuntu 12.04 é garantido para funcionar corretamente, mas as seguintes distribuições foram verificados com o pre-compilador:


* Ubuntu 12.04 ou superior
* Fedora 21
* Debian 8
