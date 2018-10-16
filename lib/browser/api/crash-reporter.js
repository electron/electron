'use strict'

const CrashReporter = require('@electron/internal/common/crash-reporter')
const ipcMain = require('@electron/internal/browser/ipc-main-internal')

class CrashReporterMain extends CrashReporter {
  sendSync (channel, ...args) {
    const event = {}
    ipcMain.emit(channel, event, ...args)
    return event.returnValue
  }
}

module.exports = new CrashReporterMain()
