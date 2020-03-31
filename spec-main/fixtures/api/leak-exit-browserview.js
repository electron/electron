const { BrowserView, app } = require('electron');
app.whenReady().then(function () {
  new BrowserView({})  // eslint-disable-line

  app.quit();
});
