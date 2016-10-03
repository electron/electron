# Integração com o ambiente desktop

Diferentes sistemas operacionais possuem diferentes formas de integrar
aplicacões desktop em seus ambientes. Por exemplo, no Windows, as aplicações podem
inserir atalhos no JumpList da barra de tarefas, no Mac, aplicações podem implementar um
menu customizado na dock.

Este guia explica como integrar suas aplicações no ambiente desktop com a API
do Electron.

## Documentos Recentes (Windows & macOS)

O Windows e o macOS disponibilizam um acesso fácil para a lista de arquivos
abertos recentemente pela aplicação através do JumpList ou Dock Menu respectivamente.

__JumpList:__

![JumpList Recent Files](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__Dock menu da Aplicação:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

Para adicionar um arquivo para os documentos recentes, você pode usar a API
[app.addRecentDocument][addrecentdocument]:

```javascript
var app = require('app')
app.addRecentDocument('/Users/USERNAME/Desktop/work.type')
```

E você pode usar a API [app.clearRecentDocuments][clearrecentdocuments] para
limpar a lista de documentos recentes.

```javascript
app.clearRecentDocuments()
```

### Notas para Windows

A fim de ser possível usar estas funcionalidades no Windows, sua aplicação deve
estar registrada como um handler daquele tipo de documento, caso contrário, o
arquivo não será exibido no JumpList mesmo depois de você ter adicionado isto.
Você pode encontrar qualquer coisa sobre o registro da aplicacão em
[Application Registration][app-registration].

Quando um usuário clica em um arquivo na JumpList, uma nova instância da sua aplicacão
deve ser iniciada com o caminho do arquivo adicionado como um argumento de
linha de comando.

### Notas para macOS

Quando um arquivo for requisitado pelo menu de documentos recentes, o evento `open-file`
do módulo `app` irá ser emitido.

## Dock Menu customizado (macOS)

macOS permite que desenvolvedores especifiquem um menu customizado para a dock,
que normalmente contém alguns atalhos para as funcionalidades mais utilizadas
da sua aplicação.

__Dock menu do Terminal.app:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

Para criar seu Dock Menu customizado, você pode usar a API `app.dock.setMenu`,
ela está disponível apenas no macOS:

```javascript
var app = require('app')
var Menu = require('menu')
var dockMenu = Menu.buildFromTemplate([
  { label: 'New Window', click: function () { console.log('New Window') } },
  { label: 'New Window with Settings', submenu: [
    { label: 'Basic' },
    { label: 'Pro'}
  ]},
  { label: 'New Command...'}
])
app.dock.setMenu(dockMenu)
```

## Tarefas do Usuário (Windows)

No Windows você pode especificar ações customizadas na categoria `Tarefas` do JumpList,
esse texto foi copiado do MSDN:

> Applications define tasks based on both the program's features and the key
> things a user is expected to do with them. Tasks should be context-free, in
> that the application does not need to be running for them to work. They
> should also be the statistically most common actions that a normal user would
> perform in an application, such as compose an email message or open the
> calendar in a mail program, create a new document in a word processor, launch
> an application in a certain mode, or launch one of its subcommands. An
> application should not clutter the menu with advanced features that standard
> users won't need or one-time actions such as registration. Do not use tasks
> for promotional items such as upgrades or special offers.
>
> It is strongly recommended that the task list be static. It should remain the
> same regardless of the state or status of the application. While it is
> possible to vary the list dynamically, you should consider that this could
> confuse the user who does not expect that portion of the destination list to
> change.

__Tarefas do Internet Explorer:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

Ao contrário do Menu Dock no macOS que é um verdadeiro menu, tarefas do usuário no Windows
funcionam como atalhos, de uma forma que quando o usuário clica em uma tarefa, um programa
deve ser executado com os argumentos especificados.

Para setar tarefas do usuário para sua aplicação, você pode usar a API
[app.setUserTasks][setusertaskstasks]:

```javascript
var app = require('app')
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
])
```

Para limpar sua lista de tarefas, apenas chame `app.setUserTasks` com um
array vazio.

```javascript
app.setUserTasks([])
```

As tarefas do usuário são exibidas mesmo depois da aplicação ser fechada,
então o ícone e o caminho do programa especificado pela tarefa deve existir
até sua aplicação ser desinstalada.

## Miniaturas na Barra de Ferramentas

No Windows você pode adicionar uma miniatura na barra de ferramentas com botões
específicos para a janela e barra de tarefas para aplicação. Isso provê ao usuário
uma forma de acessar um comando específico para janela sem ser necessário restaurar
ou ativar a janela.

Isto é ilustrado no MSDN:

> This toolbar is simply the familiar standard toolbar common control. It has a
> maximum of seven buttons. Each button's ID, image, tooltip, and state are defined
> in a structure, which is then passed to the taskbar. The application can show,
> enable, disable, or hide buttons from the thumbnail toolbar as required by its
> current state.
>
> For example, Windows Media Player might offer standard media transport controls
> such as play, pause, mute, and stop.

__Miniaturas da barra de tarefas do Windows Media Player:__

![player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

Você pode usar [BrowserWindow.setThumbarButtons][setthumbarbuttons] para criar
miniaturas na barra de ferramentas para sua aplicação.

```
var BrowserWindow = require('browser-window');
var path = require('path');
var win = new BrowserWindow({
  width: 800,
  height: 600
});
win.setThumbarButtons([
  {
    tooltip: "button1",
    icon: path.join(__dirname, 'button1.png'),
    click: function() { console.log("button2 clicked"); }
  },
  {
    tooltip: "button2",
    icon: path.join(__dirname, 'button2.png'),
    flags:['enabled', 'dismissonclick'],
    click: function() { console.log("button2 clicked."); }
  }
]);
```

Para limpar os botões na miniatura da barra de ferramentas, apenas chame
`BrowserWindow.setThumbarButtons` com um array vazio.

```javascript
win.setThumbarButtons([])
```

## Unity Launcher Shortcuts (Linux)

No Unity, você pode adicionar entradas customizadas para estes lançadores modificando
o arquivo `.desktop`, veja [Adding Shortcuts to a Launcher][unity-launcher].

__Launcher shortcuts do Audacious:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## Barra de Progresso na Barra de Tarefas (Windows & Unity)

No Windows o botão na barra de tarefas pode ser usado para exibir uma barra de progresso.
Isto permite que a janela exiba informação sobre o progresso de algum processo sem
a necessidade do usuário mudar de janela.

A Unity DE também tem uma funcionalidade parecida que permite especificar uma barra
de progresso no ícone do lançador.

__Barra de Progresso no botão da barra de tarefas:__

![Barra de Progresso na Barra de Tarefas](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

__Barra de progresso no Unity launcher:__

![Unity Launcher](https://cloud.githubusercontent.com/assets/639601/5081747/4a0a589e-6f0f-11e4-803f-91594716a546.png)

Para adicionar uma barra de progresso para uma janela, você pode ver a API:
[BrowserWindow.setProgressBar][setprogressbar]:

```javascript
var window = new BrowserWindow({...});
window.setProgressBar(0.5);
```

## Representação do arquivo na janela (macOS)

No macOS, uma janela pode possuir a representação de um arquivo na barra de título,
permitindo que ao usuário acionar um Command-Click ou Control-Click sobre o título da janela,
uma pop-up de navegação entre arquivos é exibida.

Você também pode inserir um estado de edição na janela para que o ícone do arquivo
possa indicar se o documento nesta janela foi modificado.

__Menu popup da representação de arquivo:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

Para inserir o arquivo de representacão da janela, você pode usar as API
[BrowserWindow.setRepresentedFilename][setrepresentedfilename] e
[BrowserWindow.setDocumentEdited][setdocumentedited]:

```javascript
var window = new BrowserWindow({...});
window.setRepresentedFilename('/etc/passwd');
window.setDocumentEdited(true);
```

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks
[setprogressbar]: ../api/browser-window.md#browserwindowsetprogressbarprogress
[setrepresentedfilename]: ../api/browser-window.md#browserwindowsetrepresentedfilenamefilename
[setdocumentedited]: ../api/browser-window.md#browserwindowsetdocumenteditededited
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[setthumbarbuttons]: ../api/browser-window.md#browserwindowsetthumbarbuttonsbuttons
