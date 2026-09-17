import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { webFrameMain } from 'electron/main';

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO, function (event) {
  if (event.type !== 'frame') return;
  // Report the calling frame's own renderer process, which for an
  // out-of-process iframe is not the main frame's.
  return event.sender._getProcessMemoryInfo(event.processId);
});

// Sandboxed renderers receive their preload scripts and process info via the
// browser-pushed ElectronFrame mojo interface for frames, or
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
