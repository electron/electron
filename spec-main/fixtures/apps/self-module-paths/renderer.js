const { ipcRenderer } = require('electron');

const worker = new Worker('worker.js');

worker.onmessage = (event) => {
  const workerPaths = event.data.sort().toString();
  const rendererPaths = self.module.paths.sort().toString();
  const validModulePaths = workerPaths === rendererPaths && workerPaths !== 0;

  ipcRenderer.invoke('module-paths', validModulePaths);
  worker.terminate();
};
