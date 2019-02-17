import { app } from 'electron'
import * as cp from 'child_process'
import * as os from 'os'
import * as path from 'path'

import { CrashReporterInitOptions, CrashReporterInitResult } from '@electron/internal/common/crash-reporter'

const getTempDirectory = () => {
  try {
    return app.getPath('temp')
  } catch {
    return os.tmpdir()
  }
}

export const crashReporterInit = function (options: CrashReporterInitOptions): CrashReporterInitResult {
  const productName = options.productName || app.getName()
  const crashesDirectory = path.join(getTempDirectory(), `${productName} Crashes`)
  let crashServicePid

  if (process.platform === 'win32') {
    const env = {
      ELECTRON_INTERNAL_CRASH_SERVICE: '1'
    }
    const args = [
      '--reporter-url=' + options.submitURL,
      '--application-name=' + productName,
      '--crashes-directory=' + crashesDirectory,
      '--v=1'
    ]

    const crashServiceProcess = cp.spawn(process.helperExecPath, args, {
      env,
      detached: true
    })

    crashServicePid = crashServiceProcess.pid
  }

  return {
    productName,
    crashesDirectory,
    crashServicePid,
    appVersion: app.getVersion()
  }
}
