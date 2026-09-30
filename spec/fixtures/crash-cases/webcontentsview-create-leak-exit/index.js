const { WebContentsView, app } = require('electron');

app.whenReady().then(function () {
  // oxlint-disable-next-line no-new
  new WebContentsView();

  app.quit();
});
