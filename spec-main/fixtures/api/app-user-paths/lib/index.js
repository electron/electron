const { app } = require('electron')

function exitApp() {
  const payload = {
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

if (app.commandline.hasSwitch('default')) {
  app.on('ready', () => {
    exitApp();
  })
}

if (app.hasSwitch('custom-appname')) {
  const appName = app.commandline.getSwitchValue('custom-appname');
  app.setName(appName);
  app.on('ready', () => {
    exitApp();
  })
}

if (app.hasSwitch('custom-appdata')) {
  const appData = app.commandline.getSwitchValue('custom-appdata');
  app.setPath('appData', appData);
  app.on('ready', () => {
    exitApp();
  })
}
