const { ipcRenderer } = require('electron')
const fs = require('fs')

document.getElementById('drag').ondragstart = (event) => {
  const fileName = 'drag-and-drop.md'
  fs.writeFileSync(fileName, '# Test drag and drop');
  event.preventDefault()
  ipcRenderer.send('ondragstart', process.cwd() + `/${fileName}`)
}
