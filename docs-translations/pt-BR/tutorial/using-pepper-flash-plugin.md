# Usando o Plugin Pepper Flash

Electron atualmente suporta o plugin Pepper Flash. Para usá-lo no Electron,
você deve especificar manualmente a localização do plugin e então ele será
habilitado em sua aplicação.

## Prepare uma cópia do plugin Flash

Tanto no macOS como no Linux, os detalhes do plugin Pepper Flash podem ser
encontrados navegando por `chrome://plugins` no navegador Chrome. Essa
localização e versão são úteis para o suporte do plugin Electron's Pepper Flash.
Você pode também copiar para outra localização.

## Adicionando a opçao Electron

Você pode adicionar diretamente `--ppapi-flash-path` e `ppapi-flash-version`
para a linha de comando do Electron ou usando o método
`app.commandLine.appendSwitch` após o evento pronto. Também, adicione a opção
`plugins` em `browser-window`.
Por exemplo:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

// Mantém uma referência global da janela, se não manter, a janela irá fechar
// automaticamente quando o objeto javascript for GCed.
var mainWindow = null;

// Sai assim que todas as janelas forem fechadas.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// Epecifica o caminho do flash.
// No Windows, deve ser /path/to/pepflashplayer.dll
// No macOS, /path/to/PepperFlashPlayer.plugin
// No Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Especifica a versão do flash, por exemplo, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.loadURL('file://' + __dirname + '/index.html');
  // Algo mais
});
```

## Ative o plugin Flash na tag `<webview>`

Adicione o atributo `plugins` na tag `<webview>`.

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
