BrowserWindow = require 'browser-window'
v8Util = process.atomBinding 'v8_util'

nextCommandId = 0

overrideProperty = (item, name, defaultValue) ->
  item[name] ?= defaultValue

  return unless process.platform is 'win32'
  v8Util.setHiddenValue item, name, item[name]
  Object.defineProperty item, name,
    enumerable: true
    get: -> v8Util.getHiddenValue item, name
    set: (val) ->
      v8Util.setHiddenValue item, name, val
      item.menu?._updateStates()

overrideReadOnlyProperty = (item, name, defaultValue) ->
  item[name] ?= defaultValue
  Object.defineProperty item, name,
    enumerable: true
    writable: false
    value: item[name]

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    Menu = require 'menu'

    {click, @selector, @type, @label, @sublabel, @accelerator, @enabled, @visible, @checked, @submenu} = options

    @type = 'submenu' if not @type? and @submenu?
    throw new Error('Invalid submenu') if @type is 'submenu' and @submenu?.constructor isnt Menu

    overrideReadOnlyProperty this, 'type', 'normal'
    overrideReadOnlyProperty this, 'accelerator', null
    overrideReadOnlyProperty this, 'submenu', null
    overrideProperty this, 'label', ''
    overrideProperty this, 'sublabel', ''
    overrideProperty this, 'enabled', true
    overrideProperty this, 'visible', true
    overrideProperty this, 'checked', false

    throw new Error("Unknown menu type #{@type}") if MenuItem.types.indexOf(@type) is -1

    @commandId = ++nextCommandId
    @click = =>
      if typeof click is 'function'
        click this, BrowserWindow.getFocusedWindow()
      else if typeof @selector is 'string'
        Menu.sendActionToFirstResponder @selector

module.exports = MenuItem
