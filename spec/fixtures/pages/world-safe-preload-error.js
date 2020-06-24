const { ipcRenderer, webFrame } = require('electron');

webFrame.executeJavaScript(`(() => {
  return Symbol('a');
})()`).catch((err) => {
  // Considered safe if the object is constructed in this world
  ipcRenderer.send('executejs-safe', err);
});
