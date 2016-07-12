'use strict'

const {ipcMain} = require('electron')

var NavigationController = (function () {
  function NavigationController (webContents) {
    this.webContents = webContents
  }

  NavigationController.prototype.loadURL = function (url, options) {
    if (options == null) {
      options = {}
    }
    this.webContents._loadURL(url, options)
    return this.webContents.emit('load-url', url, options)
  }

  NavigationController.prototype.reload = function () {
    return this.webContents._reload(false)
  }

  NavigationController.prototype.reloadIgnoringCache = function () {
    return this.webContents._reload(true)
  }

  return NavigationController
})()

module.exports = NavigationController
