const { webFrame } = require('electron');

webFrame.on('script-context-created', async (_e, { worldId }) => {
  const isContentScriptWorld = worldId >= (1 << 20);
  if (isContentScriptWorld) {
    const result = await webFrame.executeJavaScriptInIsolatedWorld(worldId, [
      { code: '({ hasChrome: Boolean(window.chrome), ranContentScriptForTest: window.ranContentScriptForTest });' }
    ]);

    // Copy result into electron isolated preload world.
    Object.assign(window, result);
  }
});
