module.exports =
  browserMainParts:
    preMainMessageLoopRun: ->

setImmediate ->
  module.exports.browserMainParts.preMainMessageLoopRun()
