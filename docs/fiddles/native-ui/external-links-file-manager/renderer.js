const exLinksBtn = document.getElementById('open-ex-links')
const fileManagerBtn = document.getElementById('open-file-manager')

fileManagerBtn.addEventListener('click', () => {
  window.electronAPI.openHomeDir()
})

exLinksBtn.addEventListener('click', () => {
  window.electronAPI.openExternal('https://electronjs.org')
})
