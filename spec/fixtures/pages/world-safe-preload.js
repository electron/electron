const { ipcRenderer, webFrame } = require('electron');

webFrame.executeJavaScript(`(() => {
  return {};
})()`).then((obj) => {
  // Considered safe if the object is constructed in this world
  ipcRenderer.send('executejs-safe', obj.constructor === Object);
});
