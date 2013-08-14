assert= require 'assert'
remote = require 'remote'
Menu = remote.require 'menu'

describe 'Menu API', ->
  describe 'Menu.buildFromTemplate', ->
    it 'should be able to attach extra fields', ->
      menu = Menu.buildFromTemplate [label: 'text', extra: 'field']
      assert.equal menu.items[0].extra, 'field'
