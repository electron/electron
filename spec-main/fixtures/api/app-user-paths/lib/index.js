const { app } = require('electron')

function exitApp() {
  const payload = {
    userCache: app.getPath('userCache'),
    userData: app.getPath('userData')
  }
  process.stdout.write(JSON.stringify(payload))
  process.stdout.end()

  setImmediate(() => {
    app.quit()
  })
}

if (app.hasSwitch('default')) {
  app.on('ready', () => {
    exitApp();
  })
}
