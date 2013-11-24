{spawn} = require 'child_process'
binding = process.atomBinding 'crash_reporter'

class CrashReporter
  start: (options={}) ->
    {productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra} = options

    productName ?= 'atom-shell'
    companyName ?= 'GitHub, Inc'
    submitUrl ?= 'http://54.249.141.25:1127/post'
    autoSubmit ?= true
    ignoreSystemCrashHandler ?= false
    extra ?= {}

    if process.platform isnt 'darwin'
      args = [
        "--reporter-url=#{submitUrl}",
        "--application-name=#{productName}"
      ]
      env = ATOM_SHELL_INTERNAL_CRASH_SERVICE: 1
      spawn process.execPath, args, {env}

    binding.start productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra

module.exports = new CrashReporter
