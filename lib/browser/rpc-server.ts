import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { clipboard } from 'electron/common';
import { webFrameMain } from 'electron/main';

import * as path from 'path';

// Implements window.close(). Only the top-level document may close its
// window, as in the HTML spec; the renderer override is installed for the main
// frame only, and the request is ignored here if it comes from anywhere else.
ipcMainInternal.on(IPC_MESSAGES.BROWSER_WINDOW_CLOSE, function (event) {
  event.returnValue = null;
  if (event.type !== 'frame') return;
  if (!event.senderFrame || event.senderFrame !== event.sender.mainFrame) return;

  const window = event.sender.getOwnerBrowserWindow();
  if (window) {
    window.close();
  }
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_LAST_WEB_PREFERENCES, function (event) {
  if (event.type !== 'frame') return;
  return event.sender.getLastWebPreferences();
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO, function (event) {
  if (event.type !== 'frame') return;
  // Report the calling frame's own renderer process, which for an
  // out-of-process iframe is not the main frame's.
  return event.sender._getProcessMemoryInfo(event.processId);
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

// Sandboxed renderers receive their preload scripts and process info via the
// browser-pushed ElectronFrameStartup mojo interface for frames, or
// EmbeddedWorkerStartParams for service workers (see
// electron_api_web_contents.cc and electron_browser_client.cc), not over
// sync IPC. This handler is only used by non-sandboxed renderers, which read
// their own preload files from disk and only need the path list.
ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_NONSANDBOX_LOAD, function (event) {
  if (event.type !== 'frame') {
    throw new Error(`BROWSER_NONSANDBOX_LOAD: invalid event.type (${(event as any).type})`);
  }
  const session: Electron.Session = event.sender.session;
  let preloadScripts = session.getPreloadScripts().filter((script) => script.type === 'frame');
  const webPrefPreload = event.sender._getPreloadScript();
  if (webPrefPreload) preloadScripts.push(webPrefPreload);
  // TODO(samuelmaddock): Remove filter after Session.setPreloads is fully
  // deprecated. The new API will prevent relative paths from being registered.
  preloadScripts = preloadScripts.filter((script) => path.isAbsolute(script.filePath));
  return { preloadPaths: preloadScripts.map((script) => script.filePath) };
});

ipcMainInternal.on(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, function (event, preloadPath: string, error: Error) {
  if (event.type !== 'frame') return;
  event.sender?.emit('preload-error', event, preloadPath, error);
});

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_GET_FRAME_ROUTING_ID_SYNC, function (event, frameToken: string) {
  if (event.type !== 'frame') return;
  const senderFrame = event.senderFrame;
  if (!senderFrame || senderFrame.isDestroyed()) return;
  return webFrameMain.fromFrameToken(senderFrame.processId, frameToken)?.routingId;
});

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_GET_FRAME_TOKEN_SYNC, function (event, routingId: number) {
  if (event.type !== 'frame') return;
  const senderFrame = event.senderFrame;
  if (!senderFrame || senderFrame.isDestroyed()) return;
  return webFrameMain.fromId(senderFrame.processId, routingId)?.frameToken;
});
