const common = require('../../../common/api/exports/electron')

// Import common modules.
common.defineProperties(exports)

Object.defineProperties(exports, {
  // Browser side modules, please sort alphabetically.
  // Any modules added here must also be added to the browserModules array
  // in remote.js
  app: {
    enumerable: true,
    get: function () {
      return require('../app')
    }
  },
  autoUpdater: {
    enumerable: true,
    get: function () {
      return require('../auto-updater')
    }
  },
  BrowserWindow: {
    enumerable: true,
    get: function () {
      return require('../browser-window')
    }
  },
  contentTracing: {
    enumerable: true,
    get: function () {
      return require('../content-tracing')
    }
  },
  dialog: {
    enumerable: true,
    get: function () {
      return require('../dialog')
    }
  },
  globalShortcut: {
    enumerable: true,
    get: function () {
      return require('../global-shortcut')
    }
  },
  ipcMain: {
    enumerable: true,
    get: function () {
      return require('../ipc-main')
    }
  },
  Menu: {
    enumerable: true,
    get: function () {
      return require('../menu')
    }
  },
  MenuItem: {
    enumerable: true,
    get: function () {
      return require('../menu-item')
    }
  },
  net: {
    enumerable: true,
    get: function () {
      return require('../net')
    }
  },
  powerMonitor: {
    enumerable: true,
    get: function () {
      return require('../power-monitor')
    }
  },
  powerSaveBlocker: {
    enumerable: true,
    get: function () {
      return require('../power-save-blocker')
    }
  },
  protocol: {
    enumerable: true,
    get: function () {
      return require('../protocol')
    }
  },
  screen: {
    enumerable: true,
    get: function () {
      return require('../screen')
    }
  },
  session: {
    enumerable: true,
    get: function () {
      return require('../session')
    }
  },
  systemPreferences: {
    enumerable: true,
    get: function () {
      return require('../system-preferences')
    }
  },
  TouchBar: {
    enumerable: true,
    get: function () {
      return require('../touch-bar')
    }
  },
  Tray: {
    enumerable: true,
    get: function () {
      return require('../tray')
    }
  },
  webContents: {
    enumerable: true,
    get: function () {
      return require('../web-contents')
    }
  },

  // The internal modules, invisible unless you know their names.
  NavigationController: {
    get: function () {
      return require('../navigation-controller')
    }
  }
})
