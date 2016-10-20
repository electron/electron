# Instruções de Build (OSX)

Siga as orientações abaixo para fazer o build do Electron no OSX.

## Pré-requisitos

* OSX >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external)

Se você estiver usando o Python baixado pelo Homebrew, terá também de instalar o seguinte módulo python:

* pyobjc

## Baixando o código

```bash
$ git clone https://github.com/electron/electron.git
```

## Bootstrapping

O sciprt de *bootstrap* irá baixar todas as dependencias necessárias e criar os arquivos de projeto do build. Note que estamos utilizando o [ninja](https://ninja-build.org/) para fazer o build do Electron, então não há projeto gerado pelo Xcode.

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## Building

Para fazer o build do `Release` e `Debug`:

```bash
$ ./script/build.py
```

Para fazer o build somente do `Debug`:

```bash
$ ./script/build.py -c D
```

Depois de feito o build, você pode encontrar `Electron.app` abaixo de `out/D`.

## Suporte 32bit

Electron pode ser construído somente em 64bit no OSX e não há planos para suportar 32bit no futuro.

## Limpando

Para limpar os arquivos de build:

```bash
$ npm run clean
```

## Testes

Teste suas modificações conforme o estilo de código do projeto utilizando:

```bash
$ ./script/cpplint.py
```

Teste as funcionalidades usando:

```bash
$ ./script/test.py
```
