'use strict'

const electron = require('electron')
const { EventEmitter } = require('events')
const { TopLevelWindow } = process.atomBinding('top_level_window')

Object.setPrototypeOf(TopLevelWindow.prototype, EventEmitter.prototype)

TopLevelWindow.prototype._init = function () {
  // Avoid recursive require.
  const { app } = electron

  // Simulate the application menu on platforms other than macOS.
  if (process.platform !== 'darwin') {
    const menu = app.getApplicationMenu()
    if (menu) this.setMenu(menu)
  }
}

TopLevelWindow.getFocusedWindow = () => {
  return TopLevelWindow.getAllWindows().find((win) => win.isFocused())
}

module.exports = TopLevelWindow
