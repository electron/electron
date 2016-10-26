# Função `window.open`

> Abre uma nova janela e carrega uma URL.

Quando `window.open` é chamada para criar uma nova janela de uma página web, uma
nova instância de `BrowserWindow` será criada para a `url` e um proxy será
devolvido para o `window.open` para permitir que a página tenha controle
limitado sobre ele.

O proxy tem uma funcionalidade padrão implementada de forma limitada para ser
compatível com páginas web tradicionais. Para ter controle total de uma nova
janela, você deverá criar diretamente um `BrowserWindow`.

O `BrowserWindow` recém-criado herdará as opções da janela pai por padrão. Para
substituir as opções herdadas, você poderá defini-las na string `features`.

### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (opcional)
* `features` String (opcional)

Retorna `BrowserWindowProxy` - Cria uma nova janela e retorna uma instância da
classe `BrowserWindowProxy`.

A string `features` segue o formato de um navegador padrão, mas cada recurso
(feature) tem de ser um campo das opções do `BrowserWindow`.

**Notas:**

* Integração com Node sempre estará desativada no `window` aberto se ela
  estiver desativada na janela pai.
* Recursos fora do padrão (que não são manipulados pelo Chromium ou pelo
  Electron) fornecidos em `features` serão passados para qualquer manipulador
  de eventos `new-window` do `webContent` registrado no argumento
  `additionalFeatures`.

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

Envia uma mensagem para a janela pai com a origem especificada ou `*` para
nenhuma preferência de origem.

## Classe: BrowserWindowProxy

> Manipula a janela de navegador filha

O objeto `BrowserWindowProxy` é retornado de `window.open` e fornece uma
funcionalidade limitada para a janela filha.

### Métodos de Instância

O objeto `BrowserWindowProxy` possui os seguintes métodos de instância:

#### `win.blur()`

Remove o foco da janela filha.

#### `win.close()`

Fecha forçadamente a janela filha sem chamar seu evento de descarregamento.

#### `win.eval(code)`

* `code` String

Avalia o código na janela filha.

#### `win.focus()`

Foca na janela filha (traz a janela para frente).

#### `win.print()`

Invoca o diálogo de impressão na janela filha.

#### `win.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

Envia uma mensagem para a janela filha com a origem especificada ou `*` para
nenhuma preferência de origem.

Além desses métodos, a janela filha implementa o objeto `window.opener` sem
propriedades e com apenas um método.

### Propriedades de Instância

O objeto `BrowserWindowProxy` possui as seguintes propriedades de instância:

#### `win.closed`

Um booleano que é definido como true após a janela filha ser fechada.
