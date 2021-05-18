const { contextBridge, ipcRenderer } = require('electron');
const fs = require('fs');

contextBridge.exposeInMainWorld('electron', {
  startDrag: (fileName) => {
    fs.writeFileSync(fileName, '# Test drag and drop');
    ipcRenderer.send('ondragstart', process.cwd() + `/${fileName}`)
  },
})