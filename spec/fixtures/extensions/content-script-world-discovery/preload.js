const { ipcRenderer, webFrame } = require('electron');

// Matches electron::WorldIDs::ISOLATED_WORLD_ID_EXTENSIONS in world_ids.h,
// which mirrors Chromium's extension world ID allocation base.
const kExtensionWorldIdBase = 1 << 20;

const injectIntoWorld = (worldId) => {
  if (worldId < kExtensionWorldIdBase) return;

  webFrame.executeJavaScriptInIsolatedWorld(worldId, [{
    code: 'window.__electronWorldDiscovered = true;'
  }]);
};

for (const worldId of webFrame.getIsolatedWorlds()) {
  injectIntoWorld(worldId);
}

webFrame.on('isolated-world-created', injectIntoWorld);

ipcRenderer.on('get-isolated-worlds', (event) => {
  event.sender.send('isolated-worlds', webFrame.getIsolatedWorlds());
});
