'use strict'

const CrashReporter = require('@electron/internal/common/crash-reporter')
const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')

class CrashReporterRenderer extends CrashReporter {
  sendSync (channel, ...args) {
    return ipcRenderer.sendSync(channel, ...args)
  }
}

module.exports = new CrashReporterRenderer()
