nextCommandId = 0

class MenuItem
  @types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  constructor: (options) ->
    {@type, @label, @sublabel, @click, @checked, @groupId, @submenu} = options

    @type = @type ? 'normal'
    @label = @label ? ''
    @sublabel = @sublabel ? ''

    throw new Error('Unknown menu type') if MenuItem.types.indexOf(@type) is -1
    throw new Error('Invalid menu') if @type is 'submenu' and @submenu?.constructor.name isnt 'Menu'

    @commandId = ++nextCommandId

module.exports = MenuItem
