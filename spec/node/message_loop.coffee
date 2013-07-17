assert = require 'assert'

describe 'message loop', ->
  describe 'process.nextTick', ->
    it 'should emit the callback', (done) ->
      process.nextTick done

    it 'should work in nested calls', (done) ->
      process.nextTick ->
        process.nextTick ->
          process.nextTick done

  describe 'setImmediate', ->
    it 'should emit the callback', (done) ->
      setImmediate done

    it 'should work in nested calls', (done) ->
      setImmediate ->
        setImmediate ->
          setImmediate done
