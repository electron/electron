window.textures.onSharedTexture(async (imported) => {
  try {
    // Step 6: Get VideoFrame from the imported texture
    const frame = imported.getVideoFrame();

    // Step 7: Render using WebGPU
    await window.renderFrame(frame);

    // Step 8: Release the VideoFrame as we no longer need it
    frame.close();

    // Step 9: Wait until the rendered canvas has been presented, the main
    // process captures this window once the shared texture is released.
    await window.waitForPresentation();
  } catch (error) {
    console.error('Error rendering shared texture:', error);
    window.textures.renderFailed(String(error));
  }
});

window.textures.verifyCapturedImage(window.verifyCapturedImage);

// Tell the main process to start sending textures only once the receiver
// above is registered and WebGPU is ready to render them.
window.initWebGpu().then(
  () => window.textures.webGpuReady(),
  (err) => {
    console.error('Failed to initialize WebGPU:', err);
    window.textures.webGpuUnavailable();
  }
);
