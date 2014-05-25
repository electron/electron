BrowserWindow = require 'browser-window'
EventEmitter = require('events').EventEmitter
MenuItem = require 'menu-item'

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

  unless @delegate?
    @commandsMap = {}
    @groupsMap = {}
    @items = []
    @delegate =
      isCommandIdChecked: (commandId) => @commandsMap[commandId]?.checked
      isCommandIdEnabled: (commandId) => @commandsMap[commandId]?.enabled
      isCommandIdVisible: (commandId) => @commandsMap[commandId]?.visible
      getAcceleratorForCommandId: (commandId) => @commandsMap[commandId]?.accelerator
      executeCommand: (commandId) =>
        activeItem = @commandsMap[commandId]
        if activeItem?
          switch activeItem.type
            when 'checkbox'
              activeItem.checked = !activeItem.checked
            when 'radio'
              for item in @groupsMap[activeItem.groupId]
                item.checked = false
              activeItem.checked = true
          activeItem.click()
  @items.splice pos, 0, item
  @commandsMap[item.commandId] = item

  if item.groupId?
    @groupsMap[item.groupId] ?= []
    @groupsMap[item.groupId].push item

applicationMenu = null
Menu.setApplicationMenu = (menu) ->
  throw new TypeError('Invalid menu') unless menu?.constructor is Menu
  applicationMenu = menu  # Keep a reference.

  if process.platform is 'darwin'
    bindings.setApplicationMenu menu
  else
    windows = BrowserWindow.getAllWindows()
    w.setMenu menu for w in windows

Menu.getApplicationMenu = -> applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

Menu.buildFromTemplate = (template) ->
  throw new TypeError('Invalid template for Menu') unless Array.isArray template

  menu = new Menu
  for item in template
    throw new TypeError('Invalid template for MenuItem') unless typeof item is 'object'

    item.submenu = Menu.buildFromTemplate item.submenu if item.submenu?
    menuItem = new MenuItem(item)
    menuItem[key] = value for key, value of item when not menuItem[key]?

    menu.append menuItem

  menu

module.exports = Menu
