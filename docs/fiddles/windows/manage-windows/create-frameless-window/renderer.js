const { BrowserWindow } = require('electron').remote

const newWindowBtn = document.getElementById('frameless-window')

newWindowBtn.addEventListener('click', (event) => {
  let win = new BrowserWindow({ frame: false })

  win.on('close', () => { win = null })

  win.loadURL('data:text/html,<h2>Hello World!</h2><a id="close" href="javascript:window.close()">Close this Window</a>')
  win.show()
})
