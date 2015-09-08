# shell

O módulo `shell` fornece função relacionado para integração de desktop.

Um exemplo de abrindo uma URL no browser padrão dos usuários:

```javascript
var shell = require('shell');

shell.openExternal('https://github.com');
```

## Métodos

O módulo `shell` tem os seguintes métodos:

### `shell.showItemInFolder(CaminhoCompleto)`

* `CaminhoCompleto` String

Exibir o dado do arquivo em gerenciador de arquivo. Se possivel, selecionar o arquivo.

### `shell.openItem(CaminhoCompleto)`

* `CaminhoCompleto` String

Abrir o dado do arquivo no desktop

Open the given file in the desktop's default manner.

### `shell.openExternal(url)`

* `url` String

Open the given external protocol URL in the desktop's default manner. (For
example, mailto: URLs in the user's default mail agent.)

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Move the given file to trash and returns a boolean status for the operation.

### `shell.beep()`

Play the beep sound.
