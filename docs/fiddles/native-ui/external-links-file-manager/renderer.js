const { shell } = require('electron')
const os = require('os')

const exLinksBtn = document.getElementById('open-ex-links')
const fileManagerBtn = document.getElementById('open-file-manager')

fileManagerBtn.addEventListener('click', (event) => {
  shell.showItemInFolder(os.homedir())
})

exLinksBtn.addEventListener('click', (event) => {
  shell.openExternal('https://electronjs.org')
})
