'use strict'

const {spawn} = require('child_process')
const electron = require('electron')
const {app} = process.type === 'browser' ? electron : electron.remote
const binding = process.atomBinding('crash_reporter')

class CrashReporter {
  start (options) {
    if (options == null) {
      options = {}
    }
    this.productName = options.productName
    let {autoSubmit, companyName, extra, ignoreSystemCrashHandler, submitURL} = options

    this.tempDirectory = app.getPath('temp')
    if (this.productName == null) {
      this.productName = app.getName()
    }
    if (autoSubmit == null) {
      autoSubmit = true
    }
    if (ignoreSystemCrashHandler == null) {
      ignoreSystemCrashHandler = false
    }
    if (extra == null) {
      extra = {}
    }
    if (extra._productName == null) {
      extra._productName = this.productName
    }
    if (extra._companyName == null) {
      extra._companyName = companyName
    }
    if (extra._version == null) {
      extra._version = app.getVersion()
    }
    if (companyName == null) {
      throw new Error('companyName is a required option to crashReporter.start')
    }
    if (submitURL == null) {
      throw new Error('submitURL is a required option to crashReporter.start')
    }

    if (process.platform === 'win32') {
      const args = ['--reporter-url=' + submitURL, '--application-name=' + this.productName, '--v=1']
      const env = {
        ELECTRON_INTERNAL_CRASH_SERVICE: 1
      }
      spawn(process.execPath, args, {
        env: env,
        detached: true
      })
    }

    binding.start(this.productName, companyName, submitURL, this.tempDirectory, autoSubmit, ignoreSystemCrashHandler, extra)
  }

  getLastCrashReport () {
    const reports = this.getUploadedReports()
    if (reports.length > 0) {
      return reports[0]
    } else {
      return null
    }
  }

  getUploadedReports () {
    const productName = this.productName != null ? this.productName : app.getName()
    const tempDirectory = this.tempDirectory != null ? this.tempDirectory : app.getPath('temp')
    return binding._getUploadedReports(productName, tempDirectory)
  }
}

module.exports = new CrashReporter()
