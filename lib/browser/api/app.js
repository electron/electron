'use strict'

const {Menu} = require('electron')
const {EventEmitter} = require('events')

const bindings = process.atomBinding('app')
const downloadItemBindings = process.atomBinding('download_item')
const {app} = bindings

Object.setPrototypeOf(app, EventEmitter.prototype)

let appPath = null

Object.assign(app, {
  getAppPath () { return appPath },
  setAppPath (path) { appPath = path },
  setApplicationMenu (menu) {
    return Menu.setApplicationMenu(menu)
  },
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    appendSwitch: bindings.appendSwitch,
    appendArgument: bindings.appendArgument
  }
})

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
    setMenu: bindings.dockSetMenu,
    setIcon: bindings.dockSetIcon
  }
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (let name of events) {
  app.on(name, (event, webContents, ...args) => {
    return webContents.emit.apply(webContents, [name, event].concat(args))
  })
}

// Wrappers for native classes.
downloadItemBindings._setWrapDownloadItem((downloadItem) => {
  // downloadItem is an EventEmitter.
  Object.setPrototypeOf(downloadItem, EventEmitter.prototype)
})

// Only one App object pemitted.
module.exports = app
