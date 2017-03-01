// Ensure fetch works from isolated world origin
fetch('http://localhost:1234')
fetch('https://localhost:1234')
fetch(`file://${__filename}`)

const {ipcRenderer, webFrame} = require('electron')

window.foo = 3

webFrame.executeJavaScript('window.preloadExecuteJavaScriptProperty = 1234;')


window.addEventListener('message', (event) => {
  ipcRenderer.send('isolated-world', {
    preloadContext: {
      preloadProperty: typeof window.foo,
      pageProperty: typeof window.hello,
      typeofRequire: typeof require,
      typeofProcess: typeof process,
      typeofArrayPush: typeof Array.prototype.push,
      typeofFunctionApply: typeof Function.prototype.apply
    },
    pageContext: event.data
  })
})
