'use strict'

const CrashReporter = require('@electron/internal/common/crash-reporter')
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils')

class CrashReporterRenderer extends CrashReporter {
  init (options) {
    return ipcRendererUtils.invokeSync('ELECTRON_CRASH_REPORTER_INIT', options)
  }
}

module.exports = new CrashReporterRenderer()
