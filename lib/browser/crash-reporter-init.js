'use strict'

const { app } = require('electron')
const cp = require('child_process')
const os = require('os')
const path = require('path')

const getTempDirectory = function () {
  try {
    return app.getPath('temp')
  } catch {
    return os.tmpdir()
  }
}

exports.crashReporterInit = function (options) {
  const productName = options.productName || app.getName()
  const crashesDirectory = path.join(getTempDirectory(), `${productName} Crashes`)

  return {
    productName,
    crashesDirectory,
    appVersion: app.getVersion()
  }
}
