const { WebContentsView, app } = require('electron');
app.whenReady().then(function () {
  // eslint-disable-next-line no-new
  new WebContentsView();

  app.quit();
});
