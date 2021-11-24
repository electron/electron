import { internalContextBridge } from '@electron/internal/renderer/api/context-bridge';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { webFrame } from 'electron/renderer';
import { IPC_MESSAGES } from '../common/ipc-messages';

const { contextIsolationEnabled } = internalContextBridge;

/* Corrects for some Inspector adaptations needed in Electron.
* 1) Use menu API to show context menu.
* 2) Correct for Chromium returning undefined for filesystem.
* 3) Use dialog API to override file chooser dialog.
*/
window.onload = function () {
  if (contextIsolationEnabled) {
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'InspectorFrontendHost', 'showContextMenuAtPoint'
    ], createMenu);
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'Persistence', 'FileSystemWorkspaceBinding', 'completeURL'
    ], completeURL);
    internalContextBridge.overrideGlobalValueFromIsolatedWorld([
      'UI', 'createFileSelectorElement'
    ], createFileSelectorElement);
  } else {
    window.InspectorFrontendHost!.showContextMenuAtPoint = createMenu;
    window.Persistence!.FileSystemWorkspaceBinding.completeURL = completeURL;
    window.UI!.createFileSelectorElement = createFileSelectorElement;
  }
};

// Extra / is needed as a result of MacOS requiring absolute paths
function completeURL (project: string, path: string) {
  project = 'file:///';
  return `${project}${path}`;
}

// The DOM implementation expects (message?: string) => boolean
window.confirm = function (message?: string, title?: string) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.INSPECTOR_CONFIRM, message, title) as boolean;
};

const useEditMenuItems = function (x: number, y: number, items: ContextMenuItem[]) {
  return items.length === 0 && document.elementsFromPoint(x, y).some(function (element) {
    return element.nodeName === 'INPUT' ||
      element.nodeName === 'TEXTAREA' ||
      (element as HTMLElement).isContentEditable;
  });
};

const createMenu = function (x: number, y: number, items: ContextMenuItem[]) {
  const isEditMenu = useEditMenuItems(x, y, items);
  ipcRendererInternal.invoke<number>(IPC_MESSAGES.INSPECTOR_CONTEXT_MENU, items, isEditMenu).then(id => {
    if (typeof id === 'number') {
      webFrame.executeJavaScript(`window.DevToolsAPI.contextMenuItemSelected(${JSON.stringify(id)})`);
    }

    webFrame.executeJavaScript('window.DevToolsAPI.contextMenuCleared()');
  });
};

const showFileChooserDialog = function (callback: (blob: File) => void) {
  ipcRendererInternal.invoke<[ string, any ]>(IPC_MESSAGES.INSPECTOR_SELECT_FILE).then(([path, data]) => {
    if (path && data) {
      callback(dataToHtml5FileObject(path, data));
    }
  });
};

const dataToHtml5FileObject = function (path: string, data: any) {
  return new File([data], path);
};

const createFileSelectorElement = function (this: any, callback: () => void) {
  const fileSelectorElement = document.createElement('span');
  fileSelectorElement.style.display = 'none';
  fileSelectorElement.click = showFileChooserDialog.bind(this, callback);
  return fileSelectorElement;
};
