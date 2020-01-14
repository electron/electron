const { WebContentsView, app, webContents } = require('electron')
app.on('ready', function () {
  const web = webContents.create({})
  new WebContentsView(web)  // eslint-disable-line

  app.quit()
})
