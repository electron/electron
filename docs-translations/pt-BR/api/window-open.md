# The `window.open` function

Qunado `window.open` é chamado para criar uma nova janela de uma pagina web uma nova instância de `BrowserWindow` será criado para a `url` e um proxy será devolvido para o `windows.open`, para permitir que a página tenha limitado controle sobre ele.

O proxy tem funcionalidade limitada padrão implementada para ser compatível com as páginas web tradicionais.
Para controle total da nova janela você deveria criar um `BrowserWindow` diretamente


The newly created `BrowserWindow` will inherit parent window's options by
default, to override inherited options you can set them in the `features`
string.

O recém-criado `BrowserWindow` herdará as opções da janela pai por padrão, para substituir as opções herdadas você pode definilos no `features`(string).
### `window.open(url[, frameName][, features])`

* `url` String
* `frameName` String (opcional)
* `features` String (opcional)

Cria uma nova janela e retorna uma instância da classe `BrowserWindowProxy'.

A string `features` segue o formato padrão do browser, mas cada recurso (feature) tem que ser um campo de opções do `BrowserWindow`.

### `window.opener.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

Envia uma mensagem para a janela pai com a origem especificada ou `*` preferência de origem não especificada.
Sends a message to the parent window with the specified origin or `*`
origin preference.

## Class: BrowserWindowProxy

O objeto `BrowserWindowProxy` é retornado de `window.open` e fornece uma funcionalidade limitada para a janela filha.

### `BrowserWindowProxy.blur()`

Remove o foco da janela filha.

### `BrowserWindowProxy.close()`

Forçadamente fecha a janela filha sem chamar o evento de descarregamento.

### `BrowserWindowProxy.closed`

Define como true após a janela filha ficar fechada.

### `BrowserWindowProxy.eval(code)`

* `code` String

Avalia o código na jánela filha.

### `BrowserWindowProxy.focus()`

Concentra-se a janela filha (traz a janela para frente)
### `BrowserWindowProxy.postMessage(message, targetOrigin)`

* `message` String
* `targetOrigin` String

Sends a message to the child window with the specified origin or `*` for no
origin preference.

In addition to these methods, the child window implements `window.opener` object
with no properties and a single method.
