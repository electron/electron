# app

O módulo `app` é responsável por controlar o ciclo de vida do aplicativo.

O exemplo a seguir mostra como fechar o aplicativo quando a última janela é fechada: 

```javascript
const app = require('electron').app
app.on('window-all-closed', function () {
  app.quit()
})
```

## Eventos

O objeto `app` emite os seguintes eventos:

### Evento: 'will-finish-launching'

Emitido quando o aplicativo finaliza a inicialização básica. No Windows e no Linux,
o evento `will-finish-launching` é o mesmo que o evento `ready`; No macOS,
esse evento representa a notificação `applicationWillFinishLaunching` do `NSApplication`.
Normalmente aqui seriam criados *listeners* para os eventos `open-file` e `open-url`, e inicializar o *crash reporter* e atualizador automático.

Na maioria dos casos, você deve fazer tudo no manipulador de eventos do `ready`.

### Evento: 'ready'

Emitido quando o Electron finaliza a inicialização.

### Evento: 'window-all-closed'

Emitido quando todas as janelas forem fechadas.

Este evento só é emitido quando o aplicativo não for fechar. Se o
usuário pressionou`Cmd + Q`, ou o desenvolvedor chamou `app.quit()`,
o Electron tentará primeiro fechar todas as janelas e então emitir o
evento `will-quit`, e neste caso o evento `window-all-closed` não
seria emitido.

### Evento: 'before-quit'

Retorna:

* `event` Event

Emitido antes que o aplicativo comece a fechar suas janelas.
Chamar `event.preventDefault()` irá impedir o comportamento padrão,
que é terminar o aplicativo.

### Evento: 'will-quit'

Retorna:

* `event` Event

Emitido quando todas as janelas foram fechadas e o aplicativo irá finalizar.
Chamar `event.preventDefault()` irá impedir o comportamento padrão,
que é terminar o aplicativo.

Veja a descrição do evento `window-all-closed` para as diferenças entre o
evento `will-quit` e `window-all-closed`.

### Evento: 'quit'

Emitido quando o aplicativo está finalizando.

### Evento: 'open-file' _macOS_

Retorna:

* `event` Event
* `path` String

Emitido quando o usuário deseja abrir um arquivo com o aplicativo. O evento
`open-file` normalmente é emitido quando o aplicativo já está aberto e o S.O.
quer reutilizar o aplicativo para abrir o arquivo. `open-file` também é emitido
quando um arquivo é jogado no *dock* e o aplicativo ainda não está rodando.
Certifique-se de utilizar um *listener* para o evento `open-file` cedo na
inicialização do seu aplicativo para cuidar deste caso (antes mesmo do evento
`ready` ser emitido).

Você deve chamar `event.preventDefault()` se quiser cuidar deste caso.

No Windows, você deve fazer o *parse* do `process.argv` para pegar o
endereço do arquivo.

### Evento: 'open-url' _macOS_

Retorna:

* `event` Event
* `url` String

Emitido quando o usuário deseja abrir uma URL com o aplicativo. O esquema deve
ser registrado para ser aberto pelo seu aplicativo.

Você deve chamar `event.preventDefault()` se quiser cuidar deste caso.

### Evento: 'activate' _macOS_

Retorna:

* `event` Event
* `hasVisibleWindows` Boolean

Emitido quando o aplicativo é ativado, que normalmente acontece quando o ícone
do aplicativo no *dock* é clicado.

### Evento: 'browser-window-blur'

Retorna:

* `event` Event
* `window` BrowserWindow

Emitido quando uma [browserWindow](../../../docs/api/browser-window.md) fica embaçada.

### Evento: 'browser-window-focus'

Retorna:

* `event` Event
* `window` BrowserWindow

Emitido quando uma [browserWindow](../../../docs/api/browser-window.md) é focada.

### Evento: 'browser-window-created'

Retorna:

* `event` Event
* `window` BrowserWindow

Emitido quando uma nova [browserWindow](../../../docs/api/browser-window.md) é criada.

### Evento: 'certificate-error'

Returns:

* `event` Event
* `webContents` [WebContents](../../../docs/api/web-contents.md)
* `url` URL
* `error` String - O código de erro
* `certificate` Object
  * `data` Buffer - dados codificados PEM
  * `issuerName` String
* `callback` Function

Emitido quando há uma falha na verificação do `certificate` para a `url`,
para confiar no certificado, você deve impedir o comportamento padrão com
`event.preventDefault()` e chamar `callback(true)`.

```javascript
session.on('certificate-error', function (event, webContents, url, error, certificate, callback) {
  if (url == 'https://github.com') {
    // Lógica de verificação.
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})
```

### Evento: 'select-client-certificate'

Retorna:

* `event` Event
* `webContents` [WebContents](../../../docs/api/web-contents.md)
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - dados codificados PEM
  * `issuerName` String - Nome Comum do Emissor
* `callback` Function

Emitido quando um certificado de cliente é requisitado.

