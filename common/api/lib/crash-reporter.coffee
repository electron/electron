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

    binding.start productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler, extra

module.exports = new CrashReporter
