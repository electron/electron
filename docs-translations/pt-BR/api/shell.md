# shell

O módulo `shell` fornece funções relacionadas à integração com o desktop.

Um exemplo para abrir uma URL no browser padrão do usuário:

```javascript
const shell = require('shell')
shell.openExternal('https://github.com')
```

## Métodos

O módulo `shell` tem os seguintes métodos:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Exibe o arquivo num gerenciador de arquivos. Se possivel, seleciona o arquivo.

### `shell.openItem(fullPath)`

* `fullPath` String

Abre o arquivo de maneira padrão do desktop.

### `shell.openExternal(url)`

* `url` String

Abre a URL de protocolo externo de maneira padrão do desktop. (Por
exemplo, mailto: URLs no programa de email padrão do usuário)

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Move o arquivo para a lixeira e retorna um status boolean com o resultado da operação.

### `shell.beep()`

Toca um som beep.