const { contextBridge, webUtils, sharedTexture } = require('electron');

sharedTexture.setSharedTextureReceiver(async ({ importedSharedTexture }) => {
  const frame = importedSharedTexture.getVideoFrame();
  try {
    // Test-only readback, after transfer, to verify the original snapshot.
    const bytes = new Uint8Array(frame.allocationSize({ format: 'RGBA' }));
    const [layout] = await frame.copyTo(bytes, { format: 'RGBA' });
    const pixel = (x, y) =>
      Array.from(bytes.slice(layout.offset + y * layout.stride + x * 4, layout.offset + y * layout.stride + x * 4 + 4));
    window.nativeFixturePixels = [pixel(8, 8), pixel(500, 8)];
  } finally {
    frame.close();
    importedSharedTexture.release();
  }
});

contextBridge.exposeInMainWorld('nativeHandleTest', {
  export(frame, closeEarly = false) {
    const result = webUtils.getNativeVideoFrameHandle(frame);
    if (closeEarly) frame.close();
    return result;
  },
  release: (id) => webUtils.releaseNativeVideoFrameHandle(id),
  pixels: () => window.nativeFixturePixels
});
