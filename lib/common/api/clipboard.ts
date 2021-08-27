import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';

const clipboard = process._linkedBinding('electron_common_clipboard');

if (process.type === 'renderer') {
  const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

  const makeRemoteMethod = function (method: keyof Electron.Clipboard): any {
    return (...args: any[]) => {
      const result = ipcRendererUtils.invokeSync(IPC_MESSAGES.BROWSER_CLIPBOARD_SYNC, method, ...args);
      return result;
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
