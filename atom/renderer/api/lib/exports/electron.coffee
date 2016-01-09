common = require '../../../../common/api/lib/exports/electron'

# Import common modules.
common.defineProperties exports

Object.defineProperties exports,
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
