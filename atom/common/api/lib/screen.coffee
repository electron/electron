binding = process.atomBinding 'screen'

checkAppIsReady = ->
  unless process.type is 'renderer' or require('app').isReady()
    throw new Error('Can not use screen module before the "ready" event of app module gets emitted')

module.exports =
  if process.platform in ['linux', 'win32'] and process.type is 'renderer'
    # On Linux we could not access screen in renderer process.
    require('remote').require 'screen'
  else
    getCursorScreenPoint: ->
      checkAppIsReady()
      binding.getCursorScreenPoint()
    getPrimaryDisplay: ->
      checkAppIsReady()
      binding.getPrimaryDisplay()
