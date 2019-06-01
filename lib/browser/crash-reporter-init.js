'use strict'

const { app } = require('electron')
const cp = require('child_process')
const path = require('path')

const getTempDirectory = function () {
  try {
    return app.getPath('temp')
  } catch {
    // Delibrately laze-load the os module, this file is on the hot
    // path when booting Electron and os takes between 5 - 8ms to load and we do not need it yet
    return require('os').tmpdir()
  }
}

exports.crashReporterInit = function (options) {
  const productName = options.productName || app.name
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
