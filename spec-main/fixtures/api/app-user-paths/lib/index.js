const { app } = require('electron')
const { dialog } = require('electron')

const defaultPayload = {
  'custom-appname': app.commandLine.getSwitchValue('custom-appname'),
  'custom-appdata': app.commandLine.getSwitchValue('custom-appdata'),
  defaultAppName: app.name,
  defaultAppData: app.getPath('appData'),
  defaultAppCache: app.getPath('cache'),
  defaultUserCache: app.getPath('userCache'),
  defaultUserData: app.getPath('userData')
}

function exitApp() {
  const payload = {
    ...defaultPayload,
    appName: app.name,
    appData: app.getPath('appData'),
    appCache: app.getPath('cache'),
    userCache: app.getPath('userCache'),
    userData: app.getPath('userData')
  }
  process.stdout.write(JSON.stringify(payload))

// dialog.showMessageBoxSync({
//   type: 'info',
//   message: JSON.stringify(payload, null, 4)
// })

  process.stdout.end()

  setImmediate(() => {
    app.quit()
  })
}

// dialog.showErrorBox('debug', JSON.stringify(defaultPayload, null, 4))

if (defaultPayload['custom-appname']) {
  app.name = defaultPayload['custom-appname']
}

if (defaultPayload['custom-appdata']) {
  app.setPath('appData', defaultPayload['custom-appdata'])
}

app.on('ready', () => {
    exitApp();
})


