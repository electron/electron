const init = async () => {
  await window.initWebGpu();
  window.textures.onSharedTexture(async (imported) => {
    const frame = imported.getVideoFrame();
    try {
      await window.renderFrame(frame);
    } finally {
      frame.close();
    }
  });
  window.textures.verifyCapturedImage(window.verifyCapturedImage);
  window.textures.ready();
};

init().catch((error) => window.textures.reportError(error.stack));
