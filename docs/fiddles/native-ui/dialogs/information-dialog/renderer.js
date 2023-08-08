const { ipcRenderer, shell } = require('electron')

const informationBtn = document.getElementById('information-dialog')
const links = document.querySelectorAll('a[href]')

informationBtn.addEventListener('click', event => {
  ipcRenderer.send('open-information-dialog')
})

ipcRenderer.on('information-dialog-selection', (event, index) => {
  let message = 'You selected '
  if (index === 0) message += 'yes.'
  else message += 'no.'
  document.getElementById('info-selection').innerHTML = message
})

for (const link of links) {
  const url = link.getAttribute('href')
  if (url.indexOf('http') === 0) {
    link.addEventListener('click', (e) => {
      e.preventDefault()
      shell.openExternal(url)
    })
  }
}
