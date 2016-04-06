# Estilo de Codificação

Estas são as diretrizes de estilo para codificar no Electron.

## C++ e Python

Para C++ e Python, seguimos o [Estilo de Codificação](http://www.chromium.org/developers/coding-style) do projeto Chromium. Há também um
script `script/cpplint.py` para verificar se todos os arquivos estão em conformidade.

A versão Python que estamos usando agora é a Python 2.7.

O código C++ usa  várias abstrações e tipos do Chromium, por isso é recomendado familiarizar-se com eles. Um bom lugar para começar é com a documentação do Chromium [Important Abstractions and Data Structures](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures). O documento menciona alguns tipos especiais, *scoped types* (que automaticamente liberam sua memória ao sair do escopo), mecanismos de *log* etc.

## CoffeeScript

Para CoffeeScript, seguimos o [Guia de Estilo] (https://github.com/styleguide/javascript) do GitHub com as seguintes regras:

* Os arquivos **NÃO DEVEM** terminar com uma nova linha, porque queremos corresponder aos padrões de estilo Google.

* Os nomes dos arquivos devem ser concatenados com `-` em vez de `_`, por exemplo, `file-name.coffee` em vez de `file_name.coffee`, porque no [github/atom](https://github.com/github/atom) os nomes dos módulos são geralmente da forma `module-name`. Esta regra só se aplica aos arquivos com extensão `.coffee`.
* 
## Nomes de APIs

Ao criar uma nova API, devemos preferencialmente utilizar métodos getters e setters em vez do
estilo de uma função única do jQuery. Por exemplo, `.getText()` e `.setText(text)` são preferenciais a `.text([text])`. Existe uma
[discussão](https://github.com/electron/electron/issues/46) sobre este assunto.
