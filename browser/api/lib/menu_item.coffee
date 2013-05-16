nextCommandId = 0

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    {click, @type, @label, @sublabel, @accelerator, @enabled, @visible, @checked, @groupId, @submenu} = options

    @type = @type ? 'normal'
    @label = @label ? ''
    @sublabel = @sublabel ? ''
    @enabled = @enabled ? true
    @visible = @visible ? true

    throw new Error('Unknown menu type') if MenuItem.types.indexOf(@type) is -1
    throw new Error('Invalid menu') if @type is 'submenu' and @submenu?.constructor.name isnt 'Menu'

    @commandId = ++nextCommandId
    @click = -> click() if typeof click is 'function'

module.exports = MenuItem
