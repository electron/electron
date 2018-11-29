const { BrowserView, app } = require('electron')
app.on('ready', function () {
  new BrowserView({})  // eslint-disable-line

  process.nextTick(() => app.quit())
})
