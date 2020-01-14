const { clipboard } = require('electron')

const pasteBtn = document.getElementById('paste-to')

pasteBtn.addEventListener('click', () => {
  clipboard.writeText('What a demo!')
  const message = `Clipboard contents: ${clipboard.readText()}`
  document.getElementById('paste-from').innerHTML = message
})
