common = require '../../../../common/api/lib/exports/electron'

### Import common modules. ###
common.defineProperties exports

Object.defineProperties exports,
  ### Browser side modules, please sort with alphabet order. ###
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
  session:
    enumerable: true
    get: -> require '../session'
  Tray:
    enumerable: true
    get: -> require '../tray'
  ### The internal modules, invisible unless you know their names. ###
  NavigationController:
    get: -> require '../navigation-controller'
  webContents:
    get: -> require '../web-contents'
