const { contextBridge, ipcRenderer } = require('electron');

let result;
try {
  result = contextBridge.executeInMainWorld({
    func: () => ({
      chromeType: typeof chrome,
      id: globalThis.chrome?.runtime.id,
      manifest: globalThis.chrome?.runtime.getManifest()
    })
  });
} catch (error) {
  console.error(error);
}

ipcRenderer.invoke('preload-extension-result', result);
