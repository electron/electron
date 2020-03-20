const { WebContentsView, app, webContents } = require('electron');
app.whenReady().then(function () {
  const web = webContents.create({});
  new WebContentsView(web)  // eslint-disable-line

  app.quit();
});
