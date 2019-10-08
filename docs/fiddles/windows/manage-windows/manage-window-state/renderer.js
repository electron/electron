const { BrowserWindow } = require('electron').remote
const shell = require('electron').shell

const manageWindowBtn = document.getElementById('manage-window')

const links = document.querySelectorAll('a[href]')

let win

manageWindowBtn.addEventListener('click', (event) => {
  const modalPath = 'https://electronjs.org'
  win = new BrowserWindow({ width: 400, height: 275 })

  win.on('resize', updateReply)
  win.on('move', updateReply)
  win.on('close', () => { win = null })
  win.loadURL(modalPath)
  win.show()

  function updateReply () {
    const manageWindowReply = document.getElementById('manage-window-reply')
    const message = `Size: ${win.getSize()} Position: ${win.getPosition()}`
    manageWindowReply.innerText = message
  }
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
