# Plataformas Suportadas

As plataformas suportadas por Electron são:

### macOS

Somente binarios em 64bit são construidos para macOS e a versão mínima suportada é macOS 10.9.

### Windows
Suporte para Windows 7 ou superior, versões anteriores não são suportados (e não ira funcionar) 

 Binarios em `x86` e `amd64` (x64) são construidos para Windows. Atencão: Versão `ARM` do Windows não é suportada agora.

### Linux
Binario pré-construido `ia32`(`i686`) e binario `x64`(`amd64`) são construidas no Ubuntu 12.04, binario `arm` está construido contra ARM v7 com hard-float ABI e NEION para Debian Wheezy.

Se o pré-compilador poderá ser executado por uma distribuição, depende se a distruibuicão inclui as blibliotecas que o Eletron está vinculando na plataforma de construcão, por este motivo apenas Ubuntu 12.04 é garantido para funcionar corretamente, mas as seguintes distribuições foram verificados com o pre-compilador:


* Ubuntu 12.04 ou superior
* Fedora 21
* Debian 8
