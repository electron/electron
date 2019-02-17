const binding = process.atomBinding('crash_reporter')

export type CrashReporterInitOptions = {
  productName?: string
  submitURL?: string
}

export type CrashReporterInitResult = {
  productName: string
  crashesDirectory: string
  crashServicePid?: number
  appVersion: string
}

export abstract class CrashReporter {
  private productName: string | null = null
  private crashesDirectory: string | null = null
  private crashServicePid: number | undefined = undefined

  contructor () {
    this.productName = null
    this.crashesDirectory = null
  }

  abstract init(options: CrashReporterInitOptions): CrashReporterInitResult

  start (options: Electron.CrashReporterStartOptions) {
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

  getUploadedReports (): Electron.CrashReport[] {
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

  setUploadToServer (uploadToServer: boolean) {
    if (process.type === 'browser') {
      return binding.setUploadToServer(uploadToServer)
    } else {
      throw new Error('setUploadToServer can only be called from the main process')
    }
  }

  addExtraParameter (key: string, value: string) {
    binding.addExtraParameter(key, value)
  }

  removeExtraParameter (key: string) {
    binding.removeExtraParameter(key)
  }

  getParameters () {
    return binding.getParameters()
  }
}
