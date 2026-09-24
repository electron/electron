const { app, BrowserWindow } = require('electron');

const exerciseDuringDestroyed = (callback) =>
  new Promise((resolve) => {
    const window = new BrowserWindow({ show: false });
    window.webContents.once('destroyed', () => {
      callback(window);
      resolve();
    });
    window.webContents.destroy();
  });

app.whenReady().then(async () => {
  await exerciseDuringDestroyed((window) => {
    window.setBackgroundColor('#000');
  });
  await exerciseDuringDestroyed((window) => {
    window.setBackgroundMaterial('none');
  });
  app.quit();
});
