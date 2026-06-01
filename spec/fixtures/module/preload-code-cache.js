// Preload fixture for the V8 code cache spec. Exposes enough state to verify
// the preload ran with the expected `process` shape regardless of whether the
// compile consumed a code cache or compiled from source.
const { ipcRenderer, contextBridge } = require('electron');

const result = {
  ranAt: Date.now(),
  hasEnv: typeof process.env === 'object' && Object.keys(process.env).length > 0,
  hasExecPath: typeof process.execPath === 'string' && process.execPath.length > 0,
  arch: process.arch,
  platform: process.platform,
  version: process.version,
  electronVersion: process.versions.electron,
  chromeVersion: process.versions.chrome,
  nodeVersion: process.versions.node
};

contextBridge.exposeInMainWorld('codeCacheTest', result);
ipcRenderer.send('preload-code-cache-ran', result);
