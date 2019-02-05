'use strict'

const { app } = require('electron')
const cp = require('child_process')
const os = require('os')
const path = require('path')

const getTempDirectory = function () {
  try {
    return app.getPath('temp')
  } catch (error) {
    return os.tmpdir()
  }
}

exports.crashReporterInit = function (options) {
  const productName = options.productName || app.getName()
  const crashesDirectory = path.join(getTempDirectory(), `${productName} Crashes`)
  let crashServicePid

  if (process.platform === 'win32') {
    const env = {
      ELECTRON_INTERNAL_CRASH_SERVICE: 1
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
