const { BrowserWindow } = require('electron').remote
const shell = require('electron').shell

const listenToWindowBtn = document.getElementById('listen-to-window')
const focusModalBtn = document.getElementById('focus-on-modal-window')

const links = document.querySelectorAll('a[href]')

let win

listenToWindowBtn.addEventListener('click', () => {
  const modalPath = 'https://electronjs.org'
  win = new BrowserWindow({ width: 600, height: 400 })

  const hideFocusBtn = () => {
    focusModalBtn.classList.add('disappear')
    focusModalBtn.classList.remove('smooth-appear')
    focusModalBtn.removeEventListener('click', clickHandler)
  }

  const showFocusBtn = (btn) => {
    if (!win) return
    focusModalBtn.classList.add('smooth-appear')
    focusModalBtn.classList.remove('disappear')
    focusModalBtn.addEventListener('click', clickHandler)
  }

  win.on('focus', hideFocusBtn)
  win.on('blur', showFocusBtn)
  win.on('close', () => {
    hideFocusBtn()
    win = null
  })
  win.loadURL(modalPath)
  win.show()

  const clickHandler = () => { win.focus() }
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
