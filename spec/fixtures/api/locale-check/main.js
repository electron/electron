const {app} = require('electron')

process.stdout.write(app.getLocale())
process.stdout.end()

setImmediate(() => {
  app.quit()
})
