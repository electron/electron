const { BrowserWindow } = require('electron').remote

const newWindowBtn = document.getElementById('new-window')

newWindowBtn.addEventListener('click', (event) => {

  let win = new BrowserWindow({ width: 400, height: 320 })

  win.on('close', () => { win = null })
  win.loadURL('https://electronjs.org')
  win.show()
})
