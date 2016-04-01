const common = require('../../../common/api/exports/electron')

// Import common modules.
common.defineProperties(exports)

Object.defineProperties(exports, {
  // Renderer side modules, please sort with alphabet order.
  desktopCapturer: {
    enumerable: true,
    get: function () {
      return require('../desktop-capturer')
    }
  },
  ipcRenderer: {
    enumerable: true,
    get: function () {
      return require('../ipc-renderer')
    }
  },
  remote: {
    enumerable: true,
    get: function () {
      return require('../remote')
    }
  },
  screen: {
    enumerable: true,
    get: function () {
      return require('../screen')
    }
  },
  webFrame: {
    enumerable: true,
    get: function () {
      return require('../web-frame')
    }
  }
})
