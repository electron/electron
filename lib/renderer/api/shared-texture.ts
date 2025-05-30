const sharedTextureNative = process._linkedBinding('electron_common_shared_texture');
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

type SharedTextureTransferredCallback = (importedSharedTexture: Electron.SharedTextureImported, ...args: any[]) => Promise<void>
let sharedTextureTransferredCallback: SharedTextureTransferredCallback | null = null

const transferChannelName = IPC_MESSAGES.IMPORT_SHARED_TEXTURE_TRANSFER_MAIN_TO_RENDERER
ipcRendererInternal.on(transferChannelName, async (event, requestId, ...args) => {
  const replyChannel = `${transferChannelName}_RESPONSE_${requestId}`;
  try {
    const transfer = args[0] as Electron.SharedTextureTransfer
    const imported = sharedTextureNative.finishTransferSharedTexture(transfer)
    const syncToken = imported.getFrameCreationSyncToken()
    event.sender.send(replyChannel, null, syncToken)
    await sharedTextureTransferredCallback?.(imported, ...args.slice(1))
  } catch (error) {
    event.sender.send(replyChannel, error);
  }
});

const sharedTexture = {
  importSharedTexture: sharedTextureNative.importSharedTexture,
  finishTransferSharedTexture: sharedTextureNative.finishTransferSharedTexture,
  receiveFromMain: (listener: SharedTextureTransferredCallback) => {
    sharedTextureTransferredCallback = listener
  }
}

export default sharedTexture;
