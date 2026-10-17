import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO, function (event) {
  if (event.type !== 'frame') return;
  // Report the calling frame's own renderer process, which for an
  // out-of-process iframe is not the main frame's.
  return event.sender._getProcessMemoryInfo(event.processId);
});

ipcMainInternal.on(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, function (event, preloadPath: string, error: Error) {
  if (event.type !== 'frame') return;
  event.sender?.emit('preload-error', event, preloadPath, error);
});
