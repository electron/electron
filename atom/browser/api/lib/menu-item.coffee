BrowserWindow = require 'browser-window'
v8Util = process.atomBinding 'v8_util'

nextCommandId = 0

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    Menu = require 'menu'

    {click, @selector, @type, @label, @sublabel, @accelerator, @icon, @enabled, @visible, @checked, @submenu} = options

    @type = 'submenu' if not @type? and @submenu?
    throw new Error('Invalid submenu') if @type is 'submenu' and @submenu?.constructor isnt Menu

    @overrideReadOnlyProperty 'type', 'normal'
    @overrideReadOnlyProperty 'accelerator'
    @overrideReadOnlyProperty 'icon'
    @overrideReadOnlyProperty 'submenu'
    @overrideProperty 'label', ''
    @overrideProperty 'sublabel', ''
    @overrideProperty 'enabled', true
    @overrideProperty 'visible', true
    @overrideProperty 'checked', false

    throw new Error("Unknown menu type #{@type}") if MenuItem.types.indexOf(@type) is -1

    @commandId = ++nextCommandId
    @click = =>
      # Manually flip the checked flags when clicked.
      @checked = !@checked if @type in ['checkbox', 'radio']

      if typeof click is 'function'
        click this, BrowserWindow.getFocusedWindow()
      else if typeof @selector is 'string'
        Menu.sendActionToFirstResponder @selector

  overrideProperty: (name, defaultValue=null) ->
    this[name] ?= defaultValue

  overrideReadOnlyProperty: (name, defaultValue=null) ->
    this[name] ?= defaultValue
    Object.defineProperty this, name,
      enumerable: true
      writable: false
      value: this[name]

module.exports = MenuItem
