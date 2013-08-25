assert = require 'assert'
ipc = require 'ipc'
protocol = require('remote').require 'protocol'

describe 'protocol API', ->
  describe 'protocol.registerProtocol', ->
    it 'throws error when scheme is already registered', ->
      register = -> protocol.registerProtocol('test1', ->)
      register()
      assert.throws register, /The scheme is already registered/
      protocol.unregisterProtocol 'test1'

    it 'calls the callback when scheme is visited', (done) ->
      protocol.registerProtocol 'test2', (url) ->
        assert.equal url, 'test2://test2'
        protocol.unregisterProtocol 'test2'
        done()
      $.get 'test2://test2', ->

  describe 'protocol.unregisterProtocol', ->
    it 'throws error when scheme does not exist', ->
      unregister = -> protocol.unregisterProtocol 'test3'
      assert.throws unregister, /The scheme has not been registered/

  describe 'registered protocol callback', ->
    it 'returns string should send the string as request content', (done) ->
      $.ajax
        url: 'atom-string://something'
        success: (data) ->
          assert.equal data, 'atom-string://something'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error

    it 'returns RequestStringJob should send string', (done) ->
      $.ajax
        url: 'atom-string-job://something'
        success: (data) ->
          assert.equal data, 'atom-string-job://something'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error

    it 'returns RequestFileJob should send file', (done) ->
      $.ajax
        url: 'atom-file-job://' + __filename
        success: (data) ->
          content = require('fs').readFileSync __filename
          assert.equal data, String(content)
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
