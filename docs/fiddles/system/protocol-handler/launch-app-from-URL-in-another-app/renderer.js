const { shell } = require('electron')
const path = require('path')

const openInBrowserButton = document.getElementById('open-in-browser')
const openAppLink = document.getElementById('open-app-link')
// Hides openAppLink when loaded inside Electron
openAppLink.style.display = 'none'

openInBrowserButton.addEventListener('click', () => {
  console.log('clicked')
  const pageDirectory = __dirname.replace('app.asar', 'app.asar.unpacked')
  const pagePath = path.join('file://', pageDirectory, 'index.html')
  shell.openExternal(pagePath)
})
