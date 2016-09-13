# Extensão DevTools

Para facilitar a depuração, o Electron provê suporte para a extensão [Chrome DevTools][devtools-extension].

Para a maioria das extensões DevTools você pode simplesmente baixar o código-fonte e usar a função  `BrowserWindow.addDevToolsExtension` para carregá-las. As extensões carregadas serão lembradas, assim você não precisa carregar sempre que criar uma nova janela.

**NOTA: Se o DevTools React não funcionar, verifique a issue https://github.com/electron/electron/issues/915**

Por exemplo, para usar a extensão [React DevTools](https://github.com/facebook/react-devtools), primeiro você deve baixar seu código-fonte:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

Siga as instruções em  [`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md) para fazer o build da extensão.

Agora você poderá carregar a extensão no Electron abrindo o   DevTools em qualquer janela, e executando o seguinte código no console do DevTools:

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow;
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome');
```

Para remover a extensão, você pode executar a chamada    `BrowserWindow.removeDevToolsExtension`
com o nome da extensão e ela não será carregada na próxima vez que você abrir o DevTools:

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools');
```

## Formato das extensões DevTools

Idealmente todas as extensões DevTools escritas para o navegador Chrome podem ser carregadas pelo Electron, mas elas devem estar em um diretório. Pacotes com extensão `crx` não podem ser carregados pelo Electron a não ser que tenha uma forma de extraí-los em um diretório.

## Páginas em segundo plano (background pages)

Atualmente o Electron não suporta páginas em segundo plano nas extensões do Chrome, então extensões com essa característica podem não funcionar no Electron.

## APIs `chrome.*`

Algumas extensões do Chrome podem usar a API `chrome.*`. Apesar de um esforço na implementação destas APIs no Electron, elas ainda não estão finalizadas.

Dado que nem todas as funções `chrome.*` estão implementadas, algumas extensões que utilizam `chrome.devtools.*` podem não funcionar. Você pode reportar este erro no issue tracker para que possamos adicionar suporte a essas APIs.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
