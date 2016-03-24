const deprecate = require('electron').deprecate
const ipcMain = require('electron').ipcMain

// This module is deprecated, we mirror everything from ipcMain.
deprecate.warn('ipc module', 'require("electron").ipcMain')

module.exports = ipcMain
