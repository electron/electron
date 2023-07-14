const { ipcRenderer, shell } = require('electron')

const newWindowBtn = document.getElementById('frameless-window')

newWindowBtn.addEventListener('click', () => {
  const url = 'data:text/html,<h2>Hello World!</h2><a id="close" href="javascript:window.close()">Close this Window</a>'
  ipcRenderer.send('create-frameless-window', { url })
})

const links = document.querySelectorAll('a[href]')

for (const link of links) {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
}
