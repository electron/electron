'use strict'

const { dialog, Menu } = require('electron')
const fs = require('fs')
const url = require('url')
const util = require('util')

const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')

const readFile = util.promisify(fs.readFile)

const convertToMenuTemplate = function (items, handler) {
  return items.map(function (item) {
    const transformed = item.type === 'subMenu' ? {
      type: 'submenu',
      label: item.label,
      enabled: item.enabled,
      submenu: convertToMenuTemplate(item.subItems, handler)
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
      transformed.click = () => handler(item.id)
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
  return protocol === 'devtools:'
}

const assertChromeDevTools = function (contents, api) {
  const pageURL = contents._getURL()
  if (!isChromeDevTools(pageURL)) {
    console.error(`Blocked ${pageURL} from calling ${api}`)
    throw new Error(`Blocked ${api}`)
  }
}

ipcMainUtils.handle('ELECTRON_INSPECTOR_CONTEXT_MENU', function (event, items, isEditMenu) {
  return new Promise(resolve => {
    assertChromeDevTools(event.sender, 'window.InspectorFrontendHost.showContextMenuAtPoint()')

    const template = isEditMenu ? getEditMenuItems() : convertToMenuTemplate(items, resolve)
    const menu = Menu.buildFromTemplate(template)
    const window = event.sender.getOwnerBrowserWindow()

    menu.once('menu-will-close', () => {
      // menu-will-close is emitted before the click handler, but needs to be sent after.
      // otherwise, DevToolsAPI.contextMenuCleared() would be called before
      // DevToolsAPI.contextMenuItemSelected(id) and the menu will not work properly.
      setTimeout(() => resolve())
    })

    menu.popup({ window })
  })
})

ipcMainUtils.handle('ELECTRON_INSPECTOR_SELECT_FILE', async function (event) {
  assertChromeDevTools(event.sender, 'window.UI.createFileSelectorElement()')

  const result = await dialog.showOpenDialog({})
  if (result.canceled) return []

  const path = result.filePaths[0]
  const data = await readFile(path)

  return [path, data]
})

ipcMainUtils.handle('ELECTRON_INSPECTOR_CONFIRM', async function (event, message = '', title = '') {
  assertChromeDevTools(event.sender, 'window.confirm()')

  const options = {
    message: String(message),
    title: String(title),
    buttons: ['OK', 'Cancel'],
    cancelId: 1
  }
  const window = event.sender.getOwnerBrowserWindow()
  const { response } = await dialog.showMessageBox(window, options)
  return response === 0
})
