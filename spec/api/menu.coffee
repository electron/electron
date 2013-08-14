assert= require 'assert'
remote = require 'remote'
Menu = remote.require 'menu'
MenuItem = remote.require 'menu-item'

describe 'Menu API', ->
  describe 'Menu.buildFromTemplate', ->
    it 'should be able to attach extra fields', ->
      menu = Menu.buildFromTemplate [label: 'text', extra: 'field']
      assert.equal menu.items[0].extra, 'field'

  describe 'Menu.insert', ->
    it 'should store item in @items by its index', ->
      menu = Menu.buildFromTemplate [
        {label: '1'}
        {label: '2'}
        {label: '3'}
      ]
      item = new MenuItem(label: 'inserted')
      menu.insert 1, item

      assert.equal menu.items[0].label, '1'
      assert.equal menu.items[1].label, 'inserted'
      assert.equal menu.items[2].label, '2'
      assert.equal menu.items[3].label, '3'

  describe 'MenuItem.click', ->
    it 'should be called with the item object passed', (done) ->
      menu = Menu.buildFromTemplate [
        label: 'text'
        click: (item) ->
          assert.equal item.constructor.name, 'MenuItem'
          assert.equal item.label, 'text'
          done()
      ]
      menu.delegate.executeCommand menu.items[0].commandId