A `url` corresponde à entrada de navegação requisitando o certificado do
cliente e `callback` precisa ser chamada com uma entrada filtrada da lista.
Usar `event.preventDefault()` impede o aplicativo de usar o primeiro certificado
da memória.

```javascript
app.on('select-client-certificate', function (event, webContents, url, list, callback) {
  event.preventDefault()
  callback(list[0])
})
```

### Evento: 'login'

Retorna:

* `event` Event
* `webContents` [WebContents](../../../docs/api/web-contents.md)
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

Emitido quando `webContents` deseja fazer autenticação básica.

O comportamento padrão é cancelar todas as autenticações, para sobrescrever
isto, você deve impedir esse comportamento com `event.preventDefault()` e
chamar `callback(username, password)` com as credenciais.

```javascript
app.on('login', function (event, webContents, request, authInfo, callback) {
  event.preventDefault()
  callback('username', 'secret')
})
```

### Evento: 'gpu-process-crashed'

Emitido quando o processo da gpu falha.

## Métodos

O objeto `app` possui os seguintes métodos:

**Nota:** Alguns métodos só estão disponíveis em sistemas operacionais específicos e estão rotulados como tal.

### `app.quit()`

Tente fechar todas as janelas. O evento `before-quit` será emitido primeiro. Se todas
as janelas fecharem com sucesso, o evento `will-quit` será emitido e por padrão o
aplicativo irá terminar.

Este método garante que todos os manipuladores de evento `beforeunload` e `unload`
sejam corretamente executados. É possível que uma janela cancele o processo de
encerramento ao retornar `false` no manipulador de evento `beforeunload`.

### `app.exit(exitCode)`

* `exitCode` Integer

Finaliza imediatamente com `exitCode`.

Todas as janelas serão fechadas imediatamente sem perguntar ao usuário, e os eventos
`before-quit` e `will-quit` não serão emitidos.

### `app.getAppPath()`

Retorna o atual diretório do aplicativo.

### `app.getPath(name)`

* `name` String

Retorna um endereço para um diretório especial ou arquivo associado com `nome`.
Numa falha um `Error` é lançado.

Você pode requisitar os seguintes endereços pelo nome:

* `home` Diretório *home* do usuário.
* `appData` Diretório de dados do aplicativo por usuário, que por padrão aponta para:
  * `%APPDATA%` no Windows
  * `$XDG_CONFIG_HOME` ou `~/.config` no Linux
  * `~/Library/Application Support` no macOS
* `userData` O diretório para guardar os arquivos de configuração do seu aplicativo,  que por padrão é o diretório `appData` concatenado com o nome do seu aplicativo.
* `temp` Diretório temporário.
* `exe` O arquivo executável atual.
* `module` A biblioteca `libchromiumcontent`.
* `desktop` O diretório *Desktop* do usuário atual.
* `documents` Diretório "Meus Documentos" do usuário.
* `downloads` Diretório dos downloads do usuário.
* `music` Diretório de músicas do usuário.
* `pictures` Diretório de imagens do usuário.
* `videos` Diretório de vídeos do usuário.

### `app.setPath(name, path)`

* `name` String
* `path` String

Sobrescreve o `path` para um diretório especial ou arquivo associado com `name`.
Se o endereço especifica um diretório não existente, o diretório será criado por
este método. Numa falha um `Error` é lançado.

Você pode sobrescrever apenas endereços com um `name` definido em `app.getPath`.

Por padrão, *cookies* e *caches* de páginas web serão guardadas no diretório `userData`. Se você quiser mudar esta localização, você deve sobrescrever o
endereço `userData` antes que o evento `ready` do módulo `app` seja emitido.

### `app.getVersion()`

Retorna a versão do aplicativo carregado. Se nenhuma versão for encontrada no
arquivo `package.json` do aplicativo, a versão do pacote ou executável atual é
retornada.

### `app.getName()`

Retorna o nome do aplicativo atual, que é o nome no arquivo `package.json` do
aplicativo.

Normalmente o campo `name` do `package.json` é um nome curto em letras minúsculas,
de acordo com as especificações de módulos npm. Normalmente você deve também
especificar um campo `productName`, que é o nome completo em letras maiúsculas do
seu aplicativo, e que será preferido ao `name` pelo Electron.

### `app.getLocale()`

Retorna a localidade atual do aplicativo.

### `app.addRecentDocument(path)` _macOS_ _Windows_

* `path` String

Adiciona `path` à lista de documentos recentes.

Esta lista é gerenciada pelo S.O.. No Windows você pode visitar a lista pela
barra de tarefas, e no macOS você pode visita-la pelo *dock*.

### `app.clearRecentDocuments()` _macOS_ _Windows_

Limpa a lista de documentos recentes.

### `app.setUserTasks(tasks)` _Windows_

* `tasks` Array - Vetor de objetos `Task`

Adiciona `tasks` à categoria [Tasks][tasks] do JumpList no Windows.

`tasks` é um vetor de objetos `Task` no seguinte formato:

