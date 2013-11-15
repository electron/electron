binding = process.atomBinding 'crash_reporter'

class CrashReporter
  start: (options={}) ->
    {productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler} = options

    productName ?= 'Atom-Shell'
    companyName ?= 'GitHub, Inc'
    submitUrl ?= 'http://54.249.141.25'
    autoSubmit ?= true
    ignoreSystemCrashHandler ?= false

    binding.start productName, companyName, submitUrl, autoSubmit, ignoreSystemCrashHandler

module.exports = new CrashReporter
