'use strict'

const bindings = process.atomBinding('app')
const commandLine = process.atomBinding('command_line')
const path = require('path')
const { app, App } = bindings

// Only one app object permitted.
module.exports = app

const electron = require('electron')
const { deprecate, Menu } = electron
const { EventEmitter } = require('events')

let dockMenu = null

// App is an EventEmitter.
Object.setPrototypeOf(App.prototype, EventEmitter.prototype)
EventEmitter.call(app)

Object.assign(app, {
  setApplicationMenu (menu) {
    return Menu.setApplicationMenu(menu)
  },
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    hasSwitch: (...args) => commandLine.hasSwitch(...args.map(String)),
    getSwitchValue: (...args) => commandLine.getSwitchValue(...args.map(String)),
    appendSwitch: (...args) => commandLine.appendSwitch(...args.map(String)),
    appendArgument: (...args) => commandLine.appendArgument(...args.map(String))
  },
  enableMixedSandbox () {
    deprecate.log(`'enableMixedSandbox' is deprecated. Mixed-sandbox mode is now enabled by default. You can safely remove the call to enableMixedSandbox().`)
  }
})

app.getFileIcon = deprecate.promisify(app.getFileIcon)

app.isPackaged = (() => {
  const execFile = path.basename(process.execPath).toLowerCase()
  if (process.platform === 'win32') {
    return execFile !== 'electron.exe'
  }
  return execFile !== 'electron'
})()

app._setDefaultAppPaths = (packagePath) => {
  // Set the user path according to application's name.
  app.setPath('userData', path.join(app.getPath('appData'), app.getName()))
  app.setPath('userCache', path.join(app.getPath('cache'), app.getName()))
  app.setAppPath(packagePath)

  // Add support for --user-data-dir=
  const userDataDirFlag = '--user-data-dir='
  const userDataArg = process.argv.find(arg => arg.startsWith(userDataDirFlag))
  if (userDataArg) {
    const userDataDir = userDataArg.substr(userDataDirFlag.length)
    if (path.isAbsolute(userDataDir)) app.setPath('userData', userDataDir)
  }
}

if (process.platform === 'darwin') {
  app.dock = {
    bounce (type = 'informational') {
      return bindings.dockBounce(type)
    },
    cancelBounce: bindings.dockCancelBounce,
    downloadFinished: bindings.dockDownloadFinished,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    isVisible: bindings.dockIsVisible,
    setMenu (menu) {
      dockMenu = menu
      bindings.dockSetMenu(menu)
    },
    getMenu () {
      return dockMenu
    },
    setIcon: bindings.dockSetIcon
  }
}

if (process.platform === 'linux') {
  app.launcher = {
    setBadgeCount: bindings.unityLauncherSetBadgeCount,
    getBadgeCount: bindings.unityLauncherGetBadgeCount,
    isCounterBadgeAvailable: bindings.unityLauncherAvailable,
    isUnityRunning: bindings.unityLauncherAvailable
  }
}

app.allowNTLMCredentialsForAllDomains = function (allow) {
  deprecate.warn('app.allowNTLMCredentialsForAllDomains', 'session.allowNTLMCredentialsForDomains')
  const domains = allow ? '*' : ''
  if (!this.isReady()) {
    this.commandLine.appendSwitch('auth-server-whitelist', domains)
  } else {
    electron.session.defaultSession.allowNTLMCredentialsForDomains(domains)
  }
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (const name of events) {
  app.on(name, (event, webContents, ...args) => {
    webContents.emit(name, event, ...args)
  })
}

// Wrappers for native classes.
const { DownloadItem } = process.atomBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
