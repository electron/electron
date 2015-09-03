BrowserWindow = require 'browser-window'
v8Util = process.atomBinding 'v8_util'

nextCommandId = 0

# Maps role to methods of webContents
rolesMap =
  undo: 'undo'
  redo: 'redo'
  cut: 'cut'
  copy: 'copy'
  paste: 'paste'
  selectall: 'selectAll'
  minimize: 'minimize'
  close: 'close'

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    Menu = require 'menu'

    {click, @selector, @type, @role, @label, @sublabel, @accelerator, @icon, @enabled, @visible, @checked, @submenu} = options

    @type = 'submenu' if not @type? and @submenu?
    throw new Error('Invalid submenu') if @type is 'submenu' and @submenu?.constructor isnt Menu

    @overrideReadOnlyProperty 'type', 'normal'
    @overrideReadOnlyProperty 'role'
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
    @click = (focusedWindow) =>
      # Manually flip the checked flags when clicked.
      @checked = !@checked if @type in ['checkbox', 'radio']

      if @role and rolesMap[@role] and process.platform isnt 'darwin'
        focusedWindow?[rolesMap[@role]]()
      else if typeof click is 'function'
        click this, focusedWindow
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
