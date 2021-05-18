const { contextBridge, ipcRenderer } = require('electron');
const fs = require('fs');

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => {
    // Create a new file to copy - you can copy something 
    fs.writeFileSync(fileName, '# Test drag and drop');
    ipcRenderer.send('ondragstart', process.cwd() + `/${fileName}`)
  },
})