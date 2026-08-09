const { ipcRenderer, webFrame } = require('electron');

const startupWorlds = webFrame.getIsolatedWorlds();
const createdWorlds = [];

webFrame.on('isolated-world-created', (worldId) => {
  createdWorlds.push(worldId);
});

ipcRenderer.on('get-isolated-world-navigation-state', (event) => {
  event.sender.send('isolated-world-navigation-state', {
    startupWorlds,
    createdWorlds,
    isolatedWorlds: webFrame.getIsolatedWorlds()
  });
});
