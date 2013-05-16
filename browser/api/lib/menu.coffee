EventEmitter = require('events').EventEmitter
BrowserWindow = require 'browser_window'
MenuItem = require 'menu_item'

bindings = process.atomBinding 'menu'

Menu = bindings.Menu
Menu::__proto__ = EventEmitter.prototype

popup = Menu::popup
Menu::popup = (window) ->
  throw new TypeError('Invalid window') unless window?.constructor is BrowserWindow

  popup.call this, window

Menu::append = (item) ->
  @insert @getItemCount(), item

Menu::insert = (pos, item) ->
  throw new TypeError('Invalid item') unless item?.constructor is MenuItem

  switch item.type
    when 'normal' then @insertItem pos, item.commandId, item.label
    when 'checkbox' then @insertCheckItem pos, item.commandId, item.label
    when 'radio' then @insertRadioItem pos, item.commandId, item.label, item.groupId
    when 'separator' then @insertSeparator pos
    when 'submenu' then @insertSubMenu pos, item.commandId, item.label, item.submenu

  @setSublabel pos, item.sublabel if item.sublabel?

  unless @items?
    @items = {}
    @delegate =
      isCommandIdChecked: (commandId) => @items[commandId]?.checked
      isCommandIdEnabled: (commandId) => @items[commandId]?.enabled
      isCommandIdVisible: (commandId) => @items[commandId]?.visible
      getAcceleratorForCommandId: (commandId) => @items[commandId]?.accelerator
      executeCommand: (commandId) => @items[commandId]?.click()
  @items[item.commandId] = item

Menu.setApplicationMenu = (menu) ->
  throw new TypeError('Invalid menu') unless menu?.constructor is Menu
  bindings.setApplicationMenu menu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

module.exports = Menu
