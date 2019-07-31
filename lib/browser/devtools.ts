import { dialog, Menu } from 'electron'
import * as fs from 'fs'
import * as url from 'url'

import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils'

const convertToMenuTemplate = function (items: ContextMenuItem[], handler: (id: number) => void) {
  return items.map(function (item) {
    const transformed: Electron.MenuItemConstructorOptions = item.type === 'subMenu' ? {
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

const getEditMenuItems = function (): Electron.MenuItemConstructorOptions[] {
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

const isChromeDevTools = function (pageURL: string) {
  const { protocol } = url.parse(pageURL)
  return protocol === 'devtools:'
}

const assertChromeDevTools = function (contents: Electron.WebContents, api: string) {
  const pageURL = contents._getURL()
  if (!isChromeDevTools(pageURL)) {
    console.error(`Blocked ${pageURL} from calling ${api}`)
    throw new Error(`Blocked ${api}`)
  }
}

ipcMainUtils.handle('ELECTRON_INSPECTOR_CONTEXT_MENU', function (event: Electron.IpcMainInvokeEvent, items: ContextMenuItem[], isEditMenu: boolean) {
  return new Promise(resolve => {
    assertChromeDevTools(event.sender, 'window.InspectorFrontendHost.showContextMenuAtPoint()')

    const template = isEditMenu ? getEditMenuItems() : convertToMenuTemplate(items, resolve)
    const menu = Menu.buildFromTemplate(template)
    const window = event.sender.getOwnerBrowserWindow()

    menu.popup({ window, callback: () => resolve() })
  })
})

ipcMainUtils.handle('ELECTRON_INSPECTOR_SELECT_FILE', async function (event: Electron.IpcMainInvokeEvent) {
  assertChromeDevTools(event.sender, 'window.UI.createFileSelectorElement()')

  const result = await dialog.showOpenDialog({})
  if (result.canceled) return []

  const path = result.filePaths[0]
  const data = await fs.promises.readFile(path)

  return [path, data]
})

ipcMainUtils.handle('ELECTRON_INSPECTOR_CONFIRM', async function (event: Electron.IpcMainInvokeEvent, message: string = '', title: string = '') {
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
