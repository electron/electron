inspectorFrame = null
window.onload = ->
  inspectorFrame = document.getElementById('inspector-app-iframe').contentWindow

  # Use menu API to show context menu.
  inspectorFrame.eval 'InspectorFrontendHost.showContextMenuAtPoint = parent.createMenu'

  # Use dialog API to override file chooser dialog.
  inspectorFrame.eval 'WebInspector.createFileSelectorElement = parent.createFileSelectorElement'

convertToMenuTemplate = (items) ->
  template = []
  copyMenu =
    type: 'normal'
    label: 'copy'
    click: ->
      window.selectionText = inspectorFrame.getSelection().toString()
      inspectorFrame.eval 'InspectorFrontendHost.copyText(parent.selectionText)'

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
        transformed.click = -> DevToolsAPI.contextMenuItemSelected item.id
      template.push transformed
      if item.label is 'Save as...'
        template.push copyMenu
  template

createMenu = (x, y, items, document) ->
  remote = require 'remote'
  Menu = remote.require 'menu'

  menu = Menu.buildFromTemplate convertToMenuTemplate(items)
  # The menu is expected to show asynchronously.
  setImmediate ->
    menu.popup remote.getCurrentWindow()
    DevToolsAPI.contextMenuCleared()

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

createFileSelectorElement = (callback) ->
  fileSelectorElement = document.createElement 'span'
  fileSelectorElement.style.display = 'none'
  fileSelectorElement.click = showFileChooserDialog.bind this, callback
  return fileSelectorElement

# Exposed for iframe.
window.createMenu = createMenu
window.createFileSelectorElement = createFileSelectorElement
