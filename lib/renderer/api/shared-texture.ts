import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import ipcRenderer from '@electron/internal/renderer/api/ipc-renderer';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

const sharedTextureNative = process._linkedBinding('electron_common_shared_texture');
const transferChannelName = IPC_MESSAGES.IMPORT_SHARED_TEXTURE_TRANSFER_MAIN_TO_RENDERER;

type SharedTextureReceiverCallback = (data: Electron.ReceivedSharedTextureData, ...args: any[]) => Promise<void>;
let sharedTextureReceiverCallback: SharedTextureReceiverCallback | null = null;

ipcRendererInternal.on(transferChannelName, async (event, requestId, ...args) => {
  const replyChannel = `${transferChannelName}_RESPONSE_${requestId}`;
  try {
    const transfer = args[0] as Electron.SharedTextureTransfer;
    const textureId = args[1] as string;
    const imported = sharedTextureNative.finishTransferSharedTexture(Object.assign(transfer, { id: textureId }));
    const syncToken = imported.getFrameCreationSyncToken();
    event.sender.send(replyChannel, null, syncToken);

    const wrapper: Electron.SharedTextureImported = {
      textureId,
      subtle: imported,
      getVideoFrame: imported.getVideoFrame,
      release: () => {
        imported.release(async () => {
          await ipcRenderer.invoke(IPC_MESSAGES.IMPORT_SHARED_TEXTURE_RELEASE_RENDERER_TO_MAIN, textureId);
        });
      }
    };

    const data: Electron.ReceivedSharedTextureData = { importedSharedTexture: wrapper };
    await sharedTextureReceiverCallback?.(data, ...args.slice(2));
  } catch (error) {
    event.sender.send(replyChannel, error);
  }
});

const sharedTexture = {
  subtle: sharedTextureNative,
  setSharedTextureReceiver: (callback: SharedTextureReceiverCallback) => {
    sharedTextureReceiverCallback = callback;
  }
};

export default sharedTexture;
