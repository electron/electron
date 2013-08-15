nextCommandId = 0

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    Menu = require 'menu'

    {click, @selector, @type, @label, @sublabel, @accelerator, @enabled, @visible, @checked, @groupId, @submenu} = options

    @type = 'submenu' if not @type? and @submenu?
    throw new Error('Invalid submenu') if @type is 'submenu' and @submenu?.constructor isnt Menu

    @type = @type ? 'normal'
    @label = @label ? ''
    @sublabel = @sublabel ? ''
    @accelerator = @accelerator ? null
    @enabled = @enabled ? true
    @visible = @visible ? true
    @checked = @checked ? false
    @groupId = @groupId ? null
    @submenu = @submenu ? null

    throw new Error('Unknown menu type') if MenuItem.types.indexOf(@type) is -1

    @commandId = ++nextCommandId
    @click = =>
      if typeof click is 'function'
        click.apply this, arguments
      else if typeof selector is 'string'
        Menu.sendActionToFirstResponder selector

module.exports = MenuItem
