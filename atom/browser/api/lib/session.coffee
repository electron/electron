{EventEmitter} = require 'events'

bindings = process.atomBinding 'session'

PERSIST_PERFIX = 'persist:'

exports.fromPartition = (partition='') ->
  if partition.startsWith PERSIST_PERFIX
    bindings.fromPartition partition.substr(PERSIST_PERFIX.length), false
  else
    bindings.fromPartition partition, true

wrapSession = (session) ->
  # session is an EventEmitter.
  session.__proto__ = EventEmitter.prototype

bindings._setWrapSession wrapSession
