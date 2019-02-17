import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import { invoke, invokeSync } from '@electron/internal/renderer/ipc-renderer-internal-utils'
import { stringify } from 'querystring'

window.onload = function () {
  // Use menu API to show context menu.
  window.InspectorFrontendHost!.showContextMenuAtPoint = createMenu

  // correct for Chromium returning undefined for filesystem
  window.Persistence!.FileSystemWorkspaceBinding.completeURL = completeURL

  // Use dialog API to override file chooser dialog.
  window.UI!.createFileSelectorElement = createFileSelectorElement
}

// Extra / is needed as a result of MacOS requiring absolute paths
function completeURL (project: string, path: string) {
  project = 'file:///'
  return `${project}${path}`
}

// The DOM implementation expects (message?: string) => boolean
(window.confirm as any) = function (message: string, title: string) {
  return invokeSync('ELECTRON_INSPECTOR_CONFIRM', message, title) as boolean
}

ipcRendererInternal.on('ELECTRON_INSPECTOR_CONTEXT_MENU_CLICK', function (_event: Electron.EditFlags, id: number) {
  window.DevToolsAPI!.contextMenuItemSelected(id)
})

ipcRendererInternal.on('ELECTRON_INSPECTOR_CONTEXT_MENU_CLOSE', function () {
  window.DevToolsAPI!.contextMenuCleared()
})

const useEditMenuItems = function (x: number, y: number, items: any[]) {
  return items.length === 0 && document.elementsFromPoint(x, y).some(function (element) {
    return element.nodeName === 'INPUT' ||
      element.nodeName === 'TEXTAREA' ||
      (element as HTMLElement).isContentEditable
  })
}

const createMenu = function (x: number, y: number, items: any[]) {
  const isEditMenu = useEditMenuItems(x, y, items)
  invoke('ELECTRON_INSPECTOR_CONTEXT_MENU', items, isEditMenu)
}

const showFileChooserDialog = function (callback: (blob: Blob) => void) {
  invoke<[ string, any ]>('ELECTRON_INSPECTOR_SELECT_FILE').then(([path, data]) => {
    if (path && data) {
      callback(dataToHtml5FileObject(path, data))
    }
  })
}

const dataToHtml5FileObject = function (path: string, data: any) {
  const blob = new Blob([data])
  blob.name = path
  return blob
}

const createFileSelectorElement = function (this: any, callback: () => void) {
  const fileSelectorElement = document.createElement('span')
  fileSelectorElement.style.display = 'none'
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback)
  return fileSelectorElement
}
