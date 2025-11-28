const { sharedTexture } = require('electron');
const { ipcRenderer, contextBridge } = require('electron/renderer');

contextBridge.exposeInMainWorld('textures', {
  onSharedTexture: (cb) => {
    // Step 0: Register the receiver for transferred shared texture
    sharedTexture.setSharedTextureReceiver(async (data) => {
      // Step 5: Receive the imported shared texture
      const { importedSharedTexture: imported } = data;
      await cb(imported);

      // Release the imported shared texture since we're done.
      // No need to use the callback here, as the util function automatically
      // managed the lifetime via sync token.
      imported.release();
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
