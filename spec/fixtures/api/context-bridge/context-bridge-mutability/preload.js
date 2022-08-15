const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('str', 'some-text');
contextBridge.exposeInMainWorld('obj', { prop: 'obj-prop' });
contextBridge.exposeInMainWorld('arr', [1, 2, 3, 4]);
