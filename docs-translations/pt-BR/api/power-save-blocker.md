# powerSaveBlocker

> loquear o sistema de entrar no modo de pouca energia (suspenso).

Processo: [Main](../tutorial/quick-start.md#main-process)

Por exemplo:

```javascript
const {powerSaveBlocker} = require('electron')

const id = powerSaveBlocker.start('prevent-display-sleep')
console.log(powerSaveBlocker.isStarted(id))

powerSaveBlocker.stop(id)
```

## Methods

O módulo `powerSaveBlocker` tem os seguintes métodos:

### `powerSaveBlocker.start(type)`

* `type` String - Tipo de bloqueador de economia de energia.
  * `prevent-app-suspension` - Impedir que o aplicativo seja suspenso.
    Mantém o sistema ativo, mas permite que a tela seja desligada. Exemplo de casos de uso: 
    download de um arquivo ou reprodução de áudio.
  * `prevent-display-sleep` - Impedir a exibição de entrar em modo suspenso. Mantém 
  o sistema e a tela ativos. Exemplo de caso de uso: reprodução de vídeo.

Retorna `Integer` - O ID do bloqueador que é atribuído a este bloqueador de energia

Inicia para impedir que o sistema entre no modo de baixa energia. Retorna um número inteiro 
identificando o bloqueador de economia de energia.

**Nota:** `prevent-display-sleep` Tem maior precedência sobre
`prevent-app-suspension`. Somente o tipo de precedência mais alto entra em vigor. Em 
outras palavras, `prevent-display-sleep` sempre tem procedência sobre
`prevent-app-suspension`.

Por exemplo, uma API chamada requests para `prevent-app-suspension`, e 
outra chamada B requests para `prevent-display-sleep`. `prevent-display-sleep`
será usado até que B pare seu request. Depois disso, `prevent-app-suspension`
é usado.

### `powerSaveBlocker.stop(id)`

* `id` Integer - O id do bloqueador de economia de energia retornado por `powerSaveBlocker.start`.

Interrompe o bloqueador de economia de energia especificado.

### `powerSaveBlocker.isStarted(id)`

* `id` Integer - O id do bloqueador de economia de energia retornado por `powerSaveBlocker.start`.

Retorna `Boolean` - Se o `powerSaveBlocker` correspondente foi iniciado.
