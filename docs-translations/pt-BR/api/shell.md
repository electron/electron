# shell

O módulo `shell` fornece funções relacionadas intereções com o OS do usuário.

Um exemplo para abrir uma URL no browser padrão do usuário:

```javascript
var shell = require('shell');
shell.openExternal('https://github.com');
```

## Métodos

O módulo `shell` tem os seguintes métodos:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Exibe o arquivo no gerenciador de arquivos padrão do sistema. Se possivel, seleciona o arquivo automaticamente.

### `shell.openItem(fullPath)`

* `fullPath` String

Abre o arquivo em seu programa padrão.

### `shell.openExternal(url)`

* `url` String

Abre o arquivo seguido de um protocol em seu programa padrão. (Por
exemplo, mailto:foo@bar.com.)

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Move o arquivo para a lixeira e retorna um boolean com o resultado da operação.

### `shell.beep()`

Toca um som beep.