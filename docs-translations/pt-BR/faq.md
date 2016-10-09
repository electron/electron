# FAQ Electron

## Quando Electron será atualizado para a versão mais recente do Chrome?

A versão do Chrome do Electron é geralmente lançada dentro de uma ou duas semanas após a liberação de uma nova versão estável do Chrome. Esta estimativa não é garantida e depende da quantidade de trabalho envolvido com a atualização.

Apenas o canal estável do Chrome é usado. Se uma correção importante estiver no canal beta ou dev, vamos portá-la.

Para mais informações, consulte a [introdução de segurança](tutorial/security.md).

## Quando Electron será atualizado para a versão mais recente do Node.js?

Quando uma nova versão do Node.js for lançada, geralmente esperamos por cerca de um mês antes de atualizar a do Electron. Assim, podemos evitar sermos afetados por erros introduzidos na nova versão do Node.js, o que acontece muito frequentemente.

Os novos recursos do Node.js geralmente são trazidos por V8 upgrades, desde que o Electron usa o V8 enviado pelo navegador Chrome, o novo recurso brilhante do JavaScript de uma nova versão Node.js esta geralmente no Electron.

## Como compartilhar dados entre página da web?

Para compartilhar dados entre páginas web (os processos de renderização) a maneira mais simples é usar as APIs HTML5 que já estão disponíveis nos navegadores. Bons candidatos são [Storage API][storage], [`localStorage`][local-storage],[`sessionStorage`][session-storage], e [IndexedDB][indexed-db].

Ou você pode usar o sistema IPC, que é específico para o Electron, para armazenar objetos no processo principal como uma variável global, e depois acessar os representantes através da propriedade `remote` do módulo do `electron`:

```javascript
// In the main process.
global.sharedObject = {
  someProperty: 'default value'
}
```

```javascript
// In page 1.
require('electron').remote.getGlobal('sharedObject').someProperty = 'new value'
```

```javascript
// In page 2.
console.log(require('electron').remote.getGlobal('sharedObject').someProperty)
```

## Janela/tray da minha aplicação despareceu depois de alguns minutos.

Isso acontece quando a variável que é usada para armazenar a janela/tray recebe lixo coletado.

Se você encontrar esse problema, os seguintes artigos podem ser úteis:

* [Gerenciamento de Memória][memory-management]
* [Escopo de Variável][variable-scope]

Se você quer uma solução rápida, você pode tornar as variáveis globais, alterando o seu código como este:

```javascript
const {app, Tray} = require('electron')
app.on('ready', () => {
  const tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

para este:

```javascript
const {app, Tray} = require('electron')
let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/icon.png')
  tray.setTitle('hello world')
})
```

## Eu não posso usar jQuery/RequireJS/Meteor/AngularJS no Electron.

Devido à integração Node.js do Electron, existem alguns símbolos adicionais inseridos no DOM como `module`, `exports`, `require`. Isso causa problemas para algumas bibliotecas, uma vez que pretende inserir os símbolos com os mesmos nomes.

Para resolver isso, você pode desativar a integração no Electron:

```javascript
// In the main process.
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
win.show()
```

Mas se você quiser manter as habilidade de uso de APIs Node.js e Electron, você tem que mudar o nome dos símbolos na página antes de incluir outras bibliotecas:

```html
<head>
<script>
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## `require('electron').xxx` is undefined.

Ao usar o módulo integrado do Electron você pode encontrar um erro como este:

```
> require('electron').webFrame.setZoomFactor(1.0)
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

Isso é porque tem o [npm `electron` module][electron-module] instalado localmente ou globalmente, que substitui o módulo integrado do Electron.

Para verificar se você esta usando o módulo integrado correto, você pode imprimir o caminho do módulo do `electron`:

```javascript
console.log(require.resolve('electron'))
```

e, em seguida, verificar se ele está no seguinte formato:

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

Se é algo como `node_modules/electron/index.js`, então você tem que remover o módulo npm `electron`, ou renomeá-lo.

```bash
npm uninstall electron
npm uninstall -g electron
```

No entanto, se você está usando o módulo embutido, mas ainda recebendo este erro, é muito provável que você está usando o módulo no processo errado. Por exemplo `electron.app` só pode ser usado no processo principal, enquanto o `electron.webFrame` só está disponível no processo renderizador.

[memory-management]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/en-US/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/en-US/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API
