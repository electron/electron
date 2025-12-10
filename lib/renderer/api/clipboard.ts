import * as deprecate from '@electron/internal/common/deprecate';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';

const clipboard = {} as Electron.Clipboard;
const originalClipboard = process._linkedBinding('electron_common_clipboard');

const warnDeprecatedAccess = function (method: keyof Electron.Clipboard) {
  return deprecate.warnOnceMessage(`Accessing 'clipboard.${method}' from the renderer process is
     deprecated and will be removed. Please use the 'contextBridge' API to access
     the clipboard API from the renderer.`);
};

const makeDeprecatedMethod = function (method: keyof Electron.Clipboard): any {
  const warnDeprecated = warnDeprecatedAccess(method);
  return (...args: any[]) => {
    warnDeprecated();
    return (originalClipboard[method] as any)(...args);
  };
};

const makeRemoteMethod = function (method: keyof Electron.Clipboard): any {
  const warnDeprecated = warnDeprecatedAccess(method);
  return (...args: any[]) => {
    warnDeprecated();
    return ipcRendererUtils.invokeSync(IPC_MESSAGES.BROWSER_CLIPBOARD_SYNC, method, ...args);
  };
};

if (process.platform === 'linux') {
  // On Linux we could not access clipboard in renderer process.
  for (const method of Object.keys(originalClipboard) as (keyof Electron.Clipboard)[]) {
    clipboard[method] = makeRemoteMethod(method);
  }
} else {
  for (const method of Object.keys(originalClipboard) as (keyof Electron.Clipboard)[]) {
    if (process.platform === 'darwin' && (method === 'readFindText' || method === 'writeFindText')) {
      clipboard[method] = makeRemoteMethod(method);
    } else {
      clipboard[method] = makeDeprecatedMethod(method);
    }
  }
}

export default clipboard;
