# BrowserWindow

A classe `BrowserWindow` lhe dá a habilidade de criar uma janela do browser. Por exemplo:

```javascript
const BrowserWindow = require('electron').BrowserWindow

var win = new BrowserWindow({ width: 800, height: 600, show: false })
win.on('closed', function () {
  win = null
})

win.loadURL('https://github.com')
win.show()
```

Você também pode criar uma janela sem o chrome utilizando a API [Frameless Window](../../../docs/api/frameless-window.md).

## Classe: BrowserWindow

`BrowserWindow` é um [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

Ela cria uma nova `BrowserWindow` com propriedades nativas definidas pelo `options`.

### `new BrowserWindow([options])`

`options` Objeto (opcional), propriedades:

* `width` Integer - Largura da janela em pixels. O padrão é `800`.
* `height` Integer - Altura da janela em pixels. O padrão é `600`.
* `x` Integer - Deslocamento da janela da esquerda da tela. O padrão é centralizar a janela.
* `y` Integer - Deslocamento da janela do topo da tela. O padrão é centralizar a janela.
* `useContentSize` Boolean - `width` e `height` seriam utilizados como o tamanho da página web, o que significa que o tamanho real da janela irá incluir o tamanho da moldura da janela e será um pouco maior. O padrão é `false`.
* `center` Boolean - Mostra a janela no centro da tela.
* `minWidth` Integer - Largura mínima da janela. O padrão é `0`.
* `minHeight` Integer - Altura mínima da janela. O padrão é `0`.
* `maxWidth` Integer - Largura máxima da janela. O padrão é sem limites.
* `maxHeight` Integer - Altura máxima da janela. O padrão é sem limites.
* `resizable` Boolean - Se é possível modificar o tamanho da janela. O padrão é `true`.
* `alwaysOnTop` Boolean - Se a janela deve sempre ficar à frente de outras janelas. O padrão é `false`.
* `fullscreen` Boolean - Se a janela deve estar em tela cheia. Quando definido como `false`, o botão de tela cheia estará escondido ou desabilitado no macOS. O padrão é `false`.
* `skipTaskbar` Boolean - Se deve mostrar a janela na barra de tarefas. O padrão é `false`.
* `kiosk` Boolean - Modo *kiosk*. O padrão é `false`.
* `title` String - Título padrão da janela. O padrão é `"Electron"`.
* `icon` [NativeImage](../../../docs/api/native-image.md) - O ícone da janela, quando omitido no Windows, o ícone do executável é utilizado como o ícone da janela.
* `show` Boolean - Se a janela deve ser exibida quando criada. O padrão é `true`.
* `frame` Boolean - Defina como `false` para criar uma [Frameless Window](../../../docs/api/frameless-window.md). O padrão é `true`.
* `acceptFirstMouse` Boolean - Se o *web view* aceita um evento *mouse-down* que ativa a janela simultaneamente. O padrão é `false`.
* `disableAutoHideCursor` Boolean - Se deve esconder o cursor quando estiver digitando. O padrão é `false`.
* `autoHideMenuBar` Boolean - Esconde a barra de menu automaticamente, a não ser que a tecla `Alt` seja pressionada. O padrão é `false`.
* `enableLargerThanScreen` Boolean - Possibilita que o tamanho da janela seja maior que a tela. O padrão é `false`.
* `backgroundColor` String - Cor de fundo da janela em hexadecimal, como `#66CD00` ou `#FFF`. Só é implementado no Linux e Windows. O padrão é `#000` (preto).
* `darkTheme` Boolean - Força a utilização do tema *dark* na janela, só funciona em alguns ambientes de desktop GTK+3. O padrão é `false`.
* `transparent` Boolean - Torna a janela [transparente](../../../docs/api/frameless-window.md). O padrão é `false`.
* `type` String - Define o tipo da janela, que aplica propriedades adicionais específicas da plataforma. Por padrão é indefinido e será criada uma janela de aplicativo comum. Possíveis valores:
  * No Linux, os tipos possíveis são `desktop`, `dock`, `toolbar`, `splash`,
    `notification`.
  * No macOS, os tipos possíveis são `desktop`, `textured`. O tipo `textured` adiciona a aparência degradê metálica (`NSTexturedBackgroundWindowMask`). O tipo  `desktop` coloca a janela no nível do fundo de tela do desktop (`kCGDesktopWindowLevel - 1`). Note que a janela `desktop` não irá receber foco, eventos de teclado ou mouse, mas você pode usar `globalShortcut` para receber entrada de dados ocasionalmente.
* `titleBarStyle` String, macOS - Define o estilo da barra de título da janela. Esta opção está suportada a partir da versão macOS 10.10 Yosemite. Há três possíveis valores:
  * `default` ou não definido, resulta na barra de título cinza opaca padrão do Mac.
  * `hidden` resulta numa barra de título oculta e a janela de conteúdo no tamanho máximo, porém a barra de título ainda possui os controles padrões de janela ("semáforo") no canto superior esquerdo.
  * `hidden-inset` resulta numa barra de título oculta com uma aparência alternativa onde os botões de semáforo estão ligeiramente mais longe do canto da janela.
* `webPreferences` Object - Configurações das características da página web, propriedades:
  * `nodeIntegration` Boolean - Se a integração com node está habilitada. O padrão é `true`.
  * `preload` String - Define um *script* que será carregado antes que outros scripts rodem na página. Este script sempre terá acesso às APIs do node, não importa se `nodeIntegration` esteja habilitada ou não. O valor deve ser o endereço absoluto do scriot. Quando `nodeIntegration` não está habilitada, o script `preload` pode reintroduzir símbolos globais Node de volta ao escopo global. Veja um exemplo [aqui](process.md#evento-loaded).
  * `partition` String - Define a sessão utilizada pela página. Se `partition` começa com `persist:`, a página irá utilizar uma sessão persistente disponível para todas as páginas do aplicativo com a mesma `partition`. Se não houver o prefixo `persist:`, a página irá utilizar uma sessão em memória. Ao utilizar a mesma `partition`, várias páginas podem compartilhar a mesma sessão. Se a `partition` for indefinida, então a sessão padrão do aplicativo será utilizada.
  * `zoomFactor` Number - O fator de *zoom* da página, `3.0` representa `300%`. O padrão é `1.0`.
  * `javascript` Boolean - Habilita suporte à JavaScript. O padrão é `true`.
  * `webSecurity` Boolean - Quando definido como `false`, irá desabilitar a política de mesma origem (Geralmente usando sites de teste por pessoas), e definir    `allowDisplayingInsecureContent` e `allowRunningInsecureContent` como
    `true` se estas duas opções não tiverem sido definidas pelo usuário. O padrão é `true`.
  * `allowDisplayingInsecureContent` Boolean - Permite que uma página https exiba conteúdo como imagens de URLs http. O padrão é `false`.
  * `allowRunningInsecureContent` Boolean - Permite que uma página https rode JavaScript, CSS ou plugins de URLs http. O padrão é `false`.
  * `images` Boolean - Habilita suporte a imagens. O padrão é `true`.
  * `java` Boolean - Habilita suporte a Java. O padrão é `false`.
  * `textAreasAreResizable` Boolean - Faz com que os elementos *TextArea* elements tenham tamanho variável. O padrão é `true`.
  * `webgl` Boolean - Habiltia suporte a WebGL. O padrão é `true`.
  * `webaudio` Boolean - Habilita suporte a WebAudio. O padrão é `true`.
  * `plugins` Boolean - Se plugins devem ser habilitados. O padrão é `false`.
  * `experimentalFeatures` Boolean - Habilita as características experimentais do Chromium. O padrão é `false`.
  * `experimentalCanvasFeatures` Boolean - Habilita as características experimentais de canvas do Chromium. O padrão é `false`.
  * `overlayScrollbars` Boolean - Habilita sobreposição das barras de rolagem. O padrão é `false`.
  * `overlayFullscreenVideo` Boolean - Habilita sobreposição do vídeo em tela cheia. O padrão é `false`.
  * `sharedWorker` Boolean - Habilita suporte a *Shared Worker*. O padrão é `false`.
  * `directWrite` Boolean - Habilita o sistema de renderização de fontes *DirectWrite* no Windows. O padrão é `true`.
  * `pageVisibility` Boolean - A página é forçada a permanecer visível ou oculta quando definido, em vez de refletir a visibilidade atual da janela. Usuários podem definir como `true` para evitar que os temporizadores do *DOM* sejam suprimidos. O padrão é `false`.

## Eventos

O objeto `BrowserWindow` emite os seguintes eventos:

**Nota:** Alguns eventos só estão disponíveis em sistemas operacionais específicos e estão rotulados como tal.

### Evento: 'page-title-updated'

Retorna:

* `event` Evento

Emitido quando o documento muda seu título, chamar `event.preventDefault()` previne que o título nativo da janela mude.

### Evento: 'close'

Retorna:

* `event` Evento

Emitido quando a janela for fechar. É emitido antes dos eventos `beforeunload` e `unload` do DOM. Chamar `event.preventDefault()` cancela o fechamento.

Normalmente você utiliza o manipulador de eventos do `beforeunload` para decidir se a janela deve ser fechada, que também será chamado quando a janela é recarregada. No Electron, retornar uma string vazia ou `false` cancela o fechamento. Por exemplo:

```javascript
window.onbeforeunload = function (e) {
  console.log('Não quero ser fechada')

  // Diferente de navegadores comuns, nos quais uma string deve ser retornada e
  // o usuário deve confirmar se a janela será fechada, o Electron dá mais opções
  // aos desenvolvedores. Retornar uma string vazia ou false cancela o fechamento.
  // Você também pode usar a API de diálogo para deixar que o usuário confirme o
  // fechamento do aplicativo.
  e.returnValue = false
}
```

### Evento: 'closed'

Emitido quando a janela é fechada. Após você ter recebido este evento, você deve remover a referência da janela e evitar utilizá-la.

### Evento: 'unresponsive'

Emitido quando a página web para de responder.

### Evento: 'responsive'

Emitido quando a página web que não respondia volta a responder novamente.

### Evento: 'blur'

Emitido quando a janela perde foco.

### Evento: 'focus'

Emitido quando a janela ganha foco.

### Evento: 'maximize'

Emitido quando a janela é maximizada.

### Evento: 'unmaximize'

Emitido quando a janela sai do estado maximizado.

### Evento: 'minimize'

Emitido quando a janela é minimizada.

### Evento: 'restore'

Emitido quando a janela é restaurada do estado minimizado.

### Evento: 'resize'

Emitido quando o tamanho da janela está sendo alterado.

### Evento: 'move'

Emitido quando está sendo movida para uma nova posição.

__Note__: No macOS este evento é apenas um apelido de `moved`.

### Evento: 'moved' _macOS_

Emitido uma vez quando a janela é movida para uma nova posição.

### Evento: 'enter-full-screen'

Emitido quando a janela entra no estado tela cheia.

### Evento: 'leave-full-screen'

Emitido quando a janela sai do estado de tela cheia.

### Evento: 'enter-html-full-screen'

Emitido quando a janela entra no estado tela cheia, ocasionado por uma api de html.

### Evento: 'leave-html-full-screen'

Emitido quando a janela sai do estado de tela cheia, ocasionado por uma api de html.

### Evento: 'app-command' _Windows_

Emitido quando um [App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx) é invocado. Estes estão tipicamente relacionado às teclas de mídia do teclado, ou comandos do browser, assim como o botão "Voltar" existente em alguns modelos de mouse no Windows.

```javascript
someWindow.on('app-command', function (e, cmd) {
  // Navega a janela 'para trás' quando o usuário pressiona o botão voltar do mouse
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack()
  }
})
```

## Métodos

O objeto `BrowserWindow` possui os seguintes métodos:

### `BrowserWindow.getAllWindows()`

Retorna um array de todas as janelas abertas.

### `BrowserWindow.getFocusedWindow()`

Retorna a janela que está focada no aplicativo.

### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](../../../docs/api/web-contents.md)

Acha uma janela de acordo com os `webContents` que ela possui.

### `BrowserWindow.fromId(id)`

* `id` Integer

Acha uma janela de acordo com o seu ID.

### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

Adiciona a extenção DevTools localizada no endereço `path`, e retorna o nome da extenção.

A extenção será lembrada, então você só precisa chamar esta API uma vez, esta API não é para uso de programação.

### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

Remove a extenção DevTools cujo nome é `name`.

## Propriedades de Instância

Objetos criados com `new BrowserWindow` possuem as seguintes propriedades:

```javascript
// Neste exemplo `win` é a nossa instância
var win = new BrowserWindow({ width: 800, height: 600 })
```

### `win.webContents`

Todas os eventos e operações relacionados à pagina web serão feitos através do objeto `WebContents` que a esta janela possui.

Veja a [documentação do `webContents`](../../../docs/api/web-contents.md) para seus métodos e eventos.

### `win.id`

O ID único desta janela.

## Métodos de instância

Objetos criados com `new BrowserWindow` possuem os seguintes métodos de instância:

**Nota:** Alguns métodos só estão disponíveis em sistemas operacionais específicos e estão rotulados como tal.

### `win.destroy()`

Força o fechamento da janela, os eventos `unload` e `beforeunload` não serão emitidos para a página web, e o evento `close` também não será emitido para esta janela, mas garante que o evento `closed` será emitido.

Você só deve utilizar este método quando o processo renderizador (página web) congelar.

### `win.close()`

Tenta fechar a janela, tem o mesmo efeito que o usuário manualmente clicar o botão de fechar a janela. Entretanto, a página web pode cancelar o fechamento, veja o [evento close](#evento-close).

### `win.focus()`

Foca na janela.

### `win.isFocused()`

Retorna um boolean, indicando se a janela está com foco.

### `win.show()`

Exibe e dá foco à janela.

### `win.showInactive()`

Exibe a janela, porém não dá foco à ela.

### `win.hide()`

Esconde a janela.

### `win.isVisible()`

Retorna um boolean, indicando se a janela está visível para o usuário.

### `win.maximize()`

Maximiza a janela.

### `win.unmaximize()`

Sai do estado maximizado da janela.

### `win.isMaximized()`

Retorna um boolean, indicando se a janela está maximizada.

### `win.minimize()`

Minimiza a janela. Em algumas plataformas, a janela minimizada será exibida no *Dock*.

### `win.restore()`

Restaura a janela do estado minimizado para o estado anterior.

### `win.isMinimized()`

Retorna um boolean, indicando se a janela está minimizada.

### `win.setFullScreen(flag)`

* `flag` Boolean

Define se a janela deve estar em modo tela cheia.

### `win.isFullScreen()`

Retorna um boolean, indicando se a janela está em modo tela cheia.

### `win.setAspectRatio(aspectRatio[, extraSize])` _macOS_

* `aspectRatio` A proporção que queremos manter para uma porção do conteúdo da *view*.
* `extraSize` Object (opcional) - O tamanho extra não incluído enquanto a proporção é mantida. Propriedades:
  * `width` Integer
  * `height` Integer

Isto fará com que a janela mantenha sua proporção. O `extraSize` permite que o desenvolvedor tenha espaço, definido em pixels, não incluídos no cálculo da proporção. Esta API já leva em consideração a diferença entre o tamanho da janela e o tamanho de seu conteúdo.

Suponha que exista uma janela normal com um *player* de vídeo HD e seus controles associados. Talvez tenha 15 pixels de controles na borda esquerda, 25 pixels de controles na borda direita e 50 abaixo do player. Para que seja mantida a proporção de 16:9 (proporção padrão para HD em 1920x1080) no próprio player, nós chamaríamos esta função com os argumentos 16/9 e [ 40, 50 ]. Para o segundo argumento, não interessa onde a largura e altura extras estão no conteúdo da view--apenas que elas existam. Apenas some qualquer área de largura e altura extras que você tem dentro da view do conteúdo.

### `win.setBounds(options)`

`options` Object, propriedades:

* `x` Integer
* `y` Integer
* `width` Integer
* `height` Integer

Redefine o tamanho e move a janela para `width`, `height`, `x`, `y`.

### `win.getBounds()`

Retorna um objeto que contém a largura, altura, posição x e y da janela.

### `win.setSize(width, height)`

* `width` Integer
* `height` Integer

Redefine o tamanho da janela para largura `width` e altura `height`.

### `win.getSize()`

Retorna um array que contém a largura e altura da janela.

### `win.setContentSize(width, height)`

* `width` Integer
* `height` Integer

Redefine a área de cliente da janela (a página web) para largura `width` e altura `height`.

### `win.getContentSize()`

Retorna um array que contém a largura e altura da área de cliente da janela.

### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

Define o tamanho mínimo da janela para largura `width` e altura `height`.

### `win.getMinimumSize()`

Retorna uma array que contém o tamanho mínimo da largura e altura da janela.

### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

Define o tamanho máximo da janela para largura `width` e altura `height`.

### `win.getMaximumSize()`

Retorna uma array que contém o tamanho máximo da largura e altura da janela.

### `win.setResizable(resizable)`

* `resizable` Boolean

Define se a janela pode ter seu tamanho redefinido manualmente pelo usuário.

### `win.isResizable()`

Retorna um boolean indicando se a janela pode ter seu tamanho redefinido manualmente pelo usuário.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

Define se a janela deve estar sempre em cima de outras janelas. Após definir isso, a janela continua sendo uma janela normal, não uma janela *toolbox* que não pode receber foco.

### `win.isAlwaysOnTop()`

Retorna um boolean indicando se a janela está sempre em cima de outras janelas.

### `win.center()`

Move a janela para o centro da tela.

### `win.setPosition(x, y)`

* `x` Integer
* `y` Integer

Move a janela para `x` e `y`.

### `win.getPosition()`

Retorna um array com a posição atual da janela.

### `win.setTitle(title)`

* `title` String

Muda o título da janela nativa para `title`.

### `win.getTitle()`

Retorna o título da janela nativa.

**Nota:** O título da página web pode ser diferente do título da janela nativa.

### `win.flashFrame(flag)`

* `flag` Boolean

Inicia ou para de piscar a janela para atrair a atenção do usuário.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

Faz com que a janela não apareça na barra de tarefas.

### `win.setKiosk(flag)`

* `flag` Boolean

Entra ou sai do modo *kiosk*.

### `win.isKiosk()`

Retorna um boolean indicando se janela está no modo *kiosk*.

### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

Engancha uma mensagem de janela. O `callback` é chamado quando a mensagem é recebida no WndProc.

### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

Retorna `true` ou `false` indicando se a mensagem está enganchada ou não.

### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

Desengancha a mensagem de janela.

### `win.unhookAllWindowMessages()` _Windows_

Desengancha todas as mensagens de janela.

### `win.setRepresentedFilename(filename)` _macOS_

* `filename` String

Define o endereço do arquivo que a janela representa, e o ícone do arquivo será exibido na barra de título da janela.

### `win.getRepresentedFilename()` _macOS_

Retorna o endereço do arquivo que a janela representa.

### `win.setDocumentEdited(edited)` _macOS_

* `edited` Boolean

Define se o documento da janela foi editado, e o ícone na barra de título se torna cinza quando definido como `true`.

### `win.isDocumentEdited()` _macOS_

Retorna um boolean indicando se o documento da janela foi editado.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (opcional)- A área da página a ser capturada. Propriedades:
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

Captura uma imagem da página dentro do `rect`. Após completar, `callback` será chamada com `callback(imagem)`. `imagem` é uma instância de [NativeImage](../../../docs/api/native-image.md) que guarda dados sobre a imagem. Omitir `rect` captura toda a página visível.

### `win.loadURL(url[, options])`

Igual a `webContents.loadURL(url[, options])`.

### `win.reload()`

Igual a `webContents.reload`.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

Define `menu` como a barra de menu da janela, definir como `null` remove a barra de menu.

### `win.setProgressBar(progress)`

* `progress` Double

Define o valor do progresso na barra de progresso. Extensão válida é [0, 1.0].

Remove a barra de progresso quando `progress` < 0.
Muda para o modo indeterminado quando `progress` > 1.

Na plataforma Linux, apenas suporta o ambiente de desktop Unity, você precisa definir o nome do arquivo como `*.desktop` no campo `desktopName` no `package.json`. Por padrão, irá assumir `app.getName().desktop`.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](../../../docs/api/native-image.md) - o ícone a ser exibido no canto inferior direito da barra de tarefas. Se este parâmetro for `null`, a sobreposição é eliminada.
* `description` String - uma descrição que será providenciada a leitores de tela de acessibilidade.

Define uma sobreposição de 16px sobre o ícone da barra de tarefas atual, geralmente utilizado para indicar algum tipo de status de aplicação, ou notificar passivamente o usuário.

### `win.setThumbarButtons(buttons)` _Windows 7+_

`buttons` Array de objetos `button`:

`button` Object, propriedades:

* `icon` [NativeImage](../../../docs/api/native-image.md) - O ícone exibido na barra de ferramentas miniatura.
* `tooltip` String (opcional) - O texto do balão de dica do botão.
* `flags` Array (opcional) - Controla estados e comportamentos específicos do botão. Utiliza `enabled` por padrão. Pode incluir as seguintes strings:
  * `enabled` - O botão está ativo e disponível para o usuário.
  * `disabled` - O botão está desabilitado. Está presente, mas possui um estado visual indicando que não irá responder às ações do usuário.
  * `dismissonclick` - Quando o botão é clicado, o *flyout* do botão da barra de tarefas fecha imediatamente.
  * `nobackground` - Não desenhe a borda do botão, apenas utilize a imagem.
  * `hidden` - O botão não é exibido para o usuário.
  * `noninteractive` - O botão está habilitado, mas não interage; Não é exibido o estado de botão pressionado. Este valor está destinado a instâncias nas quais o botão é utilizado em uma notificação.
* `click` - Função

Adiciona uma barra de ferramentes miniatura com um conjunto de botões específicos à imagem miniatura de uma janela no layout de um botão de barra de tarefas. Retorna um objeto `Boolean` indicando se a miniatura foi adicionada com sucesso.

O número de botões na barra de ferramentas miniatura não deve ser maior que 7 devido ao espaço limitado. Uma vez que você define a barra de ferramentas miniatura, ela não pode ser removida por causa da limitação da plataforma. Mas você pode chamar a API com um array vazio para limpar todos os botões.

### `win.showDefinitionForSelection()` _macOS_

Mostra um dicionário *pop-up* que procura a palavra selecionada na página.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

Define se a barra de menu da janela deve se esconder automaticamente. Uma vez que for definida, a barra de menu só será exibida quando usuários pressionarem a tecla `Alt`.

Se a barra de menu já estiver visível, chamar `setAutoHideMenuBar(true)` não irá escondê-la imediatamente.

### `win.isMenuBarAutoHide()`

Retorna um boolean indicando se a barra de menu se esconde automaticamente.

### `win.setMenuBarVisibility(visible)` _Windows_ _Linux_

* `visible` Boolean

Define se a barra de menu deve ser visível. Se a barra de menu se esconde automaticamente, os usuários ainda podem exibí-la ao pressionar a tecla `Alt`.

### `win.isMenuBarVisible()`

Retorna um boolean indicando se a barra de menu está visível.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

Define se a janela deve estar visível em todos os *workspaces*.

**Nota:** Esta API não faz nada no Windows.

### `win.isVisibleOnAllWorkspaces()`

Retorna um boolean indicando se a janela está visível em todos os *workspaces*.

**Nota:** Esta API sempre retorna `false` no Windows.
