Object.defineProperties(exports, {
  ipcRenderer: {
    enumerable: true,
    get: function () {
      return require('../ipc-renderer')
    }
  },
  remote: {
    enumerable: true,
    get: function () {
      return require('../../../renderer/api/remote')
    }
  },
  webFrame: {
    enumerable: true,
    get: function () {
      return require('../../../renderer/api/web-frame')
    }
  },
  crashReporter: {
    enumerable: true,
    get: function () {
      return require('../../../common/api/crash-reporter')
    }
  },
  CallbacksRegistry: {
    get: function () {
      return require('../../../common/api/callbacks-registry')
    }
  },
  isPromise: {
    get: function () {
      return require('../../../common/api/is-promise')
    }
  },
  // XXX(alexeykuzmin): It won't be available if the Desktop Capturer
  // was disabled during build time.
  desktopCapturer: {
    get: function () {
      return require('../../../renderer/api/desktop-capturer')
    }
  },
  nativeImage: {
    get: function () {
      return require('../../../common/api/native-image')
    }
  }
})
