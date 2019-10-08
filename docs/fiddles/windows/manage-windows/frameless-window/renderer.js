const { BrowserWindow } = require('electron').remote
const shell = require('electron').shell

const framelessWindowBtn = document.getElementById('frameless-window')

const links = document.querySelectorAll('a[href]')

framelessWindowBtn.addEventListener('click', (event) => {
  const modalPath = 'https://electronjs.org'
  let win = new BrowserWindow({ frame: false })

  win.on('close', () => { win = null })
  win.loadURL(modalPath)
  win.show()
})

Array.prototype.forEach.call(links, (link) => {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
})
