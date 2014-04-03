# Use menu API to show context menu.
window.onload = ->
  WebInspector.ContextMenu.prototype.show = ->
    menuObject = @_buildDescriptor()
    if menuObject.length
      WebInspector._contextMenu = this
      createMenu(menuObject, @_event)
      @_event.consume()

convertToMenuTemplate = (items) ->
  template = []
  for item in items
    if item.type is 'subMenu'
      template.push
        type: 'submenu'
        label: item.label
        submenu: convertToMenuTemplate item.subItems
    else
      template.push
        type: 'normal'
        label: item.label
  template

createMenu = (items, event) ->
  remote = require 'remote'
  Menu = remote.require 'menu'

  menu = Menu.buildFromTemplate convertToMenuTemplate(items.subItems)
  menu.popup()
  event.consume true
