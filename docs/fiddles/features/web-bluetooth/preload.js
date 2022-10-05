const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('electronAPI', {
  bluetoothPairingRequest: (callback) => ipcRenderer.on('bluetooth-pairing-request', callback),
  bluetoothPairingResponse: (response) => ipcRenderer.send('bluetooth-pairing-respnse', response)
})