# Contribuindo para Electron

:memo: Traduções Disponíveis: [Korean](https://github.com/electron/electron/tree/master/docs-translations/ko-KR/project/CONTRIBUTING.md) | [Simplified Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-CN/project/CONTRIBUTING.md) | [Português do Brasil](https://github.com/electron/electron/tree/master/docs-translations/pt-BR/project/CONTRIBUTING.md)

:+1::tada: Em primeiro lugar, obrigado pelo seu tempo para contribuir! :tada::+1:

Este projeto obedece ao Pacto do Colaborador [código de conduta](CODE_OF_CONDUCT.md).
Ao participar, é esperado que você mantenha este código. Por favor, reporte o comportamento inesperado para atom@github.com.

A seguir um conjunto de diretrizes para contribuir para Electron. Estas são apenas diretrizes, não são regras, use-as com bom senso e sinta-se livre para propor alterações a este documento através de um pull request.

## Reportando Issues

* Você pode criar uma issue [aqui](https://github.com/electron/electron/issues/new),
mas antes de fazer, por favor leia as notas abaixo e inclua o maior número de detalhes possíveis do problema. Se puder, por favor inclua:
  * A versão do Electron que estiver usando
  * O sistema operacional que estiver usando
  * Se possível, o que estava fazendo quando ocorreu o problema e o que esperava que acontecesse
* Outras coisas que poderão ajudar a resolver o problema:
  * Captura de telas e GIFs animados
  * O erro que aparece no terminal, ferramentas de desenvolvimento ou alertas lançados
  * Realizar uma [pesquisa rápida](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  para ver se um problema semelhante já não foi reportado

## Submetendo Pull Requests

* Incluir captura de telas e GIFs animados no pull request sempre que possível.
* Seguir o [estilo de código definido na documento](/docs-translations/pt-BR/development/coding-style.md) do JavaScript, C++, and Python.
* Escrever a documentação em [Markdown](https://daringfireball.net/projects/markdown).
  Veja o [guia de estilo da documentação](/docs-translations/pt-BR/styleguide.md).
* Escreva mensagens curtas e no tempo presente para o commit. Veja [guia de estilo para mensagens de commit](#git-commit-messages).

## Guias de Estilo

### Código Geral

* Terminar arquivos com uma nova linha.
* Coloque na seguinte sequência:
  * Construa um módulos nó (como `path`)
  * Construa um módulos Electron (como `ipc`, `app`)
  * Módulos locais (use caminhos relativos)
* Coloque propriedades da classe na seguinte sequência:
  * Métodos e propriedades da classe (métodos começando com um `@`)
  * Métodos e propriedades de instância
* Evite código dependente de plataforma:
  * Use `path.join()` para concatenar nomes de arquivos.
  * Use `os.tmpdir()` ao inves de `/tmp` quando precisar fazer referência ao diretório temporário.
* Use um simples `return` explicitamente no final de uma função com retorno.
  * Nao `return null`, `return undefined`, `null`, ou `undefined`

### Mensagens Git Commit

* Use o tempo presente("Add feature" não "Added feature")
* Use o modo imperativo ("Move cursor to..." não "Moves cursor to...")
* Limitar a primeira linha para 72 caracteres ou menos
* Referênciar issues e pull requests liberalmente
* Quando apenas a documentação mudar, inclua `[ci skip]` na descrição do commit
* Considere começar a mensagem do commit aplicando um emoji:
  * :art: `:art:` quando melhorar o formato/estrutura do código
  * :racehorse: `:racehorse:` quando melhorar o desempenho
  * :non-potable_water: `:non-potable_water:` quando acabar com vazamento de memória
  * :memo: `:memo:` quando escrever documentação
  * :penguin: `:penguin:` quando corrigir algo no Linux
  * :apple: `:apple:` quando corrigir algo no macOS
  * :checkered_flag: `:checkered_flag:` quando corrigir algo no Windows
  * :bug: `:bug:` quando corrigir um bug
  * :fire: `:fire:` quando remover código ou arquivos
  * :green_heart: `:green_heart:` quando corrigir a compilação CI
  * :white_check_mark: `:white_check_mark:` quando adicionar testes
  * :lock: `:lock:` quando se trata de segurança
  * :arrow_up: `:arrow_up:` quando upgrading dependências
  * :arrow_down: `:arrow_down:` quando downgrading dependências
  * :shirt: `:shirt:` quando remover advertências linter
