const { contextBridge, ipcRenderer } = require('electron')
const fs = require('fs').promises
const path = require('path')

contextBridge.exposeInMainWorld('electron', {
  startDrag: async (fileName) => {
    // Create a new file to copy - you can also copy existing files.
    await fs.writeFile(fileName, '# Test drag and drop')

    ipcRenderer.send('ondragstart', path.join(process.cwd(), fileName))
  }
})
