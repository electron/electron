const { app, BrowserWindow } = require('electron')
const { dialog } = require('electron')

const defaultPayload = {
  'custom-appname': app.commandLine.getSwitchValue('custom-appname'),
  'custom-appdata': app.commandLine.getSwitchValue('custom-appdata'),
  'custom-applogs': app.commandLine.getSwitchValue('custom-applogs'),
  'custom-userdata': app.commandLine.getSwitchValue('custom-userdata'),
  'custom-usercache': app.commandLine.getSwitchValue('custom-usercache'),
  defaultAppName: app.name,
  defaultAppData: app.getPath('appData'),
  defaultAppCache: app.getPath('cache'),
  defaultUserCache: app.getPath('userCache'),
  defaultUserData: app.getPath('userData'),
  defaultAppLogs: app.getPath('logs')
}

function exitApp () {
  const payload = {
    ...defaultPayload,
    appName: app.name,
    appData: app.getPath('appData'),
    appCache: app.getPath('cache'),
    userCache: app.getPath('userCache'),
    userData: app.getPath('userData'),
    appLogs: app.getPath('logs')
  }
  process.stdout.write(JSON.stringify(payload))
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

if (defaultPayload['custom-applogs']) {
  app.setAppLogsPath(defaultPayload['custom-applogs'])
}

if (defaultPayload['custom-userdata']) {
  app.setPath('userData', defaultPayload['custom-userdata'])
}

if (defaultPayload['custom-usercache']) {
  app.setPath('userCache', defaultPayload['custom-usercache'])
}

app.on('ready', () => {
  if (app.commandLine.getSwitchValue('create-cache')) {
    const w = new BrowserWindow({
      show: false
    })
    w.webContents.on('did-finish-load', () => {
      exitApp()
    })
    w.loadURL('about:blank')
  } else {
    exitApp()
  }
})
