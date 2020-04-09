const { WebContentsView, app } = require('electron');
app.whenReady().then(function () {
  new WebContentsView({})  // eslint-disable-line

  app.quit();
});
