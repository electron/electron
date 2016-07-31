# Usando Módulos Nativos do Node

Os módulos nativos do Node são suportados pelo Electron, desde que o Electron
esteja usando uma versão diferente da oficial V8 do Node, você tem de
especificar manualmente a localização das headers do Electron quando compilar os
módulos nativos.

## Compatibilidade de Módulos Nativos do Node

Módulos nativos podem quebrar quando utilizar a nova versão do Node, V8.
Para ter certeza que o módulo que você está interessado em trabalhar com o
Electron, você deve checar se a versão do Node utilizada é compatível com a
usada pelo Electron.
Você pode verificar qual versão do Node foi utilizada no Electron olhando na
página [releases](https://github.com/electron/electron/releases) ou usando
`process.version` (veja em [Introdução](quick-start.md)
por exemplo).

Considere usar [NAN](https://github.com/nodejs/nan/) para seus próprios
módulos, caso isso facilite o suporte da múltiplas versões do Node. Isso é
também de grande ajuda para fazer a portabilidade dos módulos antigos para as
versões mais novas do Node, assim podendo trabalhar com o Electron.

## Como Instalar os Módulos Nativos

Existem três maneiras de instalar os módulos nativos:

### O Modo Fácil

O modo mais direto para recompilar os módulos é pelo pacote
[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild),
o que lida com passos manuais para baixar as headers e construir os módulos
nativos:

```sh
npm install --save-dev electron-rebuild

# Sempre que rodar npm install, execute também:
node ./node_modules/.bin/electron-rebuild
```

### Via npm

Você pode usar também `npm` para instalar os módulos. Os passos são exatamente
os mesmos com os módulos Node, exceto que você precisa configurar algumas
variáveis da ambiente:

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.33.1
export npm_config_arch=x64
export npm_config_runtime=electron
HOME=~/.electron-gyp npm install module-name
```

### O modo node-gyp

Para compilar os módulos do Node com as headers do Electron, você precisa dizer
ao `node-gyp` onde baixar as headers e qual versão usar:

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

A tag `HOME=~/.electron-gyp` altera onde encontrar as headers de desenvolvimento.
A tag `--target=0.29.1` é a versão do Electron. A tag `--dist-url=...` especifica
onde baixar as headers. A tag `--arch=x64` diz ao módulo que é feito em 64bit.
