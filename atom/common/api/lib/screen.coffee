binding = process.atomBinding 'screen'

checkAppIsReady = ->
  unless process.type is 'renderer' or require('app').isReady()
    throw new Error('Can not use screen module before the "ready" event of app module gets emitted')

if process.platform in ['linux', 'win32'] and process.type is 'renderer'
  # On Linux and Windows we could not access screen in renderer process.
  module.exports = require('remote').require 'screen'
else
  for name, _ of binding
    do (name) ->
      module.exports[name] = (args...) ->
        checkAppIsReady()
        binding[name](args...)
