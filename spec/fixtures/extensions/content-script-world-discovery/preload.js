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

const startupWorlds = webFrame.getIsolatedWorlds();
const createdWorlds = [];

for (const worldId of startupWorlds) {
  injectIntoWorld(worldId);
}

webFrame.on('isolated-world-created', (worldId) => {
  createdWorlds.push(worldId);
  injectIntoWorld(worldId);
});

ipcRenderer.on('get-world-discovery-state', (event) => {
  event.sender.send('world-discovery-state', {
    startupWorlds,
    createdWorlds,
    isolatedWorlds: webFrame.getIsolatedWorlds()
  });
});
