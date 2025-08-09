import * as ipcMainInternalUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const sharedTextureNative = process._linkedBinding('electron_common_shared_texture');
const transferTimeout = 200;

const sharedTexture = {
  importSharedTexture: sharedTextureNative.importSharedTexture,
  finishTransferSharedTexture: sharedTextureNative.finishTransferSharedTexture,
  sendToRenderer: async (receiver: Electron.WebContents, importedSharedTexture: Electron.SharedTextureImported, ...args: any[]) => {
    const transfer = importedSharedTexture.startTransferSharedTexture();

    let timeoutHandle: NodeJS.Timeout | null = null;

    const invokePromise = ipcMainInternalUtils.invokeInWebContents<Electron.SharedTextureSyncToken>(
      receiver,
      IPC_MESSAGES.IMPORT_SHARED_TEXTURE_TRANSFER_MAIN_TO_RENDERER,
      transfer,
      ...args
    );

    const timeoutPromise = new Promise<never>((resolve, reject) => {
      timeoutHandle = setTimeout(() => {
        reject(new Error(`transfer shared texture timed out after ${transferTimeout}ms, ensure you have registered receiver at renderer process.`));
      }, transferTimeout);
    });

    try {
      const syncToken = await Promise.race([invokePromise, timeoutPromise]);
      importedSharedTexture.setReleaseSyncToken(syncToken);
    } finally {
      if (timeoutHandle) {
        clearTimeout(timeoutHandle);
      }
    }
  }
};

export default sharedTexture;
