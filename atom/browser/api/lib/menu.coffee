BrowserWindow = require 'browser-window'
EventEmitter = require('events').EventEmitter
MenuItem = require 'menu-item'
v8Util = process.atomBinding 'v8_util'

bindings = process.atomBinding 'menu'

# Automatically generated radio menu item's group id.
nextGroupId = 0

# Search between seperators to find a radio menu item and return its group id,
# otherwise generate a group id.
generateGroupId = (items, pos) ->
  if pos > 0
    for i in [pos - 1..0]
      item = items[i]
      return item.groupId if item.type is 'radio'
      break if item.type is 'separator'
  else if pos < items.length
    for i in [pos..items.length - 1]
      item = items[i]
      return item.groupId if item.type is 'radio'
      break if item.type is 'separator'
  ++nextGroupId

# Returns the index of item according to |id|.
indexOfItemById = (items, id) ->
  return i for item, i in items when item.id is id
  -1

# Returns the index of where to insert the item according to |position|.
indexToInsertByPosition = (items, position) ->
  return items.length unless position

  [query, id] = position.split '='
  insertIndex = indexOfItemById items, id
  if insertIndex is -1 and query isnt 'endof'
    console.warn "Item with id '#{id}' is not found"
    return items.length

  switch query
    when 'after'
      insertIndex++
    when 'endof'
      # If the |id| doesn't exist, then create a new group with the |id|.
      if insertIndex is -1
        items.push id: id, type: 'separator'
        insertIndex = items.length - 1

      # Find the end of the group.
      insertIndex++
      while insertIndex < items.length and items[insertIndex].type isnt 'separator'
        insertIndex++

  insertIndex

Menu = bindings.Menu
Menu::__proto__ = EventEmitter.prototype

Menu::_init = ->
  @commandsMap = {}
  @groupsMap = {}
  @items = []
  @delegate =
    isCommandIdChecked: (commandId) => @commandsMap[commandId]?.checked
    isCommandIdEnabled: (commandId) => @commandsMap[commandId]?.enabled
    isCommandIdVisible: (commandId) => @commandsMap[commandId]?.visible
    getAcceleratorForCommandId: (commandId) => @commandsMap[commandId]?.accelerator
    getIconForCommandId: (commandId) => @commandsMap[commandId]?.icon
    executeCommand: (commandId) =>
      @commandsMap[commandId]?.click BrowserWindow.getFocusedWindow()
    menuWillShow: =>
      # Make sure radio groups have at least one menu item seleted.
      for id, group of @groupsMap
        checked = false
        for radioItem in group when radioItem.checked
          checked = true
          break
        v8Util.setHiddenValue group[0], 'checked', true unless checked

Menu::popup = (window, x, y) ->
  unless window?.constructor is BrowserWindow
    # Shift.
    y = x
    x = window
    window = BrowserWindow.getFocusedWindow()
  if x? and y?
    @_popupAt(window, x, y)
  else
    @_popup window

Menu::append = (item) ->
  @insert @getItemCount(), item

Menu::insert = (pos, item) ->
  throw new TypeError('Invalid item') unless item?.constructor is MenuItem

  switch item.type
    when 'normal' then @insertItem pos, item.commandId, item.label
    when 'checkbox' then @insertCheckItem pos, item.commandId, item.label
    when 'separator' then @insertSeparator pos
    when 'submenu' then @insertSubMenu pos, item.commandId, item.label, item.submenu
    when 'radio'
      # Grouping radio menu items.
      item.overrideReadOnlyProperty 'groupId', generateGroupId(@items, pos)
      @groupsMap[item.groupId] ?= []
      @groupsMap[item.groupId].push item

      # Setting a radio menu item should flip other items in the group.
      v8Util.setHiddenValue item, 'checked', item.checked
      Object.defineProperty item, 'checked',
        enumerable: true
        get: -> v8Util.getHiddenValue item, 'checked'
        set: (val) =>
          for otherItem in @groupsMap[item.groupId] when otherItem isnt item
            v8Util.setHiddenValue otherItem, 'checked', false
          v8Util.setHiddenValue item, 'checked', true

      @insertRadioItem pos, item.commandId, item.label, item.groupId

  @setSublabel pos, item.sublabel if item.sublabel?
  @setIcon pos, item.icon if item.icon?
  @setRole pos, item.role if item.role?

  # Make menu accessable to items.
  item.overrideReadOnlyProperty 'menu', this

  # Remember the items.
  @items.splice pos, 0, item
  @commandsMap[item.commandId] = item

# Force menuWillShow to be called
Menu::_callMenuWillShow = ->
  @delegate?.menuWillShow()
  item.submenu._callMenuWillShow() for item in @items when item.submenu?

applicationMenu = null
Menu.setApplicationMenu = (menu) ->
  throw new TypeError('Invalid menu') unless menu is null or menu.constructor is Menu
  applicationMenu = menu  # Keep a reference.

  if process.platform is 'darwin'
    return if menu is null
    menu._callMenuWillShow()
    bindings.setApplicationMenu menu
  else
    windows = BrowserWindow.getAllWindows()
    w.setMenu menu for w in windows

Menu.getApplicationMenu = -> applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

Menu.buildFromTemplate = (template) ->
  throw new TypeError('Invalid template for Menu') unless Array.isArray template

  positionedTemplate = []
  insertIndex = 0

  for item in template
    if item.position
      insertIndex = indexToInsertByPosition positionedTemplate, item.position
    else
      # If no |position| is specified, insert after last item.
      insertIndex++
    positionedTemplate.splice insertIndex, 0, item

  menu = new Menu

  for item in positionedTemplate
    throw new TypeError('Invalid template for MenuItem') unless typeof item is 'object'

    item.submenu = Menu.buildFromTemplate item.submenu if item.submenu?
    menuItem = new MenuItem(item)
    menuItem[key] = value for key, value of item when not menuItem[key]?
    menu.append menuItem

  menu

module.exports = Menu
