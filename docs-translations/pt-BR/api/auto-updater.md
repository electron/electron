# autoUpdater

Este módulo oferece uma interface para o framework de atualização automática `Squirrel`.

## Avisos sobre Plataformas

Embora o `autoUpdater` ofereça uma API uniforme para diferentes plataformas, existem diferenças sutis em cada plataforma.

### macOS

No macOS, o módulo `autoUpdater` é construído sobre o [Squirrel.Mac][squirrel-mac], o que significa que você não precisa de nenhuma configuração especial para fazê-lo funcionar. Para requerimentos de servidor, você pode ler [Server Support][server-support].

### Windows

No Windows, você deve instalar seu aplicativo na máquina de um usuário antes que possa usar o auto-updater, então é recomendado utilizar o módulo [grunt-electron-installer][installer] para gerar um instalador do Windows.

O instalador gerado com Squirrel irá criar um ícone de atalho com um [Application User Model ID][app-user-model-id] no formato `com.squirrel.PACKAGE_ID.YOUR_EXE_WITHOUT_DOT_EXE`, por exemplo: `com.squirrel.slack.Slack` e `com.squirrel.code.Code`. Você precisa usar o mesmo ID para seu aplicativo a API `app.setAppUserModelId`, senão o Windows não conseguirá fixar seu aplicativo corretamente na barra de tarefas.

A configuração do servidor também é diferente do macOS. Você pode ler a documentação do [Squirrel.Windows][squirrel-windows] para mais detalhes.

### Linux

Não há suporte nativo do auto-updater para Linux, então é recomendado utilizar o gerenciador de pacotes da distribuição para atualizar seu aplicativo.

## Eventos

O objeto `autoUpdater` emite os seguintes eventos:

### Evento: 'error'

Retorna:

* `error` Error

Emitido quando há um erro durante a atualização.

### Evento: 'checking-for-update'

Emitido quando está verificando se uma atualização foi inicializada.

### Evento: 'update-available'

Emitido quando há uma atualização disponível. A autalização é baixada automaticamente.

### Evento: 'update-not-available'

Emitido quando não há uma atualização disponível.

### Evento: 'update-downloaded'

Retorna:

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateURL` String

Emitido quando uma atualização foi baixada.

No Windows apenas `releaseName` está disponível.

## Métodos

O objeto `autoUpdater` possui os seguintes métodos:

### `autoUpdater.setFeedURL(url)`

* `url` String

Define a `url` e inicializa o auto-updater. A `url` não pode ser alterada uma vez que foi definida.

### `autoUpdater.checkForUpdates()`

Pergunta ao servidor se há uma atualização. Você deve chamar `setFeedURL` antes de usar esta API.

### `autoUpdater.quitAndInstall()`

Reinicia o aplicativo e instala a atualização após esta ter sido baixada. Só deve ser chamado após o `update-downloaded` ter sido emitido.

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/atom/grunt-electron-installer
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
