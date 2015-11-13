# Estilo de Codificação

Estas são as diretrizes de estilo para codificar no Electron.

## C++ and Python

Para C ++ e Python, seguimos os padrões do projeto Chromium [Estilo de Codificação](http://www.chromium.org/developers/coding-style). Há também um
script `script/cpplint.py` para verificar se todos os arquivos estão em conformidade.

A versão Python que estamos usando agora é a Python 2.7.

O código C ++ usa do Chromium's um monte de tipos e abstrações, por isso é recomendada para se familiarizar com eles. Um bom lugar para começar com a documentação do Chromium's [Important Abstractions and Data Structures](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures). O documento menciona alguns tipos especiais, com escopo tipos (que automaticamente libera sua memória quando sai do escopo), registrando mecanismos etc.

## CoffeeScript

For CoffeeScript, we follow GitHub's [Style
Guide](https://github.com/styleguide/javascript) and the following rules:

Para CoffeeScript, seguimos o estilo do GitHub [Guia de Estilo] (https://github.com/styleguide/javascript) com as seguintes regras:

* Os arquivos devem **NÃO DEVEM** com nova linha no final, porque queremos corresponder aos padrões de estilo Google.

* Os nomes dos arquivos devem ser concatenados com o `-` em vez de`_`, por exemplo, `file-name.coffee` em vez de`file_name.coffee`, porque no [github/atom](https://github.com/github/atom) os nomes dos módulos são geralmente em o formulário `module-name`. Esta regra só se aplica aos arquivos com extensão `.coffee`.
* 
## API Names

Ao criar uma nova API, devemos preferencialmente utilizar métodos getters e setters em vez de
estilo de uma função do jQuery. Por exemplo, `.getText()` e `.setText(text)` utilize `.text([text])`. Existe uma
[discussão](https://github.com/atom/electron/issues/46) sobre este assunto.
