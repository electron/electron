# Import common modules.
module.exports = require '../../../../common/api/lib/exports/electron'

Object.defineProperties module.exports,
  # Browser side modules, please sort with alphabet order.
  app:
    enumerable: true
    get: -> require '../app'
  autoUpdater:
    enumerable: true
    get: -> require '../auto-updater'
  BrowserWindow:
    enumerable: true
    get: -> require '../browser-window'
  contentTracing:
    enumerable: true
    get: -> require '../content-tracing'
  dialog:
    enumerable: true
    get: -> require '../dialog'
  ipcMain:
    enumerable: true
    get: -> require '../ipc-main'
  globalShortcut:
    enumerable: true
    get: -> require '../global-shortcut'
  Menu:
    enumerable: true
    get: -> require '../menu'
  MenuItem:
    enumerable: true
    get: -> require '../menu-item'
  powerMonitor:
    enumerable: true
    get: -> require '../power-monitor'
  powerSaveBlocker:
    enumerable: true
    get: -> require '../power-save-blocker'
  protocol:
    enumerable: true
    get: -> require '../protocol'
  screen:
    enumerable: true
    get: -> require '../screen'
  tray:
    enumerable: true
    get: -> require '../tray'
