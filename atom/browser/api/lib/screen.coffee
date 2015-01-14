binding = process.atomBinding 'screen'

checkAppIsReady = ->
  unless process.type is 'renderer' or require('app').isReady()
    throw new Error('Can not use screen module before the "ready" event of app module gets emitted')

for name, _ of binding
  do (name) ->
    module.exports[name] = (args...) ->
      checkAppIsReady()
      binding[name](args...)
