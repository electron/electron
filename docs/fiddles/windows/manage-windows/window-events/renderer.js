const { shell, ipcRenderer } = require('electron')

const listenToWindowBtn = document.getElementById('listen-to-window')
const focusModalBtn = document.getElementById('focus-on-modal-window')

const hideFocusBtn = () => {
  focusModalBtn.classList.add('disappear')
  focusModalBtn.classList.remove('smooth-appear')
  focusModalBtn.removeEventListener('click', focusWindow)
}

const showFocusBtn = (btn) => {
  focusModalBtn.classList.add('smooth-appear')
  focusModalBtn.classList.remove('disappear')
  focusModalBtn.addEventListener('click', focusWindow)
}
const focusWindow = () => {
  ipcRenderer.send('focus-demo-window')
}

ipcRenderer.on('window-focus', hideFocusBtn)
ipcRenderer.on('window-close', hideFocusBtn)
ipcRenderer.on('window-blur', showFocusBtn)

listenToWindowBtn.addEventListener('click', () => {
  ipcRenderer.send('show-demo-window')
})

const links = document.querySelectorAll('a[href]')

Array.prototype.forEach.call(links, (link) => {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
})
