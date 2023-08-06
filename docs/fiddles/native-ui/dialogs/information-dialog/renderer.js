const { ipcRenderer } = require('electron')

const informationBtn = document.getElementById('information-dialog')

informationBtn.addEventListener('click', event => {
  ipcRenderer.send('open-information-dialog')
})

ipcRenderer.on('information-dialog-selection', (event, index) => {
  let message = 'You selected '
  if (index === 0) message += 'yes.'
  else message += 'no.'
  document.getElementById('info-selection').innerHTML = message
})
