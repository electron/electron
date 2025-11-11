const { sharedTexture } = require('electron');
const { ipcRenderer, contextBridge } = require('electron/renderer');

contextBridge.exposeInMainWorld('textures', {
  onSharedTexture: (cb) => {
    ipcRenderer.on('shared-texture', async (e, id, transfer) => {
      // Step 5: Get the shared texture from the transfer
      const importedSubtle = sharedTexture.subtle.finishTransferSharedTexture(transfer);

      // Step 6: Let the renderer render using WebGPU
      await cb(id, importedSubtle);

      // Step 10: Release the shared texture with a callback
      importedSubtle.release(() => {
        // Step 11: When GPU command buffer is done, we can notify the main process to release
        ipcRenderer.send('shared-texture-done', id);
      });
    });
  },
  webGpuUnavailable: () => {
    ipcRenderer.send('webgpu-unavailable');
  },
  verifyCapturedImage: (verify) => {
    ipcRenderer.on('verify-captured-image', (e, images) => {
      verify(images, (result) => {
        ipcRenderer.send('verify-captured-image-done', result);
      });
    });
  }
});
