const exLinksBtn = document.getElementById('open-ex-links')
const fileManagerBtn = document.getElementById('open-file-manager')

fileManagerBtn.addEventListener('click', (event) => {
  window.electronAPI.openHomeDir()
})

exLinksBtn.addEventListener('click', (event) => {
  window.electronAPI.openExternal('https://electronjs.org')
})
