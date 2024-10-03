import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { clipboard } from 'electron/common';

import * as fs from 'fs';

// Implements window.close()
ipcMainInternal.on(IPC_MESSAGES.BROWSER_WINDOW_CLOSE, function (event) {
  const window = event.sender.getOwnerBrowserWindow();
  if (window) {
    window.close();
  }
  event.returnValue = null;
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_LAST_WEB_PREFERENCES, function (event) {
  return event.sender.getLastWebPreferences();
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO, function (event) {
  return event.sender._getProcessMemoryInfo();
});

// Methods not listed in this set are called directly in the renderer process.
const allowedClipboardMethods = (() => {
  switch (process.platform) {
    case 'darwin':
      return new Set(['readFindText', 'writeFindText']);
    case 'linux':
      return new Set(Object.keys(clipboard));
    default:
      return new Set();
  }
})();

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_CLIPBOARD_SYNC, function (event, method: string, ...args: any[]) {
  if (!allowedClipboardMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return (clipboard as any)[method](...args);
});

const getPreloadScript = async function (preloadPath: string) {
  let preloadSrc = null;
  let preloadError = null;
  try {
    preloadSrc = await fs.promises.readFile(preloadPath, 'utf8');
  } catch (error) {
    preloadError = error;
  }
  return { preloadPath, preloadSrc, preloadError };
};

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_SANDBOX_LOAD, async function (event) {
  const preloadPaths = event.sender._getPreloadPaths();

  return {
    preloadScripts: await Promise.all(preloadPaths.map(path => getPreloadScript(path))),
    process: {
      arch: process.arch,
      platform: process.platform,
      env: { ...process.env },
      version: process.version,
      versions: process.versions,
      execPath: process.helperExecPath
    }
  };
});

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_NONSANDBOX_LOAD, function (event) {
  return { preloadPaths: event.sender._getPreloadPaths() };
});

ipcMainInternal.on(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, function (event, preloadPath: string, error: Error) {
  event.sender.emit('preload-error', event, preloadPath, error);
});