`Task` Object
* `program` String - Endereço do programa a ser executado, normalmente você deve especificar `process.execPath` que abre o programa atual.
* `arguments` String - Os argumentos de linha de comando quando `program` é executado.
* `title` String - A string a ser exibida em uma JumpList.
* `description` String - Descrição desta *task*.
* `iconPath` String - O endereço absoluto para um ícone a ser exibido em uma JumpList, que pode ser um arquivo arbitrário que contém um ícone. Normalmente você pode especificar `process.execPath` para mostrar o ícone do programa.
* `iconIndex` Integer - O índice do ícone do arquivo do icone. Se um arquivo de ícone consiste de dois ou mais ícones, defina este valor para identificar o ícone. Se o arquivo de ícone consiste de um ícone apenas, este valor é 0.

### `app.allowNTLMCredentialsForAllDomains(allow)`

* `allow` Boolean

Define dinamicamente se sempre envia credenciais para HTTP NTLM ou autenticação *Negotiate* - normalmente, o Electron irá mandar apenas credenciais NTLM/Kerberos para URLs que se enquadram em sites "Intranet Local" (estão no mesmo domínio que você).
Entretanto, esta detecção frequentemente falha quando redes corporativas são mal configuradas, então isso permite optar por esse comportamento e habilitá-lo para todas as URLs.

### `app.makeSingleInstance(callback)`

* `callback` Function

Este método faz da sua aplicação uma Aplicação de Instância Única - invés de permitir múltiplas instâncias do seu aplicativo rodarem, isto irá assegurar que apenas uma única instância do seu aplicativo rodará, e outras instâncias sinalizam esta instância e finalizam.

`callback` será chamado com `callback(argv, workingDirectory)` quando uma segunda instância tenha sido executada. `argv` é um vetor de argumentos de linha de comando da segunda instância, e `workingDirectory` é o atual endereço de seu diretório.
Normalmente aplicativos respondem à isso não minimizando sua janela primária e dando foco à ela.

É garantida a execução do `callback` após o evento `ready` do `app` ser emitido.

Este método retorna `false` caso seu processo seja a instância primária do aplicativo e seu aplicativo deve continuar carregando. E retorna `true` caso seu processo tenha enviado seus parâmetros para outra instância, e você deve imediatamente finalizar.

No macOS o sistema enforça instância única automaticamente quando usuários tentam abrir uma segunda instância do seu aplicativo no *Finder*, e os eventos `open-file` e `open-url` serão emitidos para isso. Entretanto, quando usuários inicializam seu aplicativo na linha de comando, o mecanismo de instância única do sistema será ignorado e você terá de utilizar esse método para assegurar-se de ter uma instância única.

Um exemplo de ativação da janela de primeira instância quando uma segunda instância inicializa:

```js
var myWindow = null;

var shouldQuit = app.makeSingleInstance(function(commandLine, workingDirectory) {
  // Alguém tentou rodar uma segunda instância, devemos focar nossa janela
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore();
    myWindow.focus();
  }
  return true;
});

if (shouldQuit) {
  app.quit();
  return;
}

// Cria myWindow, carrega o resto do aplicativo, etc...
app.on('ready', function() {
});
```

### `app.setAppUserModelId(id)` _Windows_

* `id` String

Muda o [Application User Model ID][app-user-model-id] para `id`.

### `app.commandLine.appendSwitch(switch[, value])`

Adiciona uma opção (com `value` opcional) à linha de comando do Chromium.

**Nota:** Isto não irá afetar `process.argv`, e é utilizado principalmente por desenvolvedores para controlar alguns comportamentos de baixo nível do Chromium.

### `app.commandLine.appendArgument(value)`

Adiciona um argumento à linha de comando do Chromium. O argumento será passado com aspas corretamente.

**Nota:** Isto não irá afetar `process.argv`.

### `app.dock.bounce([type])` _macOS_

* `type` String (opcional) - Pode ser `critical` ou `informational`. O padrão é
 `informational`

Quando `critical` é passado, o ícone do *dock* irá pular até que o aplicativo se torne ativo ou a requisição seja cancelada.

Quando `informational` é passado, o ícone do *dock* irá pular por um segundo.
Entretanto, a requisição se mantém ativa até que o aplicativo se torne ativo ou a requisição seja cancelada.

Retorna um ID representando a requisição.

### `app.dock.cancelBounce(id)` _macOS_

* `id` Integer

Cancela o salto do `id`.

### `app.dock.setBadge(text)` _macOS_

* `text` String

Define a string a ser exibida na área de *badging* do *dock*.

### `app.dock.getBadge()` _macOS_

Retorna a string da *badge* do *dock*.

### `app.dock.hide()` _macOS_

Esconde o ícone do *dock*.

### `app.dock.show()` _macOS_

Exibe o ícone do *dock*.

### `app.dock.setMenu(menu)` _macOS_

* `menu` Menu

Define o [menu do dock][dock-menu] do aplicativo.

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
