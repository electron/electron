window.onload = function () {
  // Use menu API to show context menu.
  window.InspectorFrontendHost.showContextMenuAtPoint = createMenu

  // Use dialog API to override file chooser dialog.
  window.WebInspector.createFileSelectorElement = createFileSelectorElement
}

const convertToMenuTemplate = function (items) {
  return items.map(function (item) {
    const transformed = item.type === 'subMenu' ? {
      type: 'submenu',
      label: item.label,
      enabled: item.enabled,
      submenu: convertToMenuTemplate(item.subItems)
    } : item.type === 'separator' ? {
      type: 'separator'
    } : item.type === 'checkbox' ? {
      type: 'checkbox',
      label: item.label,
      enabled: item.enabled,
      checked: item.checked
    } : {
      type: 'normal',
      label: item.label,
      enabled: item.enabled
    }

    if (item.id != null) {
      transformed.click = function () {
        window.DevToolsAPI.contextMenuItemSelected(item.id)
        return window.DevToolsAPI.contextMenuCleared()
      }
    }

    return transformed
  })
}

const createMenu = function (x, y, items) {
  const {remote} = require('electron')
  const {Menu} = remote
  const menu = Menu.buildFromTemplate(convertToMenuTemplate(items))

  // The menu is expected to show asynchronously.
  setTimeout(function () {
    menu.popup(remote.getCurrentWindow())
  })
}

const showFileChooserDialog = function (callback) {
  const {dialog} = require('electron').remote
  const files = dialog.showOpenDialog({})
  if (files != null) {
    callback(pathToHtml5FileObject(files[0]))
  }
}

const pathToHtml5FileObject = function (path) {
  const fs = require('fs')
  const blob = new Blob([fs.readFileSync(path)])
  blob.name = path
  return blob
}

const createFileSelectorElement = function (callback) {
  const fileSelectorElement = document.createElement('span')
  fileSelectorElement.style.display = 'none'
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback)
  return fileSelectorElement
}
