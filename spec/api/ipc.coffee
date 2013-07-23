assert = require 'assert'
remote = require 'remote'

describe 'ipc', ->
  describe 'remote.require', ->
    it 'should returns same object for the same module', ->
      dialog1 = remote.require 'dialog'
      dialog2 = remote.require 'dialog'
      assert.equal dialog1, dialog2
