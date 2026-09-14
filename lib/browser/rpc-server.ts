import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { webFrameMain } from 'electron/main';

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

// Sandboxed renderers receive their preload scripts and process info via the
// browser-pushed ElectronFrameStartup mojo interface for frames, or
// EmbeddedWorkerStartParams for service workers (see
// electron_api_web_contents.cc and electron_browser_client.cc), not over
// sync IPC. This handler is only used by non-sandboxed renderers, which read
// their own preload files from disk and only need the path list.
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
