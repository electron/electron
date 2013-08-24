assert = require 'assert'
protocol = require('remote').require 'protocol'

describe 'protocol API', ->
  describe 'protocol.registerProtocol', ->
    it 'throws error when scheme is already registered', ->
      register = -> protocol.registerProtocol('test1', ->)
      register()
      assert.throws register, /The scheme is already registered/
      protocol.unregisterProtocol 'test1'

    it 'calls the callback when scheme is visited', (done) ->
      protocol.registerProtocol 'test2', -> done()
      $.get 'test2://test2', ->
      protocol.unregisterProtocol 'test2'

  describe 'protocol.unregisterProtocol', ->
    it 'throws error when scheme does not exist', ->
      unregister = -> protocol.unregisterProtocol 'test3'
      assert.throws unregister, /The scheme has not been registered/
