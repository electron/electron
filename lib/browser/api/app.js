'use strict'

const electron = require('electron')
const {deprecate, session, Menu} = electron
const EventEmitter = require('events').EventEmitter

const bindings = process.atomBinding('app')
const downloadItemBindings = process.atomBinding('download_item')
const app = bindings.app

Object.setPrototypeOf(app, EventEmitter.prototype)

app.setApplicationMenu = function (menu) {
  return Menu.setApplicationMenu(menu)
}

app.getApplicationMenu = function () {
  return Menu.getApplicationMenu()
}

app.commandLine = {
  appendSwitch: bindings.appendSwitch,
  appendArgument: bindings.appendArgument
}

if (process.platform === 'darwin') {
  app.dock = {
    bounce: function (type) {
      if (type == null) {
        type = 'informational'
      }
      return bindings.dockBounce(type)
    },
    cancelBounce: bindings.dockCancelBounce,
    downloadFinished: bindings.dockDownloadFinished,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    setMenu: bindings.dockSetMenu,
    setIcon: bindings.dockSetIcon
  }
}

var appPath = null

app.setAppPath = function (path) {
  appPath = path
}

app.getAppPath = function () {
  return appPath
}

// Routes the events to webContents.
var ref1 = ['login', 'certificate-error', 'select-client-certificate']
var fn = function (name) {
  return app.on(name, function (event, webContents, ...args) {
    return webContents.emit.apply(webContents, [name, event].concat(args))
  })
}
var i, len
for (i = 0, len = ref1.length; i < len; i++) {
  fn(ref1[i])
}

// Deprecated.
app.getHomeDir = deprecate('app.getHomeDir', 'app.getPath', function () {
  return this.getPath('home')
})
app.getDataPath = deprecate('app.getDataPath', 'app.getPath', function () {
  return this.getPath('userData')
})
app.setDataPath = deprecate('app.setDataPath', 'app.setPath', function (path) {
  return this.setPath('userData', path)
})
app.resolveProxy = deprecate('app.resolveProxy', 'session.defaultSession.resolveProxy', function (url, callback) {
  return session.defaultSession.resolveProxy(url, callback)
})
deprecate.rename(app, 'terminate', 'quit')
deprecate.event(app, 'finish-launching', 'ready', function () {
  // give default app a chance to setup default menu.
  setImmediate(() => {
    this.emit('finish-launching')
  })
})
deprecate.event(app, 'activate-with-no-open-windows', 'activate', function (event, hasVisibleWindows) {
  if (!hasVisibleWindows) {
    return this.emit('activate-with-no-open-windows', event)
  }
})
deprecate.event(app, 'select-certificate', 'select-client-certificate')
if (process.platform === 'win32') {
  app.isAeroGlassEnabled = deprecate('app.isAeroGlassEnabled', 'systemPreferences.isAeroGlassEnabled', function () {
    return electron.systemPreferences.isAeroGlassEnabled()
  })
} else if (process.platform === 'darwin') {
  app.isDarkMode = deprecate('app.isDarkMode', 'systemPreferences.isDarkMode', function () {
    return electron.systemPreferences.isDarkMode()
  })
  app.on = app.addListener = function (event, listener) {
    if (event === 'platform-theme-changed') {
      deprecate.warn('platform-theme-changed event', "systemPreferences.subscribeNotification('AppleInterfaceThemeChangedNotification', callback)")
      electron.systemPreferences.subscribeNotification('AppleInterfaceThemeChangedNotification', function () {
        app.emit('platform-theme-changed')
      })
    }
    EventEmitter.prototype.addListener.call(app, event, listener)
  }
}

// Wrappers for native classes.
var wrapDownloadItem = function (downloadItem) {
  // downloadItem is an EventEmitter.
  Object.setPrototypeOf(downloadItem, EventEmitter.prototype)

  // Deprecated.
  deprecate.property(downloadItem, 'url', 'getURL')
  deprecate.property(downloadItem, 'filename', 'getFilename')
  deprecate.property(downloadItem, 'mimeType', 'getMimeType')
  return deprecate.rename(downloadItem, 'getUrl', 'getURL')
}

downloadItemBindings._setWrapDownloadItem(wrapDownloadItem)

// Only one App object pemitted.
module.exports = app
