fs      = require 'fs'
os      = require 'os'
path    = require 'path'
{spawn} = require 'child_process'

electron = require 'electron'
binding = process.atomBinding 'crash_reporter'

class CrashReporter
  start: (options={}) ->
    {@productName, companyName, submitURL, autoSubmit, ignoreSystemCrashHandler, extra} = options

    ### Deprecated. ###
    {deprecate} = electron
    if options.submitUrl
      submitURL ?= options.submitUrl
      deprecate.warn 'submitUrl', 'submitURL'

    {app} = if process.type is 'browser' then electron else electron.remote

    @productName ?= app.getName()
    autoSubmit ?= true
    ignoreSystemCrashHandler ?= false
    extra ?= {}

    extra._productName ?= @productName
    extra._companyName ?= companyName
    extra._version ?= app.getVersion()

    unless companyName?
      deprecate.log('companyName is now a required option to crashReporter.start')
      return

    unless submitURL?
      deprecate.log('submitURL is now a required option to crashReporter.start')
      return

    start = => binding.start @productName, companyName, submitURL, autoSubmit, ignoreSystemCrashHandler, extra

    if process.platform is 'win32'
      args = [
        "--reporter-url=#{submitURL}"
        "--application-name=#{@productName}"
        "--v=1"
      ]
      env = ATOM_SHELL_INTERNAL_CRASH_SERVICE: 1

      spawn process.execPath, args, {env, detached: true}
    start()

  getLastCrashReport: ->
    reports = this.getUploadedReports()
    if reports.length > 0 then reports[0] else null

  getUploadedReports: ->
    tmpdir =
      if process.platform is 'win32'
        os.tmpdir()
      else
        '/tmp'
    log =
      if process.platform is 'darwin'
        path.join tmpdir, "#{@productName} Crashes"
      else
        path.join tmpdir, "#{@productName} Crashes", 'uploads.log'
    binding._getUploadedReports log

crashRepoter = new CrashReporter
module.exports = crashRepoter
