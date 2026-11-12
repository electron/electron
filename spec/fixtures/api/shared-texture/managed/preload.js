const { sharedTexture } = require('electron');
const { ipcRenderer, contextBridge } = require('electron/renderer');

contextBridge.exposeInMainWorld('textures', {
  onSharedTexture: (cb) => {
    // Step 0: Register the receiver for transferred shared texture
    sharedTexture.setSharedTextureReceiver(async (data) => {
      // Step 5: Receive the imported shared texture
      const { importedSharedTexture: imported } = data;
      try {
        await cb(imported);
        ipcRenderer.send('shared-texture-rendered');
      } catch (error) {
        ipcRenderer.send('shared-texture-error', error.stack);
      } finally {
        imported.release();
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
