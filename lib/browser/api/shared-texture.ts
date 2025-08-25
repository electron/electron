import ipcMain from '@electron/internal/browser/api/ipc-main';
import { fromId as webContentsFromId } from '@electron/internal/browser/api/web-contents';
import * as ipcMainInternalUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import { randomUUID } from 'crypto';

const transferTimeout = 1000;
const sharedTextureNative = process._linkedBinding('electron_common_shared_texture');
const managedSharedTextures = new Map<string, SharedTextureImportedWrapper>();

type AllReleasedCallback = (imported: Electron.SharedTextureImported) => void;

type SharedTextureImportedWrapper = {
  texture: Electron.SharedTextureImported;
  allReferenceReleased: AllReleasedCallback | undefined;
  mainReference: boolean;
  rendererReferences: Map<number, number>;
}

ipcMain.handle(IPC_MESSAGES.IMPORT_SHARED_TEXTURE_RELEASE_RENDERER_TO_MAIN, (event, textureId: string) => {
  wrapperReleaseFromRenderer(textureId, event.sender);
});

function checkManagedSharedTextures () {
  for (const [, wrapper] of managedSharedTextures) {
    for (const [webContentsId] of wrapper.rendererReferences) {
      const wc = webContentsFromId(webContentsId);
      if (!wc || wc.isDestroyed()) {
        console.warn(`Shared texture ${wrapper.texture.textureId} is referenced by destroyed webContents ${webContentsId}, this means a imported shared texture in renderer process is not released before the process is exited.`);
        wrapper.rendererReferences.delete(webContentsId);
      }
    }
  }
}

function wrapperReleaseFromRenderer (id: string, sender: Electron.WebContents) {
  const wrapper = managedSharedTextures.get(id);
  if (!wrapper) {
    throw new Error(`Shared texture with id ${id} not found`);
  }

  let count = wrapper.rendererReferences.get(sender.id);
  if (!count) {
    throw new Error(`Shared texture ${id} is not referenced by renderer ${sender.id}`);
  }

  count -= 1;
  wrapper.rendererReferences.set(sender.id, count);
  if (count === 0) {
    wrapper.rendererReferences.delete(sender.id);
  }

  // Actually release the texture if no one is referencing it
  if (wrapper.rendererReferences.size === 0 && !wrapper.mainReference) {
    wrapper.texture.subtle.release(() => {
      wrapper.allReferenceReleased?.(wrapper.texture);
      managedSharedTextures.delete(id);
    });
  }
}

function wrapperReleaseFromMain (id: string) {
  const wrapper = managedSharedTextures.get(id);
  if (!wrapper) {
    throw new Error(`Shared texture with id ${id} not found`);
  }

  // Actually release the texture if no one is referencing it
  wrapper.mainReference = false;
  if (wrapper.rendererReferences.size === 0) {
    wrapper.texture.subtle.release(() => {
      wrapper.allReferenceReleased?.(wrapper.texture);
      managedSharedTextures.delete(id);
    });
  }
}

async function sendToRenderer (receiver: Electron.WebContents, imported: Electron.SharedTextureImported, ...args: any[]) {
  const transfer = imported.subtle.startTransferSharedTexture();

  const invokePromise = ipcMainInternalUtils.invokeInWebContents<Electron.SharedTextureSyncToken>(
    receiver,
    IPC_MESSAGES.IMPORT_SHARED_TEXTURE_TRANSFER_MAIN_TO_RENDERER,
    transfer,
    imported.textureId,
    ...args
  );

  let timeoutHandle: NodeJS.Timeout | null = null;
  const timeoutPromise = new Promise<never>((resolve, reject) => {
    timeoutHandle = setTimeout(() => {
      reject(new Error(`transfer shared texture timed out after ${transferTimeout}ms, ensure you have registered receiver at renderer process.`));
    }, transferTimeout);
  });

  try {
    const syncToken = await Promise.race([invokePromise, timeoutPromise]);
    imported.subtle.setReleaseSyncToken(syncToken);

    const wrapper = managedSharedTextures.get(imported.textureId);
    wrapper?.rendererReferences.set(receiver.id, (wrapper?.rendererReferences.get(receiver.id) ?? 0) + 1);
  } finally {
    if (timeoutHandle) {
      clearTimeout(timeoutHandle);
    }
  }
}

function importSharedTexture (options: Electron.ImportSharedTextureOptions) {
  const id = randomUUID();
  const imported = sharedTextureNative.importSharedTexture(options.textureInfo);
  const ret: Electron.SharedTextureImported = {
    textureId: id,
    subtle: imported,
    getVideoFrame: imported.getVideoFrame,
    release: () => {
      wrapperReleaseFromMain(id);
    }
  };

  const wrapper: SharedTextureImportedWrapper = {
    texture: ret,
    allReferenceReleased: options.allReferenceReleased,
    mainReference: true,
    rendererReferences: new Map()
  };
  managedSharedTextures.set(id, wrapper);

  return ret;
}

const sharedTexture = {
  subtle: sharedTextureNative,
  importSharedTexture,
  sendToRenderer
};

setInterval(checkManagedSharedTextures, 5000);
export default sharedTexture;
