{spawn} = require 'child_process'
binding = process.atomBinding 'crash_reporter'

class CrashReporter
  start: (options={}) ->
    {productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra} = options

    productName ?= 'Atom-Shell'
    companyName ?= 'GitHub, Inc'
    submitUrl ?= 'http://54.249.141.255:1127/post'
    autoSubmit ?= true
    ignoreSystemCrashHandler ?= false
    extra ?= {}

    extra._productName ?= productName
    extra._companyName ?= companyName
    extra._version ?=
      if process.__atom_type is 'browser'
        require('app').getVersion()
      else
        require('remote').require('app').getVersion()

    start = -> binding.start productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra

    if process.platform is 'win32'
      args = [
        "--reporter-url=#{submitUrl}"
        "--application-name=#{productName}"
        "--v=1"
      ]
      env = ATOM_SHELL_INTERNAL_CRASH_SERVICE: 1

      spawn process.execPath, args, {env, detached: true}
      start()
    else
      start()

module.exports = new CrashReporter
