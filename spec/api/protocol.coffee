assert = require 'assert'
protocol = require('remote').require 'protocol'

describe 'protocol API', ->
  describe 'protocol.registerProtocol', ->
    it 'throws error when scheme is already registered', ->
      register = -> protocol.registerProtocol('test', ->)
      register()
      assert.throws register, /The scheme is already registered/
      protocol.unregisterProtocol 'test'

  describe 'protocol.unregisterProtocol', ->
    it 'throws error when scheme does not exist', ->
      unregister = -> protocol.unregisterProtocol 'test'
      assert.throws unregister, /The scheme has not been registered/
