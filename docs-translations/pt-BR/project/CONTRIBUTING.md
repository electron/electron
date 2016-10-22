# Contribuindo com Electron

:memo: Traduções disponíveis: [Inglês](https://github.com/electron/electron/tree/master/CONTRIBUTING.md) | [Coreano](https://github.com/electron/electron/tree/master/docs-translations/ko-KR/project/CONTRIBUTING.md) | [Chinês Simplificado](https://github.com/electron/electron/tree/master/docs-translations/zh-CN/project/CONTRIBUTING.md)

:+1::tada: Primeiramente, obrigado por utilizar seu tempo contribuindo! :tada::+1:

Esse projeto segue um [código de conduta](CODE_OF_CONDUCT.md).
Ao participar, é esperado que o código seja seguido. Favor reportar comportamentos inaceitáveis para atom@github.com.

O texto a seguir é um conjunto de diretrizes para a contribuir com o Electron.
Estas são apenas diretrizes e não regras, use o seu bom senso e sinta-se livre para propor alterações nesse documento em um pull request.

## Reportando Issues

* Você pode criar uma issue [aqui](https://github.com/electron/electron/issues/new),
mas antes disso, por favor, leia as notas abaixo e inclua o máximo de detalhes possíveis em seu relato. Se puder, favor informar:
  * A versão do Electron utilizada
  * O sistema operacional utilizado
  * Se possível, o que estava fazendo quando o problema ocorreu e o que era esperado que acontecesse
* Alguns outros meios que irão ajudar a resolver a issue:
  * Capturas de tela e GIFs animados
  * Saída de erro que aparece no terminal, dev tools ou algum em algum alerta
  * Faça uma [pesquisa rápida](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  para verificar se já não existe uma issue similar já aberta

## Submetendo Pull Requests

* Inclua capturas de tela e GIFs animados em seu pull request sempre que possível.
* Siga os [padrões de código definidos nos documentos](/docs/development/coding-style.md) para JavaScript, C++, e Python.
* Escreva a documentação em [Markdown](https://daringfireball.net/projects/markdown).
  Veja o [Guia de estilo de documentação](/docs/styleguide.md).
* Use mensagens curtas e com a conjugação verbal no tempo presente. Veja em [Guia de estilo de mensagens de commit](#git-commit-messages).

## Guias de estilo

### Código Geral

* Terminar arquivos com uma nova linha.
* A organização deve estar na seguinte ordem:
  * Módulos embutidos do Node (como o `path`)
  * Módulos embutidos do Electron (como `ipc`, `app`)
  * Módulos locais (usando caminhos relativos)
* Defina as propriedades da classe na seguinte ordem:
  * Métodos e propriedades da classe (métodos iniciam com `@`)
  * Métodos e propriedades de instância
* Evite o uso de códigos dependentes da plataforma:
  * Use `path.join()` para concatenar nomes de arquivos.
  * Use `os.tmpdir()` ao invés de `/tmp` quando precisar referenciar o diretório temporário.
* Use um simples `return` para retornar explícitamente o fim de uma função.
  * Não usar `return null`, `return undefined`, `null`, ou `undefined`

### Mensagens de Commit do Git

* Use o tempo presente ("Adicionando função" não "Adicionada função")
* Use o modo imperativo ("Mova o cursor para..." não "O cursor deve ser movido...")
* Limite a primeira linha para 72 caracteres ou menos
* Referencie issues e pull requests livremente
* Quando alterar somente a documentação. inclua `[ci skip]` na descrição do commit
* Considerando iniciar a mensagem do commit com um emoji:
  * :art: `:art:` quando aperfeiçoar o formato/estrutura do código
  * :racehorse: `:racehorse:` quando aperfeiçoar a performance
  * :non-potable_water: `:non-potable_water:` quando previnir vazamento de memória
  * :memo: `:memo:` quando escrever documentação
  * :penguin: `:penguin:` quando corrigir algo no Linux
  * :apple: `:apple:` quando corrigir algo no OSX
  * :checkered_flag: `:checkered_flag:` quando corrigir algo no Windows
  * :bug: `:bug:` quando corrigir um bug
  * :fire: `:fire:` quando remover código ou arquivos
  * :green_heart: `:green_heart:` quando corrigir o build do CI
  * :white_check_mark: `:white_check_mark:` quando adicionar testes
  * :lock: `:lock:` quando estiver lidando com segurança
  * :arrow_up: `:arrow_up:` quando atualizar dependências
  * :arrow_down: `:arrow_down:` quando abaixar as dependências
  * :shirt: `:shirt:` quando remover avisos do linter
