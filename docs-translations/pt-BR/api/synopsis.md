# Sinopse

> Como utilizar Node.js e a API do Electron.

Todos os [módulos built-in do Node.js](http://nodejs.org/api/) estão disponíveis no Electron e os módulos de terceiros do node também têm suporte completo (inclusos os [módulos nativos](../tutorial/using-native-node-modules.md)).

O Electron também fornece alguns módulos built-in para desenvolvimento nativo para aplicações desktop. Alguns módulos estão disponíveis somente no processo principal, outros somente no processo de renderização (página web), e outros em ambos processos.

A regra básica é: se um módulo é [GUI][gui] ou de baixo nível, então deve estar disponível somente no módulo principal. Você tem de se familiarizar com o conceito de [processo principal vs. processo de renderização](../tutorial/quick-start.md#the-main-process) para estar apto a usar esses módulos.

O script do processo principal é como um script normal em Node.js:

```javascript
const {app, BrowserWindow} = require('electron')
let win = null

app.on('ready', () => {
  win = new BrowserWindow({width: 800, height: 600})
  win.loadURL('https://github.com')
})
```

O processo renderizador não é diferente de uma página web comum, exceto pela possiblidade de usar módulos node:

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const {app} = require('electron').remote
  console.log(app.getVersion())
</script>
</body>
</html>
```

Para rodar sua app, leia [Rodando sua app](../tutorial/quick-start.md#run-your-app).

## Atribuição de desestruturação

Assim como 0.37, você pode utilizar
[atribuição de desestruturação][destructuring-assignment] para facilitar o uso dos módulos built-in.

```javascript
const {app, BrowserWindow} = require('electron')

let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

Se você precisar de todo o módulo `electron`, você pode fazer a chamada e usar a desestruturação para acessar os módulos individuais.

```javascript
const electron = require('electron')
const {app, BrowserWindow} = electron

let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

É equivalente ao seguinte código:

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow
let win

app.on('ready', () => {
  win = new BrowserWindow()
  win.loadURL('https://github.com')
})
```

[gui]: https://en.wikipedia.org/wiki/Graphical_user_interface
[destructuring-assignment]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
