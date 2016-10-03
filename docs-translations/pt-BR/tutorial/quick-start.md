# Introdução

Electron permite criar aplicações desktop com puro JavaScript através de
um runtime com APIs ricas e nativas. Você pode ver isso como uma variação do
runtime do io.js que é focado em aplicações desktop em vez de web servers.

Isso não significa que o Electron é uma ligação em JavaScript para bibliotecas
de interface gráfica (GUI). Em vez disso, Electron usa páginas web como
interface gráfica, então você pode ver isso também como um navegador Chromium
mínimo, controlado por JavaScript.

### Processo Principal

No Electron, o processo que executa o script principal (main) do `package.json`
é chamado __processo principal__. O script que roda no processo principal pode
mostrar uma GUI criando páginas web.

### Processo Renderizador

Já que o Electron usa o Chromium para mostrar as páginas web, a arquitetura
multi-processo do Chromium também é usada. Cada página web no Electron roda em
seu próprio processo, o que é chamado de __processo renderizador__.

Em navegadores comuns, as páginas web normalmente rodam em um ambiente em sandbox
e não têm permissão de acesso para recursos nativos. Usuários Electron, entretanto,
têm o poder de usar as APIs do io.js nas páginas web, permitindo interações de baixo
nível no sistema operacional.

### Diferenças Entre o Processo Principal e o Processo Renderizador

O processo principal cria as páginas web criando instâncias de `BrowserWindow`.
Cada instância de `BrowserWindow` roda a página web em seu próprio processo renderizador.
Quando uma instância de `BrowserWindow` é destruída, o processo renderizador
correspondente também é finalizado.

O processo principal gerencia todas as páginas web de seus processos renderizadores
correspondentes. Cada processo renderizador é isolado e toma conta de sua
respectiva página web.

Nas páginas web, chamar APIs nativas relacionadas à GUI não é permitido porque
gerenciar recursos de GUI em páginas web é muito perigoso e torna fácil o vazamento de
recursos. Se você quer realizar operações com GUI em páginas web, o processo
renderizador da página web deve se comunicar com o processo principal para requisitar
que o processo principal realize estas operações.

No Electron, nós fornecemos o módulo [ipc](../../../docs/api/ipc-renderer.md) para
comunicação entre o processo principal e o processo renderizador. Que é também um
módulo [remoto](../../../docs/api/remote.md) para comunicação RPC.

## Crie seu Primeiro App Electron

Geralmente, um app Electron é estruturado assim:

```text
seu-app/
├── package.json
├── main.js
└── index.html
```

O formato de `package.json` é exatamente o mesmo que o dos módulos do Node,
e o script especificado pelo campo `main` é o script de inicialização do seu app,
que irá executar o processo principal. Um exemplo do seu `package.json` deve parecer
com isso:

```javascripton
{
  "name"    : "seu-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__Nota__: Se o campo `main` não estiver presente no `package.json`, o Electron irá
tentar carregar um `index.js`

O `main.js` deve criar as janelas e os manipuladores de eventos do sistema, um típico
exemplo:

```javascript
const {app, BrowserWindow} = require('electron')

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let win

function createWindow () {
  // Create the browser window.
  win = new BrowserWindow({width: 800, height: 600})

  // and load the index.html of the app.
  win.loadURL(`file://${__dirname}/index.html`)

  // Open the DevTools.
  win.webContents.openDevTools()

  // Emitted when the window is closed.
  win.on('closed', () => {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    win = null
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // On macOS it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow()
  }
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
```

Finalmente o `index.html` é a página web que você quer mostrar:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    Nós estamos usando io.js <script>document.write(process.version)</script>
    e Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Execute seu App

Uma vez que você criou seus arquivos `main.js`, `index.html`, e `package.json` iniciais,
você provavelmente vai querer tentar executar seu app localmente para testa-lo e ter
certeza que funciona como você espera.

### electron-prebuilt

Se você instalou `electron-prebuilt` globalmente com `npm`, então você irá precisar apenas
rodar o seguinte comando no diretório fonte do seu app:

```bash
electron .
```

Se você o instalou localmente, então execute:

```bash
./node_modules/.bin/electron .
```

### Binário do Electron Baixado Manualmente

Se você baixou o Electron manualmente, você pode também usar o binário incluído para
executar seu app diretamente.

#### Windows

```bash
$ .\electron\electron.exe seu-app\
```

#### Linux

```bash
$ ./electron/electron seu-app/
```

#### macOS

```bash
$ ./Electron.app/Contents/MacOS/Electron seu-app/
```

`Electron.app` aqui é uma parte do pacote de lançamento do Electron, você pode baixa-lo
[aqui](https://github.com/electron/electron/releases).

### Executar como uma distribuição

Depois de terminar seu app, você pode criar uma distribuição seguindo o guia
[Distribuição de aplicações](./application-distribution.md) e então executar o app
empacotado.
