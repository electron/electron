# Import common modules.
module.exports = require '../../../../common/api/lib/exports/electron'

Object.defineProperties module.exports,
  # Renderer side modules, please sort with alphabet order.
  desktopCapturer:
    enumerable: true
    get: -> require '../desktop-capturer'
  ipcRenderer:
    enumerable: true
    get: -> require '../ipc-renderer'
  remote:
    enumerable: true
    get: -> require '../remote'
  screen:
    enumerable: true
    get: -> require '../screen'
  webFrame:
    enumerable: true
    get: -> require '../web-frame'
