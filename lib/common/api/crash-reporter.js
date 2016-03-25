'use strict'

const os = require('os')
const path = require('path')
const spawn = require('child_process').spawn
const electron = require('electron')
const binding = process.atomBinding('crash_reporter')

var CrashReporter = (function () {
  function CrashReporter () {}

  CrashReporter.prototype.start = function (options) {
    var app, args, autoSubmit, companyName, deprecate, env, extra, ignoreSystemCrashHandler, start, submitURL
    if (options == null) {
      options = {}
    }
    this.productName = options.productName
    companyName = options.companyName
    submitURL = options.submitURL
    autoSubmit = options.autoSubmit
    ignoreSystemCrashHandler = options.ignoreSystemCrashHandler
    extra = options.extra

    // Deprecated.
    deprecate = electron.deprecate
    if (options.submitUrl) {
      if (submitURL == null) {
        submitURL = options.submitUrl
      }
      deprecate.warn('submitUrl', 'submitURL')
    }
    app = (process.type === 'browser' ? electron : electron.remote).app
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
      deprecate.log('companyName is now a required option to crashReporter.start')
      return
    }
    if (submitURL == null) {
      deprecate.log('submitURL is now a required option to crashReporter.start')
      return
    }
    start = () => {
      binding.start(this.productName, companyName, submitURL, autoSubmit, ignoreSystemCrashHandler, extra)
    }
    if (process.platform === 'win32') {
      args = ["--reporter-url=" + submitURL, "--application-name=" + this.productName, "--v=1"]
      env = {
        ATOM_SHELL_INTERNAL_CRASH_SERVICE: 1
      }
      spawn(process.execPath, args, {
        env: env,
        detached: true
      })
    }
    return start()
  }

  CrashReporter.prototype.getLastCrashReport = function () {
    var reports
    reports = this.getUploadedReports()
    if (reports.length > 0) {
      return reports[0]
    } else {
      return null
    }
  }

  CrashReporter.prototype.getUploadedReports = function () {
    var log, tmpdir
    tmpdir = process.platform === 'win32' ? os.tmpdir() : '/tmp'
    log = process.platform === 'darwin' ? path.join(tmpdir, this.productName + " Crashes") : path.join(tmpdir, this.productName + " Crashes", 'uploads.log')
    return binding._getUploadedReports(log)
  }

  return CrashReporter

})()

module.exports = new CrashReporter
