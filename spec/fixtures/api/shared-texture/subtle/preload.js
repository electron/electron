const { sharedTexture } = require('electron');
const { ipcRenderer, contextBridge } = require('electron/renderer');

contextBridge.exposeInMainWorld('textures', {
  onSharedTexture: (cb) => {
    ipcRenderer.on('shared-texture', async (e, transfer) => {
      // Step 5: Get the shared texture from the transfer
      const importedSubtle = sharedTexture.subtle.finishTransferSharedTexture(transfer);

      // Step 6: Let the renderer render using WebGPU
      try {
        await cb(importedSubtle);
      } catch (error) {
        ipcRenderer.send('shared-texture-error', error.stack);
      } finally {
        importedSubtle.release(() => {
          ipcRenderer.send('shared-texture-done');
        });
      }
    });
  },
  ready: () => {
    ipcRenderer.send('shared-texture-ready');
  },
  reportError: (error) => {
    ipcRenderer.send('shared-texture-error', error);
  },
  verifyCapturedImage: (verify) => {
    ipcRenderer.on('verify-captured-image', (e, images) => {
      verify(images, (result) => {
        ipcRenderer.send('verify-captured-image-done', result);
      });
    });
  }
});
