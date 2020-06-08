const clipboard = process.electronBinding('clipboard');

if (process.type === 'renderer') {
  const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils');
  const typeUtils = require('@electron/internal/common/type-utils');

  const makeRemoteMethod = function (method: keyof Electron.Clipboard) {
    return (...args: any[]) => {
      args = typeUtils.serialize(args);
      const result = ipcRendererUtils.invokeSync('ELECTRON_BROWSER_CLIPBOARD_SYNC', method, ...args);
      return typeUtils.deserialize(result);
    };
  };

  if (process.platform === 'linux') {
    // On Linux we could not access clipboard in renderer process.
    for (const method of Object.keys(clipboard) as (keyof Electron.Clipboard)[]) {
      clipboard[method] = makeRemoteMethod(method);
    }
  } else if (process.platform === 'darwin') {
    // Read/write to find pasteboard over IPC since only main process is notified of changes
    clipboard.readFindText = makeRemoteMethod('readFindText');
    clipboard.writeFindText = makeRemoteMethod('writeFindText');
  }
}

export default clipboard;
