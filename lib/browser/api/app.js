'use strict'

const electron = require('electron')
const {Menu} = electron
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

// Wrappers for native classes.
var wrapDownloadItem = function (downloadItem) {
  // downloadItem is an EventEmitter.
  Object.setPrototypeOf(downloadItem, EventEmitter.prototype)
}

downloadItemBindings._setWrapDownloadItem(wrapDownloadItem)

// Only one App object pemitted.
module.exports = app
