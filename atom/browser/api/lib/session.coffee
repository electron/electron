{EventEmitter} = require 'events'

bindings = process.atomBinding 'session'

PERSIST_PERFIX = 'persist:'

### Returns the Session from |partition| string. ###
exports.fromPartition = (partition='') ->
  return exports.defaultSession if partition is ''
  if partition.startsWith PERSIST_PERFIX
    bindings.fromPartition partition.substr(PERSIST_PERFIX.length), false
  else
    bindings.fromPartition partition, true

### Returns the default session. ###
Object.defineProperty exports, 'defaultSession',
  enumerable: true
  get: -> bindings.fromPartition '', false

wrapSession = (session) ->
  ### session is an EventEmitter. ###
  session.__proto__ = EventEmitter.prototype

bindings._setWrapSession wrapSession
