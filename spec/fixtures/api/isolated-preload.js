const {ipcRenderer} = require('electron')

window.foo = 3

window.addEventListener('message', (event) => {
  ipcRenderer.send('isolated-world', {
    preloadContext: {
      preloadProperty: typeof window.foo,
      pageProperty: typeof window.hello,
      typeofRequire: typeof require,
      typeofProcess: typeof process
    },
    pageContext: event.data
  })
})
