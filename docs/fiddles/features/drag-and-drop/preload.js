const { contextBridge, ipcRenderer } = require('electron')
const fs = require('fs')

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => {
    // Create a new file to copy - you can also copy existing files.
    fs.writeFileSync(fileName, '# Test drag and drop')

    ipcRenderer.send('ondragstart', process.cwd() + `/${fileName}`)
  }
})