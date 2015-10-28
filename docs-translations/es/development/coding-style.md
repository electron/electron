# Guía de estilo de código

Esta es la guía de estilo de código para Electron.

## C++ y Python

Para C++ y Python, nosotros seguimos la [guía de estilo](http://www.chromium.org/developers/coding-style) de Chromium.
Además hay un script `script/cpplint.py` para verificar si todos los archivos
siguen el estilo.

La versión de Python que estamos usando ahora es Python 2.7.

El código C++ usa muchas abstracciones y tipos de Chromium, por eso
se recomienda familiarizarse con ellos. Un buen lugar para iniciar es
el documento de Chromium sobre [Abstracciones importantes y estructras de datos](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures). El documento menciona algunos tipos especiales, tipos por alcance (que
automaticamente liberan su memoria cuando salen de su alcance), mecanismos de
registro de eventos, etcétera.

## CoffeeScript

Para CoffeeScript, nosotros seguimos la [guía de estilo](https://github.com/styleguide/javascript) de Github y también las
siguientes reglas:

* Los archivos **NO** deberían terminar con una nueva línea, por que se busca
  seguir los estilos que usa Google.
* Los nombres de los archivos debén estar concatenados con `-` en vez de `_`,
  por ejemplo `nombre-de-archivo.coffee` en vez de `nombre_de_archivo.coffee`,
  esto es por que en [github/atom](https://github.com/github/atom)
  los nombres de los módulos usualmente estan en la forma `nombre-de-modulo`.
  Esta regla aplica únicamente a los archivos `.coffee`.

## Nombres de las API

Al crear una nueva API, nosotros deberíamos preferir usar metodos `get` y `set`
en vez de usar el estilo de jQuery que utiliza una sola función. Por ejemplo,
se prefiere `.getText()` y `.setText()` por sobre `.text([text])`. Hay una
[discusión](https://github.com/atom/electron/issues/46) sobre esto.
