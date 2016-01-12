### Do not expose the internal modules to `require`. ###
exports.hideInternalModules = ->
  {globalPaths} = require 'module'
  if globalPaths.length is 3
    ### Remove the "common/api/lib" and "browser-or-renderer/api/lib". ###
    globalPaths.splice 0, 2

### Attaches properties to |exports|. ###
exports.defineProperties = (exports) ->
  Object.defineProperties exports,
    ### Common modules, please sort with alphabet order. ###
    clipboard:
      ### Must be enumerable, otherwise it woulde be invisible to remote module. ###
      enumerable: true
      get: -> require '../clipboard'
    crashReporter:
      enumerable: true
      get: -> require '../crash-reporter'
    nativeImage:
      enumerable: true
      get: -> require '../native-image'
    shell:
      enumerable: true
      get: -> require '../shell'
    ### The internal modules, invisible unless you know their names. ###
    CallbacksRegistry:
      get: -> require '../callbacks-registry'
    deprecate:
      get: -> require '../deprecate'
