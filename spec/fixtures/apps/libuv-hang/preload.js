const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  // This is not safe, do not copy this code into your app
  invoke: (...args) => ipcRenderer.invoke(...args),
  run: async () => {
    const { promises: fs } = require('node:fs');
    for (let i = 0; i < 10; i++) {
      const list = await fs.readdir('.', { withFileTypes: true });
      for (const file of list) {
        if (file.isFile()) {
          await fs.readFile(file.name, 'utf-8');
        }
      }
    }
  }
});
