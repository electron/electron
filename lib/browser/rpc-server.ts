import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { clipboard } from 'electron/common';

import * as fs from 'fs';
import * as path from 'path';

// Implements window.close()
ipcMainInternal.on(IPC_MESSAGES.BROWSER_WINDOW_CLOSE, function (event) {
  if (event.type !== 'frame') return;

  const window = event.sender.getOwnerBrowserWindow();
  if (window) {
    window.close();
  }
  event.returnValue = null;
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_LAST_WEB_PREFERENCES, function (event) {
  if (event.type !== 'frame') return;
  return event.sender.getLastWebPreferences();
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO, function (event) {
  if (event.type !== 'frame') return;
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

const getPreloadScriptsFromEvent = (event: ElectronInternal.IpcMainInternalEvent) => {
  const session: Electron.Session = event.type === 'service-worker' ? event.session : event.sender.session;
  let preloadScripts = session.getPreloadScripts();

  if (event.type === 'frame') {
    preloadScripts = preloadScripts.filter(script => script.type === 'frame');
    const preload = event.sender._getPreloadScript();
    if (preload) preloadScripts.push(preload);
  } else if (event.type === 'service-worker') {
    preloadScripts = preloadScripts.filter(script => script.type === 'service-worker');
  } else {
    throw new Error(`getPreloadScriptsFromEvent: event.type is invalid (${(event as any).type})`);
  }

  // TODO(samuelmaddock): Remove filter after Session.setPreloads is fully
  // deprecated. The new API will prevent relative paths from being registered.
  return preloadScripts.filter(script => path.isAbsolute(script.filePath));
};

const readPreloadScript = async function (script: Electron.PreloadScript): Promise<ElectronInternal.PreloadScript> {
  let contents;
  let error;
  try {
    contents = await fs.promises.readFile(script.filePath, 'utf8');
  } catch (err) {
    if (err instanceof Error) {
      error = err;
    }
  }
  return {
    ...script,
    contents,
    error
  };
};

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_SANDBOX_LOAD, async function (event) {
  const preloadScripts = getPreloadScriptsFromEvent(event);
  return {
    preloadScripts: await Promise.all(preloadScripts.map(readPreloadScript)),
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
  const preloadScripts = getPreloadScriptsFromEvent(event);
  return { preloadPaths: preloadScripts.map(script => script.filePath) };
});

ipcMainInternal.on(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, function (event, preloadPath: string, error: Error) {
  if (event.type !== 'frame') return;
  event.sender?.emit('preload-error', event, preloadPath, error);
});
