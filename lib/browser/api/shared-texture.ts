import ipcMain from '@electron/internal/browser/api/ipc-main';
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
  rendererFrameReferences: Map<number, { count: number, reference: Electron.WebFrameMain }>;
}

ipcMain.handle(IPC_MESSAGES.IMPORT_SHARED_TEXTURE_RELEASE_RENDERER_TO_MAIN, (event: Electron.IpcMainInvokeEvent, textureId: string) => {
  const frameTreeNodeId = event.frameTreeNodeId ?? event.sender.mainFrame.frameTreeNodeId;
  wrapperReleaseFromRenderer(textureId, frameTreeNodeId);
});

function checkManagedSharedTextures () {
  for (const [, wrapper] of managedSharedTextures) {
    for (const [frameTreeNodeId, entry] of wrapper.rendererFrameReferences) {
      const frame = entry.reference;
      if (!frame || frame.isDestroyed()) {
        console.warn(`Shared texture ${wrapper.texture.textureId} is referenced by a destroyed webContent/webFrameMain, this means a imported shared texture in renderer process is not released before the process is exited.`);
        wrapper.rendererFrameReferences.delete(frameTreeNodeId);
      }
    }
  }
}

function wrapperReleaseFromRenderer (id: string, frameTreeNodeId: number) {
  const wrapper = managedSharedTextures.get(id);
  if (!wrapper) {
    throw new Error(`Shared texture with id ${id} not found`);
  }

  const entry = wrapper.rendererFrameReferences.get(frameTreeNodeId);
  if (!entry) {
    throw new Error(`Shared texture ${id} is not referenced by renderer frame ${frameTreeNodeId}`);
  }

  entry.count -= 1;
  if (entry.count === 0) {
    wrapper.rendererFrameReferences.delete(frameTreeNodeId);
  } else {
    wrapper.rendererFrameReferences.set(frameTreeNodeId, entry);
  }

  // Actually release the texture if no one is referencing it
  if (wrapper.rendererFrameReferences.size === 0 && !wrapper.mainReference) {
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
  if (wrapper.rendererFrameReferences.size === 0) {
    wrapper.texture.subtle.release(() => {
      wrapper.allReferenceReleased?.(wrapper.texture);
      managedSharedTextures.delete(id);
    });
  }
}

async function sendSharedTexture (options: Electron.SendSharedTextureOptions, ...args: any[]) {
  const imported = options.importedSharedTexture;
  const transfer = imported.subtle.startTransferSharedTexture();

  let timeoutHandle: NodeJS.Timeout | null = null;
  const timeoutPromise = new Promise<never>((resolve, reject) => {
    timeoutHandle = setTimeout(() => {
      reject(new Error(`transfer shared texture timed out after ${transferTimeout}ms, ensure you have registered receiver at renderer process.`));
    }, transferTimeout);
  });

  const targetFrame: Electron.WebFrameMain | undefined = options.frame;
  if (!targetFrame) {
    throw new Error('`frame` should be provided');
  }

  const invokePromise: Promise<Electron.SharedTextureSyncToken> = ipcMainInternalUtils.invokeInWebFrameMain<Electron.SharedTextureSyncToken>(
    targetFrame,
    IPC_MESSAGES.IMPORT_SHARED_TEXTURE_TRANSFER_MAIN_TO_RENDERER,
    transfer,
    imported.textureId,
    ...args
  );

  try {
    const syncToken = await Promise.race([invokePromise, timeoutPromise]);
    imported.subtle.setReleaseSyncToken(syncToken);

    const wrapper = managedSharedTextures.get(imported.textureId);
    if (!wrapper) {
      throw new Error(`Shared texture with id ${imported.textureId} not found`);
    }

    const key = targetFrame.frameTreeNodeId;
    const existing = wrapper.rendererFrameReferences.get(key);
    if (existing) {
      existing.count += 1;
      wrapper.rendererFrameReferences.set(key, existing);
    } else {
      wrapper.rendererFrameReferences.set(key, { count: 1, reference: targetFrame });
    }
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
    rendererFrameReferences: new Map()
  };
  managedSharedTextures.set(id, wrapper);

  return ret;
}

const sharedTexture = {
  subtle: sharedTextureNative,
  importSharedTexture,
  sendSharedTexture
};

setInterval(checkManagedSharedTextures, 5000);
export default sharedTexture;
