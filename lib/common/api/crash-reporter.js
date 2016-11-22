'use strict'

const {spawn} = require('child_process')
const os = require('os')
const path = require('path')
const electron = require('electron')
const {app} = process.type === 'browser' ? electron : electron.remote
const binding = process.atomBinding('crash_reporter')

class CrashReporter {
  start (options) {
    if (options == null) {
      options = {}
    }
    this.productName = options.productName != null ? options.productName : app.getName()
    let {autoSubmit, companyName, extra, ignoreSystemCrashHandler, submitURL, uploadToServer} = options

    if (autoSubmit == null && uploadToServer == null) {
      uploadToServer = true
    } else {
      if (typeof autoSubmit !== 'undefined') {
        // TODO: Remove depreceated property in 2.0.0
        console.warn('The "autoSubmit" attribute on electron.crashReporter.start is depreceated.  Please use "uploadToServer" instead.')
      }
      uploadToServer = uploadToServer || autoSubmit
    }
    if (ignoreSystemCrashHandler == null) {
      ignoreSystemCrashHandler = false
    }
    if (extra == null) {
      extra = {}
    }
    if (extra._productName == null) {
      extra._productName = this.getProductName()
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
      const args = [
        '--reporter-url=' + submitURL,
        '--application-name=' + this.getProductName(),
        '--crashes-directory=' + this.getCrashesDirectory(),
        '--v=1'
      ]
      const env = {
        ELECTRON_INTERNAL_CRASH_SERVICE: 1
      }
      spawn(process.execPath, args, {
        env: env,
        detached: true
      })
    }

    binding.start(this.getProductName(), companyName, submitURL, this.getCrashesDirectory(), uploadToServer, ignoreSystemCrashHandler, extra)
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
    return binding._getUploadedReports(this.getCrashesDirectory())
  }

  getCrashesDirectory () {
    const crashesDir = this.getProductName() + ' Crashes'
    return path.join(this.getTempDirectory(), crashesDir)
  }

  getProductName () {
    if (this.productName == null) {
      this.productName = app.getName()
    }
    return this.productName
  }

  getTempDirectory () {
    if (this.tempDirectory == null) {
      try {
        this.tempDirectory = app.getPath('temp')
      } catch (error) {
        // app.getPath may throw so fallback to OS temp directory
        this.tempDirectory = os.tmpdir()
      }
    }
    return this.tempDirectory
  }

  getUploadToServer () {
    if (process.type === 'browser') {
      return binding._getUploadToServer()
    } else {
      throw new Error('getUploadToServer can only be called from the main process')
    }
  }

  setUploadToServer (uploadToServer) {
    if (process.type === 'browser') {
      return binding._setUploadToServer(uploadToServer)
    } else {
      throw new Error('setUploadToServer can only be called from the main process')
    }
  }
}

module.exports = new CrashReporter()
