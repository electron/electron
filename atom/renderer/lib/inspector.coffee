window.onload = ->
  # Use menu API to show context menu.
  WebInspector.ContextMenu.prototype.show = ->
    menuObject = @_buildDescriptor()
    if menuObject.length
      WebInspector._contextMenu = this
      createMenu(menuObject, @_event)
      @_event.consume()

  # Use dialog API to override file chooser dialog.
  WebInspector.createFileSelectorElement = (callback) ->
    fileSelectorElement = document.createElement 'span'
    fileSelectorElement.style.display = 'none'
    fileSelectorElement.click = showFileChooserDialog.bind this, callback
    return fileSelectorElement

convertToMenuTemplate = (items) ->
  template = []
  for item in items
    do (item) ->
      transformed =
        if item.type is 'subMenu'
          type: 'submenu'
          label: item.label
          enabled: item.enabled
          submenu: convertToMenuTemplate item.subItems
        else if item.type is 'separator'
          type: 'separator'
        else if item.type is 'checkbox'
          type: 'checkbox'
          label: item.label
          enabled: item.enabled
          checked: item.checked
        else
          type: 'normal'
          label: item.label
          enabled: item.enabled
      if item.id?
        transformed.click = -> WebInspector.contextMenuItemSelected item.id
      template.push transformed
  template

createMenu = (items, event) ->
  remote = require 'remote'
  Menu = remote.require 'menu'

  menu = Menu.buildFromTemplate convertToMenuTemplate(items)
  menu.popup remote.getCurrentWindow()
  event.consume true

showFileChooserDialog = (callback) ->
  remote = require 'remote'
  dialog = remote.require 'dialog'
  files = dialog.showOpenDialog remote.getCurrentWindow(), null
  callback pathToHtml5FileObject files[0] if files?

pathToHtml5FileObject = (path) ->
  fs = require 'fs'
  blob = new Blob([fs.readFileSync(path)])
  blob.name = path
  blob
