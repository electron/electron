'use strict'

const CrashReporter = require('@electron/internal/common/crash-reporter')
const ipcRenderer = require('@electron/internal/renderer/ipc-renderer-internal')
const errorUtils = require('@electron/internal/common/error-utils')

const invoke = function (command, ...args) {
  const [ error, result ] = ipcRenderer.sendSync(command, ...args)

  if (error) {
    throw errorUtils.deserialize(error)
  }

  return result
}

class CrashReporterRenderer extends CrashReporter {
  init (options) {
    return invoke('ELECTRON_CRASH_REPORTER_INIT', options)
  }
}

module.exports = new CrashReporterRenderer()
