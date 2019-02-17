import { CrashReporter, CrashReporterInitOptions } from '@electron/internal/common/crash-reporter'
import { crashReporterInit } from '@electron/internal/browser/crash-reporter-init'

class CrashReporterMain extends CrashReporter implements Electron.CrashReporter {
  init (options: CrashReporterInitOptions) {
    return crashReporterInit(options)
  }
}

module.exports = new CrashReporterMain()
