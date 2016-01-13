var deprecate, ipcMain, ref;

ref = require('electron'), deprecate = ref.deprecate, ipcMain = ref.ipcMain;


/* This module is deprecated, we mirror everything from ipcMain. */

deprecate.warn('ipc module', 'require("electron").ipcMain');

module.exports = ipcMain;
