window.initWebGpu().catch((err) => {
  console.error('Failed to initialize WebGPU:', err);
  window.textures.webGpuUnavailable();
});

window.textures.onSharedTexture(async (id, imported) => {
  try {
    // Step 7: Get VideoFrame from the imported texture
    const frame = imported.getVideoFrame();

    // Step 8: Render using WebGPU
    await window.renderFrame(frame);

    // Step 9: Release the VideoFrame as we no longer need it
    frame.close();
  } catch (error) {
    console.error('Error getting VideoFrame:', error);
  }
});

window.textures.verifyCapturedImage(window.verifyCapturedImage);
