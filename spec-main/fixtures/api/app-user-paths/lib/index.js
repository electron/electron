const { app } = require('electron')

function exitApp() {
  const payload = {
    appName: app.name,
    appData: app.getPath('appData'),
    userCache: app.getPath('userCache'),
    userData: app.getPath('userData')
  }
  process.stdout.write(JSON.stringify(payload))
  process.stdout.end()

  setImmediate(() => {
    app.quit()
  })
}

if (app.commandLine.hasSwitch('custom-appname')) {
  const appName = app.commandLine.getSwitchValue('custom-appname')
  app.name = appName
}

if (app.commandLine.hasSwitch('custom-appdata')) {
  const appData = app.commandLine.getSwitchValue('custom-appdata')
  app.setPath('appData', appData)
}

app.on('ready', () => {
    exitApp();
})


