'use strict'

const binding = process.atomBinding('crash_reporter')

class CrashReporter {
  contructor () {
    this.productName = null
    this.crashesDirectory = null
  }

  init (options) {
    throw new Error('Not implemented')
  }

  start (options) {
    if (options == null) options = {}

    let {
      productName,
      companyName,
      extra,
      ignoreSystemCrashHandler,
      submitURL,
      uploadToServer
    } = options

    if (uploadToServer == null) {
      uploadToServer = true
    }

    if (ignoreSystemCrashHandler == null) {
      ignoreSystemCrashHandler = false
    }

    if (companyName == null) {
      throw new Error('companyName is a required option to crashReporter.start')
    }
    if (submitURL == null) {
      throw new Error('submitURL is a required option to crashReporter.start')
    }

    const ret = this.init({
      submitURL,
      productName
    })

    this.productName = ret.productName
    this.crashesDirectory = ret.crashesDirectory
    this.crashServicePid = ret.crashServicePid

    if (extra == null) extra = {}
    if (extra._productName == null) extra._productName = ret.productName
    if (extra._companyName == null) extra._companyName = companyName
    if (extra._version == null) extra._version = ret.appVersion

    binding.start(ret.productName, companyName, submitURL, ret.crashesDirectory, uploadToServer, ignoreSystemCrashHandler, extra)
  }

  getLastCrashReport () {
    const reports = this.getUploadedReports()
      .sort((a, b) => {
        const ats = (a && a.date) ? new Date(a.date).getTime() : 0
        const bts = (b && b.date) ? new Date(b.date).getTime() : 0
        return bts - ats
      })

    return (reports.length > 0) ? reports[0] : null
  }

  getUploadedReports () {
    return binding.getUploadedReports(this.getCrashesDirectory())
  }

  getCrashesDirectory () {
    return this.crashesDirectory
  }

  getProductName () {
    return this.productName
  }

  getUploadToServer () {
    if (process.type === 'browser') {
      return binding.getUploadToServer()
    } else {
      throw new Error('getUploadToServer can only be called from the main process')
    }
  }

  setUploadToServer (uploadToServer) {
    if (process.type === 'browser') {
      return binding.setUploadToServer(uploadToServer)
    } else {
      throw new Error('setUploadToServer can only be called from the main process')
    }
  }

  addExtraParameter (key, value) {
    binding.addExtraParameter(key, value)
  }

  removeExtraParameter (key) {
    binding.removeExtraParameter(key)
  }

  getParameters (key, value) {
    return binding.getParameters()
  }
}

module.exports = CrashReporter
