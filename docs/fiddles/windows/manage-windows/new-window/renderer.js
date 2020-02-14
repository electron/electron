const { BrowserWindow } = require('electron').remote
const { shell } = require('electron').remote

const newWindowBtn = document.getElementById('new-window')
const link = document.getElementById('browser-window-link')

newWindowBtn.addEventListener('click', (event) => {

  let win = new BrowserWindow({ width: 400, height: 320 })

  win.on('close', () => { win = null })
  win.loadURL('https://electronjs.org')
  win.show()
})

link.addEventListener('click', (e) => {
  e.preventDefault()
  shell.openExternal("https://electronjs.org/docs/api/browser-window")
})
