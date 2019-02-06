'use strict'

const { dialog, Menu } = require('electron')
const fs = require('fs')
const url = require('url')

const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')

const convertToMenuTemplate = function (event, items) {
  return items.map(function (item) {
    const transformed = item.type === 'subMenu' ? {
      type: 'submenu',
      label: item.label,
      enabled: item.enabled,
      submenu: convertToMenuTemplate(event, item.subItems)
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
        event._replyInternal('ELECTRON_INSPECTOR_CONTEXT_MENU_CLICK', item.id)
      }
    }

    return transformed
  })
}

const getEditMenuItems = function () {
  return [
    { role: 'undo' },
    { role: 'redo' },
    { type: 'separator' },
    { role: 'cut' },
    { role: 'copy' },
    { role: 'paste' },
    { role: 'pasteAndMatchStyle' },
    { role: 'delete' },
    { role: 'selectAll' }
  ]
}

const isChromeDevTools = function (pageURL) {
  const { protocol } = url.parse(pageURL)
  return protocol === 'chrome-devtools:'
}

const assertChromeDevTools = function (contents, api) {
  const pageURL = contents._getURL()
  if (!isChromeDevTools(pageURL)) {
    console.error(`Blocked ${pageURL} from calling ${api}`)
    throw new Error(`Blocked ${api}`)
  }
}

ipcMainUtils.handle('ELECTRON_INSPECTOR_CONTEXT_MENU', function (event, items, isEditMenu) {
  assertChromeDevTools(event.sender, 'window.InspectorFrontendHost.showContextMenuAtPoint()')

  const template = isEditMenu ? getEditMenuItems() : convertToMenuTemplate(event, items)
  const menu = Menu.buildFromTemplate(template)
  const window = event.sender.getOwnerBrowserWindow()

  menu.once('menu-will-close', () => {
    // menu-will-close is emitted before the click handler, but needs to be sent after.
    // otherwise, DevToolsAPI.contextMenuCleared() would be called before
    // DevToolsAPI.contextMenuItemSelected(id) and the menu will not work properly.
    setTimeout(() => {
      event._replyInternal('ELECTRON_INSPECTOR_CONTEXT_MENU_CLOSE')
    })
  })

  menu.popup({ window })
})

ipcMainUtils.handle('ELECTRON_INSPECTOR_SELECT_FILE', function (event) {
  return new Promise((resolve, reject) => {
    assertChromeDevTools(event.sender, 'window.UI.createFileSelectorElement()')

    dialog.showOpenDialog({}, function (files) {
      if (files) {
        const path = files[0]
        fs.readFile(path, (error, data) => {
          if (error) {
            reject(error)
          } else {
            resolve([path, data])
          }
        })
      } else {
        resolve([])
      }
    })
  })
})

ipcMainUtils.handleSync('ELECTRON_INSPECTOR_CONFIRM', function (event, message, title) {
  return new Promise((resolve, reject) => {
    assertChromeDevTools(event.sender, 'window.confirm()')

    if (message == null) message = ''
    if (title == null) title = ''

    const options = {
      message: `${message}`,
      title: `${title}`,
      buttons: ['OK', 'Cancel'],
      cancelId: 1
    }
    const window = event.sender.getOwnerBrowserWindow()
    dialog.showMessageBox(window, options, (response) => {
      resolve(response === 0)
    })
  })
})
