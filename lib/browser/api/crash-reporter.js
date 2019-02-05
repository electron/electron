'use strict'

const CrashReporter = require('@electron/internal/common/crash-reporter')
const { crashReporterInit } = require('@electron/internal/browser/crash-reporter-init')

class CrashReporterMain extends CrashReporter {
  init (options) {
    return crashReporterInit(options)
  }
}

module.exports = new CrashReporterMain()
