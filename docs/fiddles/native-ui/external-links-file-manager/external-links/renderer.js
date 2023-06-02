const { shell } = require('electron')

const exLinksBtn = document.getElementById('open-ex-links')

exLinksBtn.addEventListener('click', (event) => {
  shell.openExternal('https://electronjs.org')
})
