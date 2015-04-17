binding = process.atomBinding 'crash_reporter'
fs      = require 'fs'
os      = require 'os'
path    = require 'path'
{spawn} = require 'child_process'

class CrashReporter
  start: (options={}) ->
    {@productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra} = options

    app =
      if process.type is 'browser'
        require 'app'
      else
        require('remote').require 'app'

    @productName ?= app.getName()
    companyName ?= 'GitHub, Inc'
    submitUrl ?= 'http://54.249.141.255:1127/post'
    autoSubmit ?= true
    ignoreSystemCrashHandler ?= false
    extra ?= {}

    extra._productName ?= @productName
    extra._companyName ?= companyName
    extra._version ?= app.getVersion()

    start = => binding.start @productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra

    if process.platform is 'win32'
      args = [
        "--reporter-url=#{submitUrl}"
        "--application-name=#{@productName}"
        "--v=1"
      ]
      env = ATOM_SHELL_INTERNAL_CRASH_SERVICE: 1

      spawn process.execPath, args, {env, detached: true}
      start()
    else
      start()

  getLastCrashReport: ->
    tmpdir =
      if process.platform is 'win32'
        os.tmpdir()
      else
        '/tmp'
    log = path.join tmpdir, "#{@productName} Crashes", 'uploads.log'
    try
      reports = String(fs.readFileSync(log)).split('\n')
      return null unless reports.length > 1
      [time, id] = reports[reports.length - 2].split ','
      return {date: new Date(parseInt(time) * 1000), id}
    catch e
      return null

crashRepoter = new CrashReporter
module.exports = crashRepoter
